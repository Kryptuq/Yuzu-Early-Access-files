// SPDX-FileCopyrightText: 2014 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fmt/format.h>
#include <libusb.h>

#include "common/logging/log.h"
#include "common/param_package.h"
#include "common/settings_input.h"
#include "common/thread.h"
#include "input_common/drivers/gc_adapter.h"

namespace InputCommon {

class LibUSBContext {
public:
    explicit LibUSBContext() {
        init_result = libusb_init(&ctx);
    }

    ~LibUSBContext() {
        libusb_exit(ctx);
    }

    LibUSBContext& operator=(const LibUSBContext&) = delete;
    LibUSBContext(const LibUSBContext&) = delete;

    LibUSBContext& operator=(LibUSBContext&&) noexcept = delete;
    LibUSBContext(LibUSBContext&&) noexcept = delete;

    [[nodiscard]] int InitResult() const noexcept {
        return init_result;
    }

    [[nodiscard]] libusb_context* get() noexcept {
        return ctx;
    }

private:
    libusb_context* ctx;
    int init_result{};
};

class LibUSBDeviceHandle {
public:
    explicit LibUSBDeviceHandle(libusb_context* ctx, uint16_t vid, uint16_t pid) noexcept {
        handle = libusb_open_device_with_vid_pid(ctx, vid, pid);
    }

    ~LibUSBDeviceHandle() noexcept {
        if (handle) {
            libusb_release_interface(handle, 1);
            libusb_close(handle);
        }
    }

    LibUSBDeviceHandle& operator=(const LibUSBDeviceHandle&) = delete;
    LibUSBDeviceHandle(const LibUSBDeviceHandle&) = delete;

    LibUSBDeviceHandle& operator=(LibUSBDeviceHandle&&) noexcept = delete;
    LibUSBDeviceHandle(LibUSBDeviceHandle&&) noexcept = delete;

    [[nodiscard]] libusb_device_handle* get() noexcept {
        return handle;
    }

private:
    libusb_device_handle* handle{};
};

GCAdapter::GCAdapter(std::string input_engine_) : InputEngine(std::move(input_engine_)) {
    if (usb_adapter_handle) {
        return;
    }
    LOG_DEBUG(Input, "Initialization started");

    libusb_ctx = std::make_unique<LibUSBContext>();
    const int init_res = libusb_ctx->InitResult();
    if (init_res == LIBUSB_SUCCESS) {
        adapter_scan_thread =
            std::jthread([this](std::stop_token stop_token) { AdapterScanThread(stop_token); });
    } else {
        LOG_ERROR(Input, "libusb could not be initialized. failed with error = {}", init_res);
    }
}

GCAdapter::~GCAdapter() {
    Reset();
}

void GCAdapter::AdapterInputThread(std::stop_token stop_token) {
    LOG_DEBUG(Input, "Input thread started");
    Common::SetCurrentThreadName("GCAdapter");
    s32 payload_size{};
    AdapterPayload adapter_payload{};

    adapter_scan_thread = {};

    while (!stop_token.stop_requested()) {
        libusb_interrupt_transfer(usb_adapter_handle->get(), input_endpoint, adapter_payload.data(),
                                  static_cast<s32>(adapter_payload.size()), &payload_size, 16);
        if (IsPayloadCorrect(adapter_payload, payload_size)) {
            UpdateControllers(adapter_payload);
            UpdateVibrations();
        }
        std::this_thread::yield();
    }

    if (restart_scan_thread) {
        adapter_scan_thread =
            std::jthread([this](std::stop_token token) { AdapterScanThread(token); });
        restart_scan_thread = false;
    }
}

bool GCAdapter::IsPayloadCorrect(const AdapterPayload& adapter_payload, s32 payload_size) {
    if (payload_size != static_cast<s32>(adapter_payload.size()) ||
        adapter_payload[0] != LIBUSB_DT_HID) {
        LOG_DEBUG(Input, "Error reading payload (size: {}, type: {:02x})", payload_size,
                  adapter_payload[0]);
        if (input_error_counter++ > 20) {
            LOG_ERROR(Input, "Timeout, Is the adapter connected?");
            adapter_input_thread.request_stop();
            restart_scan_thread = true;
        }
        return false;
    }

    input_error_counter = 0;
    return true;
}

void GCAdapter::UpdateControllers(const AdapterPayload& adapter_payload) {
    for (std::size_t port = 0; port < pads.size(); ++port) {
        const std::size_t offset = 1 + (9 * port);
        const auto type = static_cast<ControllerTypes>(adapter_payload[offset] >> 4);
        UpdatePadType(port, type);
        if (DeviceConnected(port)) {
            const u8 b1 = adapter_payload[offset + 1];
            const u8 b2 = adapter_payload[offset + 2];
            UpdateStateButtons(port, b1, b2);
            UpdateStateAxes(port, adapter_payload);
        }
    }
}

void GCAdapter::UpdatePadType(std::size_t port, ControllerTypes pad_type) {
    if (pads[port].type == pad_type) {
        return;
    }
    // Device changed reset device and set new type
    pads[port].axis_origin = {};
    pads[port].reset_origin_counter = {};
    pads[port].enable_vibration = {};
    pads[port].rumble_amplitude = {};
    pads[port].type = pad_type;
}

void GCAdapter::UpdateStateButtons(std::size_t port, [[maybe_unused]] u8 b1,
                                   [[maybe_unused]] u8 b2) {
    if (port >= pads.size()) {
        return;
    }

    static constexpr std::array<PadButton, 8> b1_buttons{
        PadButton::ButtonA,    PadButton::ButtonB,     PadButton::ButtonX,    PadButton::ButtonY,
        PadButton::ButtonLeft, PadButton::ButtonRight, PadButton::ButtonDown, PadButton::ButtonUp,
    };

    static constexpr std::array<PadButton, 4> b2_buttons{
        PadButton::ButtonStart,
        PadButton::TriggerZ,
        PadButton::TriggerR,
        PadButton::TriggerL,
    };

    for (std::size_t i = 0; i < b1_buttons.size(); ++i) {
        const bool button_status = (b1 & (1U << i)) != 0;
        const int button = static_cast<int>(b1_buttons[i]);
        SetButton(pads[port].identifier, button, button_status);
    }

    for (std::size_t j = 0; j < b2_buttons.size(); ++j) {
        const bool button_status = (b2 & (1U << j)) != 0;
        const int button = static_cast<int>(b2_buttons[j]);
        SetButton(pads[port].identifier, button, button_status);
    }
}

void GCAdapter::UpdateStateAxes(std::size_t port, const AdapterPayload& adapter_payload) {
    if (port >= pads.size()) {
        return;
    }

    const std::size_t offset = 1 + (9 * port);
    static constexpr std::array<PadAxes, 6> axes{
        PadAxes::StickX,    PadAxes::StickY,      PadAxes::SubstickX,
        PadAxes::SubstickY, PadAxes::TriggerLeft, PadAxes::TriggerRight,
    };

    for (const PadAxes axis : axes) {
        const auto index = static_cast<std::size_t>(axis);
        const u8 axis_value = adapter_payload[offset + 3 + index];
        if (pads[port].reset_origin_counter <= 18) {
            if (pads[port].axis_origin[index] != axis_value) {
                pads[port].reset_origin_counter = 0;
            }
            pads[port].axis_origin[index] = axis_value;
            pads[port].reset_origin_counter++;
        }
        const f32 axis_status = (axis_value - pads[port].axis_origin[index]) / 100.0f;
        SetAxis(pads[port].identifier, static_cast<int>(index), axis_status);
    }
}

void GCAdapter::AdapterScanThread(std::stop_token stop_token) {
    Common::SetCurrentThreadName("ScanGCAdapter");
    usb_adapter_handle = nullptr;
    pads = {};
    while (!stop_token.stop_requested() && !Setup()) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

bool GCAdapter::Setup() {
    constexpr u16 nintendo_vid = 0x057e;
    constexpr u16 gc_adapter_pid = 0x0337;
    usb_adapter_handle =
        std::make_unique<LibUSBDeviceHandle>(libusb_ctx->get(), nintendo_vid, gc_adapter_pid);
    if (!usb_adapter_handle->get()) {
        return false;
    }
    if (!CheckDeviceAccess()) {
        usb_adapter_handle = nullptr;
        return false;
    }

    libusb_device* const device = libusb_get_device(usb_adapter_handle->get());

    LOG_INFO(Input, "GC adapter is now connected");
    // GC Adapter found and accessible, registering it
    if (GetGCEndpoint(device)) {
        rumble_enabled = true;
        input_error_counter = 0;
        output_error_counter = 0;

        std::size_t port = 0;
        for (GCController& pad : pads) {
            pad.identifier = {
                .guid = Common::UUID{},
                .port = port++,
                .pad = 0,
            };
            PreSetController(pad.identifier);
        }

        adapter_input_thread =
            std::jthread([this](std::stop_token stop_token) { AdapterInputThread(stop_token); });
        return true;
    }
    return false;
}

bool GCAdapter::CheckDeviceAccess() {
    s32 kernel_driver_error = libusb_kernel_driver_active(usb_adapter_handle->get(), 0);
    if (kernel_driver_error == 1) {
        kernel_driver_error = libusb_detach_kernel_driver(usb_adapter_handle->get(), 0);
        if (kernel_driver_error != 0 && kernel_driver_error != LIBUSB_ERROR_NOT_SUPPORTED) {
            LOG_ERROR(Input, "libusb_detach_kernel_driver failed with error = {}",
                      kernel_driver_error);
        }
    }

    if (kernel_driver_error && kernel_driver_error != LIBUSB_ERROR_NOT_SUPPORTED) {
        usb_adapter_handle = nullptr;
        return false;
    }

    const int interface_claim_error = libusb_claim_interface(usb_adapter_handle->get(), 0);
    if (interface_claim_error) {
        LOG_ERROR(Input, "libusb_claim_interface failed with error = {}", interface_claim_error);
        usb_adapter_handle = nullptr;
        return false;
    }

    // This fixes payload problems from offbrand GCAdapters
    const s32 control_transfer_error =
        libusb_control_transfer(usb_adapter_handle->get(), 0x21, 11, 0x0001, 0, nullptr, 0, 1000);
    if (control_transfer_error < 0) {
        LOG_ERROR(Input, "libusb_control_transfer failed with error= {}", control_transfer_error);
    }

    return true;
}

bool GCAdapter::GetGCEndpoint(libusb_device* device) {
    libusb_config_descriptor* config = nullptr;
    const int config_descriptor_return = libusb_get_config_descriptor(device, 0, &config);
    if (config_descriptor_return != LIBUSB_SUCCESS) {
        LOG_ERROR(Input, "libusb_get_config_descriptor failed with error = {}",
                  config_descriptor_return);
        return false;
    }

    for (u8 ic = 0; ic < config->bNumInterfaces; ic++) {
        const libusb_interface* interfaceContainer = &config->interface[ic];
        for (int i = 0; i < interfaceContainer->num_altsetting; i++) {
            const libusb_interface_descriptor* interface = &interfaceContainer->altsetting[i];
            for (u8 e = 0; e < interface->bNumEndpoints; e++) {
                const libusb_endpoint_descriptor* endpoint = &interface->endpoint[e];
                if ((endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN) != 0) {
                    input_endpoint = endpoint->bEndpointAddress;
                } else {
                    output_endpoint = endpoint->bEndpointAddress;
                }
            }
        }
    }
    // This transfer seems to be responsible for clearing the state of the adapter
    // Used to clear the "busy" state of when the device is unexpectedly unplugged
    unsigned char clear_payload = 0x13;
    libusb_interrupt_transfer(usb_adapter_handle->get(), output_endpoint, &clear_payload,
                              sizeof(clear_payload), nullptr, 16);
    return true;
}

Common::Input::DriverResult GCAdapter::SetVibration(
    const PadIdentifier& identifier, const Common::Input::VibrationStatus& vibration) {
    const auto mean_amplitude = (vibration.low_amplitude + vibration.high_amplitude) * 0.5f;
    const auto processed_amplitude =
        static_cast<u8>((mean_amplitude + std::pow(mean_amplitude, 0.3f)) * 0.5f * 0x8);

    pads[identifier.port].rumble_amplitude = processed_amplitude;

    if (!rumble_enabled) {
        return Common::Input::DriverResult::Disabled;
    }
    return Common::Input::DriverResult::Success;
}

bool GCAdapter::IsVibrationEnabled([[maybe_unused]] const PadIdentifier& identifier) {
    return rumble_enabled;
}

void GCAdapter::UpdateVibrations() {
    // Use 8 states to keep the switching between on/off fast enough for
    // a human to feel different vibration strenght
    // More states == more rumble strengths == slower update time
    constexpr u8 vibration_states = 8;

    vibration_counter = (vibration_counter + 1) % vibration_states;

    for (GCController& pad : pads) {
        const bool vibrate = pad.rumble_amplitude > vibration_counter;
        vibration_changed |= vibrate != pad.enable_vibration;
        pad.enable_vibration = vibrate;
    }
    SendVibrations();
}

void GCAdapter::SendVibrations() {
    if (!rumble_enabled || !vibration_changed) {
        return;
    }
    s32 size{};
    constexpr u8 rumble_command = 0x11;
    const u8 p1 = pads[0].enable_vibration;
    const u8 p2 = pads[1].enable_vibration;
    const u8 p3 = pads[2].enable_vibration;
    const u8 p4 = pads[3].enable_vibration;
    std::array<u8, 5> payload = {rumble_command, p1, p2, p3, p4};
    const int err =
        libusb_interrupt_transfer(usb_adapter_handle->get(), output_endpoint, payload.data(),
                                  static_cast<s32>(payload.size()), &size, 16);
    if (err) {
        LOG_DEBUG(Input, "Libusb write failed: {}", libusb_error_name(err));
        if (output_error_counter++ > 5) {
            LOG_ERROR(Input, "Output timeout, Rumble disabled");
            rumble_enabled = false;
        }
        return;
    }
    output_error_counter = 0;
    vibration_changed = false;
}

bool GCAdapter::DeviceConnected(std::size_t port) const {
    return pads[port].type != ControllerTypes::None;
}

void GCAdapter::Reset() {
    adapter_scan_thread = {};
    adapter_input_thread = {};
    usb_adapter_handle = nullptr;
    pads = {};
    libusb_ctx = nullptr;
}

std::vector<Common::ParamPackage> GCAdapter::GetInputDevices() const {
    std::vector<Common::ParamPackage> devices;
    for (std::size_t port = 0; port < pads.size(); ++port) {
        if (!DeviceConnected(port)) {
            continue;
        }
        Common::ParamPackage identifier{};
        identifier.Set("engine", GetEngineName());
        identifier.Set("display", fmt::format("Gamecube Controller {}", port + 1));
        identifier.Set("port", static_cast<int>(port));
        devices.emplace_back(identifier);
    }
    return devices;
}

ButtonMapping GCAdapter::GetButtonMappingForDevice(const Common::ParamPackage& params) {
    // This list is missing ZL/ZR since those are not considered buttons.
    // We will add those afterwards
    // This list also excludes any button that can't be really mapped
    static constexpr std::array<std::pair<Settings::NativeButton::Values, PadButton>, 12>
        switch_to_gcadapter_button = {
            std::pair{Settings::NativeButton::A, PadButton::ButtonA},
            {Settings::NativeButton::B, PadButton::ButtonB},
            {Settings::NativeButton::X, PadButton::ButtonX},
            {Settings::NativeButton::Y, PadButton::ButtonY},
            {Settings::NativeButton::Plus, PadButton::ButtonStart},
            {Settings::NativeButton::DLeft, PadButton::ButtonLeft},
            {Settings::NativeButton::DUp, PadButton::ButtonUp},
            {Settings::NativeButton::DRight, PadButton::ButtonRight},
            {Settings::NativeButton::DDown, PadButton::ButtonDown},
            {Settings::NativeButton::SL, PadButton::TriggerL},
            {Settings::NativeButton::SR, PadButton::TriggerR},
            {Settings::NativeButton::R, PadButton::TriggerZ},
        };
    if (!params.Has("port")) {
        return {};
    }

    ButtonMapping mapping{};
    for (const auto& [switch_button, gcadapter_button] : switch_to_gcadapter_button) {
        Common::ParamPackage button_params{};
        button_params.Set("engine", GetEngineName());
        button_params.Set("port", params.Get("port", 0));
        button_params.Set("button", static_cast<int>(gcadapter_button));
        mapping.insert_or_assign(switch_button, std::move(button_params));
    }

    // Add the missing bindings for ZL/ZR
    static constexpr std::array<std::tuple<Settings::NativeButton::Values, PadButton, PadAxes>, 2>
        switch_to_gcadapter_axis = {
            std::tuple{Settings::NativeButton::ZL, PadButton::TriggerL, PadAxes::TriggerLeft},
            {Settings::NativeButton::ZR, PadButton::TriggerR, PadAxes::TriggerRight},
        };
    for (const auto& [switch_button, gcadapter_buton, gcadapter_axis] : switch_to_gcadapter_axis) {
        Common::ParamPackage button_params{};
        button_params.Set("engine", GetEngineName());
        button_params.Set("port", params.Get("port", 0));
        button_params.Set("button", static_cast<s32>(gcadapter_buton));
        button_params.Set("axis", static_cast<s32>(gcadapter_axis));
        button_params.Set("threshold", 0.5f);
        button_params.Set("range", 1.9f);
        button_params.Set("direction", "+");
        mapping.insert_or_assign(switch_button, std::move(button_params));
    }
    return mapping;
}

AnalogMapping GCAdapter::GetAnalogMappingForDevice(const Common::ParamPackage& params) {
    if (!params.Has("port")) {
        return {};
    }

    AnalogMapping mapping = {};
    Common::ParamPackage left_analog_params;
    left_analog_params.Set("engine", GetEngineName());
    left_analog_params.Set("port", params.Get("port", 0));
    left_analog_params.Set("axis_x", static_cast<int>(PadAxes::StickX));
    left_analog_params.Set("axis_y", static_cast<int>(PadAxes::StickY));
    mapping.insert_or_assign(Settings::NativeAnalog::LStick, std::move(left_analog_params));
    Common::ParamPackage right_analog_params;
    right_analog_params.Set("engine", GetEngineName());
    right_analog_params.Set("port", params.Get("port", 0));
    right_analog_params.Set("axis_x", static_cast<int>(PadAxes::SubstickX));
    right_analog_params.Set("axis_y", static_cast<int>(PadAxes::SubstickY));
    mapping.insert_or_assign(Settings::NativeAnalog::RStick, std::move(right_analog_params));
    return mapping;
}

Common::Input::ButtonNames GCAdapter::GetUIButtonName(const Common::ParamPackage& params) const {
    PadButton button = static_cast<PadButton>(params.Get("button", 0));
    switch (button) {
    case PadButton::ButtonLeft:
        return Common::Input::ButtonNames::ButtonLeft;
    case PadButton::ButtonRight:
        return Common::Input::ButtonNames::ButtonRight;
    case PadButton::ButtonDown:
        return Common::Input::ButtonNames::ButtonDown;
    case PadButton::ButtonUp:
        return Common::Input::ButtonNames::ButtonUp;
    case PadButton::TriggerZ:
        return Common::Input::ButtonNames::TriggerZ;
    case PadButton::TriggerR:
        return Common::Input::ButtonNames::TriggerR;
    case PadButton::TriggerL:
        return Common::Input::ButtonNames::TriggerL;
    case PadButton::ButtonA:
        return Common::Input::ButtonNames::ButtonA;
    case PadButton::ButtonB:
        return Common::Input::ButtonNames::ButtonB;
    case PadButton::ButtonX:
        return Common::Input::ButtonNames::ButtonX;
    case PadButton::ButtonY:
        return Common::Input::ButtonNames::ButtonY;
    case PadButton::ButtonStart:
        return Common::Input::ButtonNames::ButtonStart;
    default:
        return Common::Input::ButtonNames::Undefined;
    }
}

Common::Input::ButtonNames GCAdapter::GetUIName(const Common::ParamPackage& params) const {
    if (params.Has("button")) {
        return GetUIButtonName(params);
    }
    if (params.Has("axis")) {
        return Common::Input::ButtonNames::Value;
    }

    return Common::Input::ButtonNames::Invalid;
}

bool GCAdapter::IsStickInverted(const Common::ParamPackage& params) {
    if (!params.Has("port")) {
        return false;
    }

    const auto x_axis = static_cast<PadAxes>(params.Get("axis_x", 0));
    const auto y_axis = static_cast<PadAxes>(params.Get("axis_y", 0));
    if (x_axis != PadAxes::StickY && x_axis != PadAxes::SubstickY) {
        return false;
    }
    if (y_axis != PadAxes::StickX && y_axis != PadAxes::SubstickX) {
        return false;
    }
    return true;
}

} // namespace InputCommon

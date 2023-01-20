// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fmt/format.h>

#include "common/param_package.h"
#include "common/settings.h"
#include "common/thread.h"
#include "input_common/drivers/joycon.h"
#include "input_common/helpers/joycon_driver.h"
#include "input_common/helpers/joycon_protocol/joycon_types.h"

namespace InputCommon {

Joycons::Joycons(const std::string& input_engine_) : InputEngine(input_engine_) {
    // Avoid conflicting with SDL driver
    if (!Settings::values.enable_joycon_driver) {
        return;
    }
    LOG_INFO(Input, "Joycon driver Initialization started");
    const int init_res = SDL_hid_init();
    if (init_res == 0) {
        Setup();
    } else {
        LOG_ERROR(Input, "Hidapi could not be initialized. failed with error = {}", init_res);
    }
}

Joycons::~Joycons() {
    Reset();
}

void Joycons::Reset() {
    scan_thread = {};
    for (const auto& device : left_joycons) {
        if (!device) {
            continue;
        }
        device->Stop();
    }
    for (const auto& device : right_joycons) {
        if (!device) {
            continue;
        }
        device->Stop();
    }
    SDL_hid_exit();
}

void Joycons::Setup() {
    u32 port = 0;
    PreSetController(GetIdentifier(0, Joycon::ControllerType::None));
    for (auto& device : left_joycons) {
        PreSetController(GetIdentifier(port, Joycon::ControllerType::Left));
        device = std::make_shared<Joycon::JoyconDriver>(port++);
    }
    port = 0;
    for (auto& device : right_joycons) {
        PreSetController(GetIdentifier(port, Joycon::ControllerType::Right));
        device = std::make_shared<Joycon::JoyconDriver>(port++);
    }

    scan_thread = std::jthread([this](std::stop_token stop_token) { ScanThread(stop_token); });
}

void Joycons::ScanThread(std::stop_token stop_token) {
    constexpr u16 nintendo_vendor_id = 0x057e;
    Common::SetCurrentThreadName("JoyconScanThread");
    while (!stop_token.stop_requested()) {
        SDL_hid_device_info* devs = SDL_hid_enumerate(nintendo_vendor_id, 0x0);
        SDL_hid_device_info* cur_dev = devs;

        while (cur_dev) {
            if (IsDeviceNew(cur_dev)) {
                LOG_DEBUG(Input, "Device Found,type : {:04X} {:04X}", cur_dev->vendor_id,
                          cur_dev->product_id);
                RegisterNewDevice(cur_dev);
            }
            cur_dev = cur_dev->next;
        }

        SDL_hid_free_enumeration(devs);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

bool Joycons::IsDeviceNew(SDL_hid_device_info* device_info) const {
    Joycon::ControllerType type{};
    Joycon::SerialNumber serial_number{};

    const auto result = Joycon::JoyconDriver::GetDeviceType(device_info, type);
    if (result != Joycon::DriverResult::Success) {
        return false;
    }

    const auto result2 = Joycon::JoyconDriver::GetSerialNumber(device_info, serial_number);
    if (result2 != Joycon::DriverResult::Success) {
        return false;
    }

    auto is_handle_identical = [serial_number](std::shared_ptr<Joycon::JoyconDriver> device) {
        if (!device) {
            return false;
        }
        if (!device->IsConnected()) {
            return false;
        }
        if (device->GetHandleSerialNumber() != serial_number) {
            return false;
        }
        return true;
    };

    // Check if device already exist
    switch (type) {
    case Joycon::ControllerType::Left:
        for (const auto& device : left_joycons) {
            if (is_handle_identical(device)) {
                return false;
            }
        }
        break;
    case Joycon::ControllerType::Right:
        for (const auto& device : right_joycons) {
            if (is_handle_identical(device)) {
                return false;
            }
        }
        break;
    default:
        return false;
    }

    return true;
}

void Joycons::RegisterNewDevice(SDL_hid_device_info* device_info) {
    Joycon::ControllerType type{};
    auto result = Joycon::JoyconDriver::GetDeviceType(device_info, type);
    auto handle = GetNextFreeHandle(type);
    if (handle == nullptr) {
        LOG_WARNING(Input, "No free handles available");
        return;
    }
    if (result == Joycon::DriverResult::Success) {
        result = handle->RequestDeviceAccess(device_info);
    }
    if (result == Joycon::DriverResult::Success) {
        LOG_WARNING(Input, "Initialize device");

        const std::size_t port = handle->GetDevicePort();
        const Joycon::JoyconCallbacks callbacks{
            .on_battery_data = {[this, port, type](Joycon::Battery value) {
                OnBatteryUpdate(port, type, value);
            }},
            .on_color_data = {[this, port, type](Joycon::Color value) {
                OnColorUpdate(port, type, value);
            }},
            .on_button_data = {[this, port, type](int id, bool value) {
                OnButtonUpdate(port, type, id, value);
            }},
            .on_stick_data = {[this, port, type](int id, f32 value) {
                OnStickUpdate(port, type, id, value);
            }},
            .on_motion_data = {[this, port, type](int id, const Joycon::MotionData& value) {
                OnMotionUpdate(port, type, id, value);
            }},
            .on_ring_data = {[this](f32 ring_data) { OnRingConUpdate(ring_data); }},
            .on_amiibo_data = {[this, port](const std::vector<u8>& amiibo_data) {
                OnAmiiboUpdate(port, amiibo_data);
            }},
            .on_camera_data = {[this, port](const std::vector<u8>& camera_data,
                                            Joycon::IrsResolution format) {
                OnCameraUpdate(port, camera_data, format);
            }},
        };

        handle->InitializeDevice();
        handle->SetCallbacks(callbacks);
    }
}

std::shared_ptr<Joycon::JoyconDriver> Joycons::GetNextFreeHandle(
    Joycon::ControllerType type) const {
    if (type == Joycon::ControllerType::Left) {
        const auto unconnected_device =
            std::ranges::find_if(left_joycons, [](auto& device) { return !device->IsConnected(); });
        if (unconnected_device != left_joycons.end()) {
            return *unconnected_device;
        }
    }
    if (type == Joycon::ControllerType::Right) {
        const auto unconnected_device = std::ranges::find_if(
            right_joycons, [](auto& device) { return !device->IsConnected(); });

        if (unconnected_device != right_joycons.end()) {
            return *unconnected_device;
        }
    }
    return nullptr;
}

bool Joycons::IsVibrationEnabled(const PadIdentifier& identifier) {
    const auto handle = GetHandle(identifier);
    if (handle == nullptr) {
        return false;
    }
    return handle->IsVibrationEnabled();
}

Common::Input::DriverResult Joycons::SetVibration(const PadIdentifier& identifier,
                                                  const Common::Input::VibrationStatus& vibration) {
    const Joycon::VibrationValue native_vibration{
        .low_amplitude = vibration.low_amplitude,
        .low_frequency = vibration.low_frequency,
        .high_amplitude = vibration.high_amplitude,
        .high_frequency = vibration.high_frequency,
    };
    auto handle = GetHandle(identifier);
    if (handle == nullptr) {
        return Common::Input::DriverResult::InvalidHandle;
    }

    handle->SetVibration(native_vibration);
    return Common::Input::DriverResult::Success;
}

Common::Input::DriverResult Joycons::SetLeds(const PadIdentifier& identifier,
                                             const Common::Input::LedStatus& led_status) {
    auto handle = GetHandle(identifier);
    if (handle == nullptr) {
        return Common::Input::DriverResult::InvalidHandle;
    }
    int led_config = led_status.led_1 ? 1 : 0;
    led_config += led_status.led_2 ? 2 : 0;
    led_config += led_status.led_3 ? 4 : 0;
    led_config += led_status.led_4 ? 8 : 0;

    return static_cast<Common::Input::DriverResult>(
        handle->SetLedConfig(static_cast<u8>(led_config)));
}

Common::Input::DriverResult Joycons::SetCameraFormat(const PadIdentifier& identifier,
                                                     Common::Input::CameraFormat camera_format) {
    auto handle = GetHandle(identifier);
    if (handle == nullptr) {
        return Common::Input::DriverResult::InvalidHandle;
    }
    return static_cast<Common::Input::DriverResult>(handle->SetIrsConfig(
        Joycon::IrsMode::ImageTransfer, static_cast<Joycon::IrsResolution>(camera_format)));
};

Common::Input::NfcState Joycons::SupportsNfc(const PadIdentifier& identifier_) const {
    return Common::Input::NfcState::Success;
};

Common::Input::NfcState Joycons::WriteNfcData(const PadIdentifier& identifier_,
                                              const std::vector<u8>& data) {
    return Common::Input::NfcState::NotSupported;
};

Common::Input::DriverResult Joycons::SetPollingMode(const PadIdentifier& identifier,
                                                    const Common::Input::PollingMode polling_mode) {
    auto handle = GetHandle(identifier);
    if (handle == nullptr) {
        LOG_ERROR(Input, "Invalid handle {}", identifier.port);
        return Common::Input::DriverResult::InvalidHandle;
    }

    switch (polling_mode) {
    case Common::Input::PollingMode::Active:
        return static_cast<Common::Input::DriverResult>(handle->SetActiveMode());
    case Common::Input::PollingMode::Pasive:
        return static_cast<Common::Input::DriverResult>(handle->SetPasiveMode());
    case Common::Input::PollingMode::IR:
        return static_cast<Common::Input::DriverResult>(handle->SetIrMode());
    case Common::Input::PollingMode::NFC:
        return static_cast<Common::Input::DriverResult>(handle->SetNfcMode());
    case Common::Input::PollingMode::Ring:
        return static_cast<Common::Input::DriverResult>(handle->SetRingConMode());
    default:
        return Common::Input::DriverResult::NotSupported;
    }
}

void Joycons::OnBatteryUpdate(std::size_t port, Joycon::ControllerType type,
                              Joycon::Battery value) {
    const auto identifier = GetIdentifier(port, type);
    if (value.charging != 0) {
        SetBattery(identifier, Common::Input::BatteryLevel::Charging);
        return;
    }

    Common::Input::BatteryLevel battery{};
    switch (value.status) {
    case 0:
        battery = Common::Input::BatteryLevel::Empty;
        break;
    case 1:
        battery = Common::Input::BatteryLevel::Critical;
        break;
    case 2:
        battery = Common::Input::BatteryLevel::Low;
        break;
    case 3:
        battery = Common::Input::BatteryLevel::Medium;
        break;
    case 4:
    default:
        battery = Common::Input::BatteryLevel::Full;
        break;
    }
    SetBattery(identifier, battery);
}

void Joycons::OnColorUpdate(std::size_t port, Joycon::ControllerType type,
                            const Joycon::Color& value) {
    const auto identifier = GetIdentifier(port, type);
    Common::Input::BodyColorStatus color{
        .body = value.body,
        .buttons = value.buttons,
        .left_grip = value.left_grip,
        .right_grip = value.right_grip,
    };
    SetColor(identifier, color);
}

void Joycons::OnButtonUpdate(std::size_t port, Joycon::ControllerType type, int id, bool value) {
    const auto identifier = GetIdentifier(port, type);
    SetButton(identifier, id, value);
}

void Joycons::OnStickUpdate(std::size_t port, Joycon::ControllerType type, int id, f32 value) {
    const auto identifier = GetIdentifier(port, type);
    SetAxis(identifier, id, value);
}

void Joycons::OnMotionUpdate(std::size_t port, Joycon::ControllerType type, int id,
                             const Joycon::MotionData& value) {
    const auto identifier = GetIdentifier(port, type);
    BasicMotion motion_data{
        .gyro_x = value.gyro_x,
        .gyro_y = value.gyro_y,
        .gyro_z = value.gyro_z,
        .accel_x = value.accel_x,
        .accel_y = value.accel_y,
        .accel_z = value.accel_z,
        .delta_timestamp = 15000,
    };
    SetMotion(identifier, id, motion_data);
}

void Joycons::OnRingConUpdate(f32 ring_data) {
    // To simplify ring detection it will always be mapped to an empty identifier for all
    // controllers
    constexpr PadIdentifier identifier = {
        .guid = Common::UUID{},
        .port = 0,
        .pad = 0,
    };
    SetAxis(identifier, 100, ring_data);
}

void Joycons::OnAmiiboUpdate(std::size_t port, const std::vector<u8>& amiibo_data) {
    const auto identifier = GetIdentifier(port, Joycon::ControllerType::Right);
    const auto nfc_state = amiibo_data.empty() ? Common::Input::NfcState::AmiiboRemoved
                                               : Common::Input::NfcState::NewAmiibo;
    SetNfc(identifier, {nfc_state, amiibo_data});
}

void Joycons::OnCameraUpdate(std::size_t port, const std::vector<u8>& camera_data,
                             Joycon::IrsResolution format) {
    const auto identifier = GetIdentifier(port, Joycon::ControllerType::Right);
    SetCamera(identifier, {static_cast<Common::Input::CameraFormat>(format), camera_data});
}

std::shared_ptr<Joycon::JoyconDriver> Joycons::GetHandle(PadIdentifier identifier) const {
    auto is_handle_active = [&](std::shared_ptr<Joycon::JoyconDriver> device) {
        if (!device) {
            return false;
        }
        if (!device->IsConnected()) {
            return false;
        }
        if (device->GetDevicePort() == identifier.port) {
            return true;
        }
        return false;
    };
    const auto type = static_cast<Joycon::ControllerType>(identifier.pad);

    if (type == Joycon::ControllerType::Left) {
        const auto matching_device = std::ranges::find_if(
            left_joycons, [is_handle_active](auto& device) { return is_handle_active(device); });

        if (matching_device != left_joycons.end()) {
            return *matching_device;
        }
    }

    if (type == Joycon::ControllerType::Right) {
        const auto matching_device = std::ranges::find_if(
            right_joycons, [is_handle_active](auto& device) { return is_handle_active(device); });

        if (matching_device != right_joycons.end()) {
            return *matching_device;
        }
    }

    return nullptr;
}

PadIdentifier Joycons::GetIdentifier(std::size_t port, Joycon::ControllerType type) const {
    const std::array<u8, 16> guid{0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, static_cast<u8>(type)};
    return {
        .guid = Common::UUID{guid},
        .port = port,
        .pad = static_cast<std::size_t>(type),
    };
}

Common::ParamPackage Joycons::GetParamPackage(std::size_t port, Joycon::ControllerType type) const {
    const auto identifier = GetIdentifier(port, type);
    return {
        {"engine", GetEngineName()},
        {"guid", identifier.guid.RawString()},
        {"port", std::to_string(identifier.port)},
        {"pad", std::to_string(identifier.pad)},
    };
}

std::vector<Common::ParamPackage> Joycons::GetInputDevices() const {
    std::vector<Common::ParamPackage> devices{};

    auto add_entry = [&](std::shared_ptr<Joycon::JoyconDriver> device) {
        if (!device) {
            return;
        }
        if (!device->IsConnected()) {
            return;
        }
        auto param = GetParamPackage(device->GetDevicePort(), device->GetHandleDeviceType());
        std::string name = fmt::format("{} {}", JoyconName(device->GetHandleDeviceType()),
                                       device->GetDevicePort() + 1);
        param.Set("display", std::move(name));
        devices.emplace_back(param);
    };

    for (const auto& controller : left_joycons) {
        add_entry(controller);
    }
    for (const auto& controller : right_joycons) {
        add_entry(controller);
    }

    // List dual joycon pairs
    for (std::size_t i = 0; i < MaxSupportedControllers; i++) {
        if (!left_joycons[i] || !right_joycons[i]) {
            continue;
        }
        if (!left_joycons[i]->IsConnected() || !right_joycons[i]->IsConnected()) {
            continue;
        }
        auto main_param = GetParamPackage(i, left_joycons[i]->GetHandleDeviceType());
        const auto second_param = GetParamPackage(i, right_joycons[i]->GetHandleDeviceType());
        const auto type = Joycon::ControllerType::Dual;
        std::string name = fmt::format("{} {}", JoyconName(type), i + 1);

        main_param.Set("display", std::move(name));
        main_param.Set("guid2", second_param.Get("guid", ""));
        main_param.Set("pad", std::to_string(static_cast<size_t>(type)));
        devices.emplace_back(main_param);
    }

    return devices;
}

ButtonMapping Joycons::GetButtonMappingForDevice(const Common::ParamPackage& params) {
    static constexpr std::array<std::tuple<Settings::NativeButton::Values, Joycon::PadButton, bool>,
                                18>
        switch_to_joycon_button = {
            std::tuple{Settings::NativeButton::A, Joycon::PadButton::A, true},
            {Settings::NativeButton::B, Joycon::PadButton::B, true},
            {Settings::NativeButton::X, Joycon::PadButton::X, true},
            {Settings::NativeButton::Y, Joycon::PadButton::Y, true},
            {Settings::NativeButton::DLeft, Joycon::PadButton::Left, false},
            {Settings::NativeButton::DUp, Joycon::PadButton::Up, false},
            {Settings::NativeButton::DRight, Joycon::PadButton::Right, false},
            {Settings::NativeButton::DDown, Joycon::PadButton::Down, false},
            {Settings::NativeButton::L, Joycon::PadButton::L, false},
            {Settings::NativeButton::R, Joycon::PadButton::R, true},
            {Settings::NativeButton::ZL, Joycon::PadButton::ZL, false},
            {Settings::NativeButton::ZR, Joycon::PadButton::ZR, true},
            {Settings::NativeButton::Plus, Joycon::PadButton::Plus, true},
            {Settings::NativeButton::Minus, Joycon::PadButton::Minus, false},
            {Settings::NativeButton::Home, Joycon::PadButton::Home, true},
            {Settings::NativeButton::Screenshot, Joycon::PadButton::Capture, false},
            {Settings::NativeButton::LStick, Joycon::PadButton::StickL, false},
            {Settings::NativeButton::RStick, Joycon::PadButton::StickR, true},
        };

    if (!params.Has("port")) {
        return {};
    }

    ButtonMapping mapping{};
    for (const auto& [switch_button, joycon_button, side] : switch_to_joycon_button) {
        const std::size_t port = static_cast<std::size_t>(params.Get("port", 0));
        auto pad = static_cast<Joycon::ControllerType>(params.Get("pad", 0));
        if (pad == Joycon::ControllerType::Dual) {
            pad = side ? Joycon::ControllerType::Right : Joycon::ControllerType::Left;
        }

        Common::ParamPackage button_params = GetParamPackage(port, pad);
        button_params.Set("button", static_cast<int>(joycon_button));
        mapping.insert_or_assign(switch_button, std::move(button_params));
    }

    // Map SL and SR buttons for left joycons
    if (params.Get("pad", 0) == static_cast<int>(Joycon::ControllerType::Left)) {
        const std::size_t port = static_cast<std::size_t>(params.Get("port", 0));
        Common::ParamPackage button_params = GetParamPackage(port, Joycon::ControllerType::Left);

        Common::ParamPackage sl_button_params = button_params;
        Common::ParamPackage sr_button_params = button_params;
        sl_button_params.Set("button", static_cast<int>(Joycon::PadButton::LeftSL));
        sr_button_params.Set("button", static_cast<int>(Joycon::PadButton::LeftSR));
        mapping.insert_or_assign(Settings::NativeButton::SL, std::move(sl_button_params));
        mapping.insert_or_assign(Settings::NativeButton::SR, std::move(sr_button_params));
    }

    // Map SL and SR buttons for right joycons
    if (params.Get("pad", 0) == static_cast<int>(Joycon::ControllerType::Right)) {
        const std::size_t port = static_cast<std::size_t>(params.Get("port", 0));
        Common::ParamPackage button_params = GetParamPackage(port, Joycon::ControllerType::Right);

        Common::ParamPackage sl_button_params = button_params;
        Common::ParamPackage sr_button_params = button_params;
        sl_button_params.Set("button", static_cast<int>(Joycon::PadButton::RightSL));
        sr_button_params.Set("button", static_cast<int>(Joycon::PadButton::RightSR));
        mapping.insert_or_assign(Settings::NativeButton::SL, std::move(sl_button_params));
        mapping.insert_or_assign(Settings::NativeButton::SR, std::move(sr_button_params));
    }

    return mapping;
}

AnalogMapping Joycons::GetAnalogMappingForDevice(const Common::ParamPackage& params) {
    if (!params.Has("port")) {
        return {};
    }

    const std::size_t port = static_cast<std::size_t>(params.Get("port", 0));
    auto pad_left = static_cast<Joycon::ControllerType>(params.Get("pad", 0));
    auto pad_right = pad_left;
    if (pad_left == Joycon::ControllerType::Dual) {
        pad_left = Joycon::ControllerType::Left;
        pad_right = Joycon::ControllerType::Right;
    }

    AnalogMapping mapping = {};
    Common::ParamPackage left_analog_params = GetParamPackage(port, pad_left);
    left_analog_params.Set("axis_x", static_cast<int>(Joycon::PadAxes::LeftStickX));
    left_analog_params.Set("axis_y", static_cast<int>(Joycon::PadAxes::LeftStickY));
    mapping.insert_or_assign(Settings::NativeAnalog::LStick, std::move(left_analog_params));
    Common::ParamPackage right_analog_params = GetParamPackage(port, pad_right);
    right_analog_params.Set("axis_x", static_cast<int>(Joycon::PadAxes::RightStickX));
    right_analog_params.Set("axis_y", static_cast<int>(Joycon::PadAxes::RightStickY));
    mapping.insert_or_assign(Settings::NativeAnalog::RStick, std::move(right_analog_params));
    return mapping;
}

MotionMapping Joycons::GetMotionMappingForDevice(const Common::ParamPackage& params) {
    if (!params.Has("port")) {
        return {};
    }

    const std::size_t port = static_cast<std::size_t>(params.Get("port", 0));
    auto pad_left = static_cast<Joycon::ControllerType>(params.Get("pad", 0));
    auto pad_right = pad_left;
    if (pad_left == Joycon::ControllerType::Dual) {
        pad_left = Joycon::ControllerType::Left;
        pad_right = Joycon::ControllerType::Right;
    }

    MotionMapping mapping = {};
    Common::ParamPackage left_motion_params = GetParamPackage(port, pad_left);
    left_motion_params.Set("motion", 0);
    mapping.insert_or_assign(Settings::NativeMotion::MotionLeft, std::move(left_motion_params));
    Common::ParamPackage right_Motion_params = GetParamPackage(port, pad_right);
    right_Motion_params.Set("motion", 1);
    mapping.insert_or_assign(Settings::NativeMotion::MotionRight, std::move(right_Motion_params));
    return mapping;
}

Common::Input::ButtonNames Joycons::GetUIButtonName(const Common::ParamPackage& params) const {
    const auto button = static_cast<Joycon::PadButton>(params.Get("button", 0));
    switch (button) {
    case Joycon::PadButton::Left:
        return Common::Input::ButtonNames::ButtonLeft;
    case Joycon::PadButton::Right:
        return Common::Input::ButtonNames::ButtonRight;
    case Joycon::PadButton::Down:
        return Common::Input::ButtonNames::ButtonDown;
    case Joycon::PadButton::Up:
        return Common::Input::ButtonNames::ButtonUp;
    case Joycon::PadButton::LeftSL:
    case Joycon::PadButton::RightSL:
        return Common::Input::ButtonNames::TriggerSL;
    case Joycon::PadButton::LeftSR:
    case Joycon::PadButton::RightSR:
        return Common::Input::ButtonNames::TriggerSR;
    case Joycon::PadButton::L:
        return Common::Input::ButtonNames::TriggerL;
    case Joycon::PadButton::R:
        return Common::Input::ButtonNames::TriggerR;
    case Joycon::PadButton::ZL:
        return Common::Input::ButtonNames::TriggerZL;
    case Joycon::PadButton::ZR:
        return Common::Input::ButtonNames::TriggerZR;
    case Joycon::PadButton::A:
        return Common::Input::ButtonNames::ButtonA;
    case Joycon::PadButton::B:
        return Common::Input::ButtonNames::ButtonB;
    case Joycon::PadButton::X:
        return Common::Input::ButtonNames::ButtonX;
    case Joycon::PadButton::Y:
        return Common::Input::ButtonNames::ButtonY;
    case Joycon::PadButton::Plus:
        return Common::Input::ButtonNames::ButtonPlus;
    case Joycon::PadButton::Minus:
        return Common::Input::ButtonNames::ButtonMinus;
    case Joycon::PadButton::Home:
        return Common::Input::ButtonNames::ButtonHome;
    case Joycon::PadButton::Capture:
        return Common::Input::ButtonNames::ButtonCapture;
    case Joycon::PadButton::StickL:
        return Common::Input::ButtonNames::ButtonStickL;
    case Joycon::PadButton::StickR:
        return Common::Input::ButtonNames::ButtonStickR;
    default:
        return Common::Input::ButtonNames::Undefined;
    }
}

Common::Input::ButtonNames Joycons::GetUIName(const Common::ParamPackage& params) const {
    if (params.Has("button")) {
        return GetUIButtonName(params);
    }
    if (params.Has("axis")) {
        return Common::Input::ButtonNames::Value;
    }
    if (params.Has("motion")) {
        return Common::Input::ButtonNames::Engine;
    }

    return Common::Input::ButtonNames::Invalid;
}

std::string Joycons::JoyconName(Joycon::ControllerType type) const {
    switch (type) {
    case Joycon::ControllerType::Left:
        return "Left Joycon";
    case Joycon::ControllerType::Right:
        return "Right Joycon";
    case Joycon::ControllerType::Pro:
        return "Pro Controller";
    case Joycon::ControllerType::Grip:
        return "Grip Controller";
    case Joycon::ControllerType::Dual:
        return "Dual Joycon";
    default:
        return "Unknown Joycon";
    }
}
} // namespace InputCommon

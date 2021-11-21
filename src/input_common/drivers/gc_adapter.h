// Copyright 2014 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>

#include "input_common/input_engine.h"

struct libusb_context;
struct libusb_device;
struct libusb_device_handle;

namespace InputCommon {

class LibUSBContext;
class LibUSBDeviceHandle;

class GCAdapter : public InputCommon::InputEngine {
public:
    explicit GCAdapter(const std::string& input_engine_);
    ~GCAdapter();

    Common::Input::VibrationError SetRumble(
        const PadIdentifier& identifier, const Common::Input::VibrationStatus vibration) override;

    /// Used for automapping features
    std::vector<Common::ParamPackage> GetInputDevices() const override;
    ButtonMapping GetButtonMappingForDevice(const Common::ParamPackage& params) override;
    AnalogMapping GetAnalogMappingForDevice(const Common::ParamPackage& params) override;
    Common::Input::ButtonNames GetUIName(const Common::ParamPackage& params) const override;

private:
    enum class PadButton {
        Undefined = 0x0000,
        ButtonLeft = 0x0001,
        ButtonRight = 0x0002,
        ButtonDown = 0x0004,
        ButtonUp = 0x0008,
        TriggerZ = 0x0010,
        TriggerR = 0x0020,
        TriggerL = 0x0040,
        ButtonA = 0x0100,
        ButtonB = 0x0200,
        ButtonX = 0x0400,
        ButtonY = 0x0800,
        ButtonStart = 0x1000,
    };

    enum class PadAxes : u8 {
        StickX,
        StickY,
        SubstickX,
        SubstickY,
        TriggerLeft,
        TriggerRight,
        Undefined,
    };

    enum class ControllerTypes {
        None,
        Wired,
        Wireless,
    };

    struct GCController {
        ControllerTypes type = ControllerTypes::None;
        PadIdentifier identifier{};
        bool enable_vibration = false;
        u8 rumble_amplitude{};
        std::array<u8, 6> axis_origin{};
        u8 reset_origin_counter{};
    };

    using AdapterPayload = std::array<u8, 37>;

    void UpdatePadType(std::size_t port, ControllerTypes pad_type);
    void UpdateControllers(const AdapterPayload& adapter_payload);
    void UpdateStateButtons(std::size_t port, u8 b1, u8 b2);
    void UpdateStateAxes(std::size_t port, const AdapterPayload& adapter_payload);

    void AdapterInputThread(std::stop_token stop_token);

    void AdapterScanThread(std::stop_token stop_token);

    bool IsPayloadCorrect(const AdapterPayload& adapter_payload, s32 payload_size);

    /// For use in initialization, querying devices to find the adapter
    bool Setup();

    /// Returns true if we successfully gain access to GC Adapter
    bool CheckDeviceAccess();

    /// Captures GC Adapter endpoint address
    /// Returns true if the endpoint was set correctly
    bool GetGCEndpoint(libusb_device* device);

    /// Returns true if there is a device connected to port
    bool DeviceConnected(std::size_t port) const;

    /// For shutting down, clear all data, join all threads, release usb
    void Reset();

    void UpdateVibrations();

    /// Updates vibration state of all controllers
    void SendVibrations();

    Common::Input::ButtonNames GetUIButtonName(const Common::ParamPackage& params) const;

    std::unique_ptr<LibUSBDeviceHandle> usb_adapter_handle;
    std::array<GCController, 4> pads;

    std::jthread adapter_input_thread;
    std::jthread adapter_scan_thread;
    bool restart_scan_thread{};

    std::unique_ptr<LibUSBContext> libusb_ctx;

    u8 input_endpoint{0};
    u8 output_endpoint{0};
    u8 input_error_counter{0};
    u8 output_error_counter{0};
    int vibration_counter{0};

    bool rumble_enabled{true};
    bool vibration_changed{true};
};
} // namespace InputCommon

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <span>
#include <thread>
#include <SDL_hidapi.h>

#include "input_common/input_engine.h"

namespace InputCommon::Joycon {
using SerialNumber = std::array<u8, 15>;
struct Battery;
struct Color;
struct MotionData;
enum class ControllerType;
enum class DriverResult;
class JoyconDriver;
} // namespace InputCommon::Joycon

namespace InputCommon {

class Joycons final : public InputCommon::InputEngine {
public:
    explicit Joycons(const std::string& input_engine_);

    ~Joycons();

    bool IsVibrationEnabled(const PadIdentifier& identifier) override;
    Common::Input::DriverResult SetVibration(
        const PadIdentifier& identifier, const Common::Input::VibrationStatus& vibration) override;

    Common::Input::DriverResult SetLeds(const PadIdentifier& identifier,
                                        const Common::Input::LedStatus& led_status) override;

    Common::Input::DriverResult SetCameraFormat(const PadIdentifier& identifier_,
                                                Common::Input::CameraFormat camera_format) override;

    Common::Input::NfcState SupportsNfc(const PadIdentifier& identifier_) const override;
    Common::Input::NfcState WriteNfcData(const PadIdentifier& identifier_,
                                         const std::vector<u8>& data) override;

    Common::Input::DriverResult SetPollingMode(
        const PadIdentifier& identifier, const Common::Input::PollingMode polling_mode) override;

    /// Used for automapping features
    std::vector<Common::ParamPackage> GetInputDevices() const override;
    ButtonMapping GetButtonMappingForDevice(const Common::ParamPackage& params) override;
    AnalogMapping GetAnalogMappingForDevice(const Common::ParamPackage& params) override;
    MotionMapping GetMotionMappingForDevice(const Common::ParamPackage& params) override;
    Common::Input::ButtonNames GetUIName(const Common::ParamPackage& params) const override;

private:
    static constexpr std::size_t MaxSupportedControllers = 8;

    /// For shutting down, clear all data, join all threads, release usb devices
    void Reset();

    /// Registers controllers, clears all data and starts the scan thread
    void Setup();

    /// Actively searchs for new devices
    void ScanThread(std::stop_token stop_token);

    /// Returns true if device is valid and not registered
    bool IsDeviceNew(SDL_hid_device_info* device_info) const;

    /// Tries to connect to the new device
    void RegisterNewDevice(SDL_hid_device_info* device_info);

    /// Returns the next free handle
    std::shared_ptr<Joycon::JoyconDriver> GetNextFreeHandle(Joycon::ControllerType type) const;

    void OnBatteryUpdate(std::size_t port, Joycon::ControllerType type, Joycon::Battery value);
    void OnColorUpdate(std::size_t port, Joycon::ControllerType type, const Joycon::Color& value);
    void OnButtonUpdate(std::size_t port, Joycon::ControllerType type, int id, bool value);
    void OnStickUpdate(std::size_t port, Joycon::ControllerType type, int id, f32 value);
    void OnMotionUpdate(std::size_t port, Joycon::ControllerType type, int id,
                        const Joycon::MotionData& value);
    void OnRingConUpdate(f32 ring_data);
    void OnAmiiboUpdate(std::size_t port, const std::vector<u8>& amiibo_data);

    /// Returns a JoyconHandle corresponding to a PadIdentifier
    std::shared_ptr<Joycon::JoyconDriver> GetHandle(PadIdentifier identifier) const;

    /// Returns a PadIdentifier corresponding to the port number
    PadIdentifier GetIdentifier(std::size_t port, Joycon::ControllerType type) const;

    std::string JoyconName(std::size_t port) const;

    Common::Input::ButtonNames GetUIButtonName(const Common::ParamPackage& params) const;

    /// Returns the name of the device in text format
    std::string JoyconName(Joycon::ControllerType type) const;

    std::jthread scan_thread;
    bool scan_thread_running{};

    // Joycon types are split by type to ease supporting dualjoycon configurations
    std::array<std::shared_ptr<Joycon::JoyconDriver>, MaxSupportedControllers> left_joycons{};
    std::array<std::shared_ptr<Joycon::JoyconDriver>, MaxSupportedControllers> right_joycons{};
    std::array<std::shared_ptr<Joycon::JoyconDriver>, MaxSupportedControllers> pro_joycons{};
};

} // namespace InputCommon

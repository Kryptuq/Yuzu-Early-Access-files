// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include "common/logging/log.h"
#include "common/param_package.h"
#include "common/uuid.h"

namespace Common::Input {

enum class InputType {
    None,
    Battery,
    Button,
    Stick,
    Analog,
    Trigger,
    Motion,
    Touch,
    Color,
    Vibration,
    Nfc,
    Ir,
};

enum class BatteryLevel : u32 {
    None,
    Empty,
    Critical,
    Low,
    Medium,
    Full,
    Charging,
};

enum class PollingMode {
    Active,
    Pasive,
    Camera,
    NCF,
    IR,
};

enum class VibrationError {
    None,
    NotSupported,
    Disabled,
    Unknown,
};

enum class PollingError {
    None,
    NotSupported,
    Unknown,
};

// Hint for amplification curve to be used
enum class VibrationAmplificationType {
    Linear,
    Exponential,
};

struct AnalogProperties {
    float deadzone{};
    float range{1.0f};
    float threshold{0.5f};
    float offset{};
    bool inverted{};
};

struct AnalogStatus {
    float value{};
    float raw_value{};
    AnalogProperties properties{};
};

struct ButtonStatus {
    Common::UUID uuid{};
    bool value{};
    bool inverted{};
    bool toggle{};
    bool locked{};
};

using BatteryStatus = BatteryLevel;

struct StickStatus {
    Common::UUID uuid{};
    AnalogStatus x{};
    AnalogStatus y{};
    bool left{};
    bool right{};
    bool up{};
    bool down{};
};

struct TriggerStatus {
    Common::UUID uuid{};
    AnalogStatus analog{};
    ButtonStatus pressed{};
};

struct MotionSensor {
    AnalogStatus x{};
    AnalogStatus y{};
    AnalogStatus z{};
};

struct MotionStatus {
    // Gyroscope vector measurement in radians/s.
    MotionSensor gyro{};
    // Acceleration vector measurement in G force
    MotionSensor accel{};
    // Time since last measurement in microseconds
    u64 delta_timestamp{};
    // Request to update after reading the value
    bool force_update{};
};

struct TouchStatus {
    ButtonStatus pressed{};
    AnalogStatus x{};
    AnalogStatus y{};
    int id{};
};

struct BodyColorStatus {
    u32 body{};
    u32 buttons{};
};

struct VibrationStatus {
    f32 low_amplitude{};
    f32 low_frequency{};
    f32 high_amplitude{};
    f32 high_frequency{};
    VibrationAmplificationType type;
};

struct LedStatus {
    bool led_1{};
    bool led_2{};
    bool led_3{};
    bool led_4{};
};

struct CallbackStatus {
    InputType type{InputType::None};
    ButtonStatus button_status{};
    StickStatus stick_status{};
    AnalogStatus analog_status{};
    TriggerStatus trigger_status{};
    MotionStatus motion_status{};
    TouchStatus touch_status{};
    BodyColorStatus color_status{};
    BatteryStatus battery_status{};
    VibrationStatus vibration_status{};
};

struct InputCallback {
    std::function<void(CallbackStatus)> on_change;
};

/// An abstract class template for an input device (a button, an analog input, etc.).
class InputDevice {
public:
    virtual ~InputDevice() = default;

    // Request input device to update if necessary
    virtual void SoftUpdate() {
        return;
    }

    // Force input device to update data regarless of the current state
    virtual void ForceUpdate() {
        return;
    }

    void SetCallback(InputCallback callback_) {
        callback = std::move(callback_);
    }

    void TriggerOnChange(CallbackStatus status) {
        if (callback.on_change) {
            callback.on_change(status);
        }
    }

private:
    InputCallback callback;
};

/// An abstract class template for an output device (rumble, LED pattern, polling mode).
class OutputDevice {
public:
    virtual ~OutputDevice() = default;

    virtual void SetLED([[maybe_unused]] LedStatus led_status) {
        return;
    }

    virtual VibrationError SetVibration([[maybe_unused]] VibrationStatus vibration_status) {
        return VibrationError::NotSupported;
    }

    virtual PollingError SetPollingMode([[maybe_unused]] PollingMode polling_mode) {
        return PollingError::NotSupported;
    }
};

/// An abstract class template for a factory that can create input devices.
template <typename InputDeviceType>
class Factory {
public:
    virtual ~Factory() = default;
    virtual std::unique_ptr<InputDeviceType> Create(const Common::ParamPackage&) = 0;
};

namespace Impl {

template <typename InputDeviceType>
using FactoryListType = std::unordered_map<std::string, std::shared_ptr<Factory<InputDeviceType>>>;

template <typename InputDeviceType>
struct FactoryList {
    static FactoryListType<InputDeviceType> list;
};

template <typename InputDeviceType>
FactoryListType<InputDeviceType> FactoryList<InputDeviceType>::list;

} // namespace Impl

/**
 * Registers an input device factory.
 * @tparam InputDeviceType the type of input devices the factory can create
 * @param name the name of the factory. Will be used to match the "engine" parameter when creating
 *     a device
 * @param factory the factory object to register
 */
template <typename InputDeviceType>
void RegisterFactory(const std::string& name, std::shared_ptr<Factory<InputDeviceType>> factory) {
    auto pair = std::make_pair(name, std::move(factory));
    if (!Impl::FactoryList<InputDeviceType>::list.insert(std::move(pair)).second) {
        LOG_ERROR(Input, "Factory '{}' already registered", name);
    }
}

/**
 * Unregisters an input device factory.
 * @tparam InputDeviceType the type of input devices the factory can create
 * @param name the name of the factory to unregister
 */
template <typename InputDeviceType>
void UnregisterFactory(const std::string& name) {
    if (Impl::FactoryList<InputDeviceType>::list.erase(name) == 0) {
        LOG_ERROR(Input, "Factory '{}' not registered", name);
    }
}

/**
 * Create an input device from given paramters.
 * @tparam InputDeviceType the type of input devices to create
 * @param params a serialized ParamPackage string that contains all parameters for creating the
 * device
 */
template <typename InputDeviceType>
std::unique_ptr<InputDeviceType> CreateDeviceFromString(const std::string& params) {
    const Common::ParamPackage package(params);
    const std::string engine = package.Get("engine", "null");
    const auto& factory_list = Impl::FactoryList<InputDeviceType>::list;
    const auto pair = factory_list.find(engine);
    if (pair == factory_list.end()) {
        if (engine != "null") {
            LOG_ERROR(Input, "Unknown engine name: {}", engine);
        }
        return std::make_unique<InputDeviceType>();
    }
    return pair->second->Create(package);
}

/**
 * Create an input device from given paramters.
 * @tparam InputDeviceType the type of input devices to create
 * @param A ParamPackage that contains all parameters for creating the device
 */
template <typename InputDeviceType>
std::unique_ptr<InputDeviceType> CreateDevice(const Common::ParamPackage package) {
    const std::string engine = package.Get("engine", "null");
    const auto& factory_list = Impl::FactoryList<InputDeviceType>::list;
    const auto pair = factory_list.find(engine);
    if (pair == factory_list.end()) {
        if (engine != "null") {
            LOG_ERROR(Input, "Unknown engine name: {}", engine);
        }
        return std::make_unique<InputDeviceType>();
    }
    return pair->second->Create(package);
}

} // namespace Common::Input

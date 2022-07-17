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

// Type of data that is expected to recieve or send
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
    IrSensor,
};

// Internal battery charge level
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
    // Constant polling of buttons, analogs and motion data
    Active,
    // Only update on button change, digital analogs
    Pasive,
    // Enable near field communication polling
    NFC,
    // Enable infrared camera polling
    IR,
};

enum class CameraFormat {
    Size320x240,
    Size160x120,
    Size80x60,
    Size40x30,
    Size20x15,
    None,
};

// Vibration reply from the controller
enum class VibrationError {
    None,
    NotSupported,
    Disabled,
    Unknown,
};

// Polling mode reply from the controller
enum class PollingError {
    None,
    NotSupported,
    Unknown,
};

// Ir camera reply from the controller
enum class CameraError {
    None,
    NotSupported,
    Unknown,
};

// Hint for amplification curve to be used
enum class VibrationAmplificationType {
    Linear,
    Exponential,
    Test,
};

// Analog properties for calibration
struct AnalogProperties {
    // Anything below this value will be detected as zero
    float deadzone{};
    // Anyting above this values will be detected as one
    float range{1.0f};
    // Minimum value to be detected as active
    float threshold{0.5f};
    // Drift correction applied to the raw data
    float offset{};
    // Invert direction of the sensor data
    bool inverted{};
};

// Single analog sensor data
struct AnalogStatus {
    float value{};
    float raw_value{};
    AnalogProperties properties{};
};

// Button data
struct ButtonStatus {
    Common::UUID uuid{};
    bool value{};
    bool inverted{};
    bool toggle{};
    bool locked{};
};

// Internal battery data
using BatteryStatus = BatteryLevel;

// Analog and digital joystick data
struct StickStatus {
    Common::UUID uuid{};
    AnalogStatus x{};
    AnalogStatus y{};
    bool left{};
    bool right{};
    bool up{};
    bool down{};
};

// Analog and digital trigger data
struct TriggerStatus {
    Common::UUID uuid{};
    AnalogStatus analog{};
    ButtonStatus pressed{};
};

// 3D vector representing motion input
struct MotionSensor {
    AnalogStatus x{};
    AnalogStatus y{};
    AnalogStatus z{};
};

// Motion data used to calculate controller orientation
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

// Data of a single point on a touch screen
struct TouchStatus {
    ButtonStatus pressed{};
    AnalogStatus x{};
    AnalogStatus y{};
    int id{};
};

// Physical controller color in RGB format
struct BodyColorStatus {
    u32 body{};
    u32 buttons{};
};

// HD rumble data
struct VibrationStatus {
    f32 low_amplitude{};
    f32 low_frequency{};
    f32 high_amplitude{};
    f32 high_frequency{};
    VibrationAmplificationType type;
};

// Physical controller LED pattern
struct LedStatus {
    bool led_1{};
    bool led_2{};
    bool led_3{};
    bool led_4{};
};

// Raw data fom camera
struct CameraStatus {
    CameraFormat format{CameraFormat::None};
    std::vector<u8> data{};
};

// List of buttons to be passed to Qt that can be translated
enum class ButtonNames {
    Undefined,
    Invalid,
    // This will display the engine name instead of the button name
    Engine,
    // This will display the button by value instead of the button name
    Value,
    ButtonLeft,
    ButtonRight,
    ButtonDown,
    ButtonUp,
    TriggerZ,
    TriggerR,
    TriggerL,
    ButtonA,
    ButtonB,
    ButtonX,
    ButtonY,
    ButtonStart,

    // DS4 button names
    L1,
    L2,
    L3,
    R1,
    R2,
    R3,
    Circle,
    Cross,
    Square,
    Triangle,
    Share,
    Options,
    Home,
    Touch,

    // Mouse buttons
    ButtonMouseWheel,
    ButtonBackward,
    ButtonForward,
    ButtonTask,
    ButtonExtra,
};

// Callback data consisting of an input type and the equivalent data status
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
    CameraStatus camera_status{};
};

// Triggered once every input change
struct InputCallback {
    std::function<void(const CallbackStatus&)> on_change;
};

/// An abstract class template for an input device (a button, an analog input, etc.).
class InputDevice {
public:
    virtual ~InputDevice() = default;

    // Request input device to update if necessary
    virtual void SoftUpdate() {}

    // Force input device to update data regardless of the current state
    virtual void ForceUpdate() {}

    // Sets the function to be triggered when input changes
    void SetCallback(InputCallback callback_) {
        callback = std::move(callback_);
    }

    // Triggers the function set in the callback
    void TriggerOnChange(const CallbackStatus& status) {
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

    virtual void SetLED([[maybe_unused]] const LedStatus& led_status) {}

    virtual VibrationError SetVibration([[maybe_unused]] const VibrationStatus& vibration_status) {
        return VibrationError::NotSupported;
    }

    virtual PollingError SetPollingMode([[maybe_unused]] PollingMode polling_mode) {
        return PollingError::NotSupported;
    }

    virtual CameraError SetCameraFormat([[maybe_unused]] CameraFormat camera_format) {
        return CameraError::NotSupported;
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

// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#include "common/common_types.h"
#include "common/input.h"

#include "input_common/input_engine.h"
#include "input_common/input_poller.h"

namespace InputCommon {

class DummyInput final : public Common::Input::InputDevice {
public:
    explicit DummyInput() {}
    ~DummyInput() {}
};

class InputFromButton final : public Common::Input::InputDevice {
public:
    explicit InputFromButton(PadIdentifier identifier_, int button_, bool toggle_, bool inverted_,
                             InputEngine* input_engine_)
        : identifier(identifier_), button(button_), toggle(toggle_), inverted(inverted_),
          input_engine(input_engine_) {
        UpdateCallback engine_callback{[this]() { OnChange(); }};
        const InputIdentifier input_identifier{
            .identifier = identifier,
            .type = EngineInputType::Button,
            .index = button,
            .callback = engine_callback,
        };
        last_button_value = false;
        callback_key = input_engine->SetCallback(input_identifier);
    }

    ~InputFromButton() {
        input_engine->DeleteCallback(callback_key);
    }

    Common::Input::ButtonStatus GetStatus() const {
        return {
            .value = input_engine->GetButton(identifier, button),
            .inverted = inverted,
            .toggle = toggle,
        };
    }

    void ForceUpdate() {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Button,
            .button_status = GetStatus(),
        };

        last_button_value = status.button_status.value;
        TriggerOnChange(status);
    }

    void OnChange() {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Button,
            .button_status = GetStatus(),
        };

        if (status.button_status.value != last_button_value) {
            last_button_value = status.button_status.value;
            TriggerOnChange(status);
        }
    }

private:
    const PadIdentifier identifier;
    const int button;
    const bool toggle;
    const bool inverted;
    int callback_key;
    bool last_button_value;
    InputEngine* input_engine;
};

class InputFromHatButton final : public Common::Input::InputDevice {
public:
    explicit InputFromHatButton(PadIdentifier identifier_, int button_, u8 direction_, bool toggle_,
                                bool inverted_, InputEngine* input_engine_)
        : identifier(identifier_), button(button_), direction(direction_), toggle(toggle_),
          inverted(inverted_), input_engine(input_engine_) {
        UpdateCallback engine_callback{[this]() { OnChange(); }};
        const InputIdentifier input_identifier{
            .identifier = identifier,
            .type = EngineInputType::HatButton,
            .index = button,
            .callback = engine_callback,
        };
        last_button_value = false;
        callback_key = input_engine->SetCallback(input_identifier);
    }

    ~InputFromHatButton() {
        input_engine->DeleteCallback(callback_key);
    }

    Common::Input::ButtonStatus GetStatus() const {
        return {
            .value = input_engine->GetHatButton(identifier, button, direction),
            .inverted = inverted,
            .toggle = toggle,
        };
    }

    void ForceUpdate() {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Button,
            .button_status = GetStatus(),
        };

        last_button_value = status.button_status.value;
        TriggerOnChange(status);
    }

    void OnChange() {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Button,
            .button_status = GetStatus(),
        };

        if (status.button_status.value != last_button_value) {
            last_button_value = status.button_status.value;
            TriggerOnChange(status);
        }
    }

private:
    const PadIdentifier identifier;
    const int button;
    const u8 direction;
    const bool toggle;
    const bool inverted;
    int callback_key;
    bool last_button_value;
    InputEngine* input_engine;
};

class InputFromStick final : public Common::Input::InputDevice {
public:
    explicit InputFromStick(PadIdentifier identifier_, int axis_x_, int axis_y_,
                            Common::Input::AnalogProperties properties_x_,
                            Common::Input::AnalogProperties properties_y_,
                            InputEngine* input_engine_)
        : identifier(identifier_), axis_x(axis_x_), axis_y(axis_y_), properties_x(properties_x_),
          properties_y(properties_y_), input_engine(input_engine_) {
        UpdateCallback engine_callback{[this]() { OnChange(); }};
        const InputIdentifier x_input_identifier{
            .identifier = identifier,
            .type = EngineInputType::Analog,
            .index = axis_x,
            .callback = engine_callback,
        };
        const InputIdentifier y_input_identifier{
            .identifier = identifier,
            .type = EngineInputType::Analog,
            .index = axis_y,
            .callback = engine_callback,
        };
        last_axis_x_value = 0.0f;
        last_axis_y_value = 0.0f;
        callback_key_x = input_engine->SetCallback(x_input_identifier);
        callback_key_y = input_engine->SetCallback(y_input_identifier);
    }

    ~InputFromStick() {
        input_engine->DeleteCallback(callback_key_x);
        input_engine->DeleteCallback(callback_key_y);
    }

    Common::Input::StickStatus GetStatus() const {
        Common::Input::StickStatus status;
        status.x = {
            .raw_value = input_engine->GetAxis(identifier, axis_x),
            .properties = properties_x,
        };
        status.y = {
            .raw_value = input_engine->GetAxis(identifier, axis_y),
            .properties = properties_y,
        };
        return status;
    }

    void ForceUpdate() {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Stick,
            .stick_status = GetStatus(),
        };

        last_axis_x_value = status.stick_status.x.raw_value;
        last_axis_y_value = status.stick_status.y.raw_value;
        TriggerOnChange(status);
    }

    void OnChange() {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Stick,
            .stick_status = GetStatus(),
        };

        if (status.stick_status.x.raw_value != last_axis_x_value ||
            status.stick_status.y.raw_value != last_axis_y_value) {
            last_axis_x_value = status.stick_status.x.raw_value;
            last_axis_y_value = status.stick_status.y.raw_value;
            TriggerOnChange(status);
        }
    }

private:
    const PadIdentifier identifier;
    const int axis_x;
    const int axis_y;
    const Common::Input::AnalogProperties properties_x;
    const Common::Input::AnalogProperties properties_y;
    int callback_key_x;
    int callback_key_y;
    float last_axis_x_value;
    float last_axis_y_value;
    InputEngine* input_engine;
};

class InputFromTouch final : public Common::Input::InputDevice {
public:
    explicit InputFromTouch(PadIdentifier identifier_, int touch_id_, int button_, bool toggle_,
                            bool inverted_, int axis_x_, int axis_y_,
                            Common::Input::AnalogProperties properties_x_,
                            Common::Input::AnalogProperties properties_y_,
                            InputEngine* input_engine_)
        : identifier(identifier_), touch_id(touch_id_), button(button_), toggle(toggle_),
          inverted(inverted_), axis_x(axis_x_), axis_y(axis_y_), properties_x(properties_x_),
          properties_y(properties_y_), input_engine(input_engine_) {
        UpdateCallback engine_callback{[this]() { OnChange(); }};
        const InputIdentifier button_input_identifier{
            .identifier = identifier,
            .type = EngineInputType::Button,
            .index = button,
            .callback = engine_callback,
        };
        const InputIdentifier x_input_identifier{
            .identifier = identifier,
            .type = EngineInputType::Analog,
            .index = axis_x,
            .callback = engine_callback,
        };
        const InputIdentifier y_input_identifier{
            .identifier = identifier,
            .type = EngineInputType::Analog,
            .index = axis_y,
            .callback = engine_callback,
        };
        last_axis_x_value = 0.0f;
        last_axis_y_value = 0.0f;
        last_button_value = false;
        callback_key_button = input_engine->SetCallback(button_input_identifier);
        callback_key_x = input_engine->SetCallback(x_input_identifier);
        callback_key_y = input_engine->SetCallback(y_input_identifier);
    }

    ~InputFromTouch() {
        input_engine->DeleteCallback(callback_key_button);
        input_engine->DeleteCallback(callback_key_x);
        input_engine->DeleteCallback(callback_key_y);
    }

    Common::Input::TouchStatus GetStatus() const {
        Common::Input::TouchStatus status;
        status.id = touch_id;
        status.pressed = {
            .value = input_engine->GetButton(identifier, button),
            .inverted = inverted,
            .toggle = toggle,
        };
        status.x = {
            .raw_value = input_engine->GetAxis(identifier, axis_x),
            .properties = properties_x,
        };
        status.y = {
            .raw_value = input_engine->GetAxis(identifier, axis_y),
            .properties = properties_y,
        };
        return status;
    }

    void OnChange() {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Touch,
            .touch_status = GetStatus(),
        };

        if (status.touch_status.x.raw_value != last_axis_x_value ||
            status.touch_status.y.raw_value != last_axis_y_value ||
            status.touch_status.pressed.value != last_button_value) {
            last_axis_x_value = status.touch_status.x.raw_value;
            last_axis_y_value = status.touch_status.y.raw_value;
            last_button_value = status.touch_status.pressed.value;
            TriggerOnChange(status);
        }
    }

private:
    const PadIdentifier identifier;
    const int touch_id;
    const int button;
    const bool toggle;
    const bool inverted;
    const int axis_x;
    const int axis_y;
    const Common::Input::AnalogProperties properties_x;
    const Common::Input::AnalogProperties properties_y;
    int callback_key_button;
    int callback_key_x;
    int callback_key_y;
    bool last_button_value;
    float last_axis_x_value;
    float last_axis_y_value;
    InputEngine* input_engine;
};

class InputFromTrigger final : public Common::Input::InputDevice {
public:
    explicit InputFromTrigger(PadIdentifier identifier_, int button_, bool toggle_, bool inverted_,
                              int axis_, Common::Input::AnalogProperties properties_,
                              InputEngine* input_engine_)
        : identifier(identifier_), button(button_), toggle(toggle_), inverted(inverted_),
          axis(axis_), properties(properties_), input_engine(input_engine_) {
        UpdateCallback engine_callback{[this]() { OnChange(); }};
        const InputIdentifier button_input_identifier{
            .identifier = identifier,
            .type = EngineInputType::Button,
            .index = button,
            .callback = engine_callback,
        };
        const InputIdentifier axis_input_identifier{
            .identifier = identifier,
            .type = EngineInputType::Analog,
            .index = axis,
            .callback = engine_callback,
        };
        last_axis_value = 0.0f;
        last_button_value = false;
        callback_key_button = input_engine->SetCallback(button_input_identifier);
        axis_callback_key = input_engine->SetCallback(axis_input_identifier);
    }

    ~InputFromTrigger() {
        input_engine->DeleteCallback(callback_key_button);
        input_engine->DeleteCallback(axis_callback_key);
    }

    Common::Input::TriggerStatus GetStatus() const {
        const Common::Input::AnalogStatus analog_status{
            .raw_value = input_engine->GetAxis(identifier, axis),
            .properties = properties,
        };
        const Common::Input::ButtonStatus button_status{
            .value = input_engine->GetButton(identifier, button),
            .inverted = inverted,
            .toggle = toggle,
        };
        return {
            .analog = analog_status,
            .pressed = button_status,
        };
    }

    void OnChange() {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Trigger,
            .trigger_status = GetStatus(),
        };

        if (status.trigger_status.analog.raw_value != last_axis_value ||
            status.trigger_status.pressed.value != last_button_value) {
            last_axis_value = status.trigger_status.analog.raw_value;
            last_button_value = status.trigger_status.pressed.value;
            TriggerOnChange(status);
        }
    }

private:
    const PadIdentifier identifier;
    const int button;
    const bool toggle;
    const bool inverted;
    const int axis;
    const Common::Input::AnalogProperties properties;
    int callback_key_button;
    int axis_callback_key;
    bool last_button_value;
    float last_axis_value;
    InputEngine* input_engine;
};

class InputFromAnalog final : public Common::Input::InputDevice {
public:
    explicit InputFromAnalog(PadIdentifier identifier_, int axis_,
                             Common::Input::AnalogProperties properties_,
                             InputEngine* input_engine_)
        : identifier(identifier_), axis(axis_), properties(properties_),
          input_engine(input_engine_) {
        UpdateCallback engine_callback{[this]() { OnChange(); }};
        const InputIdentifier input_identifier{
            .identifier = identifier,
            .type = EngineInputType::Analog,
            .index = axis,
            .callback = engine_callback,
        };
        last_axis_value = 0.0f;
        callback_key = input_engine->SetCallback(input_identifier);
    }

    ~InputFromAnalog() {
        input_engine->DeleteCallback(callback_key);
    }

    Common::Input::AnalogStatus GetStatus() const {
        return {
            .raw_value = input_engine->GetAxis(identifier, axis),
            .properties = properties,
        };
    }

    void OnChange() {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Analog,
            .analog_status = GetStatus(),
        };

        if (status.analog_status.raw_value != last_axis_value) {
            last_axis_value = status.analog_status.raw_value;
            TriggerOnChange(status);
        }
    }

private:
    const PadIdentifier identifier;
    const int axis;
    const Common::Input::AnalogProperties properties;
    int callback_key;
    float last_axis_value;
    InputEngine* input_engine;
};

class InputFromBattery final : public Common::Input::InputDevice {
public:
    explicit InputFromBattery(PadIdentifier identifier_, InputEngine* input_engine_)
        : identifier(identifier_), input_engine(input_engine_) {
        UpdateCallback engine_callback{[this]() { OnChange(); }};
        const InputIdentifier input_identifier{
            .identifier = identifier,
            .type = EngineInputType::Battery,
            .index = 0,
            .callback = engine_callback,
        };
        last_battery_value = Common::Input::BatteryStatus::Charging;
        callback_key = input_engine->SetCallback(input_identifier);
    }

    ~InputFromBattery() {
        input_engine->DeleteCallback(callback_key);
    }

    Common::Input::BatteryStatus GetStatus() const {
        return static_cast<Common::Input::BatteryLevel>(input_engine->GetBattery(identifier));
    }

    void ForceUpdate() {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Battery,
            .battery_status = GetStatus(),
        };

        last_battery_value = status.battery_status;
        TriggerOnChange(status);
    }

    void OnChange() {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Battery,
            .battery_status = GetStatus(),
        };

        if (status.battery_status != last_battery_value) {
            last_battery_value = status.battery_status;
            TriggerOnChange(status);
        }
    }

private:
    const PadIdentifier identifier;
    int callback_key;
    Common::Input::BatteryStatus last_battery_value;
    InputEngine* input_engine;
};

class InputFromMotion final : public Common::Input::InputDevice {
public:
    explicit InputFromMotion(PadIdentifier identifier_, int motion_sensor_,
                             InputEngine* input_engine_)
        : identifier(identifier_), motion_sensor(motion_sensor_), input_engine(input_engine_) {
        UpdateCallback engine_callback{[this]() { OnChange(); }};
        const InputIdentifier input_identifier{
            .identifier = identifier,
            .type = EngineInputType::Motion,
            .index = motion_sensor,
            .callback = engine_callback,
        };
        callback_key = input_engine->SetCallback(input_identifier);
    }

    ~InputFromMotion() {
        input_engine->DeleteCallback(callback_key);
    }

    Common::Input::MotionStatus GetStatus() const {
        const auto basic_motion = input_engine->GetMotion(identifier, motion_sensor);
        Common::Input::MotionStatus status{};
        const Common::Input::AnalogProperties properties = {
            .deadzone = 0.001f,
            .range = 1.0f,
            .offset = 0.0f,
        };
        status.accel.x = {.raw_value = basic_motion.accel_x, .properties = properties};
        status.accel.y = {.raw_value = basic_motion.accel_y, .properties = properties};
        status.accel.z = {.raw_value = basic_motion.accel_z, .properties = properties};
        status.gyro.x = {.raw_value = basic_motion.gyro_x, .properties = properties};
        status.gyro.y = {.raw_value = basic_motion.gyro_y, .properties = properties};
        status.gyro.z = {.raw_value = basic_motion.gyro_z, .properties = properties};
        status.delta_timestamp = basic_motion.delta_timestamp;
        return status;
    }

    void OnChange() {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Motion,
            .motion_status = GetStatus(),
        };

        TriggerOnChange(status);
    }

private:
    const PadIdentifier identifier;
    const int motion_sensor;
    int callback_key;
    InputEngine* input_engine;
};

class InputFromAxisMotion final : public Common::Input::InputDevice {
public:
    explicit InputFromAxisMotion(PadIdentifier identifier_, int axis_x_, int axis_y_, int axis_z_,
                                 Common::Input::AnalogProperties properties_x_,
                                 Common::Input::AnalogProperties properties_y_,
                                 Common::Input::AnalogProperties properties_z_,
                                 InputEngine* input_engine_)
        : identifier(identifier_), axis_x(axis_x_), axis_y(axis_y_), axis_z(axis_z_),
          properties_x(properties_x_), properties_y(properties_y_), properties_z(properties_z_),
          input_engine(input_engine_) {
        UpdateCallback engine_callback{[this]() { OnChange(); }};
        const InputIdentifier x_input_identifier{
            .identifier = identifier,
            .type = EngineInputType::Analog,
            .index = axis_x,
            .callback = engine_callback,
        };
        const InputIdentifier y_input_identifier{
            .identifier = identifier,
            .type = EngineInputType::Analog,
            .index = axis_y,
            .callback = engine_callback,
        };
        const InputIdentifier z_input_identifier{
            .identifier = identifier,
            .type = EngineInputType::Analog,
            .index = axis_z,
            .callback = engine_callback,
        };
        last_axis_x_value = 0.0f;
        last_axis_y_value = 0.0f;
        last_axis_z_value = 0.0f;
        callback_key_x = input_engine->SetCallback(x_input_identifier);
        callback_key_y = input_engine->SetCallback(y_input_identifier);
        callback_key_z = input_engine->SetCallback(z_input_identifier);
    }

    ~InputFromAxisMotion() {
        input_engine->DeleteCallback(callback_key_x);
        input_engine->DeleteCallback(callback_key_y);
        input_engine->DeleteCallback(callback_key_z);
    }

    Common::Input::MotionStatus GetStatus() const {
        Common::Input::MotionStatus status{};
        status.gyro.x = {
            .raw_value = input_engine->GetAxis(identifier, axis_x),
            .properties = properties_x,
        };
        status.gyro.y = {
            .raw_value = input_engine->GetAxis(identifier, axis_y),
            .properties = properties_y,
        };
        status.gyro.z = {
            .raw_value = input_engine->GetAxis(identifier, axis_z),
            .properties = properties_z,
        };
        status.delta_timestamp = 5000;
        status.force_update = true;
        return status;
    }

    void ForceUpdate() {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Motion,
            .motion_status = GetStatus(),
        };

        last_axis_x_value = status.motion_status.gyro.x.raw_value;
        last_axis_y_value = status.motion_status.gyro.y.raw_value;
        last_axis_z_value = status.motion_status.gyro.z.raw_value;
        TriggerOnChange(status);
    }

    void OnChange() {
        const Common::Input::CallbackStatus status{
            .type = Common::Input::InputType::Motion,
            .motion_status = GetStatus(),
        };

        if (status.motion_status.gyro.x.raw_value != last_axis_x_value ||
            status.motion_status.gyro.y.raw_value != last_axis_y_value ||
            status.motion_status.gyro.z.raw_value != last_axis_z_value) {
            last_axis_x_value = status.motion_status.gyro.x.raw_value;
            last_axis_y_value = status.motion_status.gyro.y.raw_value;
            last_axis_z_value = status.motion_status.gyro.z.raw_value;
            TriggerOnChange(status);
        }
    }

private:
    const PadIdentifier identifier;
    const int axis_x;
    const int axis_y;
    const int axis_z;
    const Common::Input::AnalogProperties properties_x;
    const Common::Input::AnalogProperties properties_y;
    const Common::Input::AnalogProperties properties_z;
    int callback_key_x;
    int callback_key_y;
    int callback_key_z;
    float last_axis_x_value;
    float last_axis_y_value;
    float last_axis_z_value;
    InputEngine* input_engine;
};

class OutputFromIdentifier final : public Common::Input::OutputDevice {
public:
    explicit OutputFromIdentifier(PadIdentifier identifier_, InputEngine* input_engine_)
        : identifier(identifier_), input_engine(input_engine_) {}

    virtual void SetLED(Common::Input::LedStatus led_status) {
        input_engine->SetLeds(identifier, led_status);
    }

    virtual Common::Input::VibrationError SetVibration(
        Common::Input::VibrationStatus vibration_status) {
        return input_engine->SetRumble(identifier, vibration_status);
    }

    virtual Common::Input::PollingError SetPollingMode(Common::Input::PollingMode polling_mode) {
        return input_engine->SetPollingMode(identifier, polling_mode);
    }

private:
    const PadIdentifier identifier;
    InputEngine* input_engine;
};

std::unique_ptr<Common::Input::InputDevice> InputFactory::CreateButtonDevice(
    const Common::ParamPackage& params) {
    const PadIdentifier identifier = {
        .guid = Common::UUID{params.Get("guid", "")},
        .port = static_cast<std::size_t>(params.Get("port", 0)),
        .pad = static_cast<std::size_t>(params.Get("pad", 0)),
    };

    const auto button_id = params.Get("button", 0);
    const auto keyboard_key = params.Get("code", 0);
    const auto toggle = params.Get("toggle", false);
    const auto inverted = params.Get("inverted", false);
    input_engine->PreSetController(identifier);
    input_engine->PreSetButton(identifier, button_id);
    input_engine->PreSetButton(identifier, keyboard_key);
    if (keyboard_key != 0) {
        return std::make_unique<InputFromButton>(identifier, keyboard_key, toggle, inverted,
                                                 input_engine.get());
    }
    return std::make_unique<InputFromButton>(identifier, button_id, toggle, inverted,
                                             input_engine.get());
}

std::unique_ptr<Common::Input::InputDevice> InputFactory::CreateHatButtonDevice(
    const Common::ParamPackage& params) {
    const PadIdentifier identifier = {
        .guid = Common::UUID{params.Get("guid", "")},
        .port = static_cast<std::size_t>(params.Get("port", 0)),
        .pad = static_cast<std::size_t>(params.Get("pad", 0)),
    };

    const auto button_id = params.Get("hat", 0);
    const auto direction = input_engine->GetHatButtonId(params.Get("direction", ""));
    const auto toggle = params.Get("toggle", false);
    const auto inverted = params.Get("inverted", false);

    input_engine->PreSetController(identifier);
    input_engine->PreSetHatButton(identifier, button_id);
    return std::make_unique<InputFromHatButton>(identifier, button_id, direction, toggle, inverted,
                                                input_engine.get());
}

std::unique_ptr<Common::Input::InputDevice> InputFactory::CreateStickDevice(
    const Common::ParamPackage& params) {
    const auto deadzone = std::clamp(params.Get("deadzone", 0.15f), 0.0f, 1.0f);
    const auto range = std::clamp(params.Get("range", 1.0f), 0.25f, 1.50f);
    const auto threshold = std::clamp(params.Get("threshold", 0.5f), 0.0f, 1.0f);
    const PadIdentifier identifier = {
        .guid = Common::UUID{params.Get("guid", "")},
        .port = static_cast<std::size_t>(params.Get("port", 0)),
        .pad = static_cast<std::size_t>(params.Get("pad", 0)),
    };

    const auto axis_x = params.Get("axis_x", 0);
    const Common::Input::AnalogProperties properties_x = {
        .deadzone = deadzone,
        .range = range,
        .threshold = threshold,
        .offset = std::clamp(params.Get("offset_x", 0.0f), -1.0f, 1.0f),
        .inverted = params.Get("invert_x", "+") == "-",
    };

    const auto axis_y = params.Get("axis_y", 1);
    const Common::Input::AnalogProperties properties_y = {
        .deadzone = deadzone,
        .range = range,
        .threshold = threshold,
        .offset = std::clamp(params.Get("offset_y", 0.0f), -1.0f, 1.0f),
        .inverted = params.Get("invert_y", "+") != "+",
    };
    input_engine->PreSetController(identifier);
    input_engine->PreSetAxis(identifier, axis_x);
    input_engine->PreSetAxis(identifier, axis_y);
    return std::make_unique<InputFromStick>(identifier, axis_x, axis_y, properties_x, properties_y,
                                            input_engine.get());
}

std::unique_ptr<Common::Input::InputDevice> InputFactory::CreateAnalogDevice(
    const Common::ParamPackage& params) {
    const PadIdentifier identifier = {
        .guid = Common::UUID{params.Get("guid", "")},
        .port = static_cast<std::size_t>(params.Get("port", 0)),
        .pad = static_cast<std::size_t>(params.Get("pad", 0)),
    };

    const auto axis = params.Get("axis", 0);
    const Common::Input::AnalogProperties properties = {
        .deadzone = std::clamp(params.Get("deadzone", 0.0f), 0.0f, 1.0f),
        .range = std::clamp(params.Get("range", 1.0f), 0.25f, 1.50f),
        .threshold = std::clamp(params.Get("threshold", 0.5f), 0.0f, 1.0f),
        .offset = std::clamp(params.Get("offset", 0.0f), -1.0f, 1.0f),
        .inverted = params.Get("invert", "+") == "-",
    };
    input_engine->PreSetController(identifier);
    input_engine->PreSetAxis(identifier, axis);
    return std::make_unique<InputFromAnalog>(identifier, axis, properties, input_engine.get());
}

std::unique_ptr<Common::Input::InputDevice> InputFactory::CreateTriggerDevice(
    const Common::ParamPackage& params) {
    const PadIdentifier identifier = {
        .guid = Common::UUID{params.Get("guid", "")},
        .port = static_cast<std::size_t>(params.Get("port", 0)),
        .pad = static_cast<std::size_t>(params.Get("pad", 0)),
    };

    const auto button = params.Get("button", 0);
    const auto toggle = params.Get("toggle", false);
    const auto inverted = params.Get("inverted", false);

    const auto axis = params.Get("axis", 0);
    const Common::Input::AnalogProperties properties = {
        .deadzone = std::clamp(params.Get("deadzone", 0.0f), 0.0f, 1.0f),
        .range = std::clamp(params.Get("range", 1.0f), 0.25f, 2.50f),
        .threshold = std::clamp(params.Get("threshold", 0.5f), 0.0f, 1.0f),
        .offset = std::clamp(params.Get("offset", 0.0f), -1.0f, 1.0f),
        .inverted = params.Get("invert", false) != 0,
    };
    input_engine->PreSetController(identifier);
    input_engine->PreSetAxis(identifier, axis);
    input_engine->PreSetButton(identifier, button);
    return std::make_unique<InputFromTrigger>(identifier, button, toggle, inverted, axis,
                                              properties, input_engine.get());
}

std::unique_ptr<Common::Input::InputDevice> InputFactory::CreateTouchDevice(
    const Common::ParamPackage& params) {
    const auto touch_id = params.Get("touch_id", 0);
    const auto deadzone = std::clamp(params.Get("deadzone", 0.0f), 0.0f, 1.0f);
    const auto range = std::clamp(params.Get("range", 1.0f), 0.25f, 1.50f);
    const auto threshold = std::clamp(params.Get("threshold", 0.5f), 0.0f, 1.0f);
    const PadIdentifier identifier = {
        .guid = Common::UUID{params.Get("guid", "")},
        .port = static_cast<std::size_t>(params.Get("port", 0)),
        .pad = static_cast<std::size_t>(params.Get("pad", 0)),
    };

    const auto button = params.Get("button", 0);
    const auto toggle = params.Get("toggle", false);
    const auto inverted = params.Get("inverted", false);

    const auto axis_x = params.Get("axis_x", 0);
    const Common::Input::AnalogProperties properties_x = {
        .deadzone = deadzone,
        .range = range,
        .threshold = threshold,
        .offset = std::clamp(params.Get("offset_x", 0.0f), -1.0f, 1.0f),
        .inverted = params.Get("invert_x", "+") == "-",
    };

    const auto axis_y = params.Get("axis_y", 1);
    const Common::Input::AnalogProperties properties_y = {
        .deadzone = deadzone,
        .range = range,
        .threshold = threshold,
        .offset = std::clamp(params.Get("offset_y", 0.0f), -1.0f, 1.0f),
        .inverted = params.Get("invert_y", false) != 0,
    };
    input_engine->PreSetController(identifier);
    input_engine->PreSetAxis(identifier, axis_x);
    input_engine->PreSetAxis(identifier, axis_y);
    input_engine->PreSetButton(identifier, button);
    return std::make_unique<InputFromTouch>(identifier, touch_id, button, toggle, inverted, axis_x,
                                            axis_y, properties_x, properties_y, input_engine.get());
}

std::unique_ptr<Common::Input::InputDevice> InputFactory::CreateBatteryDevice(
    const Common::ParamPackage& params) {
    const PadIdentifier identifier = {
        .guid = Common::UUID{params.Get("guid", "")},
        .port = static_cast<std::size_t>(params.Get("port", 0)),
        .pad = static_cast<std::size_t>(params.Get("pad", 0)),
    };

    input_engine->PreSetController(identifier);
    return std::make_unique<InputFromBattery>(identifier, input_engine.get());
}

std::unique_ptr<Common::Input::InputDevice> InputFactory::CreateMotionDevice(
    Common::ParamPackage params) {
    const PadIdentifier identifier = {
        .guid = Common::UUID{params.Get("guid", "")},
        .port = static_cast<std::size_t>(params.Get("port", 0)),
        .pad = static_cast<std::size_t>(params.Get("pad", 0)),
    };

    if (params.Has("motion")) {
        const auto motion_sensor = params.Get("motion", 0);
        input_engine->PreSetController(identifier);
        input_engine->PreSetMotion(identifier, motion_sensor);
        return std::make_unique<InputFromMotion>(identifier, motion_sensor, input_engine.get());
    }

    const auto deadzone = std::clamp(params.Get("deadzone", 0.15f), 0.0f, 1.0f);
    const auto range = std::clamp(params.Get("range", 1.0f), 0.25f, 1.50f);
    const auto threshold = std::clamp(params.Get("threshold", 0.5f), 0.0f, 1.0f);

    const auto axis_x = params.Get("axis_x", 0);
    const Common::Input::AnalogProperties properties_x = {
        .deadzone = deadzone,
        .range = range,
        .threshold = threshold,
        .offset = std::clamp(params.Get("offset_x", 0.0f), -1.0f, 1.0f),
        .inverted = params.Get("invert_x", "+") == "-",
    };

    const auto axis_y = params.Get("axis_y", 1);
    const Common::Input::AnalogProperties properties_y = {
        .deadzone = deadzone,
        .range = range,
        .threshold = threshold,
        .offset = std::clamp(params.Get("offset_y", 0.0f), -1.0f, 1.0f),
        .inverted = params.Get("invert_y", "+") != "+",
    };

    const auto axis_z = params.Get("axis_z", 1);
    const Common::Input::AnalogProperties properties_z = {
        .deadzone = deadzone,
        .range = range,
        .threshold = threshold,
        .offset = std::clamp(params.Get("offset_z", 0.0f), -1.0f, 1.0f),
        .inverted = params.Get("invert_z", "+") != "+",
    };
    input_engine->PreSetController(identifier);
    input_engine->PreSetAxis(identifier, axis_x);
    input_engine->PreSetAxis(identifier, axis_y);
    input_engine->PreSetAxis(identifier, axis_z);
    return std::make_unique<InputFromAxisMotion>(identifier, axis_x, axis_y, axis_z, properties_x,
                                                 properties_y, properties_z, input_engine.get());
}

InputFactory::InputFactory(std::shared_ptr<InputEngine> input_engine_)
    : input_engine(std::move(input_engine_)) {}

std::unique_ptr<Common::Input::InputDevice> InputFactory::Create(
    const Common::ParamPackage& params) {
    if (params.Has("battery")) {
        return CreateBatteryDevice(params);
    }
    if (params.Has("button") && params.Has("axis")) {
        return CreateTriggerDevice(params);
    }
    if (params.Has("button") && params.Has("axis_x") && params.Has("axis_y")) {
        return CreateTouchDevice(params);
    }
    if (params.Has("button") || params.Has("code")) {
        return CreateButtonDevice(params);
    }
    if (params.Has("hat")) {
        return CreateHatButtonDevice(params);
    }
    if (params.Has("axis_x") && params.Has("axis_y") && params.Has("axis_z")) {
        return CreateMotionDevice(params);
    }
    if (params.Has("motion")) {
        return CreateMotionDevice(params);
    }
    if (params.Has("axis_x") && params.Has("axis_y")) {
        return CreateStickDevice(params);
    }
    if (params.Has("axis")) {
        return CreateAnalogDevice(params);
    }
    LOG_ERROR(Input, "Invalid parameters given");
    return std::make_unique<DummyInput>();
}

OutputFactory::OutputFactory(std::shared_ptr<InputEngine> input_engine_)
    : input_engine(std::move(input_engine_)) {}

std::unique_ptr<Common::Input::OutputDevice> OutputFactory::Create(
    const Common::ParamPackage& params) {
    const PadIdentifier identifier = {
        .guid = Common::UUID{params.Get("guid", "")},
        .port = static_cast<std::size_t>(params.Get("port", 0)),
        .pad = static_cast<std::size_t>(params.Get("pad", 0)),
    };

    input_engine->PreSetController(identifier);
    return std::make_unique<OutputFromIdentifier>(identifier, input_engine.get());
}

} // namespace InputCommon

// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#include "common/settings.h"
#include "core/hid/emulated_console.h"
#include "core/hid/input_converter.h"

namespace Core::HID {
EmulatedConsole::EmulatedConsole() = default;

EmulatedConsole::~EmulatedConsole() = default;

void EmulatedConsole::ReloadFromSettings() {
    // Using first motion device from player 1. No need to assign any unique config at the moment
    const auto& player = Settings::values.players.GetValue()[0];
    motion_params = Common::ParamPackage(player.motions[0]);

    ReloadInput();
}

void EmulatedConsole::SetTouchParams() {
    // TODO(german77): Support any number of fingers
    std::size_t index = 0;

    // Hardcode mouse, touchscreen and cemuhook parameters
    if (!Settings::values.mouse_enabled) {
        // We can't use mouse as touch if native mouse is enabled
        touch_params[index++] = Common::ParamPackage{"engine:mouse,axis_x:10,axis_y:11,button:0"};
    }
    touch_params[index++] = Common::ParamPackage{"engine:touch,axis_x:0,axis_y:1,button:0"};
    touch_params[index++] = Common::ParamPackage{"engine:touch,axis_x:2,axis_y:3,button:1"};
    touch_params[index++] =
        Common::ParamPackage{"engine:cemuhookudp,axis_x:17,axis_y:18,button:65536"};
    touch_params[index++] =
        Common::ParamPackage{"engine:cemuhookudp,axis_x:19,axis_y:20,button:131072"};

    const auto button_index =
        static_cast<u64>(Settings::values.touch_from_button_map_index.GetValue());
    const auto& touch_buttons = Settings::values.touch_from_button_maps[button_index].buttons;

    // Map the rest of the fingers from touch from button configuration
    for (const auto& config_entry : touch_buttons) {
        if (index >= touch_params.size()) {
            continue;
        }
        Common::ParamPackage params{config_entry};
        Common::ParamPackage touch_button_params;
        const int x = params.Get("x", 0);
        const int y = params.Get("y", 0);
        params.Erase("x");
        params.Erase("y");
        touch_button_params.Set("engine", "touch_from_button");
        touch_button_params.Set("button", params.Serialize());
        touch_button_params.Set("x", x);
        touch_button_params.Set("y", y);
        touch_button_params.Set("touch_id", static_cast<int>(index));
        touch_params[index] = touch_button_params;
        index++;
    }
}

void EmulatedConsole::ReloadInput() {
    // If you load any device here add the equivalent to the UnloadInput() function
    SetTouchParams();

    motion_devices = Common::Input::CreateDevice<Common::Input::InputDevice>(motion_params);
    if (motion_devices) {
        motion_devices->SetCallback({
            .on_change =
                [this](const Common::Input::CallbackStatus& callback) { SetMotion(callback); },
        });
    }

    // Unique index for identifying touch device source
    std::size_t index = 0;
    for (auto& touch_device : touch_devices) {
        touch_device = Common::Input::CreateDevice<Common::Input::InputDevice>(touch_params[index]);
        if (!touch_device) {
            continue;
        }
        touch_device->SetCallback({
            .on_change =
                [this, index](const Common::Input::CallbackStatus& callback) {
                    SetTouch(callback, index);
                },
        });
        index++;
    }
}

void EmulatedConsole::UnloadInput() {
    motion_devices.reset();
    for (auto& touch : touch_devices) {
        touch.reset();
    }
}

void EmulatedConsole::EnableConfiguration() {
    is_configuring = true;
    SaveCurrentConfig();
}

void EmulatedConsole::DisableConfiguration() {
    is_configuring = false;
}

bool EmulatedConsole::IsConfiguring() const {
    return is_configuring;
}

void EmulatedConsole::SaveCurrentConfig() {
    if (!is_configuring) {
        return;
    }
}

void EmulatedConsole::RestoreConfig() {
    if (!is_configuring) {
        return;
    }
    ReloadFromSettings();
}

Common::ParamPackage EmulatedConsole::GetMotionParam() const {
    return motion_params;
}

void EmulatedConsole::SetMotionParam(Common::ParamPackage param) {
    motion_params = param;
    ReloadInput();
}

void EmulatedConsole::SetMotion(const Common::Input::CallbackStatus& callback) {
    std::lock_guard lock{mutex};
    auto& raw_status = console.motion_values.raw_status;
    auto& emulated = console.motion_values.emulated;

    raw_status = TransformToMotion(callback);
    emulated.SetAcceleration(Common::Vec3f{
        raw_status.accel.x.value,
        raw_status.accel.y.value,
        raw_status.accel.z.value,
    });
    emulated.SetGyroscope(Common::Vec3f{
        raw_status.gyro.x.value,
        raw_status.gyro.y.value,
        raw_status.gyro.z.value,
    });
    emulated.UpdateRotation(raw_status.delta_timestamp);
    emulated.UpdateOrientation(raw_status.delta_timestamp);

    if (is_configuring) {
        TriggerOnChange(ConsoleTriggerType::Motion);
        return;
    }

    auto& motion = console.motion_state;
    motion.accel = emulated.GetAcceleration();
    motion.gyro = emulated.GetGyroscope();
    motion.rotation = emulated.GetGyroscope();
    motion.orientation = emulated.GetOrientation();
    motion.quaternion = emulated.GetQuaternion();
    motion.gyro_bias = emulated.GetGyroBias();
    motion.is_at_rest = !emulated.IsMoving(motion_sensitivity);
    // Find what is this value
    motion.verticalization_error = 0.0f;

    TriggerOnChange(ConsoleTriggerType::Motion);
}

void EmulatedConsole::SetTouch(const Common::Input::CallbackStatus& callback, std::size_t index) {
    if (index >= console.touch_values.size()) {
        return;
    }
    std::lock_guard lock{mutex};

    console.touch_values[index] = TransformToTouch(callback);

    if (is_configuring) {
        TriggerOnChange(ConsoleTriggerType::Touch);
        return;
    }

    // TODO(german77): Remap touch id in sequential order
    console.touch_state[index] = {
        .position = {console.touch_values[index].x.value, console.touch_values[index].y.value},
        .id = static_cast<u32>(console.touch_values[index].id),
        .pressed = console.touch_values[index].pressed.value,
    };

    TriggerOnChange(ConsoleTriggerType::Touch);
}

ConsoleMotionValues EmulatedConsole::GetMotionValues() const {
    return console.motion_values;
}

TouchValues EmulatedConsole::GetTouchValues() const {
    return console.touch_values;
}

ConsoleMotion EmulatedConsole::GetMotion() const {
    return console.motion_state;
}

TouchFingerState EmulatedConsole::GetTouch() const {
    return console.touch_state;
}

void EmulatedConsole::TriggerOnChange(ConsoleTriggerType type) {
    for (const auto& poller_pair : callback_list) {
        const ConsoleUpdateCallback& poller = poller_pair.second;
        if (poller.on_change) {
            poller.on_change(type);
        }
    }
}

int EmulatedConsole::SetCallback(ConsoleUpdateCallback update_callback) {
    std::lock_guard lock{mutex};
    callback_list.insert_or_assign(last_callback_key, update_callback);
    return last_callback_key++;
}

void EmulatedConsole::DeleteCallback(int key) {
    std::lock_guard lock{mutex};
    const auto& iterator = callback_list.find(key);
    if (iterator == callback_list.end()) {
        LOG_ERROR(Input, "Tried to delete non-existent callback {}", key);
        return;
    }
    callback_list.erase(iterator);
}
} // namespace Core::HID

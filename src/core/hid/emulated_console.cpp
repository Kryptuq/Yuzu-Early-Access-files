// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
    std::size_t index = 0;

    // We can't use mouse as touch if native mouse is enabled
    if (!Settings::values.mouse_enabled) {
        touch_params[index++] = Common::ParamPackage{"engine:mouse,axis_x:10,axis_y:11,button:0"};
    }

    touch_params[index++] =
        Common::ParamPackage{"engine:cemuhookudp,axis_x:17,axis_y:18,button:65536"};
    touch_params[index++] =
        Common::ParamPackage{"engine:cemuhookudp,axis_x:19,axis_y:20,button:131072"};

    for (int i = 0; i < static_cast<int>(max_active_touch_inputs); i++) {
        Common::ParamPackage touchscreen_param{};
        touchscreen_param.Set("engine", "touch");
        touchscreen_param.Set("axis_x", i * 2);
        touchscreen_param.Set("axis_y", (i * 2) + 1);
        touchscreen_param.Set("button", i);
        touch_params[index++] = touchscreen_param;
    }

    const auto button_index =
        static_cast<u64>(Settings::values.touch_from_button_map_index.GetValue());
    const auto& touch_buttons = Settings::values.touch_from_button_maps[button_index].buttons;

    // Map the rest of the fingers from touch from button configuration
    for (const auto& config_entry : touch_buttons) {
        if (index >= max_touch_devices) {
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
    std::unique_lock lock{mutex};
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
        lock.unlock();
        TriggerOnChange(ConsoleTriggerType::Motion);
        return;
    }

    auto& motion = console.motion_state;
    motion.accel = emulated.GetAcceleration();
    motion.gyro = emulated.GetGyroscope();
    motion.rotation = emulated.GetRotations();
    motion.orientation = emulated.GetOrientation();
    motion.quaternion = emulated.GetQuaternion();
    motion.gyro_bias = emulated.GetGyroBias();
    motion.is_at_rest = !emulated.IsMoving(motion_sensitivity);
    // Find what is this value
    motion.verticalization_error = 0.0f;

    lock.unlock();
    TriggerOnChange(ConsoleTriggerType::Motion);
}

void EmulatedConsole::SetTouch(const Common::Input::CallbackStatus& callback, std::size_t index) {
    if (index >= max_touch_devices) {
        return;
    }
    std::unique_lock lock{mutex};

    const auto touch_input = TransformToTouch(callback);
    auto touch_index = GetIndexFromFingerId(index);
    bool is_new_input = false;

    if (!touch_index.has_value() && touch_input.pressed.value) {
        touch_index = GetNextFreeIndex();
        is_new_input = true;
    }

    // No free entries or invalid state. Ignore input
    if (!touch_index.has_value()) {
        return;
    }

    auto& touch_value = console.touch_values[touch_index.value()];

    if (is_new_input) {
        touch_value.pressed.value = true;
        touch_value.id = static_cast<u32>(index);
    }

    touch_value.x = touch_input.x;
    touch_value.y = touch_input.y;

    if (!touch_input.pressed.value) {
        touch_value.pressed.value = false;
    }

    if (is_configuring) {
        lock.unlock();
        TriggerOnChange(ConsoleTriggerType::Touch);
        return;
    }

    // Touch outside allowed range. Ignore input
    if (touch_index.value() >= max_active_touch_inputs) {
        return;
    }

    console.touch_state[touch_index.value()] = {
        .position = {touch_value.x.value, touch_value.y.value},
        .id = static_cast<u32>(touch_index.value()),
        .pressed = touch_input.pressed.value,
    };

    lock.unlock();
    TriggerOnChange(ConsoleTriggerType::Touch);
}

ConsoleMotionValues EmulatedConsole::GetMotionValues() const {
    std::scoped_lock lock{mutex};
    return console.motion_values;
}

TouchValues EmulatedConsole::GetTouchValues() const {
    std::scoped_lock lock{mutex};
    return console.touch_values;
}

ConsoleMotion EmulatedConsole::GetMotion() const {
    std::scoped_lock lock{mutex};
    return console.motion_state;
}

TouchFingerState EmulatedConsole::GetTouch() const {
    std::scoped_lock lock{mutex};
    return console.touch_state;
}

std::optional<std::size_t> EmulatedConsole::GetIndexFromFingerId(std::size_t finger_id) const {
    for (std::size_t index = 0; index < max_touch_devices; ++index) {
        const auto& finger = console.touch_values[index];
        if (!finger.pressed.value) {
            continue;
        }
        if (finger.id == static_cast<int>(finger_id)) {
            return index;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> EmulatedConsole::GetNextFreeIndex() const {
    for (std::size_t index = 0; index < max_touch_devices; ++index) {
        if (!console.touch_values[index].pressed.value) {
            return index;
        }
    }
    return std::nullopt;
}

void EmulatedConsole::TriggerOnChange(ConsoleTriggerType type) {
    std::scoped_lock lock{callback_mutex};
    for (const auto& poller_pair : callback_list) {
        const ConsoleUpdateCallback& poller = poller_pair.second;
        if (poller.on_change) {
            poller.on_change(type);
        }
    }
}

int EmulatedConsole::SetCallback(ConsoleUpdateCallback update_callback) {
    std::scoped_lock lock{callback_mutex};
    callback_list.insert_or_assign(last_callback_key, update_callback);
    return last_callback_key++;
}

void EmulatedConsole::DeleteCallback(int key) {
    std::scoped_lock lock{callback_mutex};
    const auto& iterator = callback_list.find(key);
    if (iterator == callback_list.end()) {
        LOG_ERROR(Input, "Tried to delete non-existent callback {}", key);
        return;
    }
    callback_list.erase(iterator);
}
} // namespace Core::HID

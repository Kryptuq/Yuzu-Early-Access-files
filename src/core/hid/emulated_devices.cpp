// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <fmt/format.h>

#include "core/hid/emulated_devices.h"
#include "core/hid/input_converter.h"

namespace Core::HID {

EmulatedDevices::EmulatedDevices() = default;

EmulatedDevices::~EmulatedDevices() = default;

void EmulatedDevices::ReloadFromSettings() {
    ReloadInput();
}

void EmulatedDevices::ReloadInput() {
    // If you load any device here add the equivalent to the UnloadInput() function

    // Native Mouse is mapped on port 1, pad 0
    const Common::ParamPackage mouse_params{"engine:mouse,port:1,pad:0"};

    // Keyboard keys is mapped on port 1, pad 0 for normal keys, pad 1 for moddifier keys
    const Common::ParamPackage keyboard_params{"engine:keyboard,port:1"};

    std::size_t key_index = 0;
    for (auto& mouse_device : mouse_button_devices) {
        Common::ParamPackage mouse_button_params = mouse_params;
        mouse_button_params.Set("button", static_cast<int>(key_index));
        mouse_device = Common::Input::CreateInputDevice(mouse_button_params);
        key_index++;
    }

    Common::ParamPackage mouse_position_params = mouse_params;
    mouse_position_params.Set("axis_x", 0);
    mouse_position_params.Set("axis_y", 1);
    mouse_position_params.Set("deadzone", 0.0f);
    mouse_position_params.Set("range", 1.0f);
    mouse_position_params.Set("threshold", 0.0f);
    mouse_stick_device = Common::Input::CreateInputDevice(mouse_position_params);

    // First two axis are reserved for mouse position
    key_index = 2;
    for (auto& mouse_device : mouse_wheel_devices) {
        Common::ParamPackage mouse_wheel_params = mouse_params;
        mouse_wheel_params.Set("axis", static_cast<int>(key_index));
        mouse_device = Common::Input::CreateInputDevice(mouse_wheel_params);
        key_index++;
    }

    key_index = 0;
    for (auto& keyboard_device : keyboard_devices) {
        Common::ParamPackage keyboard_key_params = keyboard_params;
        keyboard_key_params.Set("button", static_cast<int>(key_index));
        keyboard_key_params.Set("pad", 0);
        keyboard_device = Common::Input::CreateInputDevice(keyboard_key_params);
        key_index++;
    }

    key_index = 0;
    for (auto& keyboard_device : keyboard_modifier_devices) {
        Common::ParamPackage keyboard_moddifier_params = keyboard_params;
        keyboard_moddifier_params.Set("button", static_cast<int>(key_index));
        keyboard_moddifier_params.Set("pad", 1);
        keyboard_device = Common::Input::CreateInputDevice(keyboard_moddifier_params);
        key_index++;
    }

    for (std::size_t index = 0; index < mouse_button_devices.size(); ++index) {
        if (!mouse_button_devices[index]) {
            continue;
        }
        mouse_button_devices[index]->SetCallback({
            .on_change =
                [this, index](const Common::Input::CallbackStatus& callback) {
                    SetMouseButton(callback, index);
                },
        });
    }

    for (std::size_t index = 0; index < mouse_wheel_devices.size(); ++index) {
        if (!mouse_wheel_devices[index]) {
            continue;
        }
        mouse_wheel_devices[index]->SetCallback({
            .on_change =
                [this, index](const Common::Input::CallbackStatus& callback) {
                    SetMouseWheel(callback, index);
                },
        });
    }

    if (mouse_stick_device) {
        mouse_stick_device->SetCallback({
            .on_change =
                [this](const Common::Input::CallbackStatus& callback) {
                    SetMousePosition(callback);
                },
        });
    }

    for (std::size_t index = 0; index < keyboard_devices.size(); ++index) {
        if (!keyboard_devices[index]) {
            continue;
        }
        keyboard_devices[index]->SetCallback({
            .on_change =
                [this, index](const Common::Input::CallbackStatus& callback) {
                    SetKeyboardButton(callback, index);
                },
        });
    }

    for (std::size_t index = 0; index < keyboard_modifier_devices.size(); ++index) {
        if (!keyboard_modifier_devices[index]) {
            continue;
        }
        keyboard_modifier_devices[index]->SetCallback({
            .on_change =
                [this, index](const Common::Input::CallbackStatus& callback) {
                    SetKeyboardModifier(callback, index);
                },
        });
    }
}

void EmulatedDevices::UnloadInput() {
    for (auto& button : mouse_button_devices) {
        button.reset();
    }
    for (auto& analog : mouse_wheel_devices) {
        analog.reset();
    }
    mouse_stick_device.reset();
    for (auto& button : keyboard_devices) {
        button.reset();
    }
    for (auto& button : keyboard_modifier_devices) {
        button.reset();
    }
}

void EmulatedDevices::EnableConfiguration() {
    is_configuring = true;
    SaveCurrentConfig();
}

void EmulatedDevices::DisableConfiguration() {
    is_configuring = false;
}

bool EmulatedDevices::IsConfiguring() const {
    return is_configuring;
}

void EmulatedDevices::SaveCurrentConfig() {
    if (!is_configuring) {
        return;
    }
}

void EmulatedDevices::RestoreConfig() {
    if (!is_configuring) {
        return;
    }
    ReloadFromSettings();
}

void EmulatedDevices::SetKeyboardButton(const Common::Input::CallbackStatus& callback,
                                        std::size_t index) {
    if (index >= device_status.keyboard_values.size()) {
        return;
    }
    std::unique_lock lock{mutex};
    bool value_changed = false;
    const auto new_status = TransformToButton(callback);
    auto& current_status = device_status.keyboard_values[index];
    current_status.toggle = new_status.toggle;

    // Update button status with current status
    if (!current_status.toggle) {
        current_status.locked = false;
        if (current_status.value != new_status.value) {
            current_status.value = new_status.value;
            value_changed = true;
        }
    } else {
        // Toggle button and lock status
        if (new_status.value && !current_status.locked) {
            current_status.locked = true;
            current_status.value = !current_status.value;
            value_changed = true;
        }

        // Unlock button, ready for next press
        if (!new_status.value && current_status.locked) {
            current_status.locked = false;
        }
    }

    if (!value_changed) {
        return;
    }

    if (is_configuring) {
        lock.unlock();
        TriggerOnChange(DeviceTriggerType::Keyboard);
        return;
    }

    // Index should be converted from NativeKeyboard to KeyboardKeyIndex
    UpdateKey(index, current_status.value);

    lock.unlock();
    TriggerOnChange(DeviceTriggerType::Keyboard);
}

void EmulatedDevices::UpdateKey(std::size_t key_index, bool status) {
    constexpr std::size_t KEYS_PER_BYTE = 8;
    auto& entry = device_status.keyboard_state.key[key_index / KEYS_PER_BYTE];
    const u8 mask = static_cast<u8>(1 << (key_index % KEYS_PER_BYTE));
    if (status) {
        entry = entry | mask;
    } else {
        entry = static_cast<u8>(entry & ~mask);
    }
}

void EmulatedDevices::SetKeyboardModifier(const Common::Input::CallbackStatus& callback,
                                          std::size_t index) {
    if (index >= device_status.keyboard_moddifier_values.size()) {
        return;
    }
    std::unique_lock lock{mutex};
    bool value_changed = false;
    const auto new_status = TransformToButton(callback);
    auto& current_status = device_status.keyboard_moddifier_values[index];
    current_status.toggle = new_status.toggle;

    // Update button status with current
    if (!current_status.toggle) {
        current_status.locked = false;
        if (current_status.value != new_status.value) {
            current_status.value = new_status.value;
            value_changed = true;
        }
    } else {
        // Toggle button and lock status
        if (new_status.value && !current_status.locked) {
            current_status.locked = true;
            current_status.value = !current_status.value;
            value_changed = true;
        }

        // Unlock button ready for next press
        if (!new_status.value && current_status.locked) {
            current_status.locked = false;
        }
    }

    if (!value_changed) {
        return;
    }

    if (is_configuring) {
        lock.unlock();
        TriggerOnChange(DeviceTriggerType::KeyboardModdifier);
        return;
    }

    switch (index) {
    case Settings::NativeKeyboard::LeftControl:
    case Settings::NativeKeyboard::RightControl:
        device_status.keyboard_moddifier_state.control.Assign(current_status.value);
        break;
    case Settings::NativeKeyboard::LeftShift:
    case Settings::NativeKeyboard::RightShift:
        device_status.keyboard_moddifier_state.shift.Assign(current_status.value);
        break;
    case Settings::NativeKeyboard::LeftAlt:
        device_status.keyboard_moddifier_state.left_alt.Assign(current_status.value);
        break;
    case Settings::NativeKeyboard::RightAlt:
        device_status.keyboard_moddifier_state.right_alt.Assign(current_status.value);
        break;
    case Settings::NativeKeyboard::CapsLock:
        device_status.keyboard_moddifier_state.caps_lock.Assign(current_status.value);
        break;
    case Settings::NativeKeyboard::ScrollLock:
        device_status.keyboard_moddifier_state.scroll_lock.Assign(current_status.value);
        break;
    case Settings::NativeKeyboard::NumLock:
        device_status.keyboard_moddifier_state.num_lock.Assign(current_status.value);
        break;
    }

    lock.unlock();
    TriggerOnChange(DeviceTriggerType::KeyboardModdifier);
}

void EmulatedDevices::SetMouseButton(const Common::Input::CallbackStatus& callback,
                                     std::size_t index) {
    if (index >= device_status.mouse_button_values.size()) {
        return;
    }
    std::unique_lock lock{mutex};
    bool value_changed = false;
    const auto new_status = TransformToButton(callback);
    auto& current_status = device_status.mouse_button_values[index];
    current_status.toggle = new_status.toggle;

    // Update button status with current
    if (!current_status.toggle) {
        current_status.locked = false;
        if (current_status.value != new_status.value) {
            current_status.value = new_status.value;
            value_changed = true;
        }
    } else {
        // Toggle button and lock status
        if (new_status.value && !current_status.locked) {
            current_status.locked = true;
            current_status.value = !current_status.value;
            value_changed = true;
        }

        // Unlock button ready for next press
        if (!new_status.value && current_status.locked) {
            current_status.locked = false;
        }
    }

    if (!value_changed) {
        return;
    }

    if (is_configuring) {
        lock.unlock();
        TriggerOnChange(DeviceTriggerType::Mouse);
        return;
    }

    switch (index) {
    case Settings::NativeMouseButton::Left:
        device_status.mouse_button_state.left.Assign(current_status.value);
        break;
    case Settings::NativeMouseButton::Right:
        device_status.mouse_button_state.right.Assign(current_status.value);
        break;
    case Settings::NativeMouseButton::Middle:
        device_status.mouse_button_state.middle.Assign(current_status.value);
        break;
    case Settings::NativeMouseButton::Forward:
        device_status.mouse_button_state.forward.Assign(current_status.value);
        break;
    case Settings::NativeMouseButton::Back:
        device_status.mouse_button_state.back.Assign(current_status.value);
        break;
    }

    lock.unlock();
    TriggerOnChange(DeviceTriggerType::Mouse);
}

void EmulatedDevices::SetMouseWheel(const Common::Input::CallbackStatus& callback,
                                    std::size_t index) {
    if (index >= device_status.mouse_wheel_values.size()) {
        return;
    }
    std::unique_lock lock{mutex};
    const auto analog_value = TransformToAnalog(callback);

    device_status.mouse_wheel_values[index] = analog_value;

    if (is_configuring) {
        device_status.mouse_wheel_state = {};
        lock.unlock();
        TriggerOnChange(DeviceTriggerType::Mouse);
        return;
    }

    switch (index) {
    case Settings::NativeMouseWheel::X:
        device_status.mouse_wheel_state.x = static_cast<s32>(analog_value.value);
        break;
    case Settings::NativeMouseWheel::Y:
        device_status.mouse_wheel_state.y = static_cast<s32>(analog_value.value);
        break;
    }

    lock.unlock();
    TriggerOnChange(DeviceTriggerType::Mouse);
}

void EmulatedDevices::SetMousePosition(const Common::Input::CallbackStatus& callback) {
    std::unique_lock lock{mutex};
    const auto touch_value = TransformToTouch(callback);

    device_status.mouse_stick_value = touch_value;

    if (is_configuring) {
        device_status.mouse_position_state = {};
        lock.unlock();
        TriggerOnChange(DeviceTriggerType::Mouse);
        return;
    }

    device_status.mouse_position_state.x = touch_value.x.value;
    device_status.mouse_position_state.y = touch_value.y.value;

    lock.unlock();
    TriggerOnChange(DeviceTriggerType::Mouse);
}

KeyboardValues EmulatedDevices::GetKeyboardValues() const {
    std::scoped_lock lock{mutex};
    return device_status.keyboard_values;
}

KeyboardModifierValues EmulatedDevices::GetKeyboardModdifierValues() const {
    std::scoped_lock lock{mutex};
    return device_status.keyboard_moddifier_values;
}

MouseButtonValues EmulatedDevices::GetMouseButtonsValues() const {
    std::scoped_lock lock{mutex};
    return device_status.mouse_button_values;
}

KeyboardKey EmulatedDevices::GetKeyboard() const {
    std::scoped_lock lock{mutex};
    return device_status.keyboard_state;
}

KeyboardModifier EmulatedDevices::GetKeyboardModifier() const {
    std::scoped_lock lock{mutex};
    return device_status.keyboard_moddifier_state;
}

MouseButton EmulatedDevices::GetMouseButtons() const {
    std::scoped_lock lock{mutex};
    return device_status.mouse_button_state;
}

MousePosition EmulatedDevices::GetMousePosition() const {
    std::scoped_lock lock{mutex};
    return device_status.mouse_position_state;
}

AnalogStickState EmulatedDevices::GetMouseWheel() const {
    std::scoped_lock lock{mutex};
    return device_status.mouse_wheel_state;
}

void EmulatedDevices::TriggerOnChange(DeviceTriggerType type) {
    std::scoped_lock lock{callback_mutex};
    for (const auto& poller_pair : callback_list) {
        const InterfaceUpdateCallback& poller = poller_pair.second;
        if (poller.on_change) {
            poller.on_change(type);
        }
    }
}

int EmulatedDevices::SetCallback(InterfaceUpdateCallback update_callback) {
    std::scoped_lock lock{callback_mutex};
    callback_list.insert_or_assign(last_callback_key, std::move(update_callback));
    return last_callback_key++;
}

void EmulatedDevices::DeleteCallback(int key) {
    std::scoped_lock lock{callback_mutex};
    const auto& iterator = callback_list.find(key);
    if (iterator == callback_list.end()) {
        LOG_ERROR(Input, "Tried to delete non-existent callback {}", key);
        return;
    }
    callback_list.erase(iterator);
}
} // namespace Core::HID

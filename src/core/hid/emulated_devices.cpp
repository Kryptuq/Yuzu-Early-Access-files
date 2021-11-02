// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#include <algorithm>
#include <fmt/format.h>

#include "core/hid/emulated_devices.h"
#include "core/hid/input_converter.h"

namespace Core::HID {

EmulatedDevices::EmulatedDevices() = default;

EmulatedDevices::~EmulatedDevices() = default;

void EmulatedDevices::ReloadFromSettings() {
    const auto& mouse = Settings::values.mouse_buttons;

    for (std::size_t index = 0; index < mouse.size(); ++index) {
        mouse_button_params[index] = Common::ParamPackage(mouse[index]);
    }
    ReloadInput();
}

void EmulatedDevices::ReloadInput() {
    std::transform(mouse_button_params.begin() + Settings::NativeMouseButton::MOUSE_HID_BEGIN,
                   mouse_button_params.begin() + Settings::NativeMouseButton::MOUSE_HID_END,
                   mouse_button_devices.begin(),
                   Common::Input::CreateDevice<Common::Input::InputDevice>);

    std::transform(Settings::values.keyboard_keys.begin(), Settings::values.keyboard_keys.end(),
                   keyboard_devices.begin(),
                   Common::Input::CreateDeviceFromString<Common::Input::InputDevice>);

    std::transform(Settings::values.keyboard_mods.begin(), Settings::values.keyboard_mods.end(),
                   keyboard_modifier_devices.begin(),
                   Common::Input::CreateDeviceFromString<Common::Input::InputDevice>);

    for (std::size_t index = 0; index < mouse_button_devices.size(); ++index) {
        if (!mouse_button_devices[index]) {
            continue;
        }
        Common::Input::InputCallback button_callback{
            [this, index](Common::Input::CallbackStatus callback) {
                SetMouseButton(callback, index);
            }};
        mouse_button_devices[index]->SetCallback(button_callback);
    }

    for (std::size_t index = 0; index < keyboard_devices.size(); ++index) {
        if (!keyboard_devices[index]) {
            continue;
        }
        Common::Input::InputCallback button_callback{
            [this, index](Common::Input::CallbackStatus callback) {
                SetKeyboardButton(callback, index);
            }};
        keyboard_devices[index]->SetCallback(button_callback);
    }

    for (std::size_t index = 0; index < keyboard_modifier_devices.size(); ++index) {
        if (!keyboard_modifier_devices[index]) {
            continue;
        }
        Common::Input::InputCallback button_callback{
            [this, index](Common::Input::CallbackStatus callback) {
                SetKeyboardModifier(callback, index);
            }};
        keyboard_modifier_devices[index]->SetCallback(button_callback);
    }
}

void EmulatedDevices::UnloadInput() {
    for (auto& button : mouse_button_devices) {
        button.reset();
    }
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

    auto& mouse = Settings::values.mouse_buttons;

    for (std::size_t index = 0; index < mouse.size(); ++index) {
        mouse[index] = mouse_button_params[index].Serialize();
    }
}

void EmulatedDevices::RestoreConfig() {
    if (!is_configuring) {
        return;
    }
    ReloadFromSettings();
}

Common::ParamPackage EmulatedDevices::GetMouseButtonParam(std::size_t index) const {
    if (index >= mouse_button_params.size()) {
        return {};
    }
    return mouse_button_params[index];
}

void EmulatedDevices::SetMouseButtonParam(std::size_t index, Common::ParamPackage param) {
    if (index >= mouse_button_params.size()) {
        return;
    }
    mouse_button_params[index] = param;
    ReloadInput();
}

void EmulatedDevices::SetKeyboardButton(Common::Input::CallbackStatus callback, std::size_t index) {
    if (index >= device_status.keyboard_values.size()) {
        return;
    }
    std::lock_guard lock{mutex};
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
        TriggerOnChange(DeviceTriggerType::Keyboard);
        return;
    }

    UpdateKey(index, current_status.value);

    TriggerOnChange(DeviceTriggerType::Keyboard);
}

void EmulatedDevices::UpdateKey(std::size_t key_index, bool status) {
    constexpr u8 KEYS_PER_BYTE = 8;
    auto& entry = device_status.keyboard_state.key[key_index / KEYS_PER_BYTE];
    const u8 mask = static_cast<u8>(1 << (key_index % KEYS_PER_BYTE));
    if (status) {
        entry = entry | mask;
    } else {
        entry = static_cast<u8>(entry & ~mask);
    }
}

void EmulatedDevices::SetKeyboardModifier(Common::Input::CallbackStatus callback,
                                          std::size_t index) {
    if (index >= device_status.keyboard_moddifier_values.size()) {
        return;
    }
    std::lock_guard lock{mutex};
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

    TriggerOnChange(DeviceTriggerType::KeyboardModdifier);
}

void EmulatedDevices::SetMouseButton(Common::Input::CallbackStatus callback, std::size_t index) {
    if (index >= device_status.mouse_button_values.size()) {
        return;
    }
    std::lock_guard lock{mutex};
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

    TriggerOnChange(DeviceTriggerType::Mouse);
}

KeyboardValues EmulatedDevices::GetKeyboardValues() const {
    return device_status.keyboard_values;
}

KeyboardModifierValues EmulatedDevices::GetKeyboardModdifierValues() const {
    return device_status.keyboard_moddifier_values;
}

MouseButtonValues EmulatedDevices::GetMouseButtonsValues() const {
    return device_status.mouse_button_values;
}

KeyboardKey EmulatedDevices::GetKeyboard() const {
    return device_status.keyboard_state;
}

KeyboardModifier EmulatedDevices::GetKeyboardModifier() const {
    return device_status.keyboard_moddifier_state;
}

MouseButton EmulatedDevices::GetMouseButtons() const {
    return device_status.mouse_button_state;
}

MousePosition EmulatedDevices::GetMousePosition() const {
    return device_status.mouse_position_state;
}

void EmulatedDevices::TriggerOnChange(DeviceTriggerType type) {
    for (const auto& poller_pair : callback_list) {
        const InterfaceUpdateCallback& poller = poller_pair.second;
        if (poller.on_change) {
            poller.on_change(type);
        }
    }
}

int EmulatedDevices::SetCallback(InterfaceUpdateCallback update_callback) {
    std::lock_guard lock{mutex};
    callback_list.insert_or_assign(last_callback_key, update_callback);
    return last_callback_key++;
}

void EmulatedDevices::DeleteCallback(int key) {
    std::lock_guard lock{mutex};
    const auto& iterator = callback_list.find(key);
    if (iterator == callback_list.end()) {
        LOG_ERROR(Input, "Tried to delete non-existent callback {}", key);
        return;
    }
    callback_list.erase(iterator);
}
} // namespace Core::HID

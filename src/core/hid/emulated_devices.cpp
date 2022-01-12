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
    ring_params = Common::ParamPackage(Settings::values.ringcon_analogs);
    ReloadInput();
}

void EmulatedDevices::ReloadInput() {
    // If you load any device here add the equivalent to the UnloadInput() function
    std::size_t key_index = 0;
    for (auto& mouse_device : mouse_button_devices) {
        Common::ParamPackage mouse_params;
        mouse_params.Set("engine", "mouse");
        mouse_params.Set("button", static_cast<int>(key_index));
        mouse_device = Common::Input::CreateDevice<Common::Input::InputDevice>(mouse_params);
        key_index++;
    }

    mouse_stick_device = Common::Input::CreateDeviceFromString<Common::Input::InputDevice>(
        "engine:mouse,axis_x:0,axis_y:1");

    // First two axis are reserved for mouse position
    key_index = 2;
    for (auto& mouse_device : mouse_analog_devices) {
        Common::ParamPackage mouse_params;
        mouse_params.Set("engine", "mouse");
        mouse_params.Set("axis", static_cast<int>(key_index));
        mouse_device = Common::Input::CreateDevice<Common::Input::InputDevice>(mouse_params);
        key_index++;
    }

    key_index = 0;
    for (auto& keyboard_device : keyboard_devices) {
        // Keyboard keys are only mapped on port 1, pad 0
        Common::ParamPackage keyboard_params;
        keyboard_params.Set("engine", "keyboard");
        keyboard_params.Set("button", static_cast<int>(key_index));
        keyboard_params.Set("port", 1);
        keyboard_params.Set("pad", 0);
        keyboard_device = Common::Input::CreateDevice<Common::Input::InputDevice>(keyboard_params);
        key_index++;
    }

    key_index = 0;
    for (auto& keyboard_device : keyboard_modifier_devices) {
        // Keyboard moddifiers are only mapped on port 1, pad 1
        Common::ParamPackage keyboard_params;
        keyboard_params.Set("engine", "keyboard");
        keyboard_params.Set("button", static_cast<int>(key_index));
        keyboard_params.Set("port", 1);
        keyboard_params.Set("pad", 1);
        keyboard_device = Common::Input::CreateDevice<Common::Input::InputDevice>(keyboard_params);
        key_index++;
    }

    ring_analog_device = Common::Input::CreateDevice<Common::Input::InputDevice>(ring_params);

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

    for (std::size_t index = 0; index < mouse_analog_devices.size(); ++index) {
        if (!mouse_analog_devices[index]) {
            continue;
        }
        mouse_analog_devices[index]->SetCallback({
            .on_change =
                [this, index](const Common::Input::CallbackStatus& callback) {
                    SetMouseAnalog(callback, index);
                },
        });
    }

    if (mouse_stick_device) {
        mouse_stick_device->SetCallback({
            .on_change =
                [this](const Common::Input::CallbackStatus& callback) { SetMouseStick(callback); },
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

    if (ring_analog_device) {
        ring_analog_device->SetCallback({
            .on_change =
                [this](const Common::Input::CallbackStatus& callback) { SetRingAnalog(callback); },
        });
    }
}

void EmulatedDevices::UnloadInput() {
    for (auto& button : mouse_button_devices) {
        button.reset();
    }
    for (auto& analog : mouse_analog_devices) {
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
    Settings::values.ringcon_analogs = ring_params.Serialize();
}

void EmulatedDevices::RestoreConfig() {
    if (!is_configuring) {
        return;
    }
    ReloadFromSettings();
}

Common::ParamPackage EmulatedDevices::GetRingParam() const {
    return ring_params;
}

void EmulatedDevices::SetRingParam(Common::ParamPackage param) {
    ring_params = std::move(param);
    ReloadInput();
}

void EmulatedDevices::SetKeyboardButton(const Common::Input::CallbackStatus& callback,
                                        std::size_t index) {
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

    // Index should be converted from NativeKeyboard to KeyboardKeyIndex
    UpdateKey(index, current_status.value);

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

void EmulatedDevices::SetMouseButton(const Common::Input::CallbackStatus& callback,
                                     std::size_t index) {
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

void EmulatedDevices::SetMouseAnalog(const Common::Input::CallbackStatus& callback,
                                     std::size_t index) {
    if (index >= device_status.mouse_analog_values.size()) {
        return;
    }
    std::lock_guard lock{mutex};
    const auto analog_value = TransformToAnalog(callback);

    device_status.mouse_analog_values[index] = analog_value;

    if (is_configuring) {
        device_status.mouse_position_state = {};
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

    TriggerOnChange(DeviceTriggerType::Mouse);
}

void EmulatedDevices::SetMouseStick(const Common::Input::CallbackStatus& callback) {
    std::lock_guard lock{mutex};
    const auto touch_value = TransformToTouch(callback);

    device_status.mouse_stick_value = touch_value;

    if (is_configuring) {
        device_status.mouse_position_state = {};
        TriggerOnChange(DeviceTriggerType::Mouse);
        return;
    }

    device_status.mouse_position_state.x = touch_value.x.value;
    device_status.mouse_position_state.y = touch_value.y.value;

    TriggerOnChange(DeviceTriggerType::Mouse);
}

void EmulatedDevices::SetRingAnalog(const Common::Input::CallbackStatus& callback) {
    std::lock_guard lock{mutex};
    const auto force_value = TransformToStick(callback);

    device_status.ring_analog_value = force_value.x;

    if (is_configuring) {
        device_status.ring_analog_value = {};
        TriggerOnChange(DeviceTriggerType::RingController);
        return;
    }

    device_status.ring_analog_state.force = force_value.x.value;

    TriggerOnChange(DeviceTriggerType::RingController);
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

RingAnalogValue EmulatedDevices::GetRingSensorValues() const {
    return device_status.ring_analog_value;
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

AnalogStickState EmulatedDevices::GetMouseWheel() const {
    return device_status.mouse_wheel_state;
}

RingSensorForce EmulatedDevices::GetRingSensorForce() const {
    return device_status.ring_analog_state;
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
    callback_list.insert_or_assign(last_callback_key, std::move(update_callback));
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

// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#include "common/param_package.h"
#include "common/settings_input.h"
#include "input_common/drivers/keyboard.h"

namespace InputCommon {

constexpr PadIdentifier key_identifier = {
    .guid = Common::UUID{Common::INVALID_UUID},
    .port = 0,
    .pad = 0,
};
constexpr PadIdentifier keyboard_key_identifier = {
    .guid = Common::UUID{Common::INVALID_UUID},
    .port = 1,
    .pad = 0,
};
constexpr PadIdentifier keyboard_modifier_identifier = {
    .guid = Common::UUID{Common::INVALID_UUID},
    .port = 1,
    .pad = 1,
};

Keyboard::Keyboard(std::string input_engine_) : InputEngine(std::move(input_engine_)) {
    // Keyboard is broken into 3 diferent sets:
    // key: Unfiltered intended for controllers.
    // keyboard_key: Allows only Settings::NativeKeyboard::Keys intended for keyboard emulation.
    // keyboard_modifier: Allows only Settings::NativeKeyboard::Modifiers intended for keyboard
    // emulation.
    PreSetController(key_identifier);
    PreSetController(keyboard_key_identifier);
    PreSetController(keyboard_modifier_identifier);
}

void Keyboard::PressKey(int key_code) {
    SetButton(key_identifier, key_code, true);
}

void Keyboard::ReleaseKey(int key_code) {
    SetButton(key_identifier, key_code, false);
}

void Keyboard::PressKeyboardKey(int key_index) {
    if (key_index == Settings::NativeKeyboard::None) {
        return;
    }
    SetButton(keyboard_key_identifier, key_index, true);
}

void Keyboard::ReleaseKeyboardKey(int key_index) {
    if (key_index == Settings::NativeKeyboard::None) {
        return;
    }
    SetButton(keyboard_key_identifier, key_index, false);
}

void Keyboard::SetKeyboardModifiers(int key_modifiers) {
    for (int i = 0; i < 32; ++i) {
        bool key_value = ((key_modifiers >> i) & 0x1) != 0;
        SetButton(keyboard_modifier_identifier, i, key_value);
        // Use the modifier to press the key button equivalent
        switch (i) {
        case Settings::NativeKeyboard::LeftControl:
            SetButton(keyboard_key_identifier, Settings::NativeKeyboard::LeftControlKey, key_value);
            break;
        case Settings::NativeKeyboard::LeftShift:
            SetButton(keyboard_key_identifier, Settings::NativeKeyboard::LeftShiftKey, key_value);
            break;
        case Settings::NativeKeyboard::LeftAlt:
            SetButton(keyboard_key_identifier, Settings::NativeKeyboard::LeftAltKey, key_value);
            break;
        case Settings::NativeKeyboard::LeftMeta:
            SetButton(keyboard_key_identifier, Settings::NativeKeyboard::LeftMetaKey, key_value);
            break;
        case Settings::NativeKeyboard::RightControl:
            SetButton(keyboard_key_identifier, Settings::NativeKeyboard::RightControlKey,
                      key_value);
            break;
        case Settings::NativeKeyboard::RightShift:
            SetButton(keyboard_key_identifier, Settings::NativeKeyboard::RightShiftKey, key_value);
            break;
        case Settings::NativeKeyboard::RightAlt:
            SetButton(keyboard_key_identifier, Settings::NativeKeyboard::RightAltKey, key_value);
            break;
        case Settings::NativeKeyboard::RightMeta:
            SetButton(keyboard_key_identifier, Settings::NativeKeyboard::RightMetaKey, key_value);
            break;
        default:
            // Other modifier keys should be pressed with PressKey since they stay enabled until
            // next press
            break;
        }
    }
}

void Keyboard::ReleaseAllKeys() {
    ResetButtonState();
}

std::vector<Common::ParamPackage> Keyboard::GetInputDevices() const {
    std::vector<Common::ParamPackage> devices;
    devices.emplace_back(Common::ParamPackage{
        {"engine", GetEngineName()},
        {"display", "Keyboard Only"},
    });
    return devices;
}

} // namespace InputCommon

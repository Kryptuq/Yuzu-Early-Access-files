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
constexpr PadIdentifier modifier_identifier = {
    .guid = Common::UUID{Common::INVALID_UUID},
    .port = 0,
    .pad = 1,
};

Keyboard::Keyboard(const std::string& input_engine_) : InputEngine(input_engine_) {
    PreSetController(key_identifier);
    PreSetController(modifier_identifier);
}

void Keyboard::PressKey(int key_code) {
    SetButton(key_identifier, key_code, true);
}

void Keyboard::ReleaseKey(int key_code) {
    SetButton(key_identifier, key_code, false);
}

void Keyboard::SetModifiers(int key_modifiers) {
    for (int i = 0; i < 32; ++i) {
        bool key_value = ((key_modifiers >> i) & 0x1) != 0;
        SetButton(modifier_identifier, i, key_value);
        // Use the modifier to press the key button equivalent
        switch (i) {
        case Settings::NativeKeyboard::LeftControl:
            SetButton(key_identifier, Settings::NativeKeyboard::LeftControlKey, key_value);
            break;
        case Settings::NativeKeyboard::LeftShift:
            SetButton(key_identifier, Settings::NativeKeyboard::LeftShiftKey, key_value);
            break;
        case Settings::NativeKeyboard::LeftAlt:
            SetButton(key_identifier, Settings::NativeKeyboard::LeftAltKey, key_value);
            break;
        case Settings::NativeKeyboard::LeftMeta:
            SetButton(key_identifier, Settings::NativeKeyboard::LeftMetaKey, key_value);
            break;
        case Settings::NativeKeyboard::RightControl:
            SetButton(key_identifier, Settings::NativeKeyboard::RightControlKey, key_value);
            break;
        case Settings::NativeKeyboard::RightShift:
            SetButton(key_identifier, Settings::NativeKeyboard::RightShiftKey, key_value);
            break;
        case Settings::NativeKeyboard::RightAlt:
            SetButton(key_identifier, Settings::NativeKeyboard::RightAltKey, key_value);
            break;
        case Settings::NativeKeyboard::RightMeta:
            SetButton(key_identifier, Settings::NativeKeyboard::RightMetaKey, key_value);
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

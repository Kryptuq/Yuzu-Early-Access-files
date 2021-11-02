// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#include "common/param_package.h"
#include "input_common/drivers/keyboard.h"

namespace InputCommon {

constexpr PadIdentifier identifier = {
    .guid = Common::UUID{Common::INVALID_UUID},
    .port = 0,
    .pad = 0,
};

Keyboard::Keyboard(const std::string& input_engine_) : InputEngine(input_engine_) {
    PreSetController(identifier);
}

void Keyboard::PressKey(int key_code) {
    SetButton(identifier, key_code, true);
}

void Keyboard::ReleaseKey(int key_code) {
    SetButton(identifier, key_code, false);
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

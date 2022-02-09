// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#include "common/param_package.h"
#include "input_common/drivers/touch_screen.h"

namespace InputCommon {

constexpr PadIdentifier identifier = {
    .guid = Common::UUID{},
    .port = 0,
    .pad = 0,
};

TouchScreen::TouchScreen(std::string input_engine_) : InputEngine(std::move(input_engine_)) {
    PreSetController(identifier);
}

void TouchScreen::TouchMoved(float x, float y, std::size_t finger) {
    if (finger >= 16) {
        return;
    }
    TouchPressed(x, y, finger);
}

void TouchScreen::TouchPressed(float x, float y, std::size_t finger) {
    if (finger >= 16) {
        return;
    }
    SetButton(identifier, static_cast<int>(finger), true);
    SetAxis(identifier, static_cast<int>(finger * 2), x);
    SetAxis(identifier, static_cast<int>(finger * 2 + 1), y);
}

void TouchScreen::TouchReleased(std::size_t finger) {
    if (finger >= 16) {
        return;
    }
    SetButton(identifier, static_cast<int>(finger), false);
    SetAxis(identifier, static_cast<int>(finger * 2), 0.0f);
    SetAxis(identifier, static_cast<int>(finger * 2 + 1), 0.0f);
}

void TouchScreen::ReleaseAllTouch() {
    for (int index = 0; index < 16; ++index) {
        SetButton(identifier, index, false);
        SetAxis(identifier, index * 2, 0.0f);
        SetAxis(identifier, index * 2 + 1, 0.0f);
    }
}

} // namespace InputCommon

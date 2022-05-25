// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
    ReleaseAllTouch();
}

void TouchScreen::TouchMoved(float x, float y, std::size_t finger_id) {
    const auto index = GetIndexFromFingerId(finger_id);
    if (!index) {
        // Touch doesn't exist handle it as a new one
        TouchPressed(x, y, finger_id);
        return;
    }
    const auto i = index.value();
    fingers[i].is_active = true;
    SetButton(identifier, static_cast<int>(i), true);
    SetAxis(identifier, static_cast<int>(i * 2), x);
    SetAxis(identifier, static_cast<int>(i * 2 + 1), y);
}

void TouchScreen::TouchPressed(float x, float y, std::size_t finger_id) {
    if (GetIndexFromFingerId(finger_id)) {
        // Touch already exist. Just update the data
        TouchMoved(x, y, finger_id);
        return;
    }
    const auto index = GetNextFreeIndex();
    if (!index) {
        // No free entries. Ignore input
        return;
    }
    const auto i = index.value();
    fingers[i].is_enabled = true;
    fingers[i].finger_id = finger_id;
    TouchMoved(x, y, finger_id);
}

void TouchScreen::TouchReleased(std::size_t finger_id) {
    const auto index = GetIndexFromFingerId(finger_id);
    if (!index) {
        return;
    }
    const auto i = index.value();
    fingers[i].is_enabled = false;
    SetButton(identifier, static_cast<int>(i), false);
    SetAxis(identifier, static_cast<int>(i * 2), 0.0f);
    SetAxis(identifier, static_cast<int>(i * 2 + 1), 0.0f);
}

std::optional<std::size_t> TouchScreen::GetIndexFromFingerId(std::size_t finger_id) const {
    for (std::size_t index = 0; index < MAX_FINGER_COUNT; ++index) {
        const auto& finger = fingers[index];
        if (!finger.is_enabled) {
            continue;
        }
        if (finger.finger_id == finger_id) {
            return index;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> TouchScreen::GetNextFreeIndex() const {
    for (std::size_t index = 0; index < MAX_FINGER_COUNT; ++index) {
        if (!fingers[index].is_enabled) {
            return index;
        }
    }
    return std::nullopt;
}

void TouchScreen::ClearActiveFlag() {
    for (auto& finger : fingers) {
        finger.is_active = false;
    }
}

void TouchScreen::ReleaseInactiveTouch() {
    for (const auto& finger : fingers) {
        if (!finger.is_active) {
            TouchReleased(finger.finger_id);
        }
    }
}

void TouchScreen::ReleaseAllTouch() {
    for (const auto& finger : fingers) {
        if (finger.is_enabled) {
            TouchReleased(finger.finger_id);
        }
    }
}

} // namespace InputCommon

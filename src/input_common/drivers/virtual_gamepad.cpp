// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "input_common/drivers/virtual_gamepad.h"

namespace InputCommon {
constexpr std::size_t PlayerIndexCount = 10;

VirtualGamepad::VirtualGamepad(std::string input_engine_) : InputEngine(std::move(input_engine_)) {
    for (std::size_t i = 0; i < PlayerIndexCount; i++) {
        PreSetController(GetIdentifier(i));
    }
}

void VirtualGamepad::SetButtonState(std::size_t player_index, int button_id, bool value) {
    if (player_index > PlayerIndexCount) {
        return;
    }
    const auto identifier = GetIdentifier(player_index);
    SetButton(identifier, button_id, value);
}

void VirtualGamepad::SetButtonState(std::size_t player_index, VirtualButton button_id, bool value) {
    SetButtonState(player_index, static_cast<int>(button_id), value);
}

void VirtualGamepad::SetStickPosition(std::size_t player_index, int axis_id, float x_value,
                                      float y_value) {
    if (player_index > PlayerIndexCount) {
        return;
    }
    const auto identifier = GetIdentifier(player_index);
    SetAxis(identifier, axis_id * 2, x_value);
    SetAxis(identifier, (axis_id * 2) + 1, y_value);
}

void VirtualGamepad::SetStickPosition(std::size_t player_index, VirtualStick axis_id, float x_value,
                                      float y_value) {
    SetStickPosition(player_index, static_cast<int>(axis_id), x_value, y_value);
}

void VirtualGamepad::ResetControllers() {
    for (std::size_t i = 0; i < PlayerIndexCount; i++) {
        SetStickPosition(i, VirtualStick::Left, 0.0f, 0.0f);
        SetStickPosition(i, VirtualStick::Right, 0.0f, 0.0f);

        SetButtonState(i, VirtualButton::ButtonA, false);
        SetButtonState(i, VirtualButton::ButtonB, false);
        SetButtonState(i, VirtualButton::ButtonX, false);
        SetButtonState(i, VirtualButton::ButtonY, false);
        SetButtonState(i, VirtualButton::StickL, false);
        SetButtonState(i, VirtualButton::StickR, false);
        SetButtonState(i, VirtualButton::TriggerL, false);
        SetButtonState(i, VirtualButton::TriggerR, false);
        SetButtonState(i, VirtualButton::TriggerZL, false);
        SetButtonState(i, VirtualButton::TriggerZR, false);
        SetButtonState(i, VirtualButton::ButtonPlus, false);
        SetButtonState(i, VirtualButton::ButtonMinus, false);
        SetButtonState(i, VirtualButton::ButtonLeft, false);
        SetButtonState(i, VirtualButton::ButtonUp, false);
        SetButtonState(i, VirtualButton::ButtonRight, false);
        SetButtonState(i, VirtualButton::ButtonDown, false);
        SetButtonState(i, VirtualButton::ButtonSL, false);
        SetButtonState(i, VirtualButton::ButtonSR, false);
        SetButtonState(i, VirtualButton::ButtonHome, false);
        SetButtonState(i, VirtualButton::ButtonCapture, false);
    }
}

PadIdentifier VirtualGamepad::GetIdentifier(std::size_t player_index) const {
    return {
        .guid = Common::UUID{},
        .port = player_index,
        .pad = 0,
    };
}

} // namespace InputCommon

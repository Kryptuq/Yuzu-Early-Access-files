// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#include "common/settings_input.h"
#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "yuzu/controller_navigation.h"

ControllerNavigation::ControllerNavigation(Core::HID::HIDCore& hid_core, QWidget* parent) {
    controller = hid_core.GetEmulatedController(Core::HID::NpadIdType::Player1);
    Core::HID::ControllerUpdateCallback engine_callback{
        .on_change = [this](Core::HID::ControllerTriggerType type) { ControllerUpdateEvent(type); },
        .is_npad_service = false,
    };
    callback_key = controller->SetCallback(engine_callback);
    is_controller_set = true;
}

ControllerNavigation::~ControllerNavigation() {
    UnloadController();
}

void ControllerNavigation::UnloadController() {
    if (is_controller_set) {
        controller->DeleteCallback(callback_key);
        is_controller_set = false;
    }
}

void ControllerNavigation::TriggerButton(const Core::HID::ButtonValues& buttons,
                                         Settings::NativeButton::Values native_button,
                                         Qt::Key key) {
    if (buttons[native_button].value && !button_values[native_button].locked) {
        emit TriggerKeyboardEvent(key);
    }
}

void ControllerNavigation::ControllerUpdateEvent(Core::HID::ControllerTriggerType type) {
    if (type == Core::HID::ControllerTriggerType::Button) {
        const auto controller_type = controller->GetNpadStyleIndex();
        const auto& buttons = controller->GetButtonsValues();
        for (std::size_t i = 0; i < buttons.size(); ++i) {
            // Trigger only once
            if (buttons[i].value == button_values[i].value) {
                button_values[i].locked = true;
            } else {
                button_values[i].value = buttons[i].value;
                button_values[i].locked = false;
            }
        }

        switch (controller_type) {
        case Core::HID::NpadStyleIndex::ProController:
        case Core::HID::NpadStyleIndex::JoyconDual:
        case Core::HID::NpadStyleIndex::Handheld:
        case Core::HID::NpadStyleIndex::GameCube:
            TriggerButton(buttons, Settings::NativeButton::A, Qt::Key_Enter);
            TriggerButton(buttons, Settings::NativeButton::B, Qt::Key_Escape);
            TriggerButton(buttons, Settings::NativeButton::DDown, Qt::Key_Down);
            TriggerButton(buttons, Settings::NativeButton::DLeft, Qt::Key_Left);
            TriggerButton(buttons, Settings::NativeButton::DRight, Qt::Key_Right);
            TriggerButton(buttons, Settings::NativeButton::DUp, Qt::Key_Up);
            break;
        case Core::HID::NpadStyleIndex::JoyconLeft:
            TriggerButton(buttons, Settings::NativeButton::DDown, Qt::Key_Enter);
            TriggerButton(buttons, Settings::NativeButton::DLeft, Qt::Key_Escape);
            break;
        case Core::HID::NpadStyleIndex::JoyconRight:
            TriggerButton(buttons, Settings::NativeButton::X, Qt::Key_Enter);
            TriggerButton(buttons, Settings::NativeButton::A, Qt::Key_Escape);
            break;
        default:
            break;
        }
        return;
    }

    if (type == Core::HID::ControllerTriggerType::Stick) {
        const auto controller_type = controller->GetNpadStyleIndex();
        const auto& sticks = controller->GetSticksValues();
        bool update = false;

        for (std::size_t i = 0; i < sticks.size(); ++i) {
            // Trigger only once
            if (sticks[i].down != stick_values[i].down || sticks[i].left != stick_values[i].left ||
                sticks[i].right != stick_values[i].right || sticks[i].up != stick_values[i].up) {
                update = true;
            }
            stick_values[i] = sticks[i];
        }

        if (!update) {
            return;
        }

        switch (controller_type) {
        case Core::HID::NpadStyleIndex::ProController:
        case Core::HID::NpadStyleIndex::JoyconDual:
        case Core::HID::NpadStyleIndex::Handheld:
        case Core::HID::NpadStyleIndex::GameCube:
            if (sticks[Settings::NativeAnalog::LStick].down) {
                emit TriggerKeyboardEvent(Qt::Key_Down);
                return;
            }
            if (sticks[Settings::NativeAnalog::LStick].left) {
                emit TriggerKeyboardEvent(Qt::Key_Left);
                return;
            }
            if (sticks[Settings::NativeAnalog::LStick].right) {
                emit TriggerKeyboardEvent(Qt::Key_Right);
                return;
            }
            if (sticks[Settings::NativeAnalog::LStick].up) {
                emit TriggerKeyboardEvent(Qt::Key_Up);
                return;
            }
            break;
        case Core::HID::NpadStyleIndex::JoyconLeft:
            if (sticks[Settings::NativeAnalog::LStick].left) {
                emit TriggerKeyboardEvent(Qt::Key_Down);
                return;
            }
            if (sticks[Settings::NativeAnalog::LStick].up) {
                emit TriggerKeyboardEvent(Qt::Key_Left);
                return;
            }
            if (sticks[Settings::NativeAnalog::LStick].down) {
                emit TriggerKeyboardEvent(Qt::Key_Right);
                return;
            }
            if (sticks[Settings::NativeAnalog::LStick].right) {
                emit TriggerKeyboardEvent(Qt::Key_Up);
                return;
            }
            break;
        case Core::HID::NpadStyleIndex::JoyconRight:
            if (sticks[Settings::NativeAnalog::RStick].right) {
                emit TriggerKeyboardEvent(Qt::Key_Down);
                return;
            }
            if (sticks[Settings::NativeAnalog::RStick].down) {
                emit TriggerKeyboardEvent(Qt::Key_Left);
                return;
            }
            if (sticks[Settings::NativeAnalog::RStick].up) {
                emit TriggerKeyboardEvent(Qt::Key_Right);
                return;
            }
            if (sticks[Settings::NativeAnalog::RStick].left) {
                emit TriggerKeyboardEvent(Qt::Key_Up);
                return;
            }
            break;
        default:
            break;
        }
        return;
    }
}

// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <sstream>
#include <QKeySequence>
#include <QShortcut>
#include <QTreeWidgetItem>
#include <QtGlobal>

#include "core/hid/emulated_controller.h"
#include "yuzu/hotkeys.h"
#include "yuzu/uisettings.h"

HotkeyRegistry::HotkeyRegistry() = default;
HotkeyRegistry::~HotkeyRegistry() = default;

void HotkeyRegistry::SaveHotkeys() {
    UISettings::values.shortcuts.clear();
    for (const auto& group : hotkey_groups) {
        for (const auto& hotkey : group.second) {
            UISettings::values.shortcuts.push_back(
                {hotkey.first, group.first,
                 UISettings::ContextualShortcut({hotkey.second.keyseq.toString(),
                                                 hotkey.second.controller_keyseq,
                                                 hotkey.second.context})});
        }
    }
}

void HotkeyRegistry::LoadHotkeys() {
    // Make sure NOT to use a reference here because it would become invalid once we call
    // beginGroup()
    for (auto shortcut : UISettings::values.shortcuts) {
        Hotkey& hk = hotkey_groups[shortcut.group][shortcut.name];
        if (!shortcut.shortcut.keyseq.isEmpty()) {
            hk.keyseq =
                QKeySequence::fromString(shortcut.shortcut.keyseq, QKeySequence::NativeText);
            hk.context = static_cast<Qt::ShortcutContext>(shortcut.shortcut.context);
        }
        if (!shortcut.shortcut.controller_keyseq.isEmpty()) {
            hk.controller_keyseq = shortcut.shortcut.controller_keyseq;
        }
        if (hk.shortcut) {
            hk.shortcut->disconnect();
            hk.shortcut->setKey(hk.keyseq);
        }
        if (hk.controller_shortcut) {
            hk.controller_shortcut->disconnect();
            hk.controller_shortcut->SetKey(hk.controller_keyseq);
        }
    }
}

QShortcut* HotkeyRegistry::GetHotkey(const QString& group, const QString& action, QWidget* widget) {
    Hotkey& hk = hotkey_groups[group][action];

    if (!hk.shortcut) {
        hk.shortcut = new QShortcut(hk.keyseq, widget, nullptr, nullptr, hk.context);
    }

    hk.shortcut->setAutoRepeat(false);

    return hk.shortcut;
}

ControllerShortcut* HotkeyRegistry::GetControllerHotkey(const QString& group, const QString& action,
                                                        Core::HID::EmulatedController* controller) {
    Hotkey& hk = hotkey_groups[group][action];

    if (!hk.controller_shortcut) {
        hk.controller_shortcut = new ControllerShortcut(controller);
        hk.controller_shortcut->SetKey(hk.controller_keyseq);
    }

    return hk.controller_shortcut;
}

QKeySequence HotkeyRegistry::GetKeySequence(const QString& group, const QString& action) {
    return hotkey_groups[group][action].keyseq;
}

Qt::ShortcutContext HotkeyRegistry::GetShortcutContext(const QString& group,
                                                       const QString& action) {
    return hotkey_groups[group][action].context;
}

ControllerShortcut::ControllerShortcut(Core::HID::EmulatedController* controller) {
    emulated_controller = controller;
    Core::HID::ControllerUpdateCallback engine_callback{
        .on_change = [this](Core::HID::ControllerTriggerType type) { ControllerUpdateEvent(type); },
        .is_npad_service = false,
    };
    callback_key = emulated_controller->SetCallback(engine_callback);
    is_enabled = true;
}

ControllerShortcut::~ControllerShortcut() {
    emulated_controller->DeleteCallback(callback_key);
}

void ControllerShortcut::SetKey(const ControllerButtonSequence& buttons) {
    button_sequence = buttons;
}

void ControllerShortcut::SetKey(const QString& buttons_shortcut) {
    ControllerButtonSequence sequence{};
    name = buttons_shortcut.toStdString();
    std::istringstream command_line(buttons_shortcut.toStdString());
    std::string line;
    while (std::getline(command_line, line, '+')) {
        if (line.empty()) {
            continue;
        }
        if (line == "A") {
            sequence.npad.a.Assign(1);
        }
        if (line == "B") {
            sequence.npad.b.Assign(1);
        }
        if (line == "X") {
            sequence.npad.x.Assign(1);
        }
        if (line == "Y") {
            sequence.npad.y.Assign(1);
        }
        if (line == "L") {
            sequence.npad.l.Assign(1);
        }
        if (line == "R") {
            sequence.npad.r.Assign(1);
        }
        if (line == "ZL") {
            sequence.npad.zl.Assign(1);
        }
        if (line == "ZR") {
            sequence.npad.zr.Assign(1);
        }
        if (line == "Dpad_Left") {
            sequence.npad.left.Assign(1);
        }
        if (line == "Dpad_Right") {
            sequence.npad.right.Assign(1);
        }
        if (line == "Dpad_Up") {
            sequence.npad.up.Assign(1);
        }
        if (line == "Dpad_Down") {
            sequence.npad.down.Assign(1);
        }
        if (line == "Left_Stick") {
            sequence.npad.stick_l.Assign(1);
        }
        if (line == "Right_Stick") {
            sequence.npad.stick_r.Assign(1);
        }
        if (line == "Minus") {
            sequence.npad.minus.Assign(1);
        }
        if (line == "Plus") {
            sequence.npad.plus.Assign(1);
        }
        if (line == "Home") {
            sequence.home.home.Assign(1);
        }
        if (line == "Screenshot") {
            sequence.capture.capture.Assign(1);
        }
    }

    button_sequence = sequence;
}

ControllerButtonSequence ControllerShortcut::ButtonSequence() const {
    return button_sequence;
}

void ControllerShortcut::SetEnabled(bool enable) {
    is_enabled = enable;
}

bool ControllerShortcut::IsEnabled() const {
    return is_enabled;
}

void ControllerShortcut::ControllerUpdateEvent(Core::HID::ControllerTriggerType type) {
    if (!is_enabled) {
        return;
    }
    if (type != Core::HID::ControllerTriggerType::Button) {
        return;
    }
    if (button_sequence.npad.raw == Core::HID::NpadButton::None &&
        button_sequence.capture.raw == 0 && button_sequence.home.raw == 0) {
        return;
    }

    const auto player_npad_buttons =
        emulated_controller->GetNpadButtons().raw & button_sequence.npad.raw;
    const u64 player_capture_buttons =
        emulated_controller->GetCaptureButtons().raw & button_sequence.capture.raw;
    const u64 player_home_buttons =
        emulated_controller->GetHomeButtons().raw & button_sequence.home.raw;

    if (player_npad_buttons == button_sequence.npad.raw &&
        player_capture_buttons == button_sequence.capture.raw &&
        player_home_buttons == button_sequence.home.raw && !active) {
        // Force user to press the home or capture button again
        emulated_controller->ResetSystemButtons();
        active = true;
        emit Activated();
        return;
    }
    active = false;
}

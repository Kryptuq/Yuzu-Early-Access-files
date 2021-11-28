// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#pragma once

#include <QKeyEvent>
#include <QObject>

#include "common/input.h"
#include "common/settings.h"
#include "core/hid/emulated_controller.h"

namespace Core::HID {
class HIDCore;
} // namespace Core::HID

class ControllerNavigation : public QObject {
    Q_OBJECT

public:
    explicit ControllerNavigation(Core::HID::HIDCore& hid_core, QWidget* parent = nullptr);
    ~ControllerNavigation();

    /// Disables events from the emulated controller
    void UnloadController();

signals:
    void TriggerKeyboardEvent(Qt::Key key);

private:
    void TriggerButton(const Core::HID::ButtonValues& buttons,
                       Settings::NativeButton::Values native_button, Qt::Key key);
    void ControllerUpdateEvent(Core::HID::ControllerTriggerType type);

    Core::HID::ButtonValues button_values{};
    Core::HID::SticksValues stick_values{};

    int callback_key;
    bool is_controller_set{};
    Core::HID::EmulatedController* controller;
};

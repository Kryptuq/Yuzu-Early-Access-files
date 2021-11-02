// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <QWidget>

class QAction;
class QHideEvent;
class QShowEvent;
class PlayerControlPreview;

namespace InputCommon {
class InputSubsystem;
}

namespace Core {
class System;
}

namespace Core::HID {
class EmulatedController;
enum class ControllerTriggerType;
} // namespace Core::HID

class ControllerDialog : public QWidget {
    Q_OBJECT

public:
    explicit ControllerDialog(Core::System& system_,
                              std::shared_ptr<InputCommon::InputSubsystem> input_subsystem_,
                              QWidget* parent = nullptr);

    /// Returns a QAction that can be used to toggle visibility of this dialog.
    QAction* toggleViewAction();

    /// Reloads the widget to apply any changes in the configuration
    void refreshConfiguration();

    /// Disables events from the emulated controller
    void UnloadController();

protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private:
    /// Redirects input from the widget to the TAS driver
    void ControllerUpdate(Core::HID::ControllerTriggerType type);

    int callback_key;
    bool is_controller_set{};
    Core::HID::EmulatedController* controller;

    QAction* toggle_view_action = nullptr;
    PlayerControlPreview* widget;
    Core::System& system;
    std::shared_ptr<InputCommon::InputSubsystem> input_subsystem;
};

// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QDialog>

class QPushButton;

class ConfigureInputPlayer;

class InputProfiles;

namespace Core::HID {
class HIDCore;
}

namespace InputCommon {
class InputSubsystem;
}

namespace Ui {
class ConfigureDebugController;
}

class ConfigureDebugController : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureDebugController(QWidget* parent, InputCommon::InputSubsystem* input_subsystem,
                                      InputProfiles* profiles, Core::HID::HIDCore& hid_core,
                                      bool is_powered_on);
    ~ConfigureDebugController() override;

    void ApplyConfiguration();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    std::unique_ptr<Ui::ConfigureDebugController> ui;

    ConfigureInputPlayer* debug_controller;
};

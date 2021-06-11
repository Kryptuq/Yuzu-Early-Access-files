// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QDialog>

class QPushButton;

class ConfigureInputPlayer;

class InputProfiles;

namespace InputCommon {
class InputSubsystem;
}

namespace Ui {
class ConfigureInputProfileDialog;
}

class ConfigureInputProfileDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureInputProfileDialog(QWidget* parent,
                                         InputCommon::InputSubsystem* input_subsystem,
                                         InputProfiles* profiles);
    ~ConfigureInputProfileDialog() override;

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    std::unique_ptr<Ui::ConfigureInputProfileDialog> ui;

    ConfigureInputPlayer* profile_widget;
};

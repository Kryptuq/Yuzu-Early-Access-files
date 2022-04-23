// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <QDialog>

class QPushButton;

class ConfigureInputPlayer;

class InputProfiles;

namespace Core {
class System;
}

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
                                         InputProfiles* profiles, Core::System& system);
    ~ConfigureInputProfileDialog() override;

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    std::unique_ptr<Ui::ConfigureInputProfileDialog> ui;

    ConfigureInputPlayer* profile_widget;
};

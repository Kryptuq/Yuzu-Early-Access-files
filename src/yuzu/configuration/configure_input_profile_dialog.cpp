// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "ui_configure_input_profile_dialog.h"
#include "yuzu/configuration/configure_input_player.h"
#include "yuzu/configuration/configure_input_profile_dialog.h"

ConfigureInputProfileDialog::ConfigureInputProfileDialog(
    QWidget* parent, InputCommon::InputSubsystem* input_subsystem, InputProfiles* profiles)
    : QDialog(parent), ui(std::make_unique<Ui::ConfigureInputProfileDialog>()),
      profile_widget(new ConfigureInputPlayer(this, 9, nullptr, input_subsystem, profiles, false)) {
    ui->setupUi(this);

    ui->controllerLayout->addWidget(profile_widget);

    connect(ui->clear_all_button, &QPushButton::clicked, this,
            [this] { profile_widget->ClearAll(); });
    connect(ui->restore_defaults_button, &QPushButton::clicked, this,
            [this] { profile_widget->RestoreDefaults(); });

    RetranslateUI();
}

ConfigureInputProfileDialog::~ConfigureInputProfileDialog() = default;

void ConfigureInputProfileDialog::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QDialog::changeEvent(event);
}

void ConfigureInputProfileDialog::RetranslateUI() {
    ui->retranslateUi(this);
}

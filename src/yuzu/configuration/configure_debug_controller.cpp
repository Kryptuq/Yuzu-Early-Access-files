// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hid/hid_core.h"
#include "ui_configure_debug_controller.h"
#include "yuzu/configuration/configure_debug_controller.h"
#include "yuzu/configuration/configure_input_player.h"

ConfigureDebugController::ConfigureDebugController(QWidget* parent,
                                                   InputCommon::InputSubsystem* input_subsystem,
                                                   InputProfiles* profiles,
                                                   Core::HID::HIDCore& hid_core, bool is_powered_on)
    : QDialog(parent), ui(std::make_unique<Ui::ConfigureDebugController>()),
      debug_controller(new ConfigureInputPlayer(this, 9, nullptr, input_subsystem, profiles,
                                                hid_core, is_powered_on, true)) {
    ui->setupUi(this);

    ui->controllerLayout->addWidget(debug_controller);

    connect(ui->clear_all_button, &QPushButton::clicked, this,
            [this] { debug_controller->ClearAll(); });
    connect(ui->restore_defaults_button, &QPushButton::clicked, this,
            [this] { debug_controller->RestoreDefaults(); });

    RetranslateUI();
}

ConfigureDebugController::~ConfigureDebugController() = default;

void ConfigureDebugController::ApplyConfiguration() {
    debug_controller->ApplyConfiguration();
}

void ConfigureDebugController::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QDialog::changeEvent(event);
}

void ConfigureDebugController::RetranslateUI() {
    ui->retranslateUi(this);
}

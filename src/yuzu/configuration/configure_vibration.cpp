// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <unordered_map>

#include <fmt/format.h>

#include "common/param_package.h"
#include "common/settings.h"
#include "ui_configure_vibration.h"
#include "yuzu/configuration/configure_vibration.h"

ConfigureVibration::ConfigureVibration(QWidget* parent)
    : QDialog(parent), ui(std::make_unique<Ui::ConfigureVibration>()) {
    ui->setupUi(this);

    vibration_groupboxes = {
        ui->vibrationGroupPlayer1, ui->vibrationGroupPlayer2, ui->vibrationGroupPlayer3,
        ui->vibrationGroupPlayer4, ui->vibrationGroupPlayer5, ui->vibrationGroupPlayer6,
        ui->vibrationGroupPlayer7, ui->vibrationGroupPlayer8,
    };

    vibration_spinboxes = {
        ui->vibrationSpinPlayer1, ui->vibrationSpinPlayer2, ui->vibrationSpinPlayer3,
        ui->vibrationSpinPlayer4, ui->vibrationSpinPlayer5, ui->vibrationSpinPlayer6,
        ui->vibrationSpinPlayer7, ui->vibrationSpinPlayer8,
    };

    const auto& players = Settings::values.players.GetValue();

    for (std::size_t i = 0; i < NUM_PLAYERS; ++i) {
        vibration_groupboxes[i]->setChecked(players[i].vibration_enabled);
        vibration_spinboxes[i]->setValue(players[i].vibration_strength);
    }

    ui->checkBoxAccurateVibration->setChecked(
        Settings::values.enable_accurate_vibrations.GetValue());

    if (!Settings::IsConfiguringGlobal()) {
        ui->checkBoxAccurateVibration->setDisabled(true);
    }

    RetranslateUI();
}

ConfigureVibration::~ConfigureVibration() = default;

void ConfigureVibration::ApplyConfiguration() {
    auto& players = Settings::values.players.GetValue();

    for (std::size_t i = 0; i < NUM_PLAYERS; ++i) {
        players[i].vibration_enabled = vibration_groupboxes[i]->isChecked();
        players[i].vibration_strength = vibration_spinboxes[i]->value();
    }

    Settings::values.enable_accurate_vibrations.SetValue(
        ui->checkBoxAccurateVibration->isChecked());
}

void ConfigureVibration::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QDialog::changeEvent(event);
}

void ConfigureVibration::RetranslateUI() {
    ui->retranslateUi(this);
}

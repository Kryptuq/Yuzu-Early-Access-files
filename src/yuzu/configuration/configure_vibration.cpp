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

void ConfigureVibration::SetVibrationDevices(std::size_t player_index) {
    using namespace Settings::NativeButton;
    static constexpr std::array<std::array<Settings::NativeButton::Values, 6>, 2> buttons{{
        {DLeft, DUp, DRight, DDown, L, ZL}, // Left Buttons
        {A, B, X, Y, R, ZR},                // Right Buttons
    }};

    auto& player = Settings::values.players.GetValue()[player_index];

    for (std::size_t device_idx = 0; device_idx < buttons.size(); ++device_idx) {
        std::unordered_map<std::string, int> params_count;

        for (const auto button_index : buttons[device_idx]) {
            const auto& player_button = player.buttons[button_index];

            if (params_count.find(player_button) != params_count.end()) {
                ++params_count[player_button];
                continue;
            }

            params_count.insert_or_assign(player_button, 1);
        }

        const auto it = std::max_element(
            params_count.begin(), params_count.end(),
            [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });

        auto& vibration_param_str = player.vibrations[device_idx];
        vibration_param_str.clear();

        if (it->first.empty()) {
            continue;
        }

        const auto param = Common::ParamPackage(it->first);

        const auto engine = param.Get("engine", "");
        const auto guid = param.Get("guid", "");
        const auto port = param.Get("port", "");

        if (engine.empty() || engine == "keyboard" || engine == "mouse" || engine == "tas") {
            continue;
        }

        vibration_param_str += fmt::format("engine:{}", engine);

        if (!port.empty()) {
            vibration_param_str += fmt::format(",port:{}", port);
        }
        if (!guid.empty()) {
            vibration_param_str += fmt::format(",guid:{}", guid);
        }
    }

    if (player.vibrations[0] != player.vibrations[1]) {
        return;
    }

    if (!player.vibrations[0].empty() &&
        player.controller_type != Settings::ControllerType::RightJoycon) {
        player.vibrations[1].clear();
    } else if (!player.vibrations[1].empty() &&
               player.controller_type == Settings::ControllerType::RightJoycon) {
        player.vibrations[0].clear();
    }
}

void ConfigureVibration::SetAllVibrationDevices() {
    // Set vibration devices for all player indices including handheld
    for (std::size_t player_idx = 0; player_idx < NUM_PLAYERS + 1; ++player_idx) {
        SetVibrationDevices(player_idx);
    }
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

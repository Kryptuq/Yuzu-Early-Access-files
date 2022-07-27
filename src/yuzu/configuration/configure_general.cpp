// SPDX-FileCopyrightText: 2016 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <functional>
#include <utility>
#include <QMessageBox>
#include "common/settings.h"
#include "core/core.h"
#include "ui_configure_general.h"
#include "yuzu/configuration/configuration_shared.h"
#include "yuzu/configuration/configure_general.h"
#include "yuzu/uisettings.h"

ConfigureGeneral::ConfigureGeneral(const Core::System& system_, QWidget* parent)
    : QWidget(parent), ui{std::make_unique<Ui::ConfigureGeneral>()}, system{system_} {
    ui->setupUi(this);

    SetupPerGameUI();

    SetConfiguration();

    if (Settings::IsConfiguringGlobal()) {
        connect(ui->toggle_speed_limit, &QCheckBox::clicked, ui->speed_limit,
                [this]() { ui->speed_limit->setEnabled(ui->toggle_speed_limit->isChecked()); });
    }

    connect(ui->button_reset_defaults, &QPushButton::clicked, this,
            &ConfigureGeneral::ResetDefaults);
}

ConfigureGeneral::~ConfigureGeneral() = default;

void ConfigureGeneral::SetConfiguration() {
    const bool runtime_lock = !system.IsPoweredOn();

    ui->use_multi_core->setEnabled(runtime_lock);
    ui->use_multi_core->setChecked(Settings::values.use_multi_core.GetValue());
    ui->use_extended_memory_layout->setEnabled(runtime_lock);
    ui->use_extended_memory_layout->setChecked(
        Settings::values.use_extended_memory_layout.GetValue());

    ui->toggle_check_exit->setChecked(UISettings::values.confirm_before_closing.GetValue());
    ui->toggle_user_on_boot->setChecked(UISettings::values.select_user_on_boot.GetValue());
    ui->toggle_background_pause->setChecked(UISettings::values.pause_when_in_background.GetValue());
    ui->toggle_background_mute->setChecked(UISettings::values.mute_when_in_background.GetValue());
    ui->toggle_hide_mouse->setChecked(UISettings::values.hide_mouse.GetValue());

    ui->toggle_speed_limit->setChecked(Settings::values.use_speed_limit.GetValue());
    ui->speed_limit->setValue(Settings::values.speed_limit.GetValue());

    ui->button_reset_defaults->setEnabled(runtime_lock);

    if (Settings::IsConfiguringGlobal()) {
        ui->speed_limit->setEnabled(Settings::values.use_speed_limit.GetValue());
    } else {
        ui->speed_limit->setEnabled(Settings::values.use_speed_limit.GetValue() &&
                                    use_speed_limit != ConfigurationShared::CheckState::Global);
    }
}

// Called to set the callback when resetting settings to defaults
void ConfigureGeneral::SetResetCallback(std::function<void()> callback) {
    reset_callback = std::move(callback);
}

void ConfigureGeneral::ResetDefaults() {
    QMessageBox::StandardButton answer = QMessageBox::question(
        this, tr("yuzu"),
        tr("This reset all settings and remove all per-game configurations. This will not delete "
           "game directories, profiles, or input profiles. Proceed?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer == QMessageBox::No) {
        return;
    }
    UISettings::values.reset_to_defaults = true;
    UISettings::values.is_game_list_reload_pending.exchange(true);
    reset_callback();
}

void ConfigureGeneral::ApplyConfiguration() {
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.use_multi_core, ui->use_multi_core,
                                             use_multi_core);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.use_extended_memory_layout,
                                             ui->use_extended_memory_layout,
                                             use_extended_memory_layout);

    if (Settings::IsConfiguringGlobal()) {
        UISettings::values.confirm_before_closing = ui->toggle_check_exit->isChecked();
        UISettings::values.select_user_on_boot = ui->toggle_user_on_boot->isChecked();
        UISettings::values.pause_when_in_background = ui->toggle_background_pause->isChecked();
        UISettings::values.mute_when_in_background = ui->toggle_background_mute->isChecked();
        UISettings::values.hide_mouse = ui->toggle_hide_mouse->isChecked();

        // Guard if during game and set to game-specific value
        if (Settings::values.use_speed_limit.UsingGlobal()) {
            Settings::values.use_speed_limit.SetValue(ui->toggle_speed_limit->checkState() ==
                                                      Qt::Checked);
            Settings::values.speed_limit.SetValue(ui->speed_limit->value());
        }
    } else {
        bool global_speed_limit = use_speed_limit == ConfigurationShared::CheckState::Global;
        Settings::values.use_speed_limit.SetGlobal(global_speed_limit);
        Settings::values.speed_limit.SetGlobal(global_speed_limit);
        if (!global_speed_limit) {
            Settings::values.use_speed_limit.SetValue(ui->toggle_speed_limit->checkState() ==
                                                      Qt::Checked);
            Settings::values.speed_limit.SetValue(ui->speed_limit->value());
        }
    }
}

void ConfigureGeneral::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureGeneral::RetranslateUI() {
    ui->retranslateUi(this);
}

void ConfigureGeneral::SetupPerGameUI() {
    if (Settings::IsConfiguringGlobal()) {
        // Disables each setting if:
        //  - A game is running (thus settings in use), and
        //  - A non-global setting is applied.
        ui->toggle_speed_limit->setEnabled(Settings::values.use_speed_limit.UsingGlobal());
        ui->speed_limit->setEnabled(Settings::values.speed_limit.UsingGlobal());

        return;
    }

    ui->toggle_check_exit->setVisible(false);
    ui->toggle_user_on_boot->setVisible(false);
    ui->toggle_background_pause->setVisible(false);
    ui->toggle_hide_mouse->setVisible(false);

    ui->button_reset_defaults->setVisible(false);

    ConfigurationShared::SetColoredTristate(ui->toggle_speed_limit,
                                            Settings::values.use_speed_limit, use_speed_limit);
    ConfigurationShared::SetColoredTristate(ui->use_multi_core, Settings::values.use_multi_core,
                                            use_multi_core);
    ConfigurationShared::SetColoredTristate(ui->use_extended_memory_layout,
                                            Settings::values.use_extended_memory_layout,
                                            use_extended_memory_layout);

    connect(ui->toggle_speed_limit, &QCheckBox::clicked, ui->speed_limit, [this]() {
        ui->speed_limit->setEnabled(ui->toggle_speed_limit->isChecked() &&
                                    (use_speed_limit != ConfigurationShared::CheckState::Global));
    });
}

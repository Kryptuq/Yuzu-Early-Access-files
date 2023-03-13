// SPDX-FileCopyrightText: 2016 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <optional>

#include <QFileDialog>
#include <QGraphicsItem>
#include <QMessageBox>
#include "common/settings.h"
#include "core/core.h"
#include "core/hle/service/time/time_manager.h"
#include "ui_configure_system.h"
#include "yuzu/configuration/configuration_shared.h"
#include "yuzu/configuration/configure_system.h"

constexpr std::array<u32, 7> LOCALE_BLOCKLIST{
    // pzzefezrpnkzeidfej
    // thhsrnhutlohsternp
    // BHH4CG          U
    // Raa1AB          S
    //  nn9
    //  ts
    0b0100011100001100000, // Japan
    0b0000001101001100100, // Americas
    0b0100110100001000010, // Europe
    0b0100110100001000010, // Australia
    0b0000000000000000000, // China
    0b0100111100001000000, // Korea
    0b0100111100001000000, // Taiwan
};

static bool IsValidLocale(u32 region_index, u32 language_index) {
    if (region_index >= LOCALE_BLOCKLIST.size()) {
        return false;
    }
    return ((LOCALE_BLOCKLIST.at(region_index) >> language_index) & 1) == 0;
}

ConfigureSystem::ConfigureSystem(Core::System& system_, QWidget* parent)
    : QWidget(parent), ui{std::make_unique<Ui::ConfigureSystem>()}, system{system_} {
    ui->setupUi(this);

    connect(ui->rng_seed_checkbox, &QCheckBox::stateChanged, this, [this](int state) {
        ui->rng_seed_edit->setEnabled(state == Qt::Checked);
        if (state != Qt::Checked) {
            ui->rng_seed_edit->setText(QStringLiteral("00000000"));
        }
    });

    connect(ui->custom_rtc_checkbox, &QCheckBox::stateChanged, this, [this](int state) {
        ui->custom_rtc_edit->setEnabled(state == Qt::Checked);
        if (state != Qt::Checked) {
            ui->custom_rtc_edit->setDateTime(QDateTime::currentDateTime());
        }
    });

    const auto locale_check = [this](int index) {
        const auto region_index = ConfigurationShared::GetComboboxIndex(
            Settings::values.region_index.GetValue(true), ui->combo_region);
        const auto language_index = ConfigurationShared::GetComboboxIndex(
            Settings::values.language_index.GetValue(true), ui->combo_language);
        const bool valid_locale = IsValidLocale(region_index, language_index);
        ui->label_warn_invalid_locale->setVisible(!valid_locale);
        if (!valid_locale) {
            ui->label_warn_invalid_locale->setText(
                tr("Warning: \"%1\" is not a valid language for region \"%2\"")
                    .arg(ui->combo_language->currentText())
                    .arg(ui->combo_region->currentText()));
        }
    };

    connect(ui->combo_language, qOverload<int>(&QComboBox::currentIndexChanged), this,
            locale_check);
    connect(ui->combo_region, qOverload<int>(&QComboBox::currentIndexChanged), this, locale_check);

    SetupPerGameUI();

    SetConfiguration();
}

ConfigureSystem::~ConfigureSystem() = default;

void ConfigureSystem::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureSystem::RetranslateUI() {
    ui->retranslateUi(this);
}

void ConfigureSystem::SetConfiguration() {
    enabled = !system.IsPoweredOn();
    const auto rng_seed =
        QStringLiteral("%1")
            .arg(Settings::values.rng_seed.GetValue().value_or(0), 8, 16, QLatin1Char{'0'})
            .toUpper();
    const auto rtc_time = Settings::values.custom_rtc.value_or(QDateTime::currentSecsSinceEpoch());

    ui->rng_seed_checkbox->setChecked(Settings::values.rng_seed.GetValue().has_value());
    ui->rng_seed_edit->setEnabled(Settings::values.rng_seed.GetValue().has_value() &&
                                  Settings::values.rng_seed.UsingGlobal());
    ui->rng_seed_edit->setText(rng_seed);

    ui->custom_rtc_checkbox->setChecked(Settings::values.custom_rtc.has_value());
    ui->custom_rtc_edit->setEnabled(Settings::values.custom_rtc.has_value());
    ui->custom_rtc_edit->setDateTime(QDateTime::fromSecsSinceEpoch(rtc_time));
    ui->device_name_edit->setText(
        QString::fromUtf8(Settings::values.device_name.GetValue().c_str()));

    if (Settings::IsConfiguringGlobal()) {
        ui->combo_language->setCurrentIndex(Settings::values.language_index.GetValue());
        ui->combo_region->setCurrentIndex(Settings::values.region_index.GetValue());
        ui->combo_time_zone->setCurrentIndex(Settings::values.time_zone_index.GetValue());
    } else {
        ConfigurationShared::SetPerGameSetting(ui->combo_language,
                                               &Settings::values.language_index);
        ConfigurationShared::SetPerGameSetting(ui->combo_region, &Settings::values.region_index);
        ConfigurationShared::SetPerGameSetting(ui->combo_time_zone,
                                               &Settings::values.time_zone_index);

        ConfigurationShared::SetHighlight(ui->label_language,
                                          !Settings::values.language_index.UsingGlobal());
        ConfigurationShared::SetHighlight(ui->label_region,
                                          !Settings::values.region_index.UsingGlobal());
        ConfigurationShared::SetHighlight(ui->label_timezone,
                                          !Settings::values.time_zone_index.UsingGlobal());
    }
}

void ConfigureSystem::ReadSystemSettings() {}

void ConfigureSystem::ApplyConfiguration() {
    // Allow setting custom RTC even if system is powered on,
    // to allow in-game time to be fast forwarded
    if (Settings::IsConfiguringGlobal()) {
        if (ui->custom_rtc_checkbox->isChecked()) {
            Settings::values.custom_rtc = ui->custom_rtc_edit->dateTime().toSecsSinceEpoch();
            if (system.IsPoweredOn()) {
                const s64 posix_time{*Settings::values.custom_rtc +
                                     Service::Time::TimeManager::GetExternalTimeZoneOffset()};
                system.GetTimeManager().UpdateLocalSystemClockTime(posix_time);
            }
        } else {
            Settings::values.custom_rtc = std::nullopt;
        }
    }

    Settings::values.device_name = ui->device_name_edit->text().toStdString();

    if (!enabled) {
        return;
    }

    ConfigurationShared::ApplyPerGameSetting(&Settings::values.language_index, ui->combo_language);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.region_index, ui->combo_region);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.time_zone_index,
                                             ui->combo_time_zone);

    if (Settings::IsConfiguringGlobal()) {
        // Guard if during game and set to game-specific value
        if (Settings::values.rng_seed.UsingGlobal()) {
            if (ui->rng_seed_checkbox->isChecked()) {
                Settings::values.rng_seed.SetValue(ui->rng_seed_edit->text().toUInt(nullptr, 16));
            } else {
                Settings::values.rng_seed.SetValue(std::nullopt);
            }
        }
    } else {
        switch (use_rng_seed) {
        case ConfigurationShared::CheckState::On:
        case ConfigurationShared::CheckState::Off:
            Settings::values.rng_seed.SetGlobal(false);
            if (ui->rng_seed_checkbox->isChecked()) {
                Settings::values.rng_seed.SetValue(ui->rng_seed_edit->text().toUInt(nullptr, 16));
            } else {
                Settings::values.rng_seed.SetValue(std::nullopt);
            }
            break;
        case ConfigurationShared::CheckState::Global:
            Settings::values.rng_seed.SetGlobal(false);
            Settings::values.rng_seed.SetValue(std::nullopt);
            Settings::values.rng_seed.SetGlobal(true);
            break;
        case ConfigurationShared::CheckState::Count:
            break;
        }
    }
}

void ConfigureSystem::SetupPerGameUI() {
    if (Settings::IsConfiguringGlobal()) {
        ui->combo_language->setEnabled(Settings::values.language_index.UsingGlobal());
        ui->combo_region->setEnabled(Settings::values.region_index.UsingGlobal());
        ui->combo_time_zone->setEnabled(Settings::values.time_zone_index.UsingGlobal());
        ui->rng_seed_checkbox->setEnabled(Settings::values.rng_seed.UsingGlobal());
        ui->rng_seed_edit->setEnabled(Settings::values.rng_seed.UsingGlobal());

        return;
    }

    ConfigurationShared::SetColoredComboBox(ui->combo_language, ui->label_language,
                                            Settings::values.language_index.GetValue(true));
    ConfigurationShared::SetColoredComboBox(ui->combo_region, ui->label_region,
                                            Settings::values.region_index.GetValue(true));
    ConfigurationShared::SetColoredComboBox(ui->combo_time_zone, ui->label_timezone,
                                            Settings::values.time_zone_index.GetValue(true));

    ConfigurationShared::SetColoredTristate(
        ui->rng_seed_checkbox, Settings::values.rng_seed.UsingGlobal(),
        Settings::values.rng_seed.GetValue().has_value(),
        Settings::values.rng_seed.GetValue(true).has_value(), use_rng_seed);

    ui->custom_rtc_checkbox->setVisible(false);
    ui->custom_rtc_edit->setVisible(false);
}

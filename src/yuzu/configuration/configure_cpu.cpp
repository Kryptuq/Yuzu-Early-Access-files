// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QComboBox>
#include <QMessageBox>

#include "common/common_types.h"
#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core.h"
#include "ui_configure_cpu.h"
#include "yuzu/configuration/configuration_shared.h"
#include "yuzu/configuration/configure_cpu.h"

ConfigureCpu::ConfigureCpu(QWidget* parent) : QWidget(parent), ui(new Ui::ConfigureCpu) {
    ui->setupUi(this);

    SetupPerGameUI();

    SetConfiguration();

    connect(ui->accuracy, qOverload<int>(&QComboBox::activated), this,
            &ConfigureCpu::AccuracyUpdated);
    connect(ui->accuracy, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ConfigureCpu::UpdateGroup);
}

ConfigureCpu::~ConfigureCpu() = default;

void ConfigureCpu::SetConfiguration() {
    const bool runtime_lock = !Core::System::GetInstance().IsPoweredOn();

    ui->accuracy->setEnabled(runtime_lock);
    ui->cpuopt_unsafe_unfuse_fma->setEnabled(runtime_lock);
    ui->cpuopt_unsafe_reduce_fp_error->setEnabled(runtime_lock);
    ui->cpuopt_unsafe_ignore_standard_fpcr->setEnabled(runtime_lock);
    ui->cpuopt_unsafe_inaccurate_nan->setEnabled(runtime_lock);
    ui->cpuopt_unsafe_fastmem_check->setEnabled(runtime_lock);

    ui->cpuopt_unsafe_unfuse_fma->setChecked(Settings::values.cpuopt_unsafe_unfuse_fma.GetValue());
    ui->cpuopt_unsafe_reduce_fp_error->setChecked(
        Settings::values.cpuopt_unsafe_reduce_fp_error.GetValue());
    ui->cpuopt_unsafe_ignore_standard_fpcr->setChecked(
        Settings::values.cpuopt_unsafe_ignore_standard_fpcr.GetValue());
    ui->cpuopt_unsafe_inaccurate_nan->setChecked(
        Settings::values.cpuopt_unsafe_inaccurate_nan.GetValue());
    ui->cpuopt_unsafe_fastmem_check->setChecked(
        Settings::values.cpuopt_unsafe_fastmem_check.GetValue());

    if (Settings::IsConfiguringGlobal()) {
        ui->accuracy->setCurrentIndex(static_cast<int>(Settings::values.cpu_accuracy.GetValue()));
    } else {
        ConfigurationShared::SetPerGameSetting(ui->accuracy, &Settings::values.cpu_accuracy);
        ConfigurationShared::SetHighlight(ui->widget_accuracy,
                                          !Settings::values.cpu_accuracy.UsingGlobal());
    }
    UpdateGroup(ui->accuracy->currentIndex());
}

void ConfigureCpu::AccuracyUpdated(int index) {
    if (Settings::IsConfiguringGlobal() &&
        static_cast<Settings::CPUAccuracy>(index) == Settings::CPUAccuracy::DebugMode) {
        const auto result = QMessageBox::warning(this, tr("Setting CPU to Debug Mode"),
                                                 tr("CPU Debug Mode is only intended for developer "
                                                    "use. Are you sure you want to enable this?"),
                                                 QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::No) {
            ui->accuracy->setCurrentIndex(static_cast<int>(Settings::CPUAccuracy::Accurate));
            UpdateGroup(static_cast<int>(Settings::CPUAccuracy::Accurate));
        }
    }
}

void ConfigureCpu::UpdateGroup(int index) {
    if (!Settings::IsConfiguringGlobal()) {
        index -= ConfigurationShared::USE_GLOBAL_OFFSET;
    }
    const auto accuracy = static_cast<Settings::CPUAccuracy>(index);
    ui->unsafe_group->setVisible(accuracy == Settings::CPUAccuracy::Unsafe);
}

void ConfigureCpu::ApplyConfiguration() {
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.cpuopt_unsafe_unfuse_fma,
                                             ui->cpuopt_unsafe_unfuse_fma,
                                             cpuopt_unsafe_unfuse_fma);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.cpuopt_unsafe_reduce_fp_error,
                                             ui->cpuopt_unsafe_reduce_fp_error,
                                             cpuopt_unsafe_reduce_fp_error);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.cpuopt_unsafe_ignore_standard_fpcr,
                                             ui->cpuopt_unsafe_ignore_standard_fpcr,
                                             cpuopt_unsafe_ignore_standard_fpcr);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.cpuopt_unsafe_inaccurate_nan,
                                             ui->cpuopt_unsafe_inaccurate_nan,
                                             cpuopt_unsafe_inaccurate_nan);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.cpuopt_unsafe_fastmem_check,
                                             ui->cpuopt_unsafe_fastmem_check,
                                             cpuopt_unsafe_fastmem_check);

    if (Settings::IsConfiguringGlobal()) {
        // Guard if during game and set to game-specific value
        if (Settings::values.cpu_accuracy.UsingGlobal()) {
            Settings::values.cpu_accuracy.SetValue(
                static_cast<Settings::CPUAccuracy>(ui->accuracy->currentIndex()));
        }
    } else {
        if (ui->accuracy->currentIndex() == ConfigurationShared::USE_GLOBAL_INDEX) {
            Settings::values.cpu_accuracy.SetGlobal(true);
        } else {
            Settings::values.cpu_accuracy.SetGlobal(false);
            Settings::values.cpu_accuracy.SetValue(static_cast<Settings::CPUAccuracy>(
                ui->accuracy->currentIndex() - ConfigurationShared::USE_GLOBAL_OFFSET));
        }
    }
}

void ConfigureCpu::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureCpu::RetranslateUI() {
    ui->retranslateUi(this);
}

void ConfigureCpu::SetupPerGameUI() {
    if (Settings::IsConfiguringGlobal()) {
        return;
    }

    ConfigurationShared::SetColoredComboBox(
        ui->accuracy, ui->widget_accuracy,
        static_cast<u32>(Settings::values.cpu_accuracy.GetValue(true)));
    ui->accuracy->removeItem(static_cast<u32>(Settings::CPUAccuracy::DebugMode) +
                             ConfigurationShared::USE_GLOBAL_OFFSET);

    ConfigurationShared::SetColoredTristate(ui->cpuopt_unsafe_unfuse_fma,
                                            Settings::values.cpuopt_unsafe_unfuse_fma,
                                            cpuopt_unsafe_unfuse_fma);
    ConfigurationShared::SetColoredTristate(ui->cpuopt_unsafe_reduce_fp_error,
                                            Settings::values.cpuopt_unsafe_reduce_fp_error,
                                            cpuopt_unsafe_reduce_fp_error);
    ConfigurationShared::SetColoredTristate(ui->cpuopt_unsafe_ignore_standard_fpcr,
                                            Settings::values.cpuopt_unsafe_ignore_standard_fpcr,
                                            cpuopt_unsafe_ignore_standard_fpcr);
    ConfigurationShared::SetColoredTristate(ui->cpuopt_unsafe_inaccurate_nan,
                                            Settings::values.cpuopt_unsafe_inaccurate_nan,
                                            cpuopt_unsafe_inaccurate_nan);
    ConfigurationShared::SetColoredTristate(ui->cpuopt_unsafe_fastmem_check,
                                            Settings::values.cpuopt_unsafe_fastmem_check,
                                            cpuopt_unsafe_fastmem_check);
}

// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/common_types.h"
#include "common/settings.h"
#include "core/core.h"
#include "ui_configure_cpu.h"
#include "yuzu/configuration/configuration_shared.h"
#include "yuzu/configuration/configure_cpu.h"

ConfigureCpu::ConfigureCpu(const Core::System& system_, QWidget* parent)
    : QWidget(parent), ui{std::make_unique<Ui::ConfigureCpu>()}, system{system_} {
    ui->setupUi(this);

    SetupPerGameUI();

    SetConfiguration();

    connect(ui->accuracy, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ConfigureCpu::UpdateGroup);
}

ConfigureCpu::~ConfigureCpu() = default;

void ConfigureCpu::SetConfiguration() {
    const bool runtime_lock = !system.IsPoweredOn();

    ui->accuracy->setEnabled(runtime_lock);
    ui->cpuopt_unsafe_unfuse_fma->setEnabled(runtime_lock);
    ui->cpuopt_unsafe_reduce_fp_error->setEnabled(runtime_lock);
    ui->cpuopt_unsafe_ignore_standard_fpcr->setEnabled(runtime_lock);
    ui->cpuopt_unsafe_inaccurate_nan->setEnabled(runtime_lock);
    ui->cpuopt_unsafe_fastmem_check->setEnabled(runtime_lock);
    ui->cpuopt_unsafe_ignore_global_monitor->setEnabled(runtime_lock);

    ui->cpuopt_unsafe_unfuse_fma->setChecked(Settings::values.cpuopt_unsafe_unfuse_fma.GetValue());
    ui->cpuopt_unsafe_reduce_fp_error->setChecked(
        Settings::values.cpuopt_unsafe_reduce_fp_error.GetValue());
    ui->cpuopt_unsafe_ignore_standard_fpcr->setChecked(
        Settings::values.cpuopt_unsafe_ignore_standard_fpcr.GetValue());
    ui->cpuopt_unsafe_inaccurate_nan->setChecked(
        Settings::values.cpuopt_unsafe_inaccurate_nan.GetValue());
    ui->cpuopt_unsafe_fastmem_check->setChecked(
        Settings::values.cpuopt_unsafe_fastmem_check.GetValue());
    ui->cpuopt_unsafe_ignore_global_monitor->setChecked(
        Settings::values.cpuopt_unsafe_ignore_global_monitor.GetValue());

    if (Settings::IsConfiguringGlobal()) {
        ui->accuracy->setCurrentIndex(static_cast<int>(Settings::values.cpu_accuracy.GetValue()));
    } else {
        ConfigurationShared::SetPerGameSetting(ui->accuracy, &Settings::values.cpu_accuracy);
        ConfigurationShared::SetHighlight(ui->widget_accuracy,
                                          !Settings::values.cpu_accuracy.UsingGlobal());
    }
    UpdateGroup(ui->accuracy->currentIndex());
}

void ConfigureCpu::UpdateGroup(int index) {
    if (!Settings::IsConfiguringGlobal()) {
        index -= ConfigurationShared::USE_GLOBAL_OFFSET;
    }
    const auto accuracy = static_cast<Settings::CPUAccuracy>(index);
    ui->unsafe_group->setVisible(accuracy == Settings::CPUAccuracy::Unsafe);
}

void ConfigureCpu::ApplyConfiguration() {
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.cpu_accuracy, ui->accuracy);
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
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.cpuopt_unsafe_ignore_global_monitor,
                                             ui->cpuopt_unsafe_ignore_global_monitor,
                                             cpuopt_unsafe_ignore_global_monitor);
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
    ConfigurationShared::SetColoredTristate(ui->cpuopt_unsafe_ignore_global_monitor,
                                            Settings::values.cpuopt_unsafe_ignore_global_monitor,
                                            cpuopt_unsafe_ignore_global_monitor);
}

// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QComboBox>

#include "common/common_types.h"
#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core.h"
#include "ui_configure_cpu_debug.h"
#include "yuzu/configuration/configure_cpu_debug.h"

ConfigureCpuDebug::ConfigureCpuDebug(QWidget* parent)
    : QWidget(parent), ui(new Ui::ConfigureCpuDebug) {
    ui->setupUi(this);

    SetConfiguration();
}

ConfigureCpuDebug::~ConfigureCpuDebug() = default;

void ConfigureCpuDebug::SetConfiguration() {
    const bool runtime_lock = !Core::System::GetInstance().IsPoweredOn();

    ui->cpuopt_page_tables->setEnabled(runtime_lock);
    ui->cpuopt_page_tables->setChecked(Settings::values.cpuopt_page_tables.GetValue());
    ui->cpuopt_block_linking->setEnabled(runtime_lock);
    ui->cpuopt_block_linking->setChecked(Settings::values.cpuopt_block_linking.GetValue());
    ui->cpuopt_return_stack_buffer->setEnabled(runtime_lock);
    ui->cpuopt_return_stack_buffer->setChecked(
        Settings::values.cpuopt_return_stack_buffer.GetValue());
    ui->cpuopt_fast_dispatcher->setEnabled(runtime_lock);
    ui->cpuopt_fast_dispatcher->setChecked(Settings::values.cpuopt_fast_dispatcher.GetValue());
    ui->cpuopt_context_elimination->setEnabled(runtime_lock);
    ui->cpuopt_context_elimination->setChecked(
        Settings::values.cpuopt_context_elimination.GetValue());
    ui->cpuopt_const_prop->setEnabled(runtime_lock);
    ui->cpuopt_const_prop->setChecked(Settings::values.cpuopt_const_prop.GetValue());
    ui->cpuopt_misc_ir->setEnabled(runtime_lock);
    ui->cpuopt_misc_ir->setChecked(Settings::values.cpuopt_misc_ir.GetValue());
    ui->cpuopt_reduce_misalign_checks->setEnabled(runtime_lock);
    ui->cpuopt_reduce_misalign_checks->setChecked(
        Settings::values.cpuopt_reduce_misalign_checks.GetValue());
    ui->cpuopt_fastmem->setEnabled(runtime_lock);
    ui->cpuopt_fastmem->setChecked(Settings::values.cpuopt_fastmem.GetValue());
}

void ConfigureCpuDebug::ApplyConfiguration() {
    Settings::values.cpuopt_page_tables = ui->cpuopt_page_tables->isChecked();
    Settings::values.cpuopt_block_linking = ui->cpuopt_block_linking->isChecked();
    Settings::values.cpuopt_return_stack_buffer = ui->cpuopt_return_stack_buffer->isChecked();
    Settings::values.cpuopt_fast_dispatcher = ui->cpuopt_fast_dispatcher->isChecked();
    Settings::values.cpuopt_context_elimination = ui->cpuopt_context_elimination->isChecked();
    Settings::values.cpuopt_const_prop = ui->cpuopt_const_prop->isChecked();
    Settings::values.cpuopt_misc_ir = ui->cpuopt_misc_ir->isChecked();
    Settings::values.cpuopt_reduce_misalign_checks = ui->cpuopt_reduce_misalign_checks->isChecked();
    Settings::values.cpuopt_fastmem = ui->cpuopt_fastmem->isChecked();
}

void ConfigureCpuDebug::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureCpuDebug::RetranslateUI() {
    ui->retranslateUi(this);
}

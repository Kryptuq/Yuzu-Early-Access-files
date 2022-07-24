// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>
#include "ui_configure_debug_tab.h"
#include "yuzu/configuration/configure_cpu_debug.h"
#include "yuzu/configuration/configure_debug.h"
#include "yuzu/configuration/configure_debug_tab.h"

ConfigureDebugTab::ConfigureDebugTab(const Core::System& system_, QWidget* parent)
    : QWidget(parent), ui{std::make_unique<Ui::ConfigureDebugTab>()},
      debug_tab{std::make_unique<ConfigureDebug>(system_, this)},
      cpu_debug_tab{std::make_unique<ConfigureCpuDebug>(system_, this)} {
    ui->setupUi(this);

    ui->tabWidget->addTab(debug_tab.get(), tr("Debug"));
    ui->tabWidget->addTab(cpu_debug_tab.get(), tr("CPU"));

    SetConfiguration();
}

ConfigureDebugTab::~ConfigureDebugTab() = default;

void ConfigureDebugTab::ApplyConfiguration() {
    debug_tab->ApplyConfiguration();
    cpu_debug_tab->ApplyConfiguration();
}

void ConfigureDebugTab::SetCurrentIndex(int index) {
    ui->tabWidget->setCurrentIndex(index);
}

void ConfigureDebugTab::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureDebugTab::RetranslateUI() {
    ui->retranslateUi(this);
}

void ConfigureDebugTab::SetConfiguration() {}

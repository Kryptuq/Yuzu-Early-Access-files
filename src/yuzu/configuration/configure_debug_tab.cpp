// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "ui_configure_debug_tab.h"
#include "yuzu/configuration/configure_debug_tab.h"

ConfigureDebugTab::ConfigureDebugTab(QWidget* parent)
    : QWidget(parent), ui(new Ui::ConfigureDebugTab) {
    ui->setupUi(this);

    SetConfiguration();
}

ConfigureDebugTab::~ConfigureDebugTab() = default;

void ConfigureDebugTab::ApplyConfiguration() {
    ui->debugTab->ApplyConfiguration();
    ui->cpuDebugTab->ApplyConfiguration();
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

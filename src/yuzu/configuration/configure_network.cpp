// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QtConcurrent/QtConcurrent>
#include "common/settings.h"
#include "core/core.h"
#include "core/network/network_interface.h"
#include "ui_configure_network.h"
#include "yuzu/configuration/configure_network.h"

ConfigureNetwork::ConfigureNetwork(const Core::System& system_, QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureNetwork>()), system{system_} {
    ui->setupUi(this);

    ui->network_interface->addItem(tr("None"));
    for (const auto& iface : Network::GetAvailableNetworkInterfaces()) {
        ui->network_interface->addItem(QString::fromStdString(iface.name));
    }

    this->SetConfiguration();
}

ConfigureNetwork::~ConfigureNetwork() = default;

void ConfigureNetwork::ApplyConfiguration() {
    Settings::values.network_interface = ui->network_interface->currentText().toStdString();
}

void ConfigureNetwork::RetranslateUi() {
    ui->retranslateUi(this);
}

void ConfigureNetwork::SetConfiguration() {
    const bool runtime_lock = !system.IsPoweredOn();

    const std::string& network_interface = Settings::values.network_interface.GetValue();

    ui->network_interface->setCurrentText(QString::fromStdString(network_interface));
    ui->network_interface->setEnabled(runtime_lock);
}

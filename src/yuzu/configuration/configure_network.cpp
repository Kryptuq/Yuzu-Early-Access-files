// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QGraphicsItem>
#include <QtConcurrent/QtConcurrent>
#include "common/settings.h"
#include "core/core.h"
#include "core/hle/service/bcat/backend/boxcat.h"
#include "core/network/network_interface.h"
#include "ui_configure_network.h"
#include "yuzu/configuration/configure_network.h"

#ifdef YUZU_ENABLE_BOXCAT
namespace {
QString FormatEventStatusString(const Service::BCAT::EventStatus& status) {
    QString out;

    if (status.header.has_value()) {
        out += QStringLiteral("<i>%1</i><br>").arg(QString::fromStdString(*status.header));
    }

    if (status.events.size() == 1) {
        out += QStringLiteral("%1<br>").arg(QString::fromStdString(status.events.front()));
    } else {
        for (const auto& event : status.events) {
            out += QStringLiteral("- %1<br>").arg(QString::fromStdString(event));
        }
    }

    if (status.footer.has_value()) {
        out += QStringLiteral("<i>%1</i><br>").arg(QString::fromStdString(*status.footer));
    }

    return out;
}
} // Anonymous namespace
#endif

ConfigureNetwork::ConfigureNetwork(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureNetwork>()) {
    ui->setupUi(this);

    ui->bcat_source->addItem(QStringLiteral("None"));
    ui->bcat_empty_label->setHidden(true);
    ui->bcat_empty_header->setHidden(true);

#ifdef YUZU_ENABLE_BOXCAT
    ui->bcat_source->addItem(QStringLiteral("Boxcat"), QStringLiteral("boxcat"));
#endif

    ui->network_interface->addItem(tr("None"));
    for (const auto& interface : Network::GetAvailableNetworkInterfaces()) {
        ui->network_interface->addItem(QString::fromStdString(interface.name));
    }

    connect(ui->bcat_source, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureNetwork::OnBCATImplChanged);

    this->SetConfiguration();
}

ConfigureNetwork::~ConfigureNetwork() = default;

void ConfigureNetwork::ApplyConfiguration() {
    Settings::values.bcat_backend = ui->bcat_source->currentText().toLower().toStdString();
    Settings::values.network_interface = ui->network_interface->currentText().toStdString();
}

void ConfigureNetwork::RetranslateUi() {
    ui->retranslateUi(this);
}

void ConfigureNetwork::SetConfiguration() {
    const bool runtime_lock = !Core::System::GetInstance().IsPoweredOn();

    const int index =
        ui->bcat_source->findData(QString::fromStdString(Settings::values.bcat_backend.GetValue()));
    ui->bcat_source->setCurrentIndex(index == -1 ? 0 : index);

    const std::string& network_interface = Settings::values.network_interface.GetValue();

    ui->network_interface->setCurrentText(QString::fromStdString(network_interface));
    ui->network_interface->setEnabled(runtime_lock);
}

std::pair<QString, QString> ConfigureNetwork::BCATDownloadEvents() {
#ifdef YUZU_ENABLE_BOXCAT
    std::optional<std::string> global;
    std::map<std::string, Service::BCAT::EventStatus> map;
    const auto res = Service::BCAT::Boxcat::GetStatus(global, map);

    switch (res) {
    case Service::BCAT::Boxcat::StatusResult::Success:
        break;
    case Service::BCAT::Boxcat::StatusResult::Offline:
        return {QString{},
                tr("The boxcat service is offline or you are not connected to the internet.")};
    case Service::BCAT::Boxcat::StatusResult::ParseError:
        return {QString{},
                tr("There was an error while processing the boxcat event data. Contact the yuzu "
                   "developers.")};
    case Service::BCAT::Boxcat::StatusResult::BadClientVersion:
        return {QString{},
                tr("The version of yuzu you are using is either too new or too old for the server. "
                   "Try updating to the latest official release of yuzu.")};
    }

    if (map.empty()) {
        return {QStringLiteral("Current Boxcat Events"),
                tr("There are currently no events on boxcat.")};
    }

    QString out;

    if (global.has_value()) {
        out += QStringLiteral("%1<br>").arg(QString::fromStdString(*global));
    }

    for (const auto& [key, value] : map) {
        out += QStringLiteral("%1<b>%2</b><br>%3")
                   .arg(out.isEmpty() ? QString{} : QStringLiteral("<br>"))
                   .arg(QString::fromStdString(key))
                   .arg(FormatEventStatusString(value));
    }
    return {tr("Current Boxcat Events"), std::move(out)};
#else
    return {tr("Current Boxcat Events"), tr("There are currently no events on boxcat.")};
#endif
}

void ConfigureNetwork::OnBCATImplChanged() {
#ifdef YUZU_ENABLE_BOXCAT
    const auto boxcat = ui->bcat_source->currentText() == QStringLiteral("Boxcat");
    ui->bcat_empty_header->setHidden(!boxcat);
    ui->bcat_empty_label->setHidden(!boxcat);
    ui->bcat_empty_header->setText(QString{});
    ui->bcat_empty_label->setText(tr("Yuzu is retrieving the latest boxcat status..."));

    if (!boxcat)
        return;

    const auto future = QtConcurrent::run([this] { return BCATDownloadEvents(); });

    watcher.setFuture(future);
    connect(&watcher, &QFutureWatcher<std::pair<QString, QString>>::finished, this,
            [this] { OnUpdateBCATEmptyLabel(watcher.result()); });
#endif
}

void ConfigureNetwork::OnUpdateBCATEmptyLabel(std::pair<QString, QString> string) {
#ifdef YUZU_ENABLE_BOXCAT
    const auto boxcat = ui->bcat_source->currentText() == QStringLiteral("Boxcat");
    if (boxcat) {
        ui->bcat_empty_header->setText(string.first);
        ui->bcat_empty_label->setText(string.second);
    }
#endif
}

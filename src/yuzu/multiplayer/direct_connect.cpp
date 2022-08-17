// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QComboBox>
#include <QFuture>
#include <QIntValidator>
#include <QRegExpValidator>
#include <QString>
#include <QtConcurrent/QtConcurrentRun>
#include "common/settings.h"
#include "network/network.h"
#include "ui_direct_connect.h"
#include "yuzu/main.h"
#include "yuzu/multiplayer/client_room.h"
#include "yuzu/multiplayer/direct_connect.h"
#include "yuzu/multiplayer/message.h"
#include "yuzu/multiplayer/state.h"
#include "yuzu/multiplayer/validation.h"
#include "yuzu/uisettings.h"

enum class ConnectionType : u8 { TraversalServer, IP };

DirectConnectWindow::DirectConnectWindow(Network::RoomNetwork& room_network_, QWidget* parent)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint),
      ui(std::make_unique<Ui::DirectConnect>()), room_network{room_network_} {

    ui->setupUi(this);

    // setup the watcher for background connections
    watcher = new QFutureWatcher<void>;
    connect(watcher, &QFutureWatcher<void>::finished, this, &DirectConnectWindow::OnConnection);

    ui->nickname->setValidator(validation.GetNickname());
    ui->nickname->setText(UISettings::values.multiplayer_nickname.GetValue());
    if (ui->nickname->text().isEmpty() && !Settings::values.yuzu_username.GetValue().empty()) {
        // Use yuzu Web Service user name as nickname by default
        ui->nickname->setText(QString::fromStdString(Settings::values.yuzu_username.GetValue()));
    }
    ui->ip->setValidator(validation.GetIP());
    ui->ip->setText(UISettings::values.multiplayer_ip.GetValue());
    ui->port->setValidator(validation.GetPort());
    ui->port->setText(QString::number(UISettings::values.multiplayer_port.GetValue()));

    // TODO(jroweboy): Show or hide the connection options based on the current value of the combo
    // box. Add this back in when the traversal server support is added.
    connect(ui->connect, &QPushButton::clicked, this, &DirectConnectWindow::Connect);
}

DirectConnectWindow::~DirectConnectWindow() = default;

void DirectConnectWindow::RetranslateUi() {
    ui->retranslateUi(this);
}

void DirectConnectWindow::Connect() {
    if (!ui->nickname->hasAcceptableInput()) {
        NetworkMessage::ErrorManager::ShowError(NetworkMessage::ErrorManager::USERNAME_NOT_VALID);
        return;
    }
    if (const auto member = room_network.GetRoomMember().lock()) {
        // Prevent the user from trying to join a room while they are already joining.
        if (member->GetState() == Network::RoomMember::State::Joining) {
            return;
        } else if (member->IsConnected()) {
            // And ask if they want to leave the room if they are already in one.
            if (!NetworkMessage::WarnDisconnect()) {
                return;
            }
        }
    }
    switch (static_cast<ConnectionType>(ui->connection_type->currentIndex())) {
    case ConnectionType::TraversalServer:
        break;
    case ConnectionType::IP:
        if (!ui->ip->hasAcceptableInput()) {
            NetworkMessage::ErrorManager::ShowError(
                NetworkMessage::ErrorManager::IP_ADDRESS_NOT_VALID);
            return;
        }
        if (!ui->port->hasAcceptableInput()) {
            NetworkMessage::ErrorManager::ShowError(NetworkMessage::ErrorManager::PORT_NOT_VALID);
            return;
        }
        break;
    }

    // Store settings
    UISettings::values.multiplayer_nickname = ui->nickname->text();
    UISettings::values.multiplayer_ip = ui->ip->text();
    if (ui->port->isModified() && !ui->port->text().isEmpty()) {
        UISettings::values.multiplayer_port = ui->port->text().toInt();
    } else {
        UISettings::values.multiplayer_port = UISettings::values.multiplayer_port.GetDefault();
    }

    // attempt to connect in a different thread
    QFuture<void> f = QtConcurrent::run([&] {
        if (auto room_member = room_network.GetRoomMember().lock()) {
            auto port = UISettings::values.multiplayer_port.GetValue();
            room_member->Join(ui->nickname->text().toStdString(),
                              ui->ip->text().toStdString().c_str(), port, 0, Network::NoPreferredIP,
                              ui->password->text().toStdString().c_str());
        }
    });
    watcher->setFuture(f);
    // and disable widgets and display a connecting while we wait
    BeginConnecting();
}

void DirectConnectWindow::BeginConnecting() {
    ui->connect->setEnabled(false);
    ui->connect->setText(tr("Connecting"));
}

void DirectConnectWindow::EndConnecting() {
    ui->connect->setEnabled(true);
    ui->connect->setText(tr("Connect"));
}

void DirectConnectWindow::OnConnection() {
    EndConnecting();

    if (auto room_member = room_network.GetRoomMember().lock()) {
        if (room_member->IsConnected()) {
            close();
        }
    }
}

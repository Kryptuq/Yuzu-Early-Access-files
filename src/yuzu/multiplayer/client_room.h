// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "yuzu/multiplayer/chat_room.h"

namespace Ui {
class ClientRoom;
}

class ClientRoomWindow : public QDialog {
    Q_OBJECT

public:
    explicit ClientRoomWindow(QWidget* parent, Network::RoomNetwork& room_network_);
    ~ClientRoomWindow();

    void RetranslateUi();
    void UpdateIconDisplay();

public slots:
    void OnRoomUpdate(const Network::RoomInformation&);
    void OnStateChange(const Network::RoomMember::State&);

signals:
    void RoomInformationChanged(const Network::RoomInformation&);
    void StateChanged(const Network::RoomMember::State&);
    void ShowNotification();

private:
    void Disconnect();
    void UpdateView();
    void SetModPerms(bool is_mod);

    QStandardItemModel* player_list;
    std::unique_ptr<Ui::ClientRoom> ui;
    Network::RoomNetwork& room_network;
};

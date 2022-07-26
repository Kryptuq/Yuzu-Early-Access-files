// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <QDialog>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QVariant>
#include "network/network.h"
#include "yuzu/multiplayer/chat_room.h"
#include "yuzu/multiplayer/validation.h"

namespace Ui {
class HostRoom;
}

namespace Core {
class AnnounceMultiplayerSession;
}

class ConnectionError;
class ComboBoxProxyModel;

class ChatMessage;

namespace Network::VerifyUser {
class Backend;
};

class HostRoomWindow : public QDialog {
    Q_OBJECT

public:
    explicit HostRoomWindow(QWidget* parent, QStandardItemModel* list,
                            std::shared_ptr<Core::AnnounceMultiplayerSession> session,
                            Network::RoomNetwork& room_network_);
    ~HostRoomWindow();

    /**
     * Updates the dialog with a new game list model.
     * This model should be the original model of the game list.
     */
    void UpdateGameList(QStandardItemModel* list);
    void RetranslateUi();

private:
    void Host();
    std::unique_ptr<Network::VerifyUser::Backend> CreateVerifyBackend(bool use_validation) const;

    std::unique_ptr<Ui::HostRoom> ui;
    std::weak_ptr<Core::AnnounceMultiplayerSession> announce_multiplayer_session;
    QStandardItemModel* game_list;
    ComboBoxProxyModel* proxy;
    Validation validation;
    Network::RoomNetwork& room_network;
};

/**
 * Proxy Model for the game list combo box so we can reuse the game list model while still
 * displaying the fields slightly differently
 */
class ComboBoxProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    int columnCount(const QModelIndex& idx) const override {
        return 1;
    }

    QVariant data(const QModelIndex& idx, int role) const override;

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
};

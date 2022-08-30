// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <QDialog>
#include <QFutureWatcher>
#include "core/core.h"
#include "yuzu/multiplayer/validation.h"

namespace Ui {
class DirectConnect;
}

class DirectConnectWindow : public QDialog {
    Q_OBJECT

public:
    explicit DirectConnectWindow(Core::System& system_, QWidget* parent = nullptr);
    ~DirectConnectWindow();

    void RetranslateUi();

signals:
    /**
     * Signalled by this widget when it is closing itself and destroying any state such as
     * connections that it might have.
     */
    void Closed();

private slots:
    void OnConnection();

private:
    void Connect();
    void BeginConnecting();
    void EndConnecting();

    QFutureWatcher<void>* watcher;
    std::unique_ptr<Ui::DirectConnect> ui;
    Validation validation;
    Core::System& system;
    Network::RoomNetwork& room_network;
};

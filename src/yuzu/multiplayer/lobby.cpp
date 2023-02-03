// SPDX-FileCopyrightText: Copyright 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QInputDialog>
#include <QList>
#include <QtConcurrent/QtConcurrentRun>
#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/hle/service/acc/profile_manager.h"
#include "core/internal_network/network_interface.h"
#include "network/network.h"
#include "ui_lobby.h"
#include "yuzu/game_list_p.h"
#include "yuzu/main.h"
#include "yuzu/multiplayer/client_room.h"
#include "yuzu/multiplayer/lobby.h"
#include "yuzu/multiplayer/lobby_p.h"
#include "yuzu/multiplayer/message.h"
#include "yuzu/multiplayer/state.h"
#include "yuzu/multiplayer/validation.h"
#include "yuzu/uisettings.h"
#ifdef ENABLE_WEB_SERVICE
#include "web_service/web_backend.h"
#endif

Lobby::Lobby(QWidget* parent, QStandardItemModel* list,
             std::shared_ptr<Core::AnnounceMultiplayerSession> session, Core::System& system_)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint),
      ui(std::make_unique<Ui::Lobby>()), announce_multiplayer_session(session),
      profile_manager(std::make_unique<Service::Account::ProfileManager>()), system{system_},
      room_network{system.GetRoomNetwork()} {
    ui->setupUi(this);

    // setup the watcher for background connections
    watcher = new QFutureWatcher<void>;

    model = new QStandardItemModel(ui->room_list);

    // Create a proxy to the game list to get the list of games owned
    game_list = new QStandardItemModel;
    UpdateGameList(list);

    proxy = new LobbyFilterProxyModel(this, game_list);
    proxy->setSourceModel(model);
    proxy->setDynamicSortFilter(true);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy->setSortLocaleAware(true);
    ui->room_list->setModel(proxy);
    ui->room_list->header()->setSectionResizeMode(QHeaderView::Interactive);
    ui->room_list->header()->stretchLastSection();
    ui->room_list->setAlternatingRowColors(true);
    ui->room_list->setSelectionMode(QHeaderView::SingleSelection);
    ui->room_list->setSelectionBehavior(QHeaderView::SelectRows);
    ui->room_list->setVerticalScrollMode(QHeaderView::ScrollPerPixel);
    ui->room_list->setHorizontalScrollMode(QHeaderView::ScrollPerPixel);
    ui->room_list->setSortingEnabled(true);
    ui->room_list->setEditTriggers(QHeaderView::NoEditTriggers);
    ui->room_list->setExpandsOnDoubleClick(false);
    ui->room_list->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->nickname->setValidator(validation.GetNickname());
    ui->nickname->setText(UISettings::values.multiplayer_nickname.GetValue());

    // Try find the best nickname by default
    if (ui->nickname->text().isEmpty() || ui->nickname->text() == QStringLiteral("yuzu")) {
        if (!Settings::values.yuzu_username.GetValue().empty()) {
            ui->nickname->setText(
                QString::fromStdString(Settings::values.yuzu_username.GetValue()));
        } else if (!GetProfileUsername().empty()) {
            ui->nickname->setText(QString::fromStdString(GetProfileUsername()));
        } else {
            ui->nickname->setText(QStringLiteral("yuzu"));
        }
    }

    // UI Buttons
    connect(ui->refresh_list, &QPushButton::clicked, this, &Lobby::RefreshLobby);
    connect(ui->games_owned, &QCheckBox::toggled, proxy, &LobbyFilterProxyModel::SetFilterOwned);
    connect(ui->hide_empty, &QCheckBox::toggled, proxy, &LobbyFilterProxyModel::SetFilterEmpty);
    connect(ui->hide_full, &QCheckBox::toggled, proxy, &LobbyFilterProxyModel::SetFilterFull);
    connect(ui->search, &QLineEdit::textChanged, proxy, &LobbyFilterProxyModel::SetFilterSearch);
    connect(ui->room_list, &QTreeView::doubleClicked, this, &Lobby::OnJoinRoom);
    connect(ui->room_list, &QTreeView::clicked, this, &Lobby::OnExpandRoom);

    // Actions
    connect(&room_list_watcher, &QFutureWatcher<AnnounceMultiplayerRoom::RoomList>::finished, this,
            &Lobby::OnRefreshLobby);
}

Lobby::~Lobby() = default;

void Lobby::UpdateGameList(QStandardItemModel* list) {
    game_list->clear();
    for (int i = 0; i < list->rowCount(); i++) {
        auto parent = list->item(i, 0);
        for (int j = 0; j < parent->rowCount(); j++) {
            game_list->appendRow(parent->child(j)->clone());
        }
    }
    if (proxy)
        proxy->UpdateGameList(game_list);
    ui->room_list->sortByColumn(Column::GAME_NAME, Qt::AscendingOrder);
}

void Lobby::RetranslateUi() {
    ui->retranslateUi(this);
}

QString Lobby::PasswordPrompt() {
    bool ok;
    const QString text =
        QInputDialog::getText(this, tr("Password Required to Join"), tr("Password:"),
                              QLineEdit::Password, QString(), &ok);
    return ok ? text : QString();
}

void Lobby::OnExpandRoom(const QModelIndex& index) {
    QModelIndex member_index = proxy->index(index.row(), Column::MEMBER);
    auto member_list = proxy->data(member_index, LobbyItemMemberList::MemberListRole).toList();
}

void Lobby::OnJoinRoom(const QModelIndex& source) {
    if (!Network::GetSelectedNetworkInterface()) {
        LOG_INFO(WebService, "Automatically selected network interface for room network.");
        Network::SelectFirstNetworkInterface();
    }

    if (!Network::GetSelectedNetworkInterface()) {
        NetworkMessage::ErrorManager::ShowError(
            NetworkMessage::ErrorManager::NO_INTERFACE_SELECTED);
        return;
    }

    if (system.IsPoweredOn()) {
        if (!NetworkMessage::WarnGameRunning()) {
            return;
        }
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
    QModelIndex index = source;
    // If the user double clicks on a child row (aka the player list) then use the parent instead
    if (source.parent() != QModelIndex()) {
        index = source.parent();
    }
    if (!ui->nickname->hasAcceptableInput()) {
        NetworkMessage::ErrorManager::ShowError(NetworkMessage::ErrorManager::USERNAME_NOT_VALID);
        return;
    }

    // Get a password to pass if the room is password protected
    QModelIndex password_index = proxy->index(index.row(), Column::ROOM_NAME);
    bool has_password = proxy->data(password_index, LobbyItemName::PasswordRole).toBool();
    const std::string password = has_password ? PasswordPrompt().toStdString() : "";
    if (has_password && password.empty()) {
        return;
    }

    QModelIndex connection_index = proxy->index(index.row(), Column::HOST);
    const std::string nickname = ui->nickname->text().toStdString();
    const std::string ip =
        proxy->data(connection_index, LobbyItemHost::HostIPRole).toString().toStdString();
    int port = proxy->data(connection_index, LobbyItemHost::HostPortRole).toInt();
    const std::string verify_uid =
        proxy->data(connection_index, LobbyItemHost::HostVerifyUIDRole).toString().toStdString();

    // attempt to connect in a different thread
    QFuture<void> f = QtConcurrent::run([nickname, ip, port, password, verify_uid, this] {
        std::string token;
#ifdef ENABLE_WEB_SERVICE
        if (!Settings::values.yuzu_username.GetValue().empty() &&
            !Settings::values.yuzu_token.GetValue().empty()) {
            WebService::Client client(Settings::values.web_api_url.GetValue(),
                                      Settings::values.yuzu_username.GetValue(),
                                      Settings::values.yuzu_token.GetValue());
            token = client.GetExternalJWT(verify_uid).returned_data;
            if (token.empty()) {
                LOG_ERROR(WebService, "Could not get external JWT, verification may fail");
            } else {
                LOG_INFO(WebService, "Successfully requested external JWT: size={}", token.size());
            }
        }
#endif
        if (auto room_member = room_network.GetRoomMember().lock()) {
            room_member->Join(nickname, ip.c_str(), port, 0, Network::NoPreferredIP, password,
                              token);
        }
    });
    watcher->setFuture(f);

    // TODO(jroweboy): disable widgets and display a connecting while we wait

    // Save settings
    UISettings::values.multiplayer_nickname = ui->nickname->text();
    UISettings::values.multiplayer_ip =
        proxy->data(connection_index, LobbyItemHost::HostIPRole).toString();
    UISettings::values.multiplayer_port =
        proxy->data(connection_index, LobbyItemHost::HostPortRole).toInt();
    emit SaveConfig();
}

void Lobby::ResetModel() {
    model->clear();
    model->insertColumns(0, Column::TOTAL);
    model->setHeaderData(Column::MEMBER, Qt::Horizontal, tr("Players"), Qt::DisplayRole);
    model->setHeaderData(Column::ROOM_NAME, Qt::Horizontal, tr("Room Name"), Qt::DisplayRole);
    model->setHeaderData(Column::GAME_NAME, Qt::Horizontal, tr("Preferred Game"), Qt::DisplayRole);
    model->setHeaderData(Column::HOST, Qt::Horizontal, tr("Host"), Qt::DisplayRole);
}

void Lobby::RefreshLobby() {
    if (auto session = announce_multiplayer_session.lock()) {
        ResetModel();
        ui->refresh_list->setEnabled(false);
        ui->refresh_list->setText(tr("Refreshing"));
        room_list_watcher.setFuture(
            QtConcurrent::run([session]() { return session->GetRoomList(); }));
    } else {
        // TODO(jroweboy): Display an error box about announce couldn't be started
    }
}

void Lobby::OnRefreshLobby() {
    AnnounceMultiplayerRoom::RoomList new_room_list = room_list_watcher.result();
    for (auto room : new_room_list) {
        // find the icon for the game if this person owns that game.
        QPixmap smdh_icon;
        for (int r = 0; r < game_list->rowCount(); ++r) {
            auto index = game_list->index(r, 0);
            auto game_id = game_list->data(index, GameListItemPath::ProgramIdRole).toULongLong();

            if (game_id != 0 && room.information.preferred_game.id == game_id) {
                smdh_icon = game_list->data(index, Qt::DecorationRole).value<QPixmap>();
            }
        }

        QList<QVariant> members;
        for (auto member : room.members) {
            QVariant var;
            var.setValue(LobbyMember{QString::fromStdString(member.username),
                                     QString::fromStdString(member.nickname), member.game.id,
                                     QString::fromStdString(member.game.name)});
            members.append(var);
        }

        auto first_item = new LobbyItemGame(
            room.information.preferred_game.id,
            QString::fromStdString(room.information.preferred_game.name), smdh_icon);
        auto row = QList<QStandardItem*>({
            first_item,
            new LobbyItemName(room.has_password, QString::fromStdString(room.information.name)),
            new LobbyItemMemberList(members, room.information.member_slots),
            new LobbyItemHost(QString::fromStdString(room.information.host_username),
                              QString::fromStdString(room.ip), room.information.port,
                              QString::fromStdString(room.verify_uid)),
        });
        model->appendRow(row);
        // To make the rows expandable, add the member data as a child of the first column of the
        // rows with people in them and have qt set them to colspan after the model is finished
        // resetting
        if (!room.information.description.empty()) {
            first_item->appendRow(
                new LobbyItemDescription(QString::fromStdString(room.information.description)));
        }
        if (!room.members.empty()) {
            first_item->appendRow(new LobbyItemExpandedMemberList(members));
        }
    }

    // Reenable the refresh button and resize the columns
    ui->refresh_list->setEnabled(true);
    ui->refresh_list->setText(tr("Refresh List"));
    ui->room_list->header()->stretchLastSection();
    for (int i = 0; i < Column::TOTAL - 1; ++i) {
        ui->room_list->resizeColumnToContents(i);
    }

    // Set the member list child items to span all columns
    for (int i = 0; i < proxy->rowCount(); i++) {
        auto parent = model->item(i, 0);
        for (int j = 0; j < parent->rowCount(); j++) {
            ui->room_list->setFirstColumnSpanned(j, proxy->index(i, 0), true);
        }
    }

    ui->room_list->sortByColumn(Column::GAME_NAME, Qt::AscendingOrder);
}

std::string Lobby::GetProfileUsername() {
    const auto& current_user = profile_manager->GetUser(Settings::values.current_user.GetValue());
    Service::Account::ProfileBase profile{};

    if (!current_user.has_value()) {
        return "";
    }

    if (!profile_manager->GetProfileBase(*current_user, profile)) {
        return "";
    }

    const auto text = Common::StringFromFixedZeroTerminatedBuffer(
        reinterpret_cast<const char*>(profile.username.data()), profile.username.size());

    return text;
}

LobbyFilterProxyModel::LobbyFilterProxyModel(QWidget* parent, QStandardItemModel* list)
    : QSortFilterProxyModel(parent), game_list(list) {}

void LobbyFilterProxyModel::UpdateGameList(QStandardItemModel* list) {
    game_list = list;
}

bool LobbyFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    // Prioritize filters by fastest to compute

    // pass over any child rows (aka row that shows the players in the room)
    if (sourceParent != QModelIndex()) {
        return true;
    }

    // filter by empty rooms
    if (filter_empty) {
        QModelIndex member_list = sourceModel()->index(sourceRow, Column::MEMBER, sourceParent);
        int player_count =
            sourceModel()->data(member_list, LobbyItemMemberList::MemberListRole).toList().size();
        if (player_count == 0) {
            return false;
        }
    }

    // filter by filled rooms
    if (filter_full) {
        QModelIndex member_list = sourceModel()->index(sourceRow, Column::MEMBER, sourceParent);
        int player_count =
            sourceModel()->data(member_list, LobbyItemMemberList::MemberListRole).toList().size();
        int max_players =
            sourceModel()->data(member_list, LobbyItemMemberList::MaxPlayerRole).toInt();
        if (player_count >= max_players) {
            return false;
        }
    }

    // filter by search parameters
    if (!filter_search.isEmpty()) {
        QModelIndex game_name = sourceModel()->index(sourceRow, Column::GAME_NAME, sourceParent);
        QModelIndex room_name = sourceModel()->index(sourceRow, Column::ROOM_NAME, sourceParent);
        QModelIndex host_name = sourceModel()->index(sourceRow, Column::HOST, sourceParent);
        bool preferred_game_match = sourceModel()
                                        ->data(game_name, LobbyItemGame::GameNameRole)
                                        .toString()
                                        .contains(filter_search, filterCaseSensitivity());
        bool room_name_match = sourceModel()
                                   ->data(room_name, LobbyItemName::NameRole)
                                   .toString()
                                   .contains(filter_search, filterCaseSensitivity());
        bool username_match = sourceModel()
                                  ->data(host_name, LobbyItemHost::HostUsernameRole)
                                  .toString()
                                  .contains(filter_search, filterCaseSensitivity());
        if (!preferred_game_match && !room_name_match && !username_match) {
            return false;
        }
    }

    // filter by game owned
    if (filter_owned) {
        QModelIndex game_name = sourceModel()->index(sourceRow, Column::GAME_NAME, sourceParent);
        QList<QModelIndex> owned_games;
        for (int r = 0; r < game_list->rowCount(); ++r) {
            owned_games.append(QModelIndex(game_list->index(r, 0)));
        }
        auto current_id = sourceModel()->data(game_name, LobbyItemGame::TitleIDRole).toLongLong();
        if (current_id == 0) {
            // TODO(jroweboy): homebrew often doesn't have a game id and this hides them
            return false;
        }
        bool owned = false;
        for (const auto& game : owned_games) {
            auto game_id = game_list->data(game, GameListItemPath::ProgramIdRole).toLongLong();
            if (current_id == game_id) {
                owned = true;
            }
        }
        if (!owned) {
            return false;
        }
    }

    return true;
}

void LobbyFilterProxyModel::sort(int column, Qt::SortOrder order) {
    sourceModel()->sort(column, order);
}

void LobbyFilterProxyModel::SetFilterOwned(bool filter) {
    filter_owned = filter;
    invalidate();
}

void LobbyFilterProxyModel::SetFilterEmpty(bool filter) {
    filter_empty = filter;
    invalidate();
}

void LobbyFilterProxyModel::SetFilterFull(bool filter) {
    filter_full = filter;
    invalidate();
}

void LobbyFilterProxyModel::SetFilterSearch(const QString& filter) {
    filter_search = filter;
    invalidate();
}

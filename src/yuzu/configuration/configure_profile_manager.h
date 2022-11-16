// SPDX-FileCopyrightText: 2016 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <memory>

#include <QDialog>
#include <QList>
#include <QWidget>

namespace Common {
struct UUID;
}

namespace Core {
class System;
}

class QGraphicsScene;
class QDialogButtonBox;
class QLabel;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QVBoxLayout;

namespace Service::Account {
class ProfileManager;
}

namespace Ui {
class ConfigureProfileManager;
}

class ConfigureProfileManagerDeleteDialog : public QDialog {
public:
    explicit ConfigureProfileManagerDeleteDialog(QWidget* parent);
    ~ConfigureProfileManagerDeleteDialog();

    void SetInfo(const QString& username, const Common::UUID& uuid,
                 std::function<void()> accept_callback);

private:
    QDialogButtonBox* dialog_button_box;
    QGraphicsScene* icon_scene;
    QLabel* label_info;
};

class ConfigureProfileManager : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureProfileManager(const Core::System& system_, QWidget* parent = nullptr);
    ~ConfigureProfileManager() override;

    void ApplyConfiguration();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    void SetConfiguration();

    void PopulateUserList();
    void UpdateCurrentUser();

    void SelectUser(const QModelIndex& index);
    void AddUser();
    void RenameUser();
    void ConfirmDeleteUser();
    void DeleteUser(const Common::UUID& uuid);
    void SetUserImage();

    QVBoxLayout* layout;
    QTreeView* tree_view;
    QStandardItemModel* item_model;
    QGraphicsScene* scene;

    ConfigureProfileManagerDeleteDialog* confirm_dialog;

    std::vector<QList<QStandardItem*>> list_items;

    std::unique_ptr<Ui::ConfigureProfileManager> ui;
    bool enabled = false;

    std::unique_ptr<Service::Account::ProfileManager> profile_manager;

    const Core::System& system;
};

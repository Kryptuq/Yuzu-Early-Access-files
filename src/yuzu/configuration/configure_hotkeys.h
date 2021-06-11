// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

namespace Ui {
class ConfigureHotkeys;
}

class HotkeyRegistry;
class QStandardItemModel;

class ConfigureHotkeys : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureHotkeys(QWidget* parent = nullptr);
    ~ConfigureHotkeys() override;

    void ApplyConfiguration(HotkeyRegistry& registry);

    /**
     * Populates the hotkey list widget using data from the provided registry.
     * Called everytime the Configure dialog is opened.
     * @param registry The HotkeyRegistry whose data is used to populate the list.
     */
    void Populate(const HotkeyRegistry& registry);

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    void Configure(QModelIndex index);
    std::pair<bool, QString> IsUsedKey(QKeySequence key_sequence) const;

    void RestoreDefaults();
    void ClearAll();
    void PopupContextMenu(const QPoint& menu_location);

    std::unique_ptr<Ui::ConfigureHotkeys> ui;

    QStandardItemModel* model;
};

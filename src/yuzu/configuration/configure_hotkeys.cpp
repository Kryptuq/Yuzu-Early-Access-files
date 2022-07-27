// SPDX-FileCopyrightText: 2017 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QMenu>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QTimer>

#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"

#include "ui_configure_hotkeys.h"
#include "yuzu/configuration/config.h"
#include "yuzu/configuration/configure_hotkeys.h"
#include "yuzu/hotkeys.h"
#include "yuzu/util/sequence_dialog/sequence_dialog.h"

constexpr int name_column = 0;
constexpr int hotkey_column = 1;
constexpr int controller_column = 2;

ConfigureHotkeys::ConfigureHotkeys(Core::HID::HIDCore& hid_core, QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureHotkeys>()),
      timeout_timer(std::make_unique<QTimer>()), poll_timer(std::make_unique<QTimer>()) {
    ui->setupUi(this);
    setFocusPolicy(Qt::ClickFocus);

    model = new QStandardItemModel(this);
    model->setColumnCount(3);

    connect(ui->hotkey_list, &QTreeView::doubleClicked, this, &ConfigureHotkeys::Configure);
    connect(ui->hotkey_list, &QTreeView::customContextMenuRequested, this,
            &ConfigureHotkeys::PopupContextMenu);
    ui->hotkey_list->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->hotkey_list->setModel(model);

    ui->hotkey_list->header()->setStretchLastSection(false);
    ui->hotkey_list->header()->setSectionResizeMode(name_column, QHeaderView::ResizeMode::Stretch);
    ui->hotkey_list->header()->setMinimumSectionSize(150);

    connect(ui->button_restore_defaults, &QPushButton::clicked, this,
            &ConfigureHotkeys::RestoreDefaults);
    connect(ui->button_clear_all, &QPushButton::clicked, this, &ConfigureHotkeys::ClearAll);

    controller = hid_core.GetEmulatedController(Core::HID::NpadIdType::Player1);

    connect(timeout_timer.get(), &QTimer::timeout, [this] { SetPollingResult({}, true); });

    connect(poll_timer.get(), &QTimer::timeout, [this] {
        const auto buttons = controller->GetNpadButtons();
        if (buttons.raw != Core::HID::NpadButton::None) {
            SetPollingResult(buttons.raw, false);
            return;
        }
    });
    RetranslateUI();
}

ConfigureHotkeys::~ConfigureHotkeys() = default;

void ConfigureHotkeys::Populate(const HotkeyRegistry& registry) {
    for (const auto& group : registry.hotkey_groups) {
        auto* parent_item =
            new QStandardItem(QCoreApplication::translate("Hotkeys", qPrintable(group.first)));
        parent_item->setEditable(false);
        parent_item->setData(group.first);
        for (const auto& hotkey : group.second) {
            auto* action =
                new QStandardItem(QCoreApplication::translate("Hotkeys", qPrintable(hotkey.first)));
            auto* keyseq =
                new QStandardItem(hotkey.second.keyseq.toString(QKeySequence::NativeText));
            auto* controller_keyseq = new QStandardItem(hotkey.second.controller_keyseq);
            action->setEditable(false);
            action->setData(hotkey.first);
            keyseq->setEditable(false);
            controller_keyseq->setEditable(false);
            parent_item->appendRow({action, keyseq, controller_keyseq});
        }
        model->appendRow(parent_item);
    }

    ui->hotkey_list->expandAll();
    ui->hotkey_list->resizeColumnToContents(hotkey_column);
    ui->hotkey_list->resizeColumnToContents(controller_column);
}

void ConfigureHotkeys::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureHotkeys::RetranslateUI() {
    ui->retranslateUi(this);

    model->setHorizontalHeaderLabels({tr("Action"), tr("Hotkey"), tr("Controller Hotkey")});
    for (int key_id = 0; key_id < model->rowCount(); key_id++) {
        QStandardItem* parent = model->item(key_id, 0);
        parent->setText(
            QCoreApplication::translate("Hotkeys", qPrintable(parent->data().toString())));
        for (int key_column_id = 0; key_column_id < parent->rowCount(); key_column_id++) {
            QStandardItem* action = parent->child(key_column_id, name_column);
            action->setText(
                QCoreApplication::translate("Hotkeys", qPrintable(action->data().toString())));
        }
    }
}

void ConfigureHotkeys::Configure(QModelIndex index) {
    if (!index.parent().isValid()) {
        return;
    }

    // Controller configuration is selected
    if (index.column() == controller_column) {
        ConfigureController(index);
        return;
    }

    // Swap to the hotkey column
    index = index.sibling(index.row(), hotkey_column);

    const auto previous_key = model->data(index);

    SequenceDialog hotkey_dialog{this};

    const int return_code = hotkey_dialog.exec();
    const auto key_sequence = hotkey_dialog.GetSequence();
    if (return_code == QDialog::Rejected || key_sequence.isEmpty()) {
        return;
    }
    const auto [key_sequence_used, used_action] = IsUsedKey(key_sequence);

    if (key_sequence_used && key_sequence != QKeySequence(previous_key.toString())) {
        QMessageBox::warning(
            this, tr("Conflicting Key Sequence"),
            tr("The entered key sequence is already assigned to: %1").arg(used_action));
    } else {
        model->setData(index, key_sequence.toString(QKeySequence::NativeText));
    }
}
void ConfigureHotkeys::ConfigureController(QModelIndex index) {
    if (timeout_timer->isActive()) {
        return;
    }

    const auto previous_key = model->data(index);

    input_setter = [this, index, previous_key](const Core::HID::NpadButton button,
                                               const bool cancel) {
        if (cancel) {
            model->setData(index, previous_key);
            return;
        }

        const QString button_string = tr("Home+%1").arg(GetButtonName(button));

        const auto [key_sequence_used, used_action] = IsUsedControllerKey(button_string);

        if (key_sequence_used) {
            QMessageBox::warning(
                this, tr("Conflicting Key Sequence"),
                tr("The entered key sequence is already assigned to: %1").arg(used_action));
            model->setData(index, previous_key);
        } else {
            model->setData(index, button_string);
        }
    };

    model->setData(index, tr("[waiting]"));
    timeout_timer->start(2500); // Cancel after 2.5 seconds
    poll_timer->start(200);     // Check for new inputs every 200ms
    // We need to disable configuration to be able to read npad buttons
    controller->DisableConfiguration();
    controller->DisableSystemButtons();
}

void ConfigureHotkeys::SetPollingResult(Core::HID::NpadButton button, const bool cancel) {
    timeout_timer->stop();
    poll_timer->stop();
    // Re-Enable configuration
    controller->EnableConfiguration();
    controller->EnableSystemButtons();

    (*input_setter)(button, cancel);

    input_setter = std::nullopt;
}

QString ConfigureHotkeys::GetButtonName(Core::HID::NpadButton button) const {
    Core::HID::NpadButtonState state{button};
    if (state.a) {
        return QStringLiteral("A");
    }
    if (state.b) {
        return QStringLiteral("B");
    }
    if (state.x) {
        return QStringLiteral("X");
    }
    if (state.y) {
        return QStringLiteral("Y");
    }
    if (state.l || state.right_sl || state.left_sl) {
        return QStringLiteral("L");
    }
    if (state.r || state.right_sr || state.left_sr) {
        return QStringLiteral("R");
    }
    if (state.zl) {
        return QStringLiteral("ZL");
    }
    if (state.zr) {
        return QStringLiteral("ZR");
    }
    if (state.left) {
        return QStringLiteral("Dpad_Left");
    }
    if (state.right) {
        return QStringLiteral("Dpad_Right");
    }
    if (state.up) {
        return QStringLiteral("Dpad_Up");
    }
    if (state.down) {
        return QStringLiteral("Dpad_Down");
    }
    if (state.stick_l) {
        return QStringLiteral("Left_Stick");
    }
    if (state.stick_r) {
        return QStringLiteral("Right_Stick");
    }
    if (state.minus) {
        return QStringLiteral("Minus");
    }
    if (state.plus) {
        return QStringLiteral("Plus");
    }
    return tr("Invalid");
}

std::pair<bool, QString> ConfigureHotkeys::IsUsedKey(QKeySequence key_sequence) const {
    for (int r = 0; r < model->rowCount(); ++r) {
        const QStandardItem* const parent = model->item(r, 0);

        for (int r2 = 0; r2 < parent->rowCount(); ++r2) {
            const QStandardItem* const key_seq_item = parent->child(r2, hotkey_column);
            const auto key_seq_str = key_seq_item->text();
            const auto key_seq = QKeySequence::fromString(key_seq_str, QKeySequence::NativeText);

            if (key_sequence == key_seq) {
                return std::make_pair(true, parent->child(r2, 0)->text());
            }
        }
    }

    return std::make_pair(false, QString());
}

std::pair<bool, QString> ConfigureHotkeys::IsUsedControllerKey(const QString& key_sequence) const {
    for (int r = 0; r < model->rowCount(); ++r) {
        const QStandardItem* const parent = model->item(r, 0);

        for (int r2 = 0; r2 < parent->rowCount(); ++r2) {
            const QStandardItem* const key_seq_item = parent->child(r2, controller_column);
            const auto key_seq_str = key_seq_item->text();

            if (key_sequence == key_seq_str) {
                return std::make_pair(true, parent->child(r2, 0)->text());
            }
        }
    }

    return std::make_pair(false, QString());
}

void ConfigureHotkeys::ApplyConfiguration(HotkeyRegistry& registry) {
    for (int key_id = 0; key_id < model->rowCount(); key_id++) {
        const QStandardItem* parent = model->item(key_id, 0);
        for (int key_column_id = 0; key_column_id < parent->rowCount(); key_column_id++) {
            const QStandardItem* action = parent->child(key_column_id, name_column);
            const QStandardItem* keyseq = parent->child(key_column_id, hotkey_column);
            const QStandardItem* controller_keyseq =
                parent->child(key_column_id, controller_column);
            for (auto& [group, sub_actions] : registry.hotkey_groups) {
                if (group != parent->data())
                    continue;
                for (auto& [action_name, hotkey] : sub_actions) {
                    if (action_name != action->data())
                        continue;
                    hotkey.keyseq = QKeySequence(keyseq->text());
                    hotkey.controller_keyseq = controller_keyseq->text();
                }
            }
        }
    }

    registry.SaveHotkeys();
}

void ConfigureHotkeys::RestoreDefaults() {
    for (int r = 0; r < model->rowCount(); ++r) {
        const QStandardItem* parent = model->item(r, 0);

        for (int r2 = 0; r2 < parent->rowCount(); ++r2) {
            model->item(r, 0)
                ->child(r2, hotkey_column)
                ->setText(Config::default_hotkeys[r2].shortcut.keyseq);
            model->item(r, 0)
                ->child(r2, controller_column)
                ->setText(Config::default_hotkeys[r2].shortcut.controller_keyseq);
        }
    }
}

void ConfigureHotkeys::ClearAll() {
    for (int r = 0; r < model->rowCount(); ++r) {
        const QStandardItem* parent = model->item(r, 0);

        for (int r2 = 0; r2 < parent->rowCount(); ++r2) {
            model->item(r, 0)->child(r2, hotkey_column)->setText(QString{});
            model->item(r, 0)->child(r2, controller_column)->setText(QString{});
        }
    }
}

void ConfigureHotkeys::PopupContextMenu(const QPoint& menu_location) {
    QModelIndex index = ui->hotkey_list->indexAt(menu_location);
    if (!index.parent().isValid()) {
        return;
    }

    // Swap to the hotkey column if the controller hotkey column is not selected
    if (index.column() != controller_column) {
        index = index.sibling(index.row(), hotkey_column);
    }

    QMenu context_menu;

    QAction* restore_default = context_menu.addAction(tr("Restore Default"));
    QAction* clear = context_menu.addAction(tr("Clear"));

    connect(restore_default, &QAction::triggered, [this, index] {
        if (index.column() == controller_column) {
            RestoreControllerHotkey(index);
            return;
        }
        RestoreHotkey(index);
    });
    connect(clear, &QAction::triggered, [this, index] { model->setData(index, QString{}); });

    context_menu.exec(ui->hotkey_list->viewport()->mapToGlobal(menu_location));
}

void ConfigureHotkeys::RestoreControllerHotkey(QModelIndex index) {
    const QString& default_key_sequence =
        Config::default_hotkeys[index.row()].shortcut.controller_keyseq;
    const auto [key_sequence_used, used_action] = IsUsedControllerKey(default_key_sequence);

    if (key_sequence_used && default_key_sequence != model->data(index).toString()) {
        QMessageBox::warning(
            this, tr("Conflicting Button Sequence"),
            tr("The default button sequence is already assigned to: %1").arg(used_action));
    } else {
        model->setData(index, default_key_sequence);
    }
}

void ConfigureHotkeys::RestoreHotkey(QModelIndex index) {
    const QKeySequence& default_key_sequence = QKeySequence::fromString(
        Config::default_hotkeys[index.row()].shortcut.keyseq, QKeySequence::NativeText);
    const auto [key_sequence_used, used_action] = IsUsedKey(default_key_sequence);

    if (key_sequence_used && default_key_sequence != QKeySequence(model->data(index).toString())) {
        QMessageBox::warning(
            this, tr("Conflicting Key Sequence"),
            tr("The default key sequence is already assigned to: %1").arg(used_action));
    } else {
        model->setData(index, default_key_sequence.toString(QKeySequence::NativeText));
    }
}

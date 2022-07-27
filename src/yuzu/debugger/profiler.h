// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWidget>

class QAction;
class QHideEvent;
class QShowEvent;

class MicroProfileDialog : public QWidget {
    Q_OBJECT

public:
    explicit MicroProfileDialog(QWidget* parent = nullptr);

    /// Returns a QAction that can be used to toggle visibility of this dialog.
    QAction* toggleViewAction();

protected:
    void showEvent(QShowEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private:
    QAction* toggle_view_action = nullptr;
};

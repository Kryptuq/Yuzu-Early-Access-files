// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QAction>
#include <QLayout>
#include <QString>
#include "common/settings.h"
#include "yuzu/configuration/configure_input_player_widget.h"
#include "yuzu/debugger/controller.h"

ControllerDialog::ControllerDialog(QWidget* parent) : QWidget(parent, Qt::Dialog) {
    setObjectName(QStringLiteral("Controller"));
    setWindowTitle(tr("Controller P1"));
    resize(500, 350);
    setMinimumSize(500, 350);
    // Remove the "?" button from the titlebar and enable the maximize button
    setWindowFlags((windowFlags() & ~Qt::WindowContextHelpButtonHint) |
                   Qt::WindowMaximizeButtonHint);

    widget = new PlayerControlPreview(this);
    refreshConfiguration();
    QLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(widget);
    setLayout(layout);

    // Configure focus so that widget is focusable and the dialog automatically forwards focus to
    // it.
    setFocusProxy(widget);
    widget->SetConnectedStatus(false);
    widget->setFocusPolicy(Qt::StrongFocus);
    widget->setFocus();
}

void ControllerDialog::refreshConfiguration() {
    const auto& players = Settings::values.players.GetValue();
    constexpr std::size_t player = 0;
    widget->SetPlayerInputRaw(player, players[player].buttons, players[player].analogs);
    widget->SetControllerType(players[player].controller_type);
    widget->SetConnectedStatus(players[player].connected);
}

QAction* ControllerDialog::toggleViewAction() {
    if (toggle_view_action == nullptr) {
        toggle_view_action = new QAction(tr("&Controller P1"), this);
        toggle_view_action->setCheckable(true);
        toggle_view_action->setChecked(isVisible());
        connect(toggle_view_action, &QAction::toggled, this, &ControllerDialog::setVisible);
    }

    return toggle_view_action;
}

void ControllerDialog::showEvent(QShowEvent* ev) {
    if (toggle_view_action) {
        toggle_view_action->setChecked(isVisible());
    }
    refreshConfiguration();
    QWidget::showEvent(ev);
}

void ControllerDialog::hideEvent(QHideEvent* ev) {
    if (toggle_view_action) {
        toggle_view_action->setChecked(isVisible());
    }
    widget->SetConnectedStatus(false);
    QWidget::hideEvent(ev);
}

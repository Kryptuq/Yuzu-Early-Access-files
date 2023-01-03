// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QAction>
#include <QLayout>
#include <QString>
#include "common/settings.h"
#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "input_common/drivers/tas_input.h"
#include "input_common/main.h"
#include "yuzu/configuration/configure_input_player_widget.h"
#include "yuzu/debugger/controller.h"

ControllerDialog::ControllerDialog(Core::HID::HIDCore& hid_core_,
                                   std::shared_ptr<InputCommon::InputSubsystem> input_subsystem_,
                                   QWidget* parent)
    : QWidget(parent, Qt::Dialog), hid_core{hid_core_}, input_subsystem{input_subsystem_} {
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
    widget->setFocusPolicy(Qt::StrongFocus);
    widget->setFocus();
}

void ControllerDialog::refreshConfiguration() {
    UnloadController();
    auto* player_1 = hid_core.GetEmulatedController(Core::HID::NpadIdType::Player1);
    auto* handheld = hid_core.GetEmulatedController(Core::HID::NpadIdType::Handheld);
    // Display the correct controller
    controller = handheld->IsConnected() ? handheld : player_1;

    Core::HID::ControllerUpdateCallback engine_callback{
        .on_change = [this](Core::HID::ControllerTriggerType type) { ControllerUpdate(type); },
        .is_npad_service = true,
    };
    callback_key = controller->SetCallback(engine_callback);
    widget->SetController(controller);
    is_controller_set = true;
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

void ControllerDialog::UnloadController() {
    widget->UnloadController();
    if (is_controller_set) {
        controller->DeleteCallback(callback_key);
        is_controller_set = false;
    }
}

void ControllerDialog::showEvent(QShowEvent* ev) {
    if (toggle_view_action) {
        toggle_view_action->setChecked(isVisible());
    }
    QWidget::showEvent(ev);
}

void ControllerDialog::hideEvent(QHideEvent* ev) {
    if (toggle_view_action) {
        toggle_view_action->setChecked(isVisible());
    }
    QWidget::hideEvent(ev);
}

void ControllerDialog::ControllerUpdate(Core::HID::ControllerTriggerType type) {
    // TODO(german77): Remove TAS from here
    switch (type) {
    case Core::HID::ControllerTriggerType::Button:
    case Core::HID::ControllerTriggerType::Stick: {
        const auto buttons_values = controller->GetButtonsValues();
        const auto stick_values = controller->GetSticks();
        u64 buttons = 0;
        std::size_t index = 0;
        for (const auto& button : buttons_values) {
            buttons |= button.value ? 1LLU << index : 0;
            index++;
        }
        const InputCommon::TasInput::TasAnalog left_axis = {
            .x = stick_values.left.x / 32767.f,
            .y = stick_values.left.y / 32767.f,
        };
        const InputCommon::TasInput::TasAnalog right_axis = {
            .x = stick_values.right.x / 32767.f,
            .y = stick_values.right.y / 32767.f,
        };
        input_subsystem->GetTas()->RecordInput(buttons, left_axis, right_axis);
        break;
    }
    default:
        break;
    }
}

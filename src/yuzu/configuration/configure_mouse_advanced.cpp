// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <memory>

#include <QKeyEvent>
#include <QMenu>
#include <QTimer>

#include "common/assert.h"
#include "common/param_package.h"
#include "input_common/drivers/keyboard.h"
#include "input_common/drivers/mouse.h"
#include "input_common/main.h"
#include "ui_configure_mouse_advanced.h"
#include "yuzu/bootmanager.h"
#include "yuzu/configuration/config.h"
#include "yuzu/configuration/configure_mouse_advanced.h"

static QString GetKeyName(int key_code) {
    switch (key_code) {
    case Qt::LeftButton:
        return QObject::tr("Click 0");
    case Qt::RightButton:
        return QObject::tr("Click 1");
    case Qt::MiddleButton:
        return QObject::tr("Click 2");
    case Qt::BackButton:
        return QObject::tr("Click 3");
    case Qt::ForwardButton:
        return QObject::tr("Click 4");
    case Qt::Key_Shift:
        return QObject::tr("Shift");
    case Qt::Key_Control:
        return QObject::tr("Ctrl");
    case Qt::Key_Alt:
        return QObject::tr("Alt");
    case Qt::Key_Meta:
        return {};
    default:
        return QKeySequence(key_code).toString();
    }
}

static QString ButtonToText(const Common::ParamPackage& param) {
    if (!param.Has("engine")) {
        return QObject::tr("[not set]");
    }

    if (param.Get("engine", "") == "keyboard") {
        return GetKeyName(param.Get("code", 0));
    }

    if (param.Get("engine", "") == "sdl") {
        if (param.Has("hat")) {
            const QString hat_str = QString::fromStdString(param.Get("hat", ""));
            const QString direction_str = QString::fromStdString(param.Get("direction", ""));

            return QObject::tr("Hat %1 %2").arg(hat_str, direction_str);
        }

        if (param.Has("axis")) {
            const QString axis_str = QString::fromStdString(param.Get("axis", ""));
            const QString direction_str = QString::fromStdString(param.Get("direction", ""));

            return QObject::tr("Axis %1%2").arg(axis_str, direction_str);
        }

        if (param.Has("button")) {
            const QString button_str = QString::fromStdString(param.Get("button", ""));

            return QObject::tr("Button %1").arg(button_str);
        }
        return {};
    }

    return QObject::tr("[unknown]");
}

ConfigureMouseAdvanced::ConfigureMouseAdvanced(QWidget* parent,
                                               InputCommon::InputSubsystem* input_subsystem_)
    : QDialog(parent),
      ui(std::make_unique<Ui::ConfigureMouseAdvanced>()), input_subsystem{input_subsystem_},
      timeout_timer(std::make_unique<QTimer>()), poll_timer(std::make_unique<QTimer>()) {
    ui->setupUi(this);
    setFocusPolicy(Qt::ClickFocus);

    button_map = {
        ui->left_button, ui->right_button, ui->middle_button, ui->forward_button, ui->back_button,
    };

    for (int button_id = 0; button_id < Settings::NativeMouseButton::NumMouseButtons; button_id++) {
        auto* const button = button_map[button_id];
        if (button == nullptr) {
            continue;
        }

        button->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(button, &QPushButton::clicked, [=, this] {
            HandleClick(
                button_map[button_id],
                [=, this](const Common::ParamPackage& params) {
                    buttons_param[button_id] = params;
                },
                InputCommon::Polling::InputType::Button);
        });
        connect(button, &QPushButton::customContextMenuRequested,
                [=, this](const QPoint& menu_location) {
                    QMenu context_menu;
                    context_menu.addAction(tr("Clear"), [&] {
                        buttons_param[button_id].Clear();
                        button_map[button_id]->setText(tr("[not set]"));
                    });
                    context_menu.addAction(tr("Restore Default"), [&] {
                        buttons_param[button_id] =
                            Common::ParamPackage{InputCommon::GenerateKeyboardParam(
                                Config::default_mouse_buttons[button_id])};
                        button_map[button_id]->setText(ButtonToText(buttons_param[button_id]));
                    });
                    context_menu.exec(button_map[button_id]->mapToGlobal(menu_location));
                });
    }

    connect(ui->buttonClearAll, &QPushButton::clicked, [this] { ClearAll(); });
    connect(ui->buttonRestoreDefaults, &QPushButton::clicked, [this] { RestoreDefaults(); });

    timeout_timer->setSingleShot(true);
    connect(timeout_timer.get(), &QTimer::timeout, [this] { SetPollingResult({}, true); });

    connect(poll_timer.get(), &QTimer::timeout, [this] {
        const auto& params = input_subsystem->GetNextInput();
        if (params.Has("engine")) {
            SetPollingResult(params, false);
            return;
        }
    });

    LoadConfiguration();
    resize(0, 0);
}

ConfigureMouseAdvanced::~ConfigureMouseAdvanced() = default;

void ConfigureMouseAdvanced::ApplyConfiguration() {
    std::transform(buttons_param.begin(), buttons_param.end(),
                   Settings::values.mouse_buttons.begin(),
                   [](const Common::ParamPackage& param) { return param.Serialize(); });
}

void ConfigureMouseAdvanced::LoadConfiguration() {
    std::transform(Settings::values.mouse_buttons.begin(), Settings::values.mouse_buttons.end(),
                   buttons_param.begin(),
                   [](const std::string& str) { return Common::ParamPackage(str); });
    UpdateButtonLabels();
}

void ConfigureMouseAdvanced::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QDialog::changeEvent(event);
}

void ConfigureMouseAdvanced::RetranslateUI() {
    ui->retranslateUi(this);
}

void ConfigureMouseAdvanced::RestoreDefaults() {
    for (int button_id = 0; button_id < Settings::NativeMouseButton::NumMouseButtons; button_id++) {
        buttons_param[button_id] = Common::ParamPackage{
            InputCommon::GenerateKeyboardParam(Config::default_mouse_buttons[button_id])};
    }

    UpdateButtonLabels();
}

void ConfigureMouseAdvanced::ClearAll() {
    for (int i = 0; i < Settings::NativeMouseButton::NumMouseButtons; ++i) {
        const auto* const button = button_map[i];
        if (button != nullptr && button->isEnabled()) {
            buttons_param[i].Clear();
        }
    }

    UpdateButtonLabels();
}

void ConfigureMouseAdvanced::UpdateButtonLabels() {
    for (int button = 0; button < Settings::NativeMouseButton::NumMouseButtons; button++) {
        button_map[button]->setText(ButtonToText(buttons_param[button]));
    }
}

void ConfigureMouseAdvanced::HandleClick(
    QPushButton* button, std::function<void(const Common::ParamPackage&)> new_input_setter,
    InputCommon::Polling::InputType type) {
    button->setText(tr("[press key]"));
    button->setFocus();

    input_setter = new_input_setter;

    input_subsystem->BeginMapping(type);

    QWidget::grabMouse();
    QWidget::grabKeyboard();

    timeout_timer->start(2500); // Cancel after 2.5 seconds
    poll_timer->start(50);      // Check for new inputs every 50ms
}

void ConfigureMouseAdvanced::SetPollingResult(const Common::ParamPackage& params, bool abort) {
    timeout_timer->stop();
    poll_timer->stop();
    input_subsystem->StopMapping();

    QWidget::releaseMouse();
    QWidget::releaseKeyboard();

    if (!abort) {
        (*input_setter)(params);
    }

    UpdateButtonLabels();
    input_setter = std::nullopt;
}

void ConfigureMouseAdvanced::mousePressEvent(QMouseEvent* event) {
    if (!input_setter || !event) {
        return;
    }

    const auto button = GRenderWindow::QtButtonToMouseButton(event->button());
    input_subsystem->GetMouse()->PressButton(0, 0, 0, 0, button);
}

void ConfigureMouseAdvanced::keyPressEvent(QKeyEvent* event) {
    if (!input_setter || !event) {
        return;
    }

    if (event->key() != Qt::Key_Escape) {
        input_subsystem->GetKeyboard()->PressKey(event->key());
    }
}

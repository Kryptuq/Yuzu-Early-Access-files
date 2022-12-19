// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QKeyEvent>
#include <QScreen>

#include "core/core.h"
#include "core/hid/hid_types.h"
#include "core/hid/input_interpreter.h"
#include "ui_overlay_dialog.h"
#include "yuzu/util/overlay_dialog.h"

namespace {

constexpr float BASE_TITLE_FONT_SIZE = 14.0f;
constexpr float BASE_FONT_SIZE = 18.0f;
constexpr float BASE_WIDTH = 1280.0f;
constexpr float BASE_HEIGHT = 720.0f;

} // Anonymous namespace

OverlayDialog::OverlayDialog(QWidget* parent, Core::System& system, const QString& title_text,
                             const QString& body_text, const QString& left_button_text,
                             const QString& right_button_text, Qt::Alignment alignment,
                             bool use_rich_text_)
    : QDialog(parent), ui{std::make_unique<Ui::OverlayDialog>()}, use_rich_text{use_rich_text_} {
    ui->setupUi(this);

    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowTitleHint |
                   Qt::WindowSystemMenuHint | Qt::CustomizeWindowHint);
    setWindowModality(Qt::WindowModal);
    setAttribute(Qt::WA_TranslucentBackground);

    if (use_rich_text) {
        InitializeRichTextDialog(title_text, body_text, left_button_text, right_button_text,
                                 alignment);
    } else {
        InitializeRegularTextDialog(title_text, body_text, left_button_text, right_button_text,
                                    alignment);
    }

    MoveAndResizeWindow();

    // TODO (Morph): Remove this when InputInterpreter no longer relies on the HID backend
    if (system.IsPoweredOn() && !ui->buttonsDialog->isHidden()) {
        input_interpreter = std::make_unique<InputInterpreter>(system);

        StartInputThread();
    }
}

OverlayDialog::~OverlayDialog() {
    StopInputThread();
}

void OverlayDialog::InitializeRegularTextDialog(const QString& title_text, const QString& body_text,
                                                const QString& left_button_text,
                                                const QString& right_button_text,
                                                Qt::Alignment alignment) {
    ui->stackedDialog->setCurrentIndex(0);

    ui->label_title->setText(title_text);
    ui->label_dialog->setText(body_text);
    ui->button_cancel->setText(left_button_text);
    ui->button_ok_label->setText(right_button_text);

    ui->label_dialog->setAlignment(alignment);

    if (title_text.isEmpty()) {
        ui->label_title->hide();
        ui->verticalLayout_2->setStretch(0, 0);
        ui->verticalLayout_2->setStretch(1, 219);
        ui->verticalLayout_2->setStretch(2, 82);
    }

    if (left_button_text.isEmpty()) {
        ui->button_cancel->hide();
        ui->button_cancel->setEnabled(false);
    }

    if (right_button_text.isEmpty()) {
        ui->button_ok_label->hide();
        ui->button_ok_label->setEnabled(false);
    }

    if (ui->button_cancel->isHidden() && ui->button_ok_label->isHidden()) {
        ui->buttonsDialog->hide();
        return;
    }

    connect(
        ui->button_cancel, &QPushButton::clicked, this,
        [this](bool) {
            StopInputThread();
            QDialog::reject();
        },
        Qt::QueuedConnection);
    connect(
        ui->button_ok_label, &QPushButton::clicked, this,
        [this](bool) {
            StopInputThread();
            QDialog::accept();
        },
        Qt::QueuedConnection);
}

void OverlayDialog::InitializeRichTextDialog(const QString& title_text, const QString& body_text,
                                             const QString& left_button_text,
                                             const QString& right_button_text,
                                             Qt::Alignment alignment) {
    ui->stackedDialog->setCurrentIndex(1);

    ui->label_title_rich->setText(title_text);
    ui->text_browser_dialog->setText(body_text);
    ui->button_cancel_rich->setText(left_button_text);
    ui->button_ok_rich->setText(right_button_text);

    // TODO (Morph/Rei): Replace this with something that works better
    ui->text_browser_dialog->setAlignment(alignment);

    if (title_text.isEmpty()) {
        ui->label_title_rich->hide();
        ui->verticalLayout_3->setStretch(0, 0);
        ui->verticalLayout_3->setStretch(1, 438);
        ui->verticalLayout_3->setStretch(2, 82);
    }

    if (left_button_text.isEmpty()) {
        ui->button_cancel_rich->hide();
        ui->button_cancel_rich->setEnabled(false);
    }

    if (right_button_text.isEmpty()) {
        ui->button_ok_rich->hide();
        ui->button_ok_rich->setEnabled(false);
    }

    if (ui->button_cancel_rich->isHidden() && ui->button_ok_rich->isHidden()) {
        ui->buttonsRichDialog->hide();
        return;
    }

    connect(
        ui->button_cancel_rich, &QPushButton::clicked, this,
        [this](bool) {
            StopInputThread();
            QDialog::reject();
        },
        Qt::QueuedConnection);
    connect(
        ui->button_ok_rich, &QPushButton::clicked, this,
        [this](bool) {
            StopInputThread();
            QDialog::accept();
        },
        Qt::QueuedConnection);
}

void OverlayDialog::MoveAndResizeWindow() {
    const auto pos = parentWidget()->mapToGlobal(parentWidget()->rect().topLeft());
    const auto width = static_cast<float>(parentWidget()->width());
    const auto height = static_cast<float>(parentWidget()->height());

    // High DPI
    const float dpi_scale = qApp->screenAt(pos)->logicalDotsPerInch() / 96.0f;

    const auto title_text_font_size = BASE_TITLE_FONT_SIZE * (height / BASE_HEIGHT) / dpi_scale;
    const auto body_text_font_size =
        BASE_FONT_SIZE * (((width / BASE_WIDTH) + (height / BASE_HEIGHT)) / 2.0f) / dpi_scale;
    const auto button_text_font_size = BASE_FONT_SIZE * (height / BASE_HEIGHT) / dpi_scale;

    QFont title_text_font(QStringLiteral("MS Shell Dlg 2"), title_text_font_size, QFont::Normal);
    QFont body_text_font(QStringLiteral("MS Shell Dlg 2"), body_text_font_size, QFont::Normal);
    QFont button_text_font(QStringLiteral("MS Shell Dlg 2"), button_text_font_size, QFont::Normal);

    if (use_rich_text) {
        ui->label_title_rich->setFont(title_text_font);
        ui->text_browser_dialog->setFont(body_text_font);
        ui->button_cancel_rich->setFont(button_text_font);
        ui->button_ok_rich->setFont(button_text_font);
    } else {
        ui->label_title->setFont(title_text_font);
        ui->label_dialog->setFont(body_text_font);
        ui->button_cancel->setFont(button_text_font);
        ui->button_ok_label->setFont(button_text_font);
    }

    QDialog::move(pos);
    QDialog::resize(width, height);
}

template <Core::HID::NpadButton... T>
void OverlayDialog::HandleButtonPressedOnce() {
    const auto f = [this](Core::HID::NpadButton button) {
        if (input_interpreter->IsButtonPressedOnce(button)) {
            TranslateButtonPress(button);
        }
    };

    (f(T), ...);
}

void OverlayDialog::TranslateButtonPress(Core::HID::NpadButton button) {
    QPushButton* left_button = use_rich_text ? ui->button_cancel_rich : ui->button_cancel;
    QPushButton* right_button = use_rich_text ? ui->button_ok_rich : ui->button_ok_label;

    // TODO (Morph): Handle QTextBrowser text scrolling
    // TODO (Morph): focusPrevious/NextChild() doesn't work well with the rich text dialog, fix it

    switch (button) {
    case Core::HID::NpadButton::A:
    case Core::HID::NpadButton::B:
        if (left_button->hasFocus()) {
            left_button->click();
        } else if (right_button->hasFocus()) {
            right_button->click();
        }
        break;
    case Core::HID::NpadButton::Left:
    case Core::HID::NpadButton::StickLLeft:
        focusPreviousChild();
        break;
    case Core::HID::NpadButton::Right:
    case Core::HID::NpadButton::StickLRight:
        focusNextChild();
        break;
    default:
        break;
    }
}

void OverlayDialog::StartInputThread() {
    if (input_thread_running) {
        return;
    }

    input_thread_running = true;

    input_thread = std::thread(&OverlayDialog::InputThread, this);
}

void OverlayDialog::StopInputThread() {
    input_thread_running = false;

    if (input_thread.joinable()) {
        input_thread.join();
    }
}

void OverlayDialog::InputThread() {
    while (input_thread_running) {
        input_interpreter->PollInput();

        HandleButtonPressedOnce<Core::HID::NpadButton::A, Core::HID::NpadButton::B,
                                Core::HID::NpadButton::Left, Core::HID::NpadButton::Right,
                                Core::HID::NpadButton::StickLLeft,
                                Core::HID::NpadButton::StickLRight>();

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

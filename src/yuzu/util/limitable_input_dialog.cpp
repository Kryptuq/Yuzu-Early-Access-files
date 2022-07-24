// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "yuzu/util/limitable_input_dialog.h"

LimitableInputDialog::LimitableInputDialog(QWidget* parent) : QDialog{parent} {
    CreateUI();
    ConnectEvents();
}

LimitableInputDialog::~LimitableInputDialog() = default;

void LimitableInputDialog::CreateUI() {
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    text_label = new QLabel(this);
    text_entry = new QLineEdit(this);
    text_label_invalid = new QLabel(this);
    buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto* const layout = new QVBoxLayout;
    layout->addWidget(text_label);
    layout->addWidget(text_entry);
    layout->addWidget(text_label_invalid);
    layout->addWidget(buttons);

    setLayout(layout);
}

void LimitableInputDialog::ConnectEvents() {
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString LimitableInputDialog::GetText(QWidget* parent, const QString& title, const QString& text,
                                      int min_character_limit, int max_character_limit,
                                      InputLimiter limit_type) {
    Q_ASSERT(min_character_limit <= max_character_limit);

    LimitableInputDialog dialog{parent};
    dialog.setWindowTitle(title);
    dialog.text_label->setText(text);
    dialog.text_entry->setMaxLength(max_character_limit);
    dialog.text_label_invalid->show();

    switch (limit_type) {
    case InputLimiter::Filesystem:
        dialog.invalid_characters = QStringLiteral("<>:;\"/\\|,.!?*");
        break;
    default:
        dialog.invalid_characters.clear();
        dialog.text_label_invalid->hide();
        break;
    }
    dialog.text_label_invalid->setText(
        tr("The text can't contain any of the following characters:\n%1")
            .arg(dialog.invalid_characters));

    auto* const ok_button = dialog.buttons->button(QDialogButtonBox::Ok);
    ok_button->setEnabled(false);
    connect(dialog.text_entry, &QLineEdit::textEdited, [&] {
        if (!dialog.invalid_characters.isEmpty()) {
            dialog.RemoveInvalidCharacters();
        }
        ok_button->setEnabled(dialog.text_entry->text().length() >= min_character_limit);
    });

    if (dialog.exec() != QDialog::Accepted) {
        return {};
    }

    return dialog.text_entry->text();
}

void LimitableInputDialog::RemoveInvalidCharacters() {
    auto cpos = text_entry->cursorPosition();
    for (int i = 0; i < text_entry->text().length(); i++) {
        if (invalid_characters.contains(text_entry->text().at(i))) {
            text_entry->setText(text_entry->text().remove(i, 1));
            i--;
            cpos--;
        }
    }
    text_entry->setCursorPosition(cpos);
}

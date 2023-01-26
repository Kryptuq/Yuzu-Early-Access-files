// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include "yuzu/install_dialog.h"
#include "yuzu/uisettings.h"

InstallDialog::InstallDialog(QWidget* parent, const QStringList& files) : QDialog(parent) {
    file_list = new QListWidget(this);

    for (const QString& file : files) {
        QListWidgetItem* item = new QListWidgetItem(QFileInfo(file).fileName(), file_list);
        item->setData(Qt::UserRole, file);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
    }

    file_list->setMinimumWidth((file_list->sizeHintForColumn(0) * 11) / 10);

    vbox_layout = new QVBoxLayout;

    hbox_layout = new QHBoxLayout;

    description = new QLabel(tr("Please confirm these are the files you wish to install."));

    update_description =
        new QLabel(tr("Installing an Update or DLC will overwrite the previously installed one."));

    buttons = new QDialogButtonBox;
    buttons->addButton(QDialogButtonBox::Cancel);
    buttons->addButton(tr("Install"), QDialogButtonBox::AcceptRole);

    connect(buttons, &QDialogButtonBox::accepted, this, &InstallDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &InstallDialog::reject);

    hbox_layout->addWidget(buttons);

    vbox_layout->addWidget(description);
    vbox_layout->addWidget(update_description);
    vbox_layout->addWidget(file_list);
    vbox_layout->addLayout(hbox_layout);

    setLayout(vbox_layout);
    setWindowTitle(tr("Install Files to NAND"));
}

InstallDialog::~InstallDialog() = default;

QStringList InstallDialog::GetFiles() const {
    QStringList files;

    for (int i = 0; i < file_list->count(); ++i) {
        const QListWidgetItem* item = file_list->item(i);
        if (item->checkState() == Qt::Checked) {
            files.append(item->data(Qt::UserRole).toString());
        }
    }

    return files;
}

int InstallDialog::GetMinimumWidth() const {
    return file_list->width();
}

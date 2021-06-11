// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;
class QHBoxLayout;
class QLabel;
class QListWidget;
class QVBoxLayout;

class InstallDialog : public QDialog {
    Q_OBJECT

public:
    explicit InstallDialog(QWidget* parent, const QStringList& files);
    ~InstallDialog() override;

    [[nodiscard]] QStringList GetFiles() const;
    [[nodiscard]] int GetMinimumWidth() const;

private:
    QListWidget* file_list;

    QVBoxLayout* vbox_layout;
    QHBoxLayout* hbox_layout;

    QLabel* description;
    QLabel* update_description;
    QDialogButtonBox* buttons;
};

// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <QWidget>

class QLineEdit;

namespace Ui {
class ConfigureFilesystem;
}

class ConfigureFilesystem : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureFilesystem(QWidget* parent = nullptr);
    ~ConfigureFilesystem() override;

    void ApplyConfiguration();

private:
    void changeEvent(QEvent* event) override;

    void RetranslateUI();
    void SetConfiguration();

    enum class DirectoryTarget {
        NAND,
        SD,
        Gamecard,
        Dump,
        Load,
    };

    void SetDirectory(DirectoryTarget target, QLineEdit* edit);
    void ResetMetadata();
    void UpdateEnabledControls();

    std::unique_ptr<Ui::ConfigureFilesystem> ui;
};

// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

class QLineEdit;

namespace Ui {
class ConfigureTas;
}

class ConfigureTasDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureTasDialog(QWidget* parent);
    ~ConfigureTasDialog() override;

    /// Save all button configurations to settings file
    void ApplyConfiguration();

private:
    enum class DirectoryTarget {
        TAS,
    };

    void LoadConfiguration();

    void SetDirectory(DirectoryTarget target, QLineEdit* edit);

    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    void HandleApplyButtonClicked();

    std::unique_ptr<Ui::ConfigureTas> ui;
};

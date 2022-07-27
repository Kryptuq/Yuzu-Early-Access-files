// SPDX-FileCopyrightText: 2016 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <QDialog>

namespace Ui {
class ConfigureTouchscreenAdvanced;
}

class ConfigureTouchscreenAdvanced : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureTouchscreenAdvanced(QWidget* parent);
    ~ConfigureTouchscreenAdvanced() override;

    void ApplyConfiguration();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    /// Load configuration settings.
    void LoadConfiguration();
    /// Restore all buttons to their default values.
    void RestoreDefaults();

    std::unique_ptr<Ui::ConfigureTouchscreenAdvanced> ui;
};

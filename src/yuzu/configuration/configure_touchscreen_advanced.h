// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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

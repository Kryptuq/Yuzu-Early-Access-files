// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <optional>
#include <QDialog>

class QCheckBox;
class QPushButton;
class QTimer;

namespace InputCommon {
class InputSubsystem;
}

namespace Ui {
class ConfigureMouseAdvanced;
}

class ConfigureMouseAdvanced : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureMouseAdvanced(QWidget* parent, InputCommon::InputSubsystem* input_subsystem_);
    ~ConfigureMouseAdvanced() override;

    void ApplyConfiguration();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    /// Load configuration settings.
    void LoadConfiguration();
    /// Restore all buttons to their default values.
    void RestoreDefaults();
    /// Clear all input configuration
    void ClearAll();

    /// Update UI to reflect current configuration.
    void UpdateButtonLabels();

    /// Called when the button was pressed.
    void HandleClick(QPushButton* button,
                     std::function<void(const Common::ParamPackage&)> new_input_setter,
                     InputCommon::Polling::DeviceType type);

    /// Finish polling and configure input using the input_setter
    void SetPollingResult(const Common::ParamPackage& params, bool abort);

    /// Handle mouse button press events.
    void mousePressEvent(QMouseEvent* event) override;

    /// Handle key press events.
    void keyPressEvent(QKeyEvent* event) override;

    std::unique_ptr<Ui::ConfigureMouseAdvanced> ui;

    InputCommon::InputSubsystem* input_subsystem;

    /// This will be the the setting function when an input is awaiting configuration.
    std::optional<std::function<void(const Common::ParamPackage&)>> input_setter;

    std::array<QPushButton*, Settings::NativeMouseButton::NumMouseButtons> button_map;
    std::array<Common::ParamPackage, Settings::NativeMouseButton::NumMouseButtons> buttons_param;

    std::vector<std::unique_ptr<InputCommon::Polling::DevicePoller>> device_pollers;

    std::unique_ptr<QTimer> timeout_timer;
    std::unique_ptr<QTimer> poll_timer;

    /// A flag to indicate if keyboard keys are okay when configuring an input. If this is false,
    /// keyboard events are ignored.
    bool want_keyboard_mouse = false;
};

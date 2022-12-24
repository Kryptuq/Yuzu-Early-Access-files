// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>
#include <memory>
#include <QDialog>

namespace InputCommon {
class InputSubsystem;
} // namespace InputCommon

namespace Core::HID {
class HIDCore;
class EmulatedController;
} // namespace Core::HID

namespace Ui {
class ConfigureRingController;
} // namespace Ui

class ConfigureRingController : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureRingController(QWidget* parent, InputCommon::InputSubsystem* input_subsystem_,
                                     Core::HID::HIDCore& hid_core_);
    ~ConfigureRingController() override;

    void ApplyConfiguration();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    void UpdateUI();

    /// Load configuration settings.
    void LoadConfiguration();

    /// Restore all buttons to their default values.
    void RestoreDefaults();

    /// Called when the button was pressed.
    void HandleClick(QPushButton* button,
                     std::function<void(const Common::ParamPackage&)> new_input_setter,
                     InputCommon::Polling::InputType type);

    /// Finish polling and configure input using the input_setter.
    void SetPollingResult(const Common::ParamPackage& params, bool abort);

    /// Checks whether a given input can be accepted.
    bool IsInputAcceptable(const Common::ParamPackage& params) const;

    /// Handle mouse button press events.
    void mousePressEvent(QMouseEvent* event) override;

    /// Handle key press events.
    void keyPressEvent(QKeyEvent* event) override;

    QString ButtonToText(const Common::ParamPackage& param);

    QString AnalogToText(const Common::ParamPackage& param, const std::string& dir);

    static constexpr int ANALOG_SUB_BUTTONS_NUM = 2;

    // A group of four QPushButtons represent one analog input. The buttons each represent left,
    // right, respectively.
    std::array<QPushButton*, ANALOG_SUB_BUTTONS_NUM> analog_map_buttons;

    static const std::array<std::string, ANALOG_SUB_BUTTONS_NUM> analog_sub_buttons;

    std::unique_ptr<QTimer> timeout_timer;
    std::unique_ptr<QTimer> poll_timer;

    /// This will be the the setting function when an input is awaiting configuration.
    std::optional<std::function<void(const Common::ParamPackage&)>> input_setter;

    InputCommon::InputSubsystem* input_subsystem;
    Core::HID::EmulatedController* emulated_controller;

    std::unique_ptr<Ui::ConfigureRingController> ui;
};

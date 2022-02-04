// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <memory>
#include <QDialog>

class QGroupBox;
class QSpinBox;

namespace Ui {
class ConfigureVibration;
}

namespace Core::HID {
enum class ControllerTriggerType;
class HIDCore;
} // namespace Core::HID

class ConfigureVibration : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureVibration(QWidget* parent, Core::HID::HIDCore& hid_core_);
    ~ConfigureVibration() override;

    void ApplyConfiguration();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();
    void VibrateController(Core::HID::ControllerTriggerType type, std::size_t player_index);
    void StopVibrations();

    std::unique_ptr<Ui::ConfigureVibration> ui;

    static constexpr std::size_t NUM_PLAYERS = 8;

    /// Groupboxes encapsulating the vibration strength spinbox.
    std::array<QGroupBox*, NUM_PLAYERS> vibration_groupboxes;

    /// Spinboxes representing the vibration strength percentage.
    std::array<QSpinBox*, NUM_PLAYERS> vibration_spinboxes;

    /// Callback index to stop the controllers events
    std::array<int, NUM_PLAYERS> controller_callback_key;

    Core::HID::HIDCore& hid_core;
};

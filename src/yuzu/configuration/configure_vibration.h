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

class ConfigureVibration : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureVibration(QWidget* parent);
    ~ConfigureVibration() override;

    void ApplyConfiguration();

    static void SetVibrationDevices(std::size_t player_index);
    static void SetAllVibrationDevices();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    std::unique_ptr<Ui::ConfigureVibration> ui;

    static constexpr std::size_t NUM_PLAYERS = 8;

    // Groupboxes encapsulating the vibration strength spinbox.
    std::array<QGroupBox*, NUM_PLAYERS> vibration_groupboxes;

    // Spinboxes representing the vibration strength percentage.
    std::array<QSpinBox*, NUM_PLAYERS> vibration_spinboxes;
};

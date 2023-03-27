// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <memory>
#include <QDialog>
#include "core/frontend/applets/controller.h"

class GMainWindow;
class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QGroupBox;
class QLabel;

class InputProfiles;

namespace InputCommon {
class InputSubsystem;
}

namespace Ui {
class QtControllerSelectorDialog;
}

namespace Core {
class System;
}

namespace Core::HID {
class HIDCore;
enum class NpadStyleIndex : u8;
} // namespace Core::HID

class QtControllerSelectorDialog final : public QDialog {
    Q_OBJECT

public:
    explicit QtControllerSelectorDialog(QWidget* parent,
                                        Core::Frontend::ControllerParameters parameters_,
                                        InputCommon::InputSubsystem* input_subsystem_,
                                        Core::System& system_);
    ~QtControllerSelectorDialog() override;

    int exec() override;

private:
    // Applies the current configuration.
    void ApplyConfiguration();

    // Loads the current input configuration into the frontend applet.
    void LoadConfiguration();

    // Initializes the "Configure Vibration" Dialog.
    void CallConfigureVibrationDialog();

    // Initializes the "Configure Motion / Touch" Dialog.
    void CallConfigureMotionTouchDialog();

    // Initializes the "Create Input Profile" Dialog.
    void CallConfigureInputProfileDialog();

    // Checks the current configuration against the given parameters.
    // This sets and returns the value of parameters_met.
    bool CheckIfParametersMet();

    // Sets the controller icons for "Supported Controller Types".
    void SetSupportedControllers();

    // Sets the emulated controllers per player.
    void SetEmulatedControllers(std::size_t player_index);

    // Gets the Controller Type for a given controller combobox index per player.
    Core::HID::NpadStyleIndex GetControllerTypeFromIndex(int index, std::size_t player_index) const;

    // Gets the controller combobox index for a given Controller Type per player.
    int GetIndexFromControllerType(Core::HID::NpadStyleIndex type, std::size_t player_index) const;

    // Updates the controller icons per player.
    void UpdateControllerIcon(std::size_t player_index);

    // Updates the controller state (type and connection status) per player.
    void UpdateControllerState(std::size_t player_index);

    // Updates the LED pattern per player.
    void UpdateLEDPattern(std::size_t player_index);

    // Updates the border color per player.
    void UpdateBorderColor(std::size_t player_index);

    // Sets the "Explain Text" per player.
    void SetExplainText(std::size_t player_index);

    // Updates the console mode.
    void UpdateDockedState(bool is_handheld);

    // Disables and disconnects unsupported players based on the given parameters.
    void DisableUnsupportedPlayers();

    std::unique_ptr<Ui::QtControllerSelectorDialog> ui;

    // Parameters sent in from the backend HLE applet.
    Core::Frontend::ControllerParameters parameters;

    InputCommon::InputSubsystem* input_subsystem;

    std::unique_ptr<InputProfiles> input_profiles;

    Core::System& system;

    // This is true if and only if all parameters are met. Otherwise, this is false.
    // This determines whether the "OK" button can be clicked to exit the applet.
    bool parameters_met{false};

    static constexpr std::size_t NUM_PLAYERS = 8;

    // Widgets encapsulating the groupboxes and comboboxes per player.
    std::array<QWidget*, NUM_PLAYERS> player_widgets;

    // Groupboxes encapsulating the controller icons and LED patterns per player.
    std::array<QGroupBox*, NUM_PLAYERS> player_groupboxes;

    // Icons for currently connected controllers/players.
    std::array<QWidget*, NUM_PLAYERS> connected_controller_icons;

    // Labels that represent the player numbers in place of the controller icons.
    std::array<QLabel*, NUM_PLAYERS> player_labels;

    // LED patterns for currently connected controllers/players.
    std::array<std::array<QCheckBox*, 4>, NUM_PLAYERS> led_patterns_boxes;

    // Labels representing additional information known as "Explain Text" per player.
    std::array<QLabel*, NUM_PLAYERS> explain_text_labels;

    // Comboboxes with a list of emulated controllers per player.
    std::array<QComboBox*, NUM_PLAYERS> emulated_controllers;

    /// Pairs of emulated controller index and Controller Type enum per player.
    std::array<std::vector<std::pair<int, Core::HID::NpadStyleIndex>>, NUM_PLAYERS>
        index_controller_type_pairs;

    // Labels representing the number of connected controllers
    // above the "Connected Controllers" checkboxes.
    std::array<QLabel*, NUM_PLAYERS> connected_controller_labels;

    // Checkboxes representing the "Connected Controllers".
    std::array<QCheckBox*, NUM_PLAYERS> connected_controller_checkboxes;
};

class QtControllerSelector final : public QObject, public Core::Frontend::ControllerApplet {
    Q_OBJECT

public:
    explicit QtControllerSelector(GMainWindow& parent);
    ~QtControllerSelector() override;

    void Close() const override;
    void ReconfigureControllers(
        ReconfigureCallback callback_,
        const Core::Frontend::ControllerParameters& parameters) const override;

signals:
    void MainWindowReconfigureControllers(
        const Core::Frontend::ControllerParameters& parameters) const;
    void MainWindowRequestExit() const;

private:
    void MainWindowReconfigureFinished();

    mutable ReconfigureCallback callback;
};

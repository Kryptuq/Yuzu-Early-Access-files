// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <QWidget>

#include "common/param_package.h"
#include "common/settings.h"
#include "ui_configure_input.h"

class QCheckBox;
class QKeyEvent;
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;
class QString;
class QTimer;
class QWidget;

class InputProfiles;

namespace Core {
class System;
}

namespace InputCommon {
class InputSubsystem;
}

namespace InputCommon::Polling {
enum class InputType;
} // namespace InputCommon::Polling

namespace Ui {
class ConfigureInputPlayer;
}

namespace Core {
class System;
}

namespace Core::HID {
class EmulatedController;
enum class NpadType : u8;
} // namespace Core::HID

class ConfigureInputPlayer : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureInputPlayer(QWidget* parent, std::size_t player_index, QWidget* bottom_row,
                                  InputCommon::InputSubsystem* input_subsystem_,
                                  InputProfiles* profiles_, Core::System& system_,
                                  bool debug = false);
    ~ConfigureInputPlayer() override;

    /// Save all button configurations to settings file.
    void ApplyConfiguration();

    /// Set the connection state checkbox (used to sync state).
    void ConnectPlayer(bool connected);

    /// Update the input devices combobox.
    void UpdateInputDeviceCombobox();

    /// Updates the list of controller profiles.
    void UpdateInputProfiles();

    /// Restore all buttons to their default values.
    void RestoreDefaults();

    /// Clear all input configuration.
    void ClearAll();

signals:
    /// Emitted when this controller is connected by the user.
    void Connected(bool connected);
    /// Emitted when the Handheld mode is selected (undocked with dual joycons attached).
    void HandheldStateChanged(bool is_handheld);
    /// Emitted when the input devices combobox is being refreshed.
    void RefreshInputDevices();
    /**
     * Emitted when the input profiles combobox is being refreshed.
     * The player_index represents the current player's index, and the profile combobox
     * will not be updated for this index as they are already updated by other mechanisms.
     */
    void RefreshInputProfiles(std::size_t player_index);

protected:
    void showEvent(QShowEvent* event) override;

private:
    QString ButtonToText(const Common::ParamPackage& param);

    QString AnalogToText(const Common::ParamPackage& param, const std::string& dir);

    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    /// Load configuration settings.
    void LoadConfiguration();

    /// Called when the button was pressed.
    void HandleClick(QPushButton* button, std::size_t button_id,
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

    /// Update UI to reflect current configuration.
    void UpdateUI();

    /// Sets the available controllers.
    void SetConnectableControllers();

    /// Gets the Controller Type for a given controller combobox index.
    Core::HID::NpadType GetControllerTypeFromIndex(int index) const;

    /// Gets the controller combobox index for a given Controller Type.
    int GetIndexFromControllerType(Core::HID::NpadType type) const;

    /// Update the available input devices.
    void UpdateInputDevices();

    /// Hides and disables controller settings based on the current controller type.
    void UpdateControllerAvailableButtons();

    /// Disables controller settings based on the current controller type.
    void UpdateControllerEnabledButtons();

    /// Shows or hides motion groupboxes based on the current controller type.
    void UpdateMotionButtons();

    /// Alters the button names based on the current controller type.
    void UpdateControllerButtonNames();

    /// Gets the default controller mapping for this device and auto configures the input to match.
    void UpdateMappingWithDefaults();

    /// Creates a controller profile.
    void CreateProfile();

    /// Deletes the selected controller profile.
    void DeleteProfile();

    /// Loads the selected controller profile.
    void LoadProfile();

    /// Saves the current controller configuration into a selected controller profile.
    void SaveProfile();

    std::unique_ptr<Ui::ConfigureInputPlayer> ui;

    std::size_t player_index;
    bool debug;

    InputCommon::InputSubsystem* input_subsystem;

    InputProfiles* profiles;

    std::unique_ptr<QTimer> timeout_timer;
    std::unique_ptr<QTimer> poll_timer;

    /// Stores a pair of "Connected Controllers" combobox index and Controller Type enum.
    std::vector<std::pair<int, Core::HID::NpadType>> index_controller_type_pairs;

    static constexpr int PLAYER_COUNT = 8;
    std::array<QCheckBox*, PLAYER_COUNT> player_connected_checkbox;

    /// This will be the the setting function when an input is awaiting configuration.
    std::optional<std::function<void(const Common::ParamPackage&)>> input_setter;

    Core::HID::EmulatedController* emulated_controller;

    static constexpr int ANALOG_SUB_BUTTONS_NUM = 4;

    /// Each button input is represented by a QPushButton.
    std::array<QPushButton*, Settings::NativeButton::NumButtons> button_map;

    /// A group of four QPushButtons represent one analog input. The buttons each represent up,
    /// down, left, right, respectively.
    std::array<std::array<QPushButton*, ANALOG_SUB_BUTTONS_NUM>, Settings::NativeAnalog::NumAnalogs>
        analog_map_buttons;

    /// Each motion input is represented by a QPushButton.
    std::array<QPushButton*, Settings::NativeMotion::NumMotions> motion_map;

    std::array<QLabel*, Settings::NativeAnalog::NumAnalogs> analog_map_deadzone_label;
    std::array<QSlider*, Settings::NativeAnalog::NumAnalogs> analog_map_deadzone_slider;
    std::array<QGroupBox*, Settings::NativeAnalog::NumAnalogs> analog_map_modifier_groupbox;
    std::array<QPushButton*, Settings::NativeAnalog::NumAnalogs> analog_map_modifier_button;
    std::array<QLabel*, Settings::NativeAnalog::NumAnalogs> analog_map_modifier_label;
    std::array<QSlider*, Settings::NativeAnalog::NumAnalogs> analog_map_modifier_slider;
    std::array<QGroupBox*, Settings::NativeAnalog::NumAnalogs> analog_map_range_groupbox;
    std::array<QSpinBox*, Settings::NativeAnalog::NumAnalogs> analog_map_range_spinbox;

    static const std::array<std::string, ANALOG_SUB_BUTTONS_NUM> analog_sub_buttons;

    /// A flag to indicate that the "Map Analog Stick" pop-up has been shown and accepted once.
    bool map_analog_stick_accepted{};

    /// List of physical devices users can map with. If a SDL backed device is selected, then you
    /// can use this device to get a default mapping.
    std::vector<Common::ParamPackage> input_devices;

    /// Bottom row is where console wide settings are held, and its "owned" by the parent
    /// ConfigureInput widget. On show, add this widget to the main layout. This will change the
    /// parent of the widget to this widget (but thats fine).
    QWidget* bottom_row;

    Core::System& system;
};

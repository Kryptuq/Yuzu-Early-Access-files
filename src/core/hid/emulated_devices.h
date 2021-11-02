// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "common/common_types.h"
#include "common/input.h"
#include "common/param_package.h"
#include "common/settings.h"
#include "core/hid/hid_types.h"
#include "core/hid/motion_input.h"

namespace Core::HID {

using KeyboardDevices = std::array<std::unique_ptr<Common::Input::InputDevice>,
                                   Settings::NativeKeyboard::NumKeyboardKeys>;
using KeyboardModifierDevices = std::array<std::unique_ptr<Common::Input::InputDevice>,
                                           Settings::NativeKeyboard::NumKeyboardMods>;
using MouseButtonDevices = std::array<std::unique_ptr<Common::Input::InputDevice>,
                                      Settings::NativeMouseButton::NumMouseButtons>;

using MouseButtonParams =
    std::array<Common::ParamPackage, Settings::NativeMouseButton::NumMouseButtons>;

using KeyboardValues =
    std::array<Common::Input::ButtonStatus, Settings::NativeKeyboard::NumKeyboardKeys>;
using KeyboardModifierValues =
    std::array<Common::Input::ButtonStatus, Settings::NativeKeyboard::NumKeyboardMods>;
using MouseButtonValues =
    std::array<Common::Input::ButtonStatus, Settings::NativeMouseButton::NumMouseButtons>;

struct MousePosition {
    s32 x;
    s32 y;
    s32 delta_wheel_x;
    s32 delta_wheel_y;
};

struct DeviceStatus {
    // Data from input_common
    KeyboardValues keyboard_values{};
    KeyboardModifierValues keyboard_moddifier_values{};
    MouseButtonValues mouse_button_values{};

    // Data for HID serices
    KeyboardKey keyboard_state{};
    KeyboardModifier keyboard_moddifier_state{};
    MouseButton mouse_button_state{};
    MousePosition mouse_position_state{};
};

enum class DeviceTriggerType {
    Keyboard,
    KeyboardModdifier,
    Mouse,
};

struct InterfaceUpdateCallback {
    std::function<void(DeviceTriggerType)> on_change;
};

class EmulatedDevices {
public:
    /**
     * Contains all input data related to external devices that aren't necesarily a controller
     * like keyboard and mouse
     */
    EmulatedDevices();
    ~EmulatedDevices();

    YUZU_NON_COPYABLE(EmulatedDevices);
    YUZU_NON_MOVEABLE(EmulatedDevices);

    /// Removes all callbacks created from input devices
    void UnloadInput();

    /// Sets the emulated console into configuring mode. Locking all HID service events from being
    /// moddified
    void EnableConfiguration();

    /// Returns the emulated console to the normal behaivour
    void DisableConfiguration();

    /// Returns true if the emulated device is on configuring mode
    bool IsConfiguring() const;

    /// Reload all input devices
    void ReloadInput();

    /// Overrides current mapped devices with the stored configuration and reloads all input devices
    void ReloadFromSettings();

    /// Saves the current mapped configuration
    void SaveCurrentConfig();

    /// Reverts any mapped changes made that weren't saved
    void RestoreConfig();

    /// Returns the current mapped motion device
    Common::ParamPackage GetMouseButtonParam(std::size_t index) const;

    /**
     * Updates the current mapped mouse button device
     * @param ParamPackage with controller data to be mapped
     */
    void SetMouseButtonParam(std::size_t index, Common::ParamPackage param);

    /// Returns the latest status of button input from the keyboard with parameters
    KeyboardValues GetKeyboardValues() const;

    /// Returns the latest status of button input from the keyboard modifiers with parameters
    KeyboardModifierValues GetKeyboardModdifierValues() const;

    /// Returns the latest status of button input from the mouse with parameters
    MouseButtonValues GetMouseButtonsValues() const;

    /// Returns the latest status of button input from the keyboard
    KeyboardKey GetKeyboard() const;

    /// Returns the latest status of button input from the keyboard modifiers
    KeyboardModifier GetKeyboardModifier() const;

    /// Returns the latest status of button input from the mouse
    MouseButton GetMouseButtons() const;

    /// Returns the latest mouse coordinates
    MousePosition GetMousePosition() const;

    /**
     * Adds a callback to the list of events
     * @param ConsoleUpdateCallback that will be triggered
     * @return an unique key corresponding to the callback index in the list
     */
    int SetCallback(InterfaceUpdateCallback update_callback);

    /**
     * Removes a callback from the list stopping any future events to this object
     * @param Key corresponding to the callback index in the list
     */
    void DeleteCallback(int key);

private:
    /// Helps assigning a value to keyboard_state
    void UpdateKey(std::size_t key_index, bool status);

    /**
     * Updates the touch status of the console
     * @param callback: A CallbackStatus containing the key status
     * @param index: key ID to be updated
     */
    void SetKeyboardButton(Common::Input::CallbackStatus callback, std::size_t index);

    /**
     * Updates the touch status of the console
     * @param callback: A CallbackStatus containing the modifier key status
     * @param index: modifier key ID to be updated
     */
    void SetKeyboardModifier(Common::Input::CallbackStatus callback, std::size_t index);

    /**
     * Updates the touch status of the console
     * @param callback: A CallbackStatus containing the button status
     * @param index: Button ID of the to be updated
     */
    void SetMouseButton(Common::Input::CallbackStatus callback, std::size_t index);

    /**
     * Triggers a callback that something has changed on the device status
     * @param Input type of the event to trigger
     */
    void TriggerOnChange(DeviceTriggerType type);

    bool is_configuring{false};

    MouseButtonParams mouse_button_params;

    KeyboardDevices keyboard_devices;
    KeyboardModifierDevices keyboard_modifier_devices;
    MouseButtonDevices mouse_button_devices;

    mutable std::mutex mutex;
    std::unordered_map<int, InterfaceUpdateCallback> callback_list;
    int last_callback_key = 0;

    // Stores the current status of all external device input
    DeviceStatus device_status;
};

} // namespace Core::HID

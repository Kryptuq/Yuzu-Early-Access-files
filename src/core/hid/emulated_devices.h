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

namespace Core::HID {
using KeyboardDevices = std::array<std::unique_ptr<Common::Input::InputDevice>,
                                   Settings::NativeKeyboard::NumKeyboardKeys>;
using KeyboardModifierDevices = std::array<std::unique_ptr<Common::Input::InputDevice>,
                                           Settings::NativeKeyboard::NumKeyboardMods>;
using MouseButtonDevices = std::array<std::unique_ptr<Common::Input::InputDevice>,
                                      Settings::NativeMouseButton::NumMouseButtons>;
using MouseAnalogDevices = std::array<std::unique_ptr<Common::Input::InputDevice>,
                                      Settings::NativeMouseWheel::NumMouseWheels>;
using MouseStickDevice = std::unique_ptr<Common::Input::InputDevice>;
using RingAnalogDevice = std::unique_ptr<Common::Input::InputDevice>;

using MouseButtonParams =
    std::array<Common::ParamPackage, Settings::NativeMouseButton::NumMouseButtons>;
using RingAnalogParams = Common::ParamPackage;

using KeyboardValues =
    std::array<Common::Input::ButtonStatus, Settings::NativeKeyboard::NumKeyboardKeys>;
using KeyboardModifierValues =
    std::array<Common::Input::ButtonStatus, Settings::NativeKeyboard::NumKeyboardMods>;
using MouseButtonValues =
    std::array<Common::Input::ButtonStatus, Settings::NativeMouseButton::NumMouseButtons>;
using MouseAnalogValues =
    std::array<Common::Input::AnalogStatus, Settings::NativeMouseWheel::NumMouseWheels>;
using MouseStickValue = Common::Input::TouchStatus;
using RingAnalogValue = Common::Input::AnalogStatus;

struct MousePosition {
    f32 x;
    f32 y;
};

struct RingSensorForce {
    f32 force;
};

struct DeviceStatus {
    // Data from input_common
    KeyboardValues keyboard_values{};
    KeyboardModifierValues keyboard_moddifier_values{};
    MouseButtonValues mouse_button_values{};
    MouseAnalogValues mouse_analog_values{};
    MouseStickValue mouse_stick_value{};
    RingAnalogValue ring_analog_value{};

    // Data for HID serices
    KeyboardKey keyboard_state{};
    KeyboardModifier keyboard_moddifier_state{};
    MouseButton mouse_button_state{};
    MousePosition mouse_position_state{};
    AnalogStickState mouse_wheel_state{};
    RingSensorForce ring_analog_state{};
};

enum class DeviceTriggerType {
    Keyboard,
    KeyboardModdifier,
    Mouse,
    RingController,
};

struct InterfaceUpdateCallback {
    std::function<void(DeviceTriggerType)> on_change;
};

class EmulatedDevices {
public:
    /**
     * Contains all input data related to external devices that aren't necesarily a controller
     * This includes devices such as the keyboard or mouse
     */
    explicit EmulatedDevices();
    ~EmulatedDevices();

    YUZU_NON_COPYABLE(EmulatedDevices);
    YUZU_NON_MOVEABLE(EmulatedDevices);

    /// Removes all callbacks created from input devices
    void UnloadInput();

    /**
     * Sets the emulated devices into configuring mode
     * This prevents the modification of the HID state of the emulated devices by input commands
     */
    void EnableConfiguration();

    /// Returns the emulated devices into normal mode, allowing the modification of the HID state
    void DisableConfiguration();

    /// Returns true if the emulated device is in configuring mode
    bool IsConfiguring() const;

    /// Reload all input devices
    void ReloadInput();

    /// Overrides current mapped devices with the stored configuration and reloads all input devices
    void ReloadFromSettings();

    /// Saves the current mapped configuration
    void SaveCurrentConfig();

    /// Reverts any mapped changes made that weren't saved
    void RestoreConfig();

    // Returns the current mapped ring device
    Common::ParamPackage GetRingParam() const;

    /**
     * Updates the current mapped ring device
     * @param param ParamPackage with ring sensor data to be mapped
     */
    void SetRingParam(Common::ParamPackage param);

    /// Returns the latest status of button input from the keyboard with parameters
    KeyboardValues GetKeyboardValues() const;

    /// Returns the latest status of button input from the keyboard modifiers with parameters
    KeyboardModifierValues GetKeyboardModdifierValues() const;

    /// Returns the latest status of button input from the mouse with parameters
    MouseButtonValues GetMouseButtonsValues() const;

    /// Returns the latest status of analog input from the ring sensor with parameters
    RingAnalogValue GetRingSensorValues() const;

    /// Returns the latest status of button input from the keyboard
    KeyboardKey GetKeyboard() const;

    /// Returns the latest status of button input from the keyboard modifiers
    KeyboardModifier GetKeyboardModifier() const;

    /// Returns the latest status of button input from the mouse
    MouseButton GetMouseButtons() const;

    /// Returns the latest mouse coordinates
    MousePosition GetMousePosition() const;

    /// Returns the latest mouse wheel change
    AnalogStickState GetMouseWheel() const;

    /// Returns the latest ringcon force sensor value
    RingSensorForce GetRingSensorForce() const;

    /**
     * Adds a callback to the list of events
     * @param update_callback InterfaceUpdateCallback that will be triggered
     * @return an unique key corresponding to the callback index in the list
     */
    int SetCallback(InterfaceUpdateCallback update_callback);

    /**
     * Removes a callback from the list stopping any future events to this object
     * @param key Key corresponding to the callback index in the list
     */
    void DeleteCallback(int key);

private:
    /// Helps assigning a value to keyboard_state
    void UpdateKey(std::size_t key_index, bool status);

    /**
     * Updates the touch status of the keyboard device
     * @param callback A CallbackStatus containing the key status
     * @param index key ID to be updated
     */
    void SetKeyboardButton(const Common::Input::CallbackStatus& callback, std::size_t index);

    /**
     * Updates the keyboard status of the keyboard device
     * @param callback A CallbackStatus containing the modifier key status
     * @param index modifier key ID to be updated
     */
    void SetKeyboardModifier(const Common::Input::CallbackStatus& callback, std::size_t index);

    /**
     * Updates the mouse button status of the mouse device
     * @param callback A CallbackStatus containing the button status
     * @param index Button ID to be updated
     */
    void SetMouseButton(const Common::Input::CallbackStatus& callback, std::size_t index);

    /**
     * Updates the mouse wheel status of the mouse device
     * @param callback A CallbackStatus containing the wheel status
     * @param index wheel ID to be updated
     */
    void SetMouseAnalog(const Common::Input::CallbackStatus& callback, std::size_t index);

    /**
     * Updates the mouse position status of the mouse device
     * @param callback A CallbackStatus containing the position status
     */
    void SetMouseStick(const Common::Input::CallbackStatus& callback);

    /**
     * Updates the ring analog sensor status of the ring controller
     * @param callback A CallbackStatus containing the force status
     */
    void SetRingAnalog(const Common::Input::CallbackStatus& callback);

    /**
     * Triggers a callback that something has changed on the device status
     * @param type Input type of the event to trigger
     */
    void TriggerOnChange(DeviceTriggerType type);

    bool is_configuring{false};

    RingAnalogParams ring_params;

    KeyboardDevices keyboard_devices;
    KeyboardModifierDevices keyboard_modifier_devices;
    MouseButtonDevices mouse_button_devices;
    MouseAnalogDevices mouse_analog_devices;
    MouseStickDevice mouse_stick_device;
    RingAnalogDevice ring_analog_device;

    mutable std::mutex mutex;
    mutable std::mutex callback_mutex;
    std::unordered_map<int, InterfaceUpdateCallback> callback_list;
    int last_callback_key = 0;

    // Stores the current status of all external device input
    DeviceStatus device_status;
};

} // namespace Core::HID

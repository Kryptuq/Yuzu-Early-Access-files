// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#pragma once

namespace Common::Input {
struct CallbackStatus;
enum class BatteryLevel : u32;
using BatteryStatus = BatteryLevel;
struct AnalogStatus;
struct ButtonStatus;
struct MotionStatus;
struct StickStatus;
struct TouchStatus;
struct TriggerStatus;
}; // namespace Common::Input

namespace Core::HID {

/**
 * Converts raw input data into a valid battery status.
 *
 * @param Supported callbacks: Analog, Battery, Trigger.
 * @return A valid BatteryStatus object.
 */
Common::Input::BatteryStatus TransformToBattery(const Common::Input::CallbackStatus& callback);

/**
 * Converts raw input data into a valid button status. Applies invert properties to the output.
 *
 * @param Supported callbacks: Analog, Button, Trigger.
 * @return A valid TouchStatus object.
 */
Common::Input::ButtonStatus TransformToButton(const Common::Input::CallbackStatus& callback);

/**
 * Converts raw input data into a valid motion status.
 *
 * @param Supported callbacks: Motion.
 * @return A valid TouchStatus object.
 */
Common::Input::MotionStatus TransformToMotion(const Common::Input::CallbackStatus& callback);

/**
 * Converts raw input data into a valid stick status. Applies offset, deadzone, range and invert
 * properties to the output.
 *
 * @param Supported callbacks: Stick.
 * @return A valid StickStatus object.
 */
Common::Input::StickStatus TransformToStick(const Common::Input::CallbackStatus& callback);

/**
 * Converts raw input data into a valid touch status.
 *
 * @param Supported callbacks: Touch.
 * @return A valid TouchStatus object.
 */
Common::Input::TouchStatus TransformToTouch(const Common::Input::CallbackStatus& callback);

/**
 * Converts raw input data into a valid trigger status. Applies offset, deadzone, range and
 * invert properties to the output. Button status uses the threshold property if necessary.
 *
 * @param Supported callbacks: Analog, Button, Trigger.
 * @return A valid TriggerStatus object.
 */
Common::Input::TriggerStatus TransformToTrigger(const Common::Input::CallbackStatus& callback);

/**
 * Converts raw input data into a valid analog status. Applies offset, deadzone, range and
 * invert properties to the output.
 *
 * @param Supported callbacks: Analog.
 * @return A valid AnalogStatus object.
 */
Common::Input::AnalogStatus TransformToAnalog(const Common::Input::CallbackStatus& callback);

/**
 * Converts raw analog data into a valid analog value
 * @param An analog object containing raw data and properties, bool that determines if the value
 * needs to be clamped between -1.0f and 1.0f.
 */
void SanitizeAnalog(Common::Input::AnalogStatus& analog, bool clamp_value);

/**
 * Converts raw stick data into a valid stick value
 * @param Two analog objects containing raw data and properties, bool that determines if the value
 * needs to be clamped into the unit circle.
 */
void SanitizeStick(Common::Input::AnalogStatus& analog_x, Common::Input::AnalogStatus& analog_y,
                   bool clamp_value);

} // namespace Core::HID

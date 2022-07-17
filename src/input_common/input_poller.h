// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Input {
class InputDevice;

template <typename InputDevice>
class Factory;
}; // namespace Input

namespace InputCommon {
class InputEngine;

class OutputFactory final : public Common::Input::Factory<Common::Input::OutputDevice> {
public:
    explicit OutputFactory(std::shared_ptr<InputEngine> input_engine_);

    /**
     * Creates an output device from the parameters given.
     * @param params contains parameters for creating the device:
     *               - "guid" text string for identifying controllers
     *               - "port": port of the connected device
     *               - "pad": slot of the connected controller
     * @returns a unique output device with the parameters specified
     */
    std::unique_ptr<Common::Input::OutputDevice> Create(
        const Common::ParamPackage& params) override;

private:
    std::shared_ptr<InputEngine> input_engine;
};

/**
 * An Input factory. It receives input events and forward them to all input devices it created.
 */
class InputFactory final : public Common::Input::Factory<Common::Input::InputDevice> {
public:
    explicit InputFactory(std::shared_ptr<InputEngine> input_engine_);

    /**
     * Creates an input device from the parameters given. Identifies the type of input to be
     * returned if it contains the following parameters:
     * - button: Contains "button" or "code"
     * - hat_button: Contains "hat"
     * - analog: Contains "axis"
     * - trigger: Contains "button" and  "axis"
     * - stick: Contains "axis_x" and "axis_y"
     * - motion: Contains "axis_x", "axis_y" and "axis_z"
     * - motion: Contains "motion"
     * - touch: Contains "button", "axis_x" and "axis_y"
     * - battery: Contains "battery"
     * - output: Contains "output"
     * @param params contains parameters for creating the device:
     *               - "code": the code of the keyboard key to bind with the input
     *               - "button": same as "code" but for controller buttons
     *               - "hat": similar as "button" but it's a group of hat buttons from SDL
     *               - "axis": the axis number of the axis to bind with the input
     *               - "motion": the motion number of the motion to bind with the input
     *               - "axis_x": same as axis but specifying horizontal direction
     *               - "axis_y": same as axis but specifying vertical direction
     *               - "axis_z": same as axis but specifying forward direction
     *               - "battery": Only used as a placeholder to set the input type
     * @returns a unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> Create(const Common::ParamPackage& params) override;

private:
    /**
     * Creates a button device from the parameters given.
     * @param params contains parameters for creating the device:
     *               - "code": the code of the keyboard key to bind with the input
     *               - "button": same as "code" but for controller buttons
     *               - "toggle": press once to enable, press again to disable
     *               - "inverted": inverts the output of the button
     *               - "guid": text string for identifying controllers
     *               - "port": port of the connected device
     *               - "pad": slot of the connected controller
     * @returns a unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateButtonDevice(
        const Common::ParamPackage& params);

    /**
     * Creates a hat button device from the parameters given.
     * @param params contains parameters for creating the device:
     *               - "button": the controller hat id to bind with the input
     *               - "direction": the direction id to be detected
     *               - "toggle": press once to enable, press again to disable
     *               - "inverted": inverts the output of the button
     *               - "guid": text string for identifying controllers
     *               - "port": port of the connected device
     *               - "pad": slot of the connected controller
     * @returns a unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateHatButtonDevice(
        const Common::ParamPackage& params);

    /**
     * Creates a stick device from the parameters given.
     * @param params contains parameters for creating the device:
     *               - "axis_x": the controller horizontal axis id to bind with the input
     *               - "axis_y": the controller vertical axis id to bind with the input
     *               - "deadzone": the minimum required value to be detected
     *               - "range": the maximum value required to reach 100%
     *               - "threshold": the minimum required value to considered pressed
     *               - "offset_x": the amount of offset in the x axis
     *               - "offset_y": the amount of offset in the y axis
     *               - "invert_x": inverts the sign of the horizontal axis
     *               - "invert_y": inverts the sign of the vertical axis
     *               - "guid": text string for identifying controllers
     *               - "port": port of the connected device
     *               - "pad": slot of the connected controller
     * @returns a unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateStickDevice(
        const Common::ParamPackage& params);

    /**
     * Creates an analog device from the parameters given.
     * @param params contains parameters for creating the device:
     *               - "axis": the controller axis id to bind with the input
     *               - "deadzone": the minimum required value to be detected
     *               - "range": the maximum value required to reach 100%
     *               - "threshold": the minimum required value to considered pressed
     *               - "offset": the amount of offset in the axis
     *               - "invert": inverts the sign of the axis
     *               - "guid": text string for identifying controllers
     *               - "port": port of the connected device
     *               - "pad": slot of the connected controller
     * @returns a unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateAnalogDevice(
        const Common::ParamPackage& params);

    /**
     * Creates a trigger device from the parameters given.
     * @param params contains parameters for creating the device:
     *               - "button": the controller hat id to bind with the input
     *               - "direction": the direction id to be detected
     *               - "toggle": press once to enable, press again to disable
     *               - "inverted": inverts the output of the button
     *               - "axis": the controller axis id to bind with the input
     *               - "deadzone": the minimum required value to be detected
     *               - "range": the maximum value required to reach 100%
     *               - "threshold": the minimum required value to considered pressed
     *               - "offset": the amount of offset in the axis
     *               - "invert": inverts the sign of the axis
     *               - "guid": text string for identifying controllers
     *               - "port": port of the connected device
     *               - "pad": slot of the connected controller
     * @returns a unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateTriggerDevice(
        const Common::ParamPackage& params);

    /**
     * Creates a touch device from the parameters given.
     * @param params contains parameters for creating the device:
     *               - "button": the controller hat id to bind with the input
     *               - "direction": the direction id to be detected
     *               - "toggle": press once to enable, press again to disable
     *               - "inverted": inverts the output of the button
     *               - "axis_x": the controller horizontal axis id to bind with the input
     *               - "axis_y": the controller vertical axis id to bind with the input
     *               - "deadzone": the minimum required value to be detected
     *               - "range": the maximum value required to reach 100%
     *               - "threshold": the minimum required value to considered pressed
     *               - "offset_x": the amount of offset in the x axis
     *               - "offset_y": the amount of offset in the y axis
     *               - "invert_x": inverts the sign of the horizontal axis
     *               - "invert_y": inverts the sign of the vertical axis
     *               - "guid": text string for identifying controllers
     *               - "port": port of the connected device
     *               - "pad": slot of the connected controller
     * @returns a unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateTouchDevice(
        const Common::ParamPackage& params);

    /**
     * Creates a battery device from the parameters given.
     * @param params contains parameters for creating the device:
     *               - "guid": text string for identifying controllers
     *               - "port": port of the connected device
     *               - "pad": slot of the connected controller
     * @returns a unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateBatteryDevice(
        const Common::ParamPackage& params);

    /**
     * Creates a motion device from the parameters given.
     * @param params contains parameters for creating the device:
     *               - "axis_x": the controller horizontal axis id to bind with the input
     *               - "axis_y": the controller vertical axis id to bind with the input
     *               - "axis_z": the controller forward axis id to bind with the input
     *               - "deadzone": the minimum required value to be detected
     *               - "range": the maximum value required to reach 100%
     *               - "offset_x": the amount of offset in the x axis
     *               - "offset_y": the amount of offset in the y axis
     *               - "offset_z": the amount of offset in the z axis
     *               - "invert_x": inverts the sign of the horizontal axis
     *               - "invert_y": inverts the sign of the vertical axis
     *               - "invert_z": inverts the sign of the forward axis
     *               - "guid": text string for identifying controllers
     *               - "port": port of the connected device
     *               - "pad": slot of the connected controller
     * @returns a unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateMotionDevice(Common::ParamPackage params);

    /**
     * Creates a camera device from the parameters given.
     * @param params contains parameters for creating the device:
     *               - "guid": text string for identifying controllers
     *               - "port": port of the connected device
     *               - "pad": slot of the connected controller
     * @returns a unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateCameraDevice(
        const Common::ParamPackage& params);

    std::shared_ptr<InputEngine> input_engine;
};
} // namespace InputCommon

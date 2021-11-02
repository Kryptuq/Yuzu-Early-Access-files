// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included

#pragma once

namespace Input {
class InputDevice;

template <typename InputDevice>
class Factory;
}; // namespace Input

namespace InputCommon {
class InputEngine;
/**
 * An Input factory. It receives input events and forward them to all input devices it created.
 */

class OutputFactory final : public Common::Input::Factory<Common::Input::OutputDevice> {
public:
    explicit OutputFactory(std::shared_ptr<InputEngine> input_engine_);

    /**
     * Creates an output device from the parameters given.
     * @param params contains parameters for creating the device:
     * @param    - "guid": text string for identifing controllers
     * @param    - "port": port of the connected device
     * @param    - "pad": slot of the connected controller
     * @return an unique ouput device with the parameters specified
     */
    std::unique_ptr<Common::Input::OutputDevice> Create(
        const Common::ParamPackage& params) override;

private:
    std::shared_ptr<InputEngine> input_engine;
};

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
     * @param    - "code": the code of the keyboard key to bind with the input
     * @param    - "button": same as "code" but for controller buttons
     * @param    - "hat": similar as "button" but it's a group of hat buttons from SDL
     * @param    - "axis": the axis number of the axis to bind with the input
     * @param    - "motion": the motion number of the motion to bind with the input
     * @param    - "axis_x": same as axis but specifing horizontal direction
     * @param    - "axis_y": same as axis but specifing vertical direction
     * @param    - "axis_z": same as axis but specifing forward direction
     * @param    - "battery": Only used as a placeholder to set the input type
     * @return an unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> Create(const Common::ParamPackage& params) override;

private:
    /**
     * Creates a button device from the parameters given.
     * @param params contains parameters for creating the device:
     * @param    - "code": the code of the keyboard key to bind with the input
     * @param    - "button": same as "code" but for controller buttons
     * @param    - "toggle": press once to enable, press again to disable
     * @param    - "inverted": inverts the output of the button
     * @param    - "guid": text string for identifing controllers
     * @param    - "port": port of the connected device
     * @param    - "pad": slot of the connected controller
     * @return an unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateButtonDevice(
        const Common::ParamPackage& params);

    /**
     * Creates a hat button device from the parameters given.
     * @param params contains parameters for creating the device:
     * @param    - "button": the controller hat id to bind with the input
     * @param    - "direction": the direction id to be detected
     * @param    - "toggle": press once to enable, press again to disable
     * @param    - "inverted": inverts the output of the button
     * @param    - "guid": text string for identifing controllers
     * @param    - "port": port of the connected device
     * @param    - "pad": slot of the connected controller
     * @return an unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateHatButtonDevice(
        const Common::ParamPackage& params);

    /**
     * Creates a stick device from the parameters given.
     * @param params contains parameters for creating the device:
     * @param    - "axis_x": the controller horizontal axis id to bind with the input
     * @param    - "axis_y": the controller vertical axis id to bind with the input
     * @param    - "deadzone": the mimimum required value to be detected
     * @param    - "range": the maximum value required to reach 100%
     * @param    - "threshold": the mimimum required value to considered pressed
     * @param    - "offset_x": the amount of offset in the x axis
     * @param    - "offset_y": the amount of offset in the y axis
     * @param    - "invert_x": inverts the sign of the horizontal axis
     * @param    - "invert_y": inverts the sign of the vertical axis
     * @param    - "guid": text string for identifing controllers
     * @param    - "port": port of the connected device
     * @param    - "pad": slot of the connected controller
     * @return an unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateStickDevice(
        const Common::ParamPackage& params);

    /**
     * Creates an analog device from the parameters given.
     * @param params contains parameters for creating the device:
     * @param    - "axis": the controller axis id to bind with the input
     * @param    - "deadzone": the mimimum required value to be detected
     * @param    - "range": the maximum value required to reach 100%
     * @param    - "threshold": the mimimum required value to considered pressed
     * @param    - "offset": the amount of offset in the axis
     * @param    - "invert": inverts the sign of the axis
     * @param    - "guid": text string for identifing controllers
     * @param    - "port": port of the connected device
     * @param    - "pad": slot of the connected controller
     * @return an unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateAnalogDevice(
        const Common::ParamPackage& params);

    /**
     * Creates a trigger device from the parameters given.
     * @param params contains parameters for creating the device:
     * @param    - "button": the controller hat id to bind with the input
     * @param    - "direction": the direction id to be detected
     * @param    - "toggle": press once to enable, press again to disable
     * @param    - "inverted": inverts the output of the button
     * @param    - "axis": the controller axis id to bind with the input
     * @param    - "deadzone": the mimimum required value to be detected
     * @param    - "range": the maximum value required to reach 100%
     * @param    - "threshold": the mimimum required value to considered pressed
     * @param    - "offset": the amount of offset in the axis
     * @param    - "invert": inverts the sign of the axis
     * @param    - "guid": text string for identifing controllers
     * @param    - "port": port of the connected device
     * @param    - "pad": slot of the connected controller
     * @return an unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateTriggerDevice(
        const Common::ParamPackage& params);

    /**
     * Creates a touch device from the parameters given.
     * @param params contains parameters for creating the device:
     * @param    - "button": the controller hat id to bind with the input
     * @param    - "direction": the direction id to be detected
     * @param    - "toggle": press once to enable, press again to disable
     * @param    - "inverted": inverts the output of the button
     * @param    - "axis_x": the controller horizontal axis id to bind with the input
     * @param    - "axis_y": the controller vertical axis id to bind with the input
     * @param    - "deadzone": the mimimum required value to be detected
     * @param    - "range": the maximum value required to reach 100%
     * @param    - "threshold": the mimimum required value to considered pressed
     * @param    - "offset_x": the amount of offset in the x axis
     * @param    - "offset_y": the amount of offset in the y axis
     * @param    - "invert_x": inverts the sign of the horizontal axis
     * @param    - "invert_y": inverts the sign of the vertical axis
     * @param    - "guid": text string for identifing controllers
     * @param    - "port": port of the connected device
     * @param    - "pad": slot of the connected controller
     * @return an unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateTouchDevice(
        const Common::ParamPackage& params);

    /**
     * Creates a battery device from the parameters given.
     * @param params contains parameters for creating the device:
     * @param    - "guid": text string for identifing controllers
     * @param    - "port": port of the connected device
     * @param    - "pad": slot of the connected controller
     * @return an unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateBatteryDevice(
        const Common::ParamPackage& params);

    /**
     * Creates a motion device from the parameters given.
     * @param params contains parameters for creating the device:
     * @param    - "axis_x": the controller horizontal axis id to bind with the input
     * @param    - "axis_y": the controller vertical axis id to bind with the input
     * @param    - "axis_z": the controller fordward axis id to bind with the input
     * @param    - "deadzone": the mimimum required value to be detected
     * @param    - "range": the maximum value required to reach 100%
     * @param    - "offset_x": the amount of offset in the x axis
     * @param    - "offset_y": the amount of offset in the y axis
     * @param    - "offset_z": the amount of offset in the z axis
     * @param    - "invert_x": inverts the sign of the horizontal axis
     * @param    - "invert_y": inverts the sign of the vertical axis
     * @param    - "invert_z": inverts the sign of the fordward axis
     * @param    - "guid": text string for identifing controllers
     * @param    - "port": port of the connected device
     * @param    - "pad": slot of the connected controller
     * @return an unique input device with the parameters specified
     */
    std::unique_ptr<Common::Input::InputDevice> CreateMotionDevice(Common::ParamPackage params);

    std::shared_ptr<InputEngine> input_engine;
};
} // namespace InputCommon

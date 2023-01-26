// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Based on dkms-hid-nintendo implementation, CTCaer joycon toolkit and dekuNukem reverse
// engineering https://github.com/nicman23/dkms-hid-nintendo/blob/master/src/hid-nintendo.c
// https://github.com/CTCaer/jc_toolkit
// https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering

#pragma once

#include "input_common/helpers/joycon_protocol/common_protocol.h"
#include "input_common/helpers/joycon_protocol/joycon_types.h"

namespace InputCommon::Joycon {

/// Joycon driver functions that easily implemented
class GenericProtocol final : private JoyconCommonProtocol {
public:
    explicit GenericProtocol(std::shared_ptr<JoyconHandle> handle);

    /// Enables passive mode. This mode only sends button data on change. Sticks will return digital
    /// data instead of analog. Motion will be disabled
    DriverResult EnablePassiveMode();

    /// Enables active mode. This mode will return the current status every 5-15ms
    DriverResult EnableActiveMode();

    /// Enables or disables the low power mode
    DriverResult SetLowPowerMode(bool enable);

    /// Unknown function used by the switch
    DriverResult TriggersElapsed();

    /**
     * Sends a request to obtain the joycon firmware and mac from handle
     * @returns controller device info
     */
    DriverResult GetDeviceInfo(DeviceInfo& controller_type);

    /**
     * Sends a request to obtain the joycon type from handle
     * @returns controller type of the joycon
     */
    DriverResult GetControllerType(ControllerType& controller_type);

    /**
     * Enables motion input
     * @param enable if true motion data will be enabled
     */
    DriverResult EnableImu(bool enable);

    /**
     * Configures the motion sensor with the specified parameters
     * @param gsen gyroscope sensor sensitvity in degrees per second
     * @param gfrec gyroscope sensor frequency in hertz
     * @param asen accelerometer sensitivity in G force
     * @param afrec accelerometer frequency in hertz
     */
    DriverResult SetImuConfig(GyroSensitivity gsen, GyroPerformance gfrec,
                              AccelerometerSensitivity asen, AccelerometerPerformance afrec);

    /**
     * Request battery level from the device
     * @returns battery level
     */
    DriverResult GetBattery(u32& battery_level);

    /**
     * Request joycon colors from the device
     * @returns colors of the body and buttons
     */
    DriverResult GetColor(Color& color);

    /**
     * Request joycon serial number from the device
     * @returns 16 byte serial number
     */
    DriverResult GetSerialNumber(SerialNumber& serial_number);

    /**
     * Request joycon serial number from the device
     * @returns 16 byte serial number
     */
    DriverResult GetTemperature(u32& temperature);

    /**
     * Request joycon serial number from the device
     * @returns 16 byte serial number
     */
    DriverResult GetVersionNumber(FirmwareVersion& version);

    /**
     * Sets home led behaviour
     */
    DriverResult SetHomeLight();

    /**
     * Sets home led into a slow breathing state
     */
    DriverResult SetLedBusy();

    /**
     * Sets the 4 player leds on the joycon on a solid state
     * @params bit flag containing the led state
     */
    DriverResult SetLedPattern(u8 leds);

    /**
     * Sets the 4 player leds on the joycon on a blinking state
     * @returns bit flag containing the led state
     */
    DriverResult SetLedBlinkPattern(u8 leds);
};
} // namespace InputCommon::Joycon

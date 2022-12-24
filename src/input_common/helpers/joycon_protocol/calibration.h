// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Based on dkms-hid-nintendo implementation, CTCaer joycon toolkit and dekuNukem reverse
// engineering https://github.com/nicman23/dkms-hid-nintendo/blob/master/src/hid-nintendo.c
// https://github.com/CTCaer/jc_toolkit
// https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering

#pragma once

#include <vector>

#include "input_common/helpers/joycon_protocol/common_protocol.h"

namespace InputCommon::Joycon {
enum class DriverResult;
struct JoyStickCalibration;
struct IMUCalibration;
struct JoyconHandle;
} // namespace InputCommon::Joycon

namespace InputCommon::Joycon {

/// Driver functions related to retrieving calibration data from the device
class CalibrationProtocol final : private JoyconCommonProtocol {
public:
    explicit CalibrationProtocol(std::shared_ptr<JoyconHandle> handle);

    /**
     * Sends a request to obtain the left stick calibration from memory
     * @param is_factory_calibration if true factory values will be returned
     * @returns JoyStickCalibration of the left joystick
     */
    DriverResult GetLeftJoyStickCalibration(JoyStickCalibration& calibration);

    /**
     * Sends a request to obtain the right stick calibration from memory
     * @param is_factory_calibration if true factory values will be returned
     * @returns JoyStickCalibration of the right joystick
     */
    DriverResult GetRightJoyStickCalibration(JoyStickCalibration& calibration);

    /**
     * Sends a request to obtain the motion calibration from memory
     * @returns ImuCalibration of the motion sensor
     */
    DriverResult GetImuCalibration(MotionCalibration& calibration);

    /**
     * Calculates on run time the proper calibration of the ring controller
     * @returns RingCalibration of the ring sensor
     */
    DriverResult GetRingCalibration(RingCalibration& calibration, s16 current_value);

private:
    void ValidateCalibration(JoyStickCalibration& calibration);
    void ValidateCalibration(MotionCalibration& calibration);

    s16 ring_data_max = 0;
    s16 ring_data_default = 0;
    s16 ring_data_min = 0;
};

} // namespace InputCommon::Joycon

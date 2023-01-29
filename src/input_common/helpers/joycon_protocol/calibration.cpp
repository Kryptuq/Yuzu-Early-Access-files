// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

#include "input_common/helpers/joycon_protocol/calibration.h"
#include "input_common/helpers/joycon_protocol/joycon_types.h"

namespace InputCommon::Joycon {

CalibrationProtocol::CalibrationProtocol(std::shared_ptr<JoyconHandle> handle)
    : JoyconCommonProtocol(std::move(handle)) {}

DriverResult CalibrationProtocol::GetLeftJoyStickCalibration(JoyStickCalibration& calibration) {
    ScopedSetBlocking sb(this);
    std::vector<u8> buffer;
    DriverResult result{DriverResult::Success};
    calibration = {};

    result = ReadSPI(CalAddr::USER_LEFT_MAGIC, sizeof(u16), buffer);

    if (result == DriverResult::Success) {
        const bool has_user_calibration = buffer[0] == 0xB2 && buffer[1] == 0xA1;
        if (has_user_calibration) {
            result = ReadSPI(CalAddr::USER_LEFT_DATA, 9, buffer);
        } else {
            result = ReadSPI(CalAddr::FACT_LEFT_DATA, 9, buffer);
        }
    }

    if (result == DriverResult::Success) {
        calibration.x.max = static_cast<u16>(((buffer[1] & 0x0F) << 8) | buffer[0]);
        calibration.y.max = static_cast<u16>((buffer[2] << 4) | (buffer[1] >> 4));
        calibration.x.center = static_cast<u16>(((buffer[4] & 0x0F) << 8) | buffer[3]);
        calibration.y.center = static_cast<u16>((buffer[5] << 4) | (buffer[4] >> 4));
        calibration.x.min = static_cast<u16>(((buffer[7] & 0x0F) << 8) | buffer[6]);
        calibration.y.min = static_cast<u16>((buffer[8] << 4) | (buffer[7] >> 4));
    }

    // Nintendo fix for drifting stick
    // result = ReadSPI(0x60, 0x86 ,buffer, 16);
    // calibration.deadzone = (u16)((buffer[4] << 8) & 0xF00 | buffer[3]);

    // Set a valid default calibration if data is missing
    ValidateCalibration(calibration);

    return result;
}

DriverResult CalibrationProtocol::GetRightJoyStickCalibration(JoyStickCalibration& calibration) {
    ScopedSetBlocking sb(this);
    std::vector<u8> buffer;
    DriverResult result{DriverResult::Success};
    calibration = {};

    result = ReadSPI(CalAddr::USER_RIGHT_MAGIC, sizeof(u16), buffer);

    if (result == DriverResult::Success) {
        const bool has_user_calibration = buffer[0] == 0xB2 && buffer[1] == 0xA1;
        if (has_user_calibration) {
            result = ReadSPI(CalAddr::USER_RIGHT_DATA, 9, buffer);
        } else {
            result = ReadSPI(CalAddr::FACT_RIGHT_DATA, 9, buffer);
        }
    }

    if (result == DriverResult::Success) {
        calibration.x.center = static_cast<u16>(((buffer[1] & 0x0F) << 8) | buffer[0]);
        calibration.y.center = static_cast<u16>((buffer[2] << 4) | (buffer[1] >> 4));
        calibration.x.min = static_cast<u16>(((buffer[4] & 0x0F) << 8) | buffer[3]);
        calibration.y.min = static_cast<u16>((buffer[5] << 4) | (buffer[4] >> 4));
        calibration.x.max = static_cast<u16>(((buffer[7] & 0x0F) << 8) | buffer[6]);
        calibration.y.max = static_cast<u16>((buffer[8] << 4) | (buffer[7] >> 4));
    }

    // Nintendo fix for drifting stick
    // buffer = ReadSPI(0x60, 0x98 , 16);
    // joystick.deadzone = (u16)((buffer[4] << 8) & 0xF00 | buffer[3]);

    // Set a valid default calibration if data is missing
    ValidateCalibration(calibration);

    return result;
}

DriverResult CalibrationProtocol::GetImuCalibration(MotionCalibration& calibration) {
    ScopedSetBlocking sb(this);
    std::vector<u8> buffer;
    DriverResult result{DriverResult::Success};
    calibration = {};

    result = ReadSPI(CalAddr::USER_IMU_MAGIC, sizeof(u16), buffer);

    if (result == DriverResult::Success) {
        const bool has_user_calibration = buffer[0] == 0xB2 && buffer[1] == 0xA1;
        if (has_user_calibration) {
            result = ReadSPI(CalAddr::USER_IMU_DATA, sizeof(IMUCalibration), buffer);
        } else {
            result = ReadSPI(CalAddr::FACT_IMU_DATA, sizeof(IMUCalibration), buffer);
        }
    }

    if (result == DriverResult::Success) {
        IMUCalibration device_calibration{};
        memcpy(&device_calibration, buffer.data(), sizeof(IMUCalibration));
        calibration.accelerometer[0].offset = device_calibration.accelerometer_offset[0];
        calibration.accelerometer[1].offset = device_calibration.accelerometer_offset[1];
        calibration.accelerometer[2].offset = device_calibration.accelerometer_offset[2];

        calibration.accelerometer[0].scale = device_calibration.accelerometer_scale[0];
        calibration.accelerometer[1].scale = device_calibration.accelerometer_scale[1];
        calibration.accelerometer[2].scale = device_calibration.accelerometer_scale[2];

        calibration.gyro[0].offset = device_calibration.gyroscope_offset[0];
        calibration.gyro[1].offset = device_calibration.gyroscope_offset[1];
        calibration.gyro[2].offset = device_calibration.gyroscope_offset[2];

        calibration.gyro[0].scale = device_calibration.gyroscope_scale[0];
        calibration.gyro[1].scale = device_calibration.gyroscope_scale[1];
        calibration.gyro[2].scale = device_calibration.gyroscope_scale[2];
    }

    ValidateCalibration(calibration);

    return result;
}

DriverResult CalibrationProtocol::GetRingCalibration(RingCalibration& calibration,
                                                     s16 current_value) {
    // TODO: Get default calibration form ring itself
    if (ring_data_max == 0 && ring_data_min == 0) {
        ring_data_max = current_value + 800;
        ring_data_min = current_value - 800;
        ring_data_default = current_value;
    }
    ring_data_max = std::max(ring_data_max, current_value);
    ring_data_min = std::min(ring_data_min, current_value);
    calibration = {
        .default_value = ring_data_default,
        .max_value = ring_data_max,
        .min_value = ring_data_min,
    };
    return DriverResult::Success;
}

void CalibrationProtocol::ValidateCalibration(JoyStickCalibration& calibration) {
    constexpr u16 DefaultStickCenter{2048};
    constexpr u16 DefaultStickRange{1740};

    if (calibration.x.center == 0xFFF || calibration.x.center == 0) {
        calibration.x.center = DefaultStickCenter;
    }
    if (calibration.x.max == 0xFFF || calibration.x.max == 0) {
        calibration.x.max = DefaultStickRange;
    }
    if (calibration.x.min == 0xFFF || calibration.x.min == 0) {
        calibration.x.min = DefaultStickRange;
    }

    if (calibration.y.center == 0xFFF || calibration.y.center == 0) {
        calibration.y.center = DefaultStickCenter;
    }
    if (calibration.y.max == 0xFFF || calibration.y.max == 0) {
        calibration.y.max = DefaultStickRange;
    }
    if (calibration.y.min == 0xFFF || calibration.y.min == 0) {
        calibration.y.min = DefaultStickRange;
    }
}

void CalibrationProtocol::ValidateCalibration(MotionCalibration& calibration) {
    for (auto& sensor : calibration.accelerometer) {
        if (sensor.scale == 0) {
            sensor.scale = 0x4000;
        }
    }
    for (auto& sensor : calibration.gyro) {
        if (sensor.scale == 0) {
            sensor.scale = 0x3be7;
        }
    }
}

} // namespace InputCommon::Joycon

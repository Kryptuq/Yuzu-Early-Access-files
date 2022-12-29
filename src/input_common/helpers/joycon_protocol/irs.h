// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Based on dkms-hid-nintendo implementation, CTCaer joycon toolkit and dekuNukem reverse
// engineering https://github.com/nicman23/dkms-hid-nintendo/blob/master/src/hid-nintendo.c
// https://github.com/CTCaer/jc_toolkit
// https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering

#pragma once

#include <vector>

#include "input_common/helpers/joycon_protocol/common_protocol.h"
#include "input_common/helpers/joycon_protocol/joycon_types.h"

namespace InputCommon::Joycon {

class IrsProtocol final : private JoyconCommonProtocol {
public:
    explicit IrsProtocol(std::shared_ptr<JoyconHandle> handle);

    DriverResult EnableIrs();

    DriverResult DisableIrs();

    bool IsEnabled() const;

private:
    DriverResult ConfigureIrs();

    DriverResult WriteRegistersStep1();
    DriverResult WriteRegistersStep2();

    bool is_enabled{};

    u8 resolution = 0x69;
    u8 leds = 0x00;
    u8 ex_light_filter = 0x03;
    u8 image_flip = 0x00;
    u8 digital_gain = 0x01;
    u16 exposure = 0x2490;
    u16 led_intensity = 0x0f10;
    u32 denoise = 0x012344;
};

} // namespace InputCommon::Joycon

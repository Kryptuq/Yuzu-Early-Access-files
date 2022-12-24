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

class RumbleProtocol final : private JoyconCommonProtocol {
public:
    explicit RumbleProtocol(std::shared_ptr<JoyconHandle> handle);

    DriverResult EnableRumble(bool is_enabled);

    DriverResult SendVibration(const VibrationValue& vibration);

private:
    u16 EncodeHighFrequency(f32 frequency) const;
    u8 EncodeLowFrequency(f32 frequency) const;
    u8 EncodeHighAmplitude(f32 amplitude) const;
    u16 EncodeLowAmplitude(f32 amplitude) const;
};

} // namespace InputCommon::Joycon

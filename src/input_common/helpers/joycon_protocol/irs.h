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

    DriverResult SetIrsConfig(IrsMode mode, IrsResolution format);

    DriverResult RequestImage(std::span<u8> buffer);

    std::vector<u8> GetImage() const;

    IrsResolution GetIrsFormat() const;

    bool IsEnabled() const;

private:
    DriverResult ConfigureIrs();

    DriverResult WriteRegistersStep1();
    DriverResult WriteRegistersStep2();

    DriverResult RequestFrame(u8 frame);
    DriverResult ResendFrame(u8 frame);

    IrsMode irs_mode{IrsMode::ImageTransfer};
    IrsResolution resolution{IrsResolution::Size40x30};
    IrsResolutionCode resolution_code{IrsResolutionCode::Size40x30};
    IrsFragments fragments{IrsFragments::Size40x30};
    IrLeds leds{IrLeds::BrightAndDim};
    IrExLedFilter led_filter{IrExLedFilter::Enabled};
    IrImageFlip image_flip{IrImageFlip::Normal};
    u8 digital_gain{0x01};
    u16 exposure{0x2490};
    u16 led_intensity{0x0f10};
    u32 denoise{0x012344};

    u8 packet_fragment{};
    std::vector<u8> buf_image; // 8bpp greyscale image.

    bool is_enabled{};
};

} // namespace InputCommon::Joycon

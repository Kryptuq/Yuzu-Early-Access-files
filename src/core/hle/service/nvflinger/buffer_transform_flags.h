// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021 yuzu Emulator Project

#pragma once

#include "common/common_types.h"

namespace Service::android {

enum class BufferTransformFlags : u32 {
    /// No transform flags are set
    Unset = 0x00,
    /// Flip source image horizontally (around the vertical axis)
    FlipH = 0x01,
    /// Flip source image vertically (around the horizontal axis)
    FlipV = 0x02,
    /// Rotate source image 90 degrees clockwise
    Rotate90 = 0x04,
    /// Rotate source image 180 degrees
    Rotate180 = 0x03,
    /// Rotate source image 270 degrees clockwise
    Rotate270 = 0x07,
};

} // namespace Service::android

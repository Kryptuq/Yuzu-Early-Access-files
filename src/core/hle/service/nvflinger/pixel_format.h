// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021 yuzu Emulator Project

#pragma once

#include "common/common_types.h"

namespace Service::android {

enum class PixelFormat : u32 {
    NoFormat = 0,
    Rgba8888 = 1,
    Rgbx8888 = 2,
    Rgb888 = 3,
    Rgb565 = 4,
    Bgra8888 = 5,
    Rgba5551 = 6,
    Rgba4444 = 7,
};

} // namespace Service::android

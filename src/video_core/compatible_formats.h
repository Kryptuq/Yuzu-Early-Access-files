// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "video_core/surface.h"

namespace VideoCore::Surface {

bool IsViewCompatible(PixelFormat format_a, PixelFormat format_b, bool broken_views,
                      bool native_bgr);

bool IsCopyCompatible(PixelFormat format_a, PixelFormat format_b, bool native_bgr);

} // namespace VideoCore::Surface

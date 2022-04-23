// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "video_core/surface.h"

namespace VideoCore::Surface {

bool IsViewCompatible(PixelFormat format_a, PixelFormat format_b, bool broken_views,
                      bool native_bgr);

bool IsCopyCompatible(PixelFormat format_a, PixelFormat format_b, bool native_bgr);

} // namespace VideoCore::Surface

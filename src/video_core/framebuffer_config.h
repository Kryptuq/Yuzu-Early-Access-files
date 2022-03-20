// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "common/math_util.h"
#include "core/hle/service/nvflinger/buffer_transform_flags.h"
#include "core/hle/service/nvflinger/pixel_format.h"

namespace Tegra {

/**
 * Struct describing framebuffer configuration
 */
struct FramebufferConfig {
    VAddr address{};
    u32 offset{};
    u32 width{};
    u32 height{};
    u32 stride{};
    Service::android::PixelFormat pixel_format{};
    Service::android::BufferTransformFlags transform_flags{};
    Common::Rectangle<int> crop_rect;
};

} // namespace Tegra

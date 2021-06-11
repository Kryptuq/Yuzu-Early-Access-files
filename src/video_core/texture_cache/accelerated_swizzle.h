// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>

#include "common/common_types.h"
#include "video_core/texture_cache/image_info.h"
#include "video_core/texture_cache/types.h"

namespace VideoCommon::Accelerated {

struct BlockLinearSwizzle2DParams {
    alignas(16) std::array<u32, 3> origin;
    alignas(16) std::array<s32, 3> destination;
    u32 bytes_per_block_log2;
    u32 layer_stride;
    u32 block_size;
    u32 x_shift;
    u32 block_height;
    u32 block_height_mask;
};

struct BlockLinearSwizzle3DParams {
    std::array<u32, 3> origin;
    std::array<s32, 3> destination;
    u32 bytes_per_block_log2;
    u32 slice_size;
    u32 block_size;
    u32 x_shift;
    u32 block_height;
    u32 block_height_mask;
    u32 block_depth;
    u32 block_depth_mask;
};

[[nodiscard]] BlockLinearSwizzle2DParams MakeBlockLinearSwizzle2DParams(
    const SwizzleParameters& swizzle, const ImageInfo& info);

[[nodiscard]] BlockLinearSwizzle3DParams MakeBlockLinearSwizzle3DParams(
    const SwizzleParameters& swizzle, const ImageInfo& info);

} // namespace VideoCommon::Accelerated

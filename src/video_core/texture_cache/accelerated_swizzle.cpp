// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <bit>

#include "common/alignment.h"
#include "common/common_types.h"
#include "common/div_ceil.h"
#include "video_core/surface.h"
#include "video_core/texture_cache/accelerated_swizzle.h"
#include "video_core/texture_cache/util.h"
#include "video_core/textures/decoders.h"

namespace VideoCommon::Accelerated {

using Tegra::Texture::GOB_SIZE_SHIFT;
using Tegra::Texture::GOB_SIZE_X;
using Tegra::Texture::GOB_SIZE_X_SHIFT;
using Tegra::Texture::GOB_SIZE_Y_SHIFT;
using VideoCore::Surface::BytesPerBlock;

BlockLinearSwizzle2DParams MakeBlockLinearSwizzle2DParams(const SwizzleParameters& swizzle,
                                                          const ImageInfo& info) {
    const Extent3D block = swizzle.block;
    const Extent3D num_tiles = swizzle.num_tiles;
    const u32 bytes_per_block = BytesPerBlock(info.format);
    const u32 stride_alignment = CalculateLevelStrideAlignment(info, swizzle.level);
    const u32 stride = Common::AlignUpLog2(num_tiles.width, stride_alignment) * bytes_per_block;
    const u32 gobs_in_x = Common::DivCeilLog2(stride, GOB_SIZE_X_SHIFT);
    return BlockLinearSwizzle2DParams{
        .origin{0, 0, 0},
        .destination{0, 0, 0},
        .bytes_per_block_log2 = static_cast<u32>(std::countr_zero(bytes_per_block)),
        .layer_stride = info.layer_stride,
        .block_size = gobs_in_x << (GOB_SIZE_SHIFT + block.height + block.depth),
        .x_shift = GOB_SIZE_SHIFT + block.height + block.depth,
        .block_height = block.height,
        .block_height_mask = (1U << block.height) - 1,
    };
}

BlockLinearSwizzle3DParams MakeBlockLinearSwizzle3DParams(const SwizzleParameters& swizzle,
                                                          const ImageInfo& info) {
    const Extent3D block = swizzle.block;
    const Extent3D num_tiles = swizzle.num_tiles;
    const u32 bytes_per_block = BytesPerBlock(info.format);
    const u32 stride_alignment = CalculateLevelStrideAlignment(info, swizzle.level);
    const u32 stride = Common::AlignUpLog2(num_tiles.width, stride_alignment) * bytes_per_block;

    const u32 gobs_in_x = (stride + GOB_SIZE_X - 1) >> GOB_SIZE_X_SHIFT;
    const u32 block_size = gobs_in_x << (GOB_SIZE_SHIFT + block.height + block.depth);
    const u32 slice_size =
        Common::DivCeilLog2(num_tiles.height, block.height + GOB_SIZE_Y_SHIFT) * block_size;
    return BlockLinearSwizzle3DParams{
        .origin{0, 0, 0},
        .destination{0, 0, 0},
        .bytes_per_block_log2 = static_cast<u32>(std::countr_zero(bytes_per_block)),
        .slice_size = slice_size,
        .block_size = block_size,
        .x_shift = GOB_SIZE_SHIFT + block.height + block.depth,
        .block_height = block.height,
        .block_height_mask = (1U << block.height) - 1,
        .block_depth = block.depth,
        .block_depth_mask = (1U << block.depth) - 1,
    };
}

} // namespace VideoCommon::Accelerated
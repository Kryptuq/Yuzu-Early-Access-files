// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <cmath>
#include <cstring>
#include <span>
#include <utility>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/bit_util.h"
#include "common/div_ceil.h"
#include "video_core/gpu.h"
#include "video_core/textures/decoders.h"
#include "video_core/textures/texture.h"

namespace Tegra::Texture {
namespace {
template <bool TO_LINEAR>
void Swizzle(std::span<u8> output, std::span<const u8> input, u32 bytes_per_pixel, u32 width,
             u32 height, u32 depth, u32 block_height, u32 block_depth, u32 stride_alignment) {
    // The origin of the transformation can be configured here, leave it as zero as the current API
    // doesn't expose it.
    static constexpr u32 origin_x = 0;
    static constexpr u32 origin_y = 0;
    static constexpr u32 origin_z = 0;

    // We can configure here a custom pitch
    // As it's not exposed 'width * bpp' will be the expected pitch.
    const u32 pitch = width * bytes_per_pixel;
    const u32 stride = Common::AlignUpLog2(width, stride_alignment) * bytes_per_pixel;

    const u32 gobs_in_x = Common::DivCeilLog2(stride, GOB_SIZE_X_SHIFT);
    const u32 block_size = gobs_in_x << (GOB_SIZE_SHIFT + block_height + block_depth);
    const u32 slice_size =
        Common::DivCeilLog2(height, block_height + GOB_SIZE_Y_SHIFT) * block_size;

    const u32 block_height_mask = (1U << block_height) - 1;
    const u32 block_depth_mask = (1U << block_depth) - 1;
    const u32 x_shift = GOB_SIZE_SHIFT + block_height + block_depth;

    for (u32 slice = 0; slice < depth; ++slice) {
        const u32 z = slice + origin_z;
        const u32 offset_z = (z >> block_depth) * slice_size +
                             ((z & block_depth_mask) << (GOB_SIZE_SHIFT + block_height));
        for (u32 line = 0; line < height; ++line) {
            const u32 y = line + origin_y;
            const auto& table = SWIZZLE_TABLE[y % GOB_SIZE_Y];

            const u32 block_y = y >> GOB_SIZE_Y_SHIFT;
            const u32 offset_y = (block_y >> block_height) * block_size +
                                 ((block_y & block_height_mask) << GOB_SIZE_SHIFT);

            for (u32 column = 0; column < width; ++column) {
                const u32 x = (column + origin_x) * bytes_per_pixel;
                const u32 offset_x = (x >> GOB_SIZE_X_SHIFT) << x_shift;

                const u32 base_swizzled_offset = offset_z + offset_y + offset_x;
                const u32 swizzled_offset = base_swizzled_offset + table[x % GOB_SIZE_X];

                const u32 unswizzled_offset =
                    slice * pitch * height + line * pitch + column * bytes_per_pixel;

                if (const auto offset = (TO_LINEAR ? unswizzled_offset : swizzled_offset);
                    offset >= input.size()) {
                    // TODO(Rodrigo): This is an out of bounds access that should never happen. To
                    // avoid crashing the emulator, break.
                    ASSERT_MSG(false, "offset {} exceeds input size {}!", offset, input.size());
                    break;
                }

                u8* const dst = &output[TO_LINEAR ? swizzled_offset : unswizzled_offset];
                const u8* const src = &input[TO_LINEAR ? unswizzled_offset : swizzled_offset];
                std::memcpy(dst, src, bytes_per_pixel);
            }
        }
    }
}
} // Anonymous namespace

void UnswizzleTexture(std::span<u8> output, std::span<const u8> input, u32 bytes_per_pixel,
                      u32 width, u32 height, u32 depth, u32 block_height, u32 block_depth,
                      u32 stride_alignment) {
    Swizzle<false>(output, input, bytes_per_pixel, width, height, depth, block_height, block_depth,
                   stride_alignment);
}

void SwizzleTexture(std::span<u8> output, std::span<const u8> input, u32 bytes_per_pixel, u32 width,
                    u32 height, u32 depth, u32 block_height, u32 block_depth,
                    u32 stride_alignment) {
    Swizzle<true>(output, input, bytes_per_pixel, width, height, depth, block_height, block_depth,
                  stride_alignment);
}

void SwizzleSubrect(u32 subrect_width, u32 subrect_height, u32 source_pitch, u32 swizzled_width,
                    u32 bytes_per_pixel, u8* swizzled_data, const u8* unswizzled_data,
                    u32 block_height_bit, u32 offset_x, u32 offset_y) {
    const u32 block_height = 1U << block_height_bit;
    const u32 image_width_in_gobs =
        (swizzled_width * bytes_per_pixel + (GOB_SIZE_X - 1)) / GOB_SIZE_X;
    for (u32 line = 0; line < subrect_height; ++line) {
        const u32 dst_y = line + offset_y;
        const u32 gob_address_y =
            (dst_y / (GOB_SIZE_Y * block_height)) * GOB_SIZE * block_height * image_width_in_gobs +
            ((dst_y % (GOB_SIZE_Y * block_height)) / GOB_SIZE_Y) * GOB_SIZE;
        const auto& table = SWIZZLE_TABLE[dst_y % GOB_SIZE_Y];
        for (u32 x = 0; x < subrect_width; ++x) {
            const u32 dst_x = x + offset_x;
            const u32 gob_address =
                gob_address_y + (dst_x * bytes_per_pixel / GOB_SIZE_X) * GOB_SIZE * block_height;
            const u32 swizzled_offset = gob_address + table[(dst_x * bytes_per_pixel) % GOB_SIZE_X];
            const u32 unswizzled_offset = line * source_pitch + x * bytes_per_pixel;

            const u8* const source_line = unswizzled_data + unswizzled_offset;
            u8* const dest_addr = swizzled_data + swizzled_offset;
            std::memcpy(dest_addr, source_line, bytes_per_pixel);
        }
    }
}

void UnswizzleSubrect(u32 line_length_in, u32 line_count, u32 pitch, u32 width, u32 bytes_per_pixel,
                      u32 block_height, u32 origin_x, u32 origin_y, u8* output, const u8* input) {
    const u32 stride = width * bytes_per_pixel;
    const u32 gobs_in_x = (stride + GOB_SIZE_X - 1) / GOB_SIZE_X;
    const u32 block_size = gobs_in_x << (GOB_SIZE_SHIFT + block_height);

    const u32 block_height_mask = (1U << block_height) - 1;
    const u32 x_shift = GOB_SIZE_SHIFT + block_height;

    for (u32 line = 0; line < line_count; ++line) {
        const u32 src_y = line + origin_y;
        const auto& table = SWIZZLE_TABLE[src_y % GOB_SIZE_Y];

        const u32 block_y = src_y >> GOB_SIZE_Y_SHIFT;
        const u32 src_offset_y = (block_y >> block_height) * block_size +
                                 ((block_y & block_height_mask) << GOB_SIZE_SHIFT);
        for (u32 column = 0; column < line_length_in; ++column) {
            const u32 src_x = (column + origin_x) * bytes_per_pixel;
            const u32 src_offset_x = (src_x >> GOB_SIZE_X_SHIFT) << x_shift;

            const u32 swizzled_offset = src_offset_y + src_offset_x + table[src_x % GOB_SIZE_X];
            const u32 unswizzled_offset = line * pitch + column * bytes_per_pixel;

            std::memcpy(output + unswizzled_offset, input + swizzled_offset, bytes_per_pixel);
        }
    }
}

void SwizzleSliceToVoxel(u32 line_length_in, u32 line_count, u32 pitch, u32 width, u32 height,
                         u32 bytes_per_pixel, u32 block_height, u32 block_depth, u32 origin_x,
                         u32 origin_y, u8* output, const u8* input) {
    UNIMPLEMENTED_IF(origin_x > 0);
    UNIMPLEMENTED_IF(origin_y > 0);

    const u32 stride = width * bytes_per_pixel;
    const u32 gobs_in_x = (stride + GOB_SIZE_X - 1) / GOB_SIZE_X;
    const u32 block_size = gobs_in_x << (GOB_SIZE_SHIFT + block_height + block_depth);

    const u32 block_height_mask = (1U << block_height) - 1;
    const u32 x_shift = static_cast<u32>(GOB_SIZE_SHIFT) + block_height + block_depth;

    for (u32 line = 0; line < line_count; ++line) {
        const auto& table = SWIZZLE_TABLE[line % GOB_SIZE_Y];
        const u32 block_y = line / GOB_SIZE_Y;
        const u32 dst_offset_y =
            (block_y >> block_height) * block_size + (block_y & block_height_mask) * GOB_SIZE;
        for (u32 x = 0; x < line_length_in; ++x) {
            const u32 dst_offset =
                ((x / GOB_SIZE_X) << x_shift) + dst_offset_y + table[x % GOB_SIZE_X];
            const u32 src_offset = x * bytes_per_pixel + line * pitch;
            std::memcpy(output + dst_offset, input + src_offset, bytes_per_pixel);
        }
    }
}

void SwizzleKepler(const u32 width, const u32 height, const u32 dst_x, const u32 dst_y,
                   const u32 block_height_bit, const std::size_t copy_size, const u8* source_data,
                   u8* swizzle_data) {
    const u32 block_height = 1U << block_height_bit;
    const u32 image_width_in_gobs{(width + GOB_SIZE_X - 1) / GOB_SIZE_X};
    std::size_t count = 0;
    for (std::size_t y = dst_y; y < height && count < copy_size; ++y) {
        const std::size_t gob_address_y =
            (y / (GOB_SIZE_Y * block_height)) * GOB_SIZE * block_height * image_width_in_gobs +
            ((y % (GOB_SIZE_Y * block_height)) / GOB_SIZE_Y) * GOB_SIZE;
        const auto& table = SWIZZLE_TABLE[y % GOB_SIZE_Y];
        for (std::size_t x = dst_x; x < width && count < copy_size; ++x) {
            const std::size_t gob_address =
                gob_address_y + (x / GOB_SIZE_X) * GOB_SIZE * block_height;
            const std::size_t swizzled_offset = gob_address + table[x % GOB_SIZE_X];
            const u8* source_line = source_data + count;
            u8* dest_addr = swizzle_data + swizzled_offset;
            count++;

            std::memcpy(dest_addr, source_line, 1);
        }
    }
}

std::size_t CalculateSize(bool tiled, u32 bytes_per_pixel, u32 width, u32 height, u32 depth,
                          u32 block_height, u32 block_depth) {
    if (tiled) {
        const u32 aligned_width = Common::AlignUpLog2(width * bytes_per_pixel, GOB_SIZE_X_SHIFT);
        const u32 aligned_height = Common::AlignUpLog2(height, GOB_SIZE_Y_SHIFT + block_height);
        const u32 aligned_depth = Common::AlignUpLog2(depth, GOB_SIZE_Z_SHIFT + block_depth);
        return aligned_width * aligned_height * aligned_depth;
    } else {
        return width * height * depth * bytes_per_pixel;
    }
}

u64 GetGOBOffset(u32 width, u32 height, u32 dst_x, u32 dst_y, u32 block_height,
                 u32 bytes_per_pixel) {
    auto div_ceil = [](const u32 x, const u32 y) { return ((x + y - 1) / y); };
    const u32 gobs_in_block = 1 << block_height;
    const u32 y_blocks = GOB_SIZE_Y << block_height;
    const u32 x_per_gob = GOB_SIZE_X / bytes_per_pixel;
    const u32 x_blocks = div_ceil(width, x_per_gob);
    const u32 block_size = GOB_SIZE * gobs_in_block;
    const u32 stride = block_size * x_blocks;
    const u32 base = (dst_y / y_blocks) * stride + (dst_x / x_per_gob) * block_size;
    const u32 relative_y = dst_y % y_blocks;
    return base + (relative_y / GOB_SIZE_Y) * GOB_SIZE;
}

} // namespace Tegra::Texture

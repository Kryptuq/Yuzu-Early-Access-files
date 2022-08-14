// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cmath>
#include <cstring>
#include <span>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/bit_util.h"
#include "common/div_ceil.h"
#include "video_core/gpu.h"
#include "video_core/textures/decoders.h"

namespace Tegra::Texture {
namespace {
template <u32 mask>
constexpr u32 pdep(u32 value) {
    u32 result = 0;
    u32 m = mask;
    for (u32 bit = 1; m; bit += bit) {
        if (value & bit)
            result |= m & -m;
        m &= m - 1;
    }
    return result;
}

template <u32 mask, u32 incr_amount>
void incrpdep(u32& value) {
    constexpr u32 swizzled_incr = pdep<mask>(incr_amount);
    value = ((value | ~mask) + swizzled_incr) & mask;
}

template <bool TO_LINEAR, u32 BYTES_PER_PIXEL>
void SwizzleImpl(std::span<u8> output, std::span<const u8> input, u32 width, u32 height, u32 depth,
                 u32 block_height, u32 block_depth, u32 stride) {
    // The origin of the transformation can be configured here, leave it as zero as the current API
    // doesn't expose it.
    static constexpr u32 origin_x = 0;
    static constexpr u32 origin_y = 0;
    static constexpr u32 origin_z = 0;

    // We can configure here a custom pitch
    // As it's not exposed 'width * BYTES_PER_PIXEL' will be the expected pitch.
    const u32 pitch = width * BYTES_PER_PIXEL;

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
            const u32 swizzled_y = pdep<SWIZZLE_Y_BITS>(y);

            const u32 block_y = y >> GOB_SIZE_Y_SHIFT;
            const u32 offset_y = (block_y >> block_height) * block_size +
                                 ((block_y & block_height_mask) << GOB_SIZE_SHIFT);

            u32 swizzled_x = pdep<SWIZZLE_X_BITS>(origin_x * BYTES_PER_PIXEL);
            for (u32 column = 0; column < width;
                 ++column, incrpdep<SWIZZLE_X_BITS, BYTES_PER_PIXEL>(swizzled_x)) {
                const u32 x = (column + origin_x) * BYTES_PER_PIXEL;
                const u32 offset_x = (x >> GOB_SIZE_X_SHIFT) << x_shift;

                const u32 base_swizzled_offset = offset_z + offset_y + offset_x;
                const u32 swizzled_offset = base_swizzled_offset + (swizzled_x | swizzled_y);

                const u32 unswizzled_offset =
                    slice * pitch * height + line * pitch + column * BYTES_PER_PIXEL;

                u8* const dst = &output[TO_LINEAR ? swizzled_offset : unswizzled_offset];
                const u8* const src = &input[TO_LINEAR ? unswizzled_offset : swizzled_offset];

                std::memcpy(dst, src, BYTES_PER_PIXEL);
            }
        }
    }
}

template <bool TO_LINEAR, u32 BYTES_PER_PIXEL>
void SwizzleSubrectImpl(std::span<u8> output, std::span<const u8> input, u32 width, u32 height,
                        u32 depth, u32 origin_x, u32 origin_y, u32 extent_x, u32 num_lines,
                        u32 block_height, u32 block_depth, u32 pitch_linear) {
    // The origin of the transformation can be configured here, leave it as zero as the current API
    // doesn't expose it.
    static constexpr u32 origin_z = 0;

    // We can configure here a custom pitch
    // As it's not exposed 'width * BYTES_PER_PIXEL' will be the expected pitch.
    const u32 pitch = pitch_linear;
    const u32 stride = Common::AlignUpLog2(width * BYTES_PER_PIXEL, GOB_SIZE_X_SHIFT);

    const u32 gobs_in_x = Common::DivCeilLog2(stride, GOB_SIZE_X_SHIFT);
    const u32 block_size = gobs_in_x << (GOB_SIZE_SHIFT + block_height + block_depth);
    const u32 slice_size =
        Common::DivCeilLog2(height, block_height + GOB_SIZE_Y_SHIFT) * block_size;

    const u32 block_height_mask = (1U << block_height) - 1;
    const u32 block_depth_mask = (1U << block_depth) - 1;
    const u32 x_shift = GOB_SIZE_SHIFT + block_height + block_depth;

    u32 unprocessed_lines = num_lines;
    u32 extent_y = std::min(num_lines, height - origin_y);

    for (u32 slice = 0; slice < depth; ++slice) {
        const u32 z = slice + origin_z;
        const u32 offset_z = (z >> block_depth) * slice_size +
                             ((z & block_depth_mask) << (GOB_SIZE_SHIFT + block_height));
        const u32 lines_in_y = std::min(unprocessed_lines, extent_y);
        for (u32 line = 0; line < lines_in_y; ++line) {
            const u32 y = line + origin_y;
            const u32 swizzled_y = pdep<SWIZZLE_Y_BITS>(y);

            const u32 block_y = y >> GOB_SIZE_Y_SHIFT;
            const u32 offset_y = (block_y >> block_height) * block_size +
                                 ((block_y & block_height_mask) << GOB_SIZE_SHIFT);

            u32 swizzled_x = 0;
            for (u32 column = 0; column < extent_x;
                 ++column, incrpdep<SWIZZLE_X_BITS, BYTES_PER_PIXEL>(swizzled_x)) {
                const u32 x = (column + origin_x) * BYTES_PER_PIXEL;
                const u32 offset_x = (x >> GOB_SIZE_X_SHIFT) << x_shift;

                const u32 base_swizzled_offset = offset_z + offset_y + offset_x;
                const u32 swizzled_offset = base_swizzled_offset + (swizzled_x | swizzled_y);

                const u32 unswizzled_offset =
                    slice * pitch * height + line * pitch + column * BYTES_PER_PIXEL;

                u8* const dst = &output[TO_LINEAR ? swizzled_offset : unswizzled_offset];
                const u8* const src = &input[TO_LINEAR ? unswizzled_offset : swizzled_offset];

                std::memcpy(dst, src, BYTES_PER_PIXEL);
            }
        }
        unprocessed_lines -= lines_in_y;
        if (unprocessed_lines == 0) {
            return;
        }
    }
}

template <bool TO_LINEAR>
void Swizzle(std::span<u8> output, std::span<const u8> input, u32 bytes_per_pixel, u32 width,
             u32 height, u32 depth, u32 block_height, u32 block_depth, u32 stride_alignment) {
    switch (bytes_per_pixel) {
#define BPP_CASE(x)                                                                                \
    case x:                                                                                        \
        return SwizzleImpl<TO_LINEAR, x>(output, input, width, height, depth, block_height,        \
                                         block_depth, stride_alignment);
        BPP_CASE(1)
        BPP_CASE(2)
        BPP_CASE(3)
        BPP_CASE(4)
        BPP_CASE(6)
        BPP_CASE(8)
        BPP_CASE(12)
        BPP_CASE(16)
#undef BPP_CASE
    default:
        ASSERT_MSG(false, "Invalid bytes_per_pixel={}", bytes_per_pixel);
    }
}

template <u32 BYTES_PER_PIXEL>
void SwizzleSubrect(u32 subrect_width, u32 subrect_height, u32 source_pitch, u32 swizzled_width,
                    u8* swizzled_data, const u8* unswizzled_data, u32 block_height_bit,
                    u32 offset_x, u32 offset_y) {
    const u32 block_height = 1U << block_height_bit;
    const u32 image_width_in_gobs =
        (swizzled_width * BYTES_PER_PIXEL + (GOB_SIZE_X - 1)) / GOB_SIZE_X;
    for (u32 line = 0; line < subrect_height; ++line) {
        const u32 dst_y = line + offset_y;
        const u32 gob_address_y =
            (dst_y / (GOB_SIZE_Y * block_height)) * GOB_SIZE * block_height * image_width_in_gobs +
            ((dst_y % (GOB_SIZE_Y * block_height)) / GOB_SIZE_Y) * GOB_SIZE;

        const u32 swizzled_y = pdep<SWIZZLE_Y_BITS>(dst_y);
        u32 swizzled_x = pdep<SWIZZLE_X_BITS>(offset_x * BYTES_PER_PIXEL);
        for (u32 x = 0; x < subrect_width;
             ++x, incrpdep<SWIZZLE_X_BITS, BYTES_PER_PIXEL>(swizzled_x)) {
            const u32 dst_x = x + offset_x;
            const u32 gob_address =
                gob_address_y + (dst_x * BYTES_PER_PIXEL / GOB_SIZE_X) * GOB_SIZE * block_height;
            const u32 swizzled_offset = gob_address + (swizzled_x | swizzled_y);
            const u32 unswizzled_offset = line * source_pitch + x * BYTES_PER_PIXEL;

            const u8* const source_line = unswizzled_data + unswizzled_offset;
            u8* const dest_addr = swizzled_data + swizzled_offset;
            std::memcpy(dest_addr, source_line, BYTES_PER_PIXEL);
        }
    }
}

template <u32 BYTES_PER_PIXEL>
void UnswizzleSubrect(u32 line_length_in, u32 line_count, u32 pitch, u32 width, u32 block_height,
                      u32 origin_x, u32 origin_y, u8* output, const u8* input) {
    const u32 stride = width * BYTES_PER_PIXEL;
    const u32 gobs_in_x = (stride + GOB_SIZE_X - 1) / GOB_SIZE_X;
    const u32 block_size = gobs_in_x << (GOB_SIZE_SHIFT + block_height);

    const u32 block_height_mask = (1U << block_height) - 1;
    const u32 x_shift = GOB_SIZE_SHIFT + block_height;

    for (u32 line = 0; line < line_count; ++line) {
        const u32 src_y = line + origin_y;
        const u32 swizzled_y = pdep<SWIZZLE_Y_BITS>(src_y);

        const u32 block_y = src_y >> GOB_SIZE_Y_SHIFT;
        const u32 src_offset_y = (block_y >> block_height) * block_size +
                                 ((block_y & block_height_mask) << GOB_SIZE_SHIFT);

        u32 swizzled_x = pdep<SWIZZLE_X_BITS>(origin_x * BYTES_PER_PIXEL);
        for (u32 column = 0; column < line_length_in;
             ++column, incrpdep<SWIZZLE_X_BITS, BYTES_PER_PIXEL>(swizzled_x)) {
            const u32 src_x = (column + origin_x) * BYTES_PER_PIXEL;
            const u32 src_offset_x = (src_x >> GOB_SIZE_X_SHIFT) << x_shift;

            const u32 swizzled_offset = src_offset_y + src_offset_x + (swizzled_x | swizzled_y);
            const u32 unswizzled_offset = line * pitch + column * BYTES_PER_PIXEL;

            std::memcpy(output + unswizzled_offset, input + swizzled_offset, BYTES_PER_PIXEL);
        }
    }
}

template <u32 BYTES_PER_PIXEL>
void SwizzleSliceToVoxel(u32 line_length_in, u32 line_count, u32 pitch, u32 width, u32 height,
                         u32 block_height, u32 block_depth, u32 origin_x, u32 origin_y, u8* output,
                         const u8* input) {
    UNIMPLEMENTED_IF(origin_x > 0);
    UNIMPLEMENTED_IF(origin_y > 0);

    const u32 stride = width * BYTES_PER_PIXEL;
    const u32 gobs_in_x = (stride + GOB_SIZE_X - 1) / GOB_SIZE_X;
    const u32 block_size = gobs_in_x << (GOB_SIZE_SHIFT + block_height + block_depth);

    const u32 block_height_mask = (1U << block_height) - 1;
    const u32 x_shift = static_cast<u32>(GOB_SIZE_SHIFT) + block_height + block_depth;

    for (u32 line = 0; line < line_count; ++line) {
        const u32 swizzled_y = pdep<SWIZZLE_Y_BITS>(line);
        const u32 block_y = line / GOB_SIZE_Y;
        const u32 dst_offset_y =
            (block_y >> block_height) * block_size + (block_y & block_height_mask) * GOB_SIZE;

        u32 swizzled_x = 0;
        for (u32 x = 0; x < line_length_in; ++x, incrpdep<SWIZZLE_X_BITS, 1>(swizzled_x)) {
            const u32 dst_offset =
                ((x / GOB_SIZE_X) << x_shift) + dst_offset_y + (swizzled_x | swizzled_y);
            const u32 src_offset = x * BYTES_PER_PIXEL + line * pitch;
            std::memcpy(output + dst_offset, input + src_offset, BYTES_PER_PIXEL);
        }
    }
}

} // Anonymous namespace

void UnswizzleTexture(std::span<u8> output, std::span<const u8> input, u32 bytes_per_pixel,
                      u32 width, u32 height, u32 depth, u32 block_height, u32 block_depth,
                      u32 stride_alignment) {
    const u32 stride = Common::AlignUpLog2(width, stride_alignment) * bytes_per_pixel;
    const u32 new_bpp = std::min(4U, static_cast<u32>(std::countr_zero(width * bytes_per_pixel)));
    width = (width * bytes_per_pixel) >> new_bpp;
    bytes_per_pixel = 1U << new_bpp;
    Swizzle<false>(output, input, bytes_per_pixel, width, height, depth, block_height, block_depth,
                   stride);
}

void SwizzleTexture(std::span<u8> output, std::span<const u8> input, u32 bytes_per_pixel, u32 width,
                    u32 height, u32 depth, u32 block_height, u32 block_depth,
                    u32 stride_alignment) {
    const u32 stride = Common::AlignUpLog2(width, stride_alignment) * bytes_per_pixel;
    const u32 new_bpp = std::min(4U, static_cast<u32>(std::countr_zero(width * bytes_per_pixel)));
    width = (width * bytes_per_pixel) >> new_bpp;
    bytes_per_pixel = 1U << new_bpp;
    Swizzle<true>(output, input, bytes_per_pixel, width, height, depth, block_height, block_depth,
                  stride);
}

void SwizzleSubrect(std::span<u8> output, std::span<const u8> input, u32 bytes_per_pixel, u32 width,
                    u32 height, u32 depth, u32 origin_x, u32 origin_y, u32 extent_x, u32 extent_y,
                    u32 block_height, u32 block_depth, u32 pitch_linear) {
    switch (bytes_per_pixel) {
#define BPP_CASE(x)                                                                                \
    case x:                                                                                        \
        return SwizzleSubrectImpl<true, x>(output, input, width, height, depth, origin_x,          \
                                           origin_y, extent_x, extent_y, block_height,             \
                                           block_depth, pitch_linear);
        BPP_CASE(1)
        BPP_CASE(2)
        BPP_CASE(3)
        BPP_CASE(4)
        BPP_CASE(6)
        BPP_CASE(8)
        BPP_CASE(12)
        BPP_CASE(16)
#undef BPP_CASE
    default:
        ASSERT_MSG(false, "Invalid bytes_per_pixel={}", bytes_per_pixel);
    }
}

void UnswizzleSubrect(std::span<u8> output, std::span<const u8> input, u32 bytes_per_pixel,
                      u32 width, u32 height, u32 depth, u32 origin_x, u32 origin_y, u32 extent_x,
                      u32 extent_y, u32 block_height, u32 block_depth, u32 pitch_linear) {
    switch (bytes_per_pixel) {
#define BPP_CASE(x)                                                                                \
    case x:                                                                                        \
        return SwizzleSubrectImpl<false, x>(output, input, width, height, depth, origin_x,         \
                                            origin_y, extent_x, extent_y, block_height,            \
                                            block_depth, pitch_linear);
        BPP_CASE(1)
        BPP_CASE(2)
        BPP_CASE(3)
        BPP_CASE(4)
        BPP_CASE(6)
        BPP_CASE(8)
        BPP_CASE(12)
        BPP_CASE(16)
#undef BPP_CASE
    default:
        ASSERT_MSG(false, "Invalid bytes_per_pixel={}", bytes_per_pixel);
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

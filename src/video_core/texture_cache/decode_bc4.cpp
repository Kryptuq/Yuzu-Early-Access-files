// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <span>

#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/texture_cache/decode_bc4.h"
#include "video_core/texture_cache/types.h"

namespace VideoCommon {

// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_compression_rgtc.txt
[[nodiscard]] constexpr u32 DecompressBlock(u64 bits, u32 x, u32 y) {
    const u32 code_offset = 16 + 3 * (4 * y + x);
    const u32 code = (bits >> code_offset) & 7;
    const u32 red0 = (bits >> 0) & 0xff;
    const u32 red1 = (bits >> 8) & 0xff;
    if (red0 > red1) {
        switch (code) {
        case 0:
            return red0;
        case 1:
            return red1;
        case 2:
            return (6 * red0 + 1 * red1) / 7;
        case 3:
            return (5 * red0 + 2 * red1) / 7;
        case 4:
            return (4 * red0 + 3 * red1) / 7;
        case 5:
            return (3 * red0 + 4 * red1) / 7;
        case 6:
            return (2 * red0 + 5 * red1) / 7;
        case 7:
            return (1 * red0 + 6 * red1) / 7;
        }
    } else {
        switch (code) {
        case 0:
            return red0;
        case 1:
            return red1;
        case 2:
            return (4 * red0 + 1 * red1) / 5;
        case 3:
            return (3 * red0 + 2 * red1) / 5;
        case 4:
            return (2 * red0 + 3 * red1) / 5;
        case 5:
            return (1 * red0 + 4 * red1) / 5;
        case 6:
            return 0;
        case 7:
            return 0xff;
        }
    }
    return 0;
}

void DecompressBC4(std::span<const u8> input, Extent3D extent, std::span<u8> output) {
    UNIMPLEMENTED_IF_MSG(extent.width % 4 != 0, "Unaligned width={}", extent.width);
    UNIMPLEMENTED_IF_MSG(extent.height % 4 != 0, "Unaligned height={}", extent.height);
    static constexpr u32 BLOCK_SIZE = 4;
    size_t input_offset = 0;
    for (u32 slice = 0; slice < extent.depth; ++slice) {
        for (u32 block_y = 0; block_y < extent.height / 4; ++block_y) {
            for (u32 block_x = 0; block_x < extent.width / 4; ++block_x) {
                u64 bits;
                std::memcpy(&bits, &input[input_offset], sizeof(bits));
                input_offset += sizeof(bits);

                for (u32 y = 0; y < BLOCK_SIZE; ++y) {
                    for (u32 x = 0; x < BLOCK_SIZE; ++x) {
                        const u32 linear_z = slice;
                        const u32 linear_y = block_y * BLOCK_SIZE + y;
                        const u32 linear_x = block_x * BLOCK_SIZE + x;
                        const u32 offset_z = linear_z * extent.width * extent.height;
                        const u32 offset_y = linear_y * extent.width;
                        const u32 offset_x = linear_x;
                        const u32 output_offset = (offset_z + offset_y + offset_x) * 4ULL;
                        const u32 color = DecompressBlock(bits, x, y);
                        output[output_offset + 0] = static_cast<u8>(color);
                        output[output_offset + 1] = 0;
                        output[output_offset + 2] = 0;
                        output[output_offset + 3] = 0xff;
                    }
                }
            }
        }
    }
}

} // namespace VideoCommon

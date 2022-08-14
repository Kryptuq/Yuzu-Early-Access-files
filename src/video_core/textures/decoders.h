// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "common/common_types.h"
#include "video_core/textures/texture.h"

namespace Tegra::Texture {

constexpr u32 GOB_SIZE_X = 64;
constexpr u32 GOB_SIZE_Y = 8;
constexpr u32 GOB_SIZE_Z = 1;
constexpr u32 GOB_SIZE = GOB_SIZE_X * GOB_SIZE_Y * GOB_SIZE_Z;

constexpr u32 GOB_SIZE_X_SHIFT = 6;
constexpr u32 GOB_SIZE_Y_SHIFT = 3;
constexpr u32 GOB_SIZE_Z_SHIFT = 0;
constexpr u32 GOB_SIZE_SHIFT = GOB_SIZE_X_SHIFT + GOB_SIZE_Y_SHIFT + GOB_SIZE_Z_SHIFT;

constexpr u32 SWIZZLE_X_BITS = 0b100101111;
constexpr u32 SWIZZLE_Y_BITS = 0b011010000;

using SwizzleTable = std::array<std::array<u32, GOB_SIZE_X>, GOB_SIZE_Y>;

/**
 * This table represents the internal swizzle of a gob, in format 16 bytes x 2 sector packing.
 * Calculates the offset of an (x, y) position within a swizzled texture.
 * Taken from the Tegra X1 Technical Reference Manual. pages 1187-1188
 */
constexpr SwizzleTable MakeSwizzleTable() {
    SwizzleTable table{};
    for (u32 y = 0; y < table.size(); ++y) {
        for (u32 x = 0; x < table[0].size(); ++x) {
            table[y][x] = ((x % 64) / 32) * 256 + ((y % 8) / 2) * 64 + ((x % 32) / 16) * 32 +
                          (y % 2) * 16 + (x % 16);
        }
    }
    return table;
}

/// Unswizzles a block linear texture into linear memory.
void UnswizzleTexture(std::span<u8> output, std::span<const u8> input, u32 bytes_per_pixel,
                      u32 width, u32 height, u32 depth, u32 block_height, u32 block_depth,
                      u32 stride_alignment = 1);

/// Swizzles linear memory into a block linear texture.
void SwizzleTexture(std::span<u8> output, std::span<const u8> input, u32 bytes_per_pixel, u32 width,
                    u32 height, u32 depth, u32 block_height, u32 block_depth,
                    u32 stride_alignment = 1);

/// This function calculates the correct size of a texture depending if it's tiled or not.
std::size_t CalculateSize(bool tiled, u32 bytes_per_pixel, u32 width, u32 height, u32 depth,
                          u32 block_height, u32 block_depth);

/// Copies an untiled subrectangle into a tiled surface.
void SwizzleSubrect(std::span<u8> output, std::span<const u8> input, u32 bytes_per_pixel, u32 width,
                    u32 height, u32 depth, u32 origin_x, u32 origin_y, u32 extent_x, u32 extent_y,
                    u32 block_height, u32 block_depth, u32 pitch_linear);

/// Copies a tiled subrectangle into a linear surface.
void UnswizzleSubrect(std::span<u8> output, std::span<const u8> input, u32 bytes_per_pixel,
                      u32 width, u32 height, u32 depth, u32 origin_x, u32 origin_y, u32 extent_x,
                      u32 extent_y, u32 block_height, u32 block_depth, u32 pitch_linear);

/// Obtains the offset of the gob for positions 'dst_x' & 'dst_y'
u64 GetGOBOffset(u32 width, u32 height, u32 dst_x, u32 dst_y, u32 block_height,
                 u32 bytes_per_pixel);

} // namespace Tegra::Texture

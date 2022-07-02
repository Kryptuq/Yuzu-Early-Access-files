// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cstddef>

#include "common/common_types.h"
#include "video_core/compatible_formats.h"
#include "video_core/surface.h"

namespace VideoCore::Surface {
namespace {
using Table = std::array<std::array<u64, 2>, MaxPixelFormat>;

// Compatibility table taken from Table 3.X.2 in:
// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_view.txt

constexpr std::array VIEW_CLASS_128_BITS{
    PixelFormat::R32G32B32A32_FLOAT,
    PixelFormat::R32G32B32A32_UINT,
    PixelFormat::R32G32B32A32_SINT,
};

constexpr std::array VIEW_CLASS_96_BITS{
    PixelFormat::R32G32B32_FLOAT,
};
// Missing formats:
// PixelFormat::RGB32UI,
// PixelFormat::RGB32I,

constexpr std::array VIEW_CLASS_64_BITS{
    PixelFormat::R32G32_FLOAT,       PixelFormat::R32G32_UINT,
    PixelFormat::R32G32_SINT,        PixelFormat::R16G16B16A16_FLOAT,
    PixelFormat::R16G16B16A16_UNORM, PixelFormat::R16G16B16A16_SNORM,
    PixelFormat::R16G16B16A16_UINT,  PixelFormat::R16G16B16A16_SINT,
};

// TODO: How should we handle 48 bits?

constexpr std::array VIEW_CLASS_32_BITS{
    PixelFormat::R16G16_FLOAT,      PixelFormat::B10G11R11_FLOAT, PixelFormat::R32_FLOAT,
    PixelFormat::A2B10G10R10_UNORM, PixelFormat::R16G16_UINT,     PixelFormat::R32_UINT,
    PixelFormat::R16G16_SINT,       PixelFormat::R32_SINT,        PixelFormat::A8B8G8R8_UNORM,
    PixelFormat::R16G16_UNORM,      PixelFormat::A8B8G8R8_SNORM,  PixelFormat::R16G16_SNORM,
    PixelFormat::A8B8G8R8_SRGB,     PixelFormat::E5B9G9R9_FLOAT,  PixelFormat::B8G8R8A8_UNORM,
    PixelFormat::B8G8R8A8_SRGB,     PixelFormat::A8B8G8R8_UINT,   PixelFormat::A8B8G8R8_SINT,
    PixelFormat::A2B10G10R10_UINT,
};

constexpr std::array VIEW_CLASS_32_BITS_NO_BGR{
    PixelFormat::R16G16_FLOAT,      PixelFormat::B10G11R11_FLOAT,  PixelFormat::R32_FLOAT,
    PixelFormat::A2B10G10R10_UNORM, PixelFormat::R16G16_UINT,      PixelFormat::R32_UINT,
    PixelFormat::R16G16_SINT,       PixelFormat::R32_SINT,         PixelFormat::A8B8G8R8_UNORM,
    PixelFormat::R16G16_UNORM,      PixelFormat::A8B8G8R8_SNORM,   PixelFormat::R16G16_SNORM,
    PixelFormat::A8B8G8R8_SRGB,     PixelFormat::E5B9G9R9_FLOAT,   PixelFormat::A8B8G8R8_UINT,
    PixelFormat::A8B8G8R8_SINT,     PixelFormat::A2B10G10R10_UINT,
};

// TODO: How should we handle 24 bits?

constexpr std::array VIEW_CLASS_16_BITS{
    PixelFormat::R16_FLOAT,  PixelFormat::R8G8_UINT,  PixelFormat::R16_UINT,
    PixelFormat::R16_SINT,   PixelFormat::R8G8_UNORM, PixelFormat::R16_UNORM,
    PixelFormat::R8G8_SNORM, PixelFormat::R16_SNORM,  PixelFormat::R8G8_SINT,
};

constexpr std::array VIEW_CLASS_8_BITS{
    PixelFormat::R8_UINT,
    PixelFormat::R8_UNORM,
    PixelFormat::R8_SINT,
    PixelFormat::R8_SNORM,
};

constexpr std::array VIEW_CLASS_RGTC1_RED{
    PixelFormat::BC4_UNORM,
    PixelFormat::BC4_SNORM,
};

constexpr std::array VIEW_CLASS_RGTC2_RG{
    PixelFormat::BC5_UNORM,
    PixelFormat::BC5_SNORM,
};

constexpr std::array VIEW_CLASS_BPTC_UNORM{
    PixelFormat::BC7_UNORM,
    PixelFormat::BC7_SRGB,
};

constexpr std::array VIEW_CLASS_BPTC_FLOAT{
    PixelFormat::BC6H_SFLOAT,
    PixelFormat::BC6H_UFLOAT,
};

constexpr std::array VIEW_CLASS_ASTC_4x4_RGBA{
    PixelFormat::ASTC_2D_4X4_UNORM,
    PixelFormat::ASTC_2D_4X4_SRGB,
};

constexpr std::array VIEW_CLASS_ASTC_5x4_RGBA{
    PixelFormat::ASTC_2D_5X4_UNORM,
    PixelFormat::ASTC_2D_5X4_SRGB,
};

constexpr std::array VIEW_CLASS_ASTC_5x5_RGBA{
    PixelFormat::ASTC_2D_5X5_UNORM,
    PixelFormat::ASTC_2D_5X5_SRGB,
};

constexpr std::array VIEW_CLASS_ASTC_6x5_RGBA{
    PixelFormat::ASTC_2D_6X5_UNORM,
    PixelFormat::ASTC_2D_6X5_SRGB,
};

constexpr std::array VIEW_CLASS_ASTC_6x6_RGBA{
    PixelFormat::ASTC_2D_6X6_UNORM,
    PixelFormat::ASTC_2D_6X6_SRGB,
};

constexpr std::array VIEW_CLASS_ASTC_8x5_RGBA{
    PixelFormat::ASTC_2D_8X5_UNORM,
    PixelFormat::ASTC_2D_8X5_SRGB,
};

constexpr std::array VIEW_CLASS_ASTC_8x8_RGBA{
    PixelFormat::ASTC_2D_8X8_UNORM,
    PixelFormat::ASTC_2D_8X8_SRGB,
};

// Missing formats:
// PixelFormat::ASTC_2D_10X5_UNORM
// PixelFormat::ASTC_2D_10X5_SRGB

// Missing formats:
// PixelFormat::ASTC_2D_10X6_SRGB

constexpr std::array VIEW_CLASS_ASTC_10x6_RGBA{
    PixelFormat::ASTC_2D_10X6_UNORM,
};

constexpr std::array VIEW_CLASS_ASTC_10x8_RGBA{
    PixelFormat::ASTC_2D_10X8_UNORM,
    PixelFormat::ASTC_2D_10X8_SRGB,
};

constexpr std::array VIEW_CLASS_ASTC_10x10_RGBA{
    PixelFormat::ASTC_2D_10X10_UNORM,
    PixelFormat::ASTC_2D_10X10_SRGB,
};

// Missing formats
// ASTC_2D_12X10_UNORM,
// ASTC_2D_12X10_SRGB,

constexpr std::array VIEW_CLASS_ASTC_12x12_RGBA{
    PixelFormat::ASTC_2D_12X12_UNORM,
    PixelFormat::ASTC_2D_12X12_SRGB,
};

// Compatibility table taken from Table 4.X.1 in:
// https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_copy_image.txt

constexpr std::array COPY_CLASS_128_BITS{
    PixelFormat::R32G32B32A32_UINT, PixelFormat::R32G32B32A32_FLOAT, PixelFormat::R32G32B32A32_SINT,
    PixelFormat::BC2_UNORM,         PixelFormat::BC2_SRGB,           PixelFormat::BC3_UNORM,
    PixelFormat::BC3_SRGB,          PixelFormat::BC5_UNORM,          PixelFormat::BC5_SNORM,
    PixelFormat::BC7_UNORM,         PixelFormat::BC7_SRGB,           PixelFormat::BC6H_SFLOAT,
    PixelFormat::BC6H_UFLOAT,
};
// Missing formats:
// PixelFormat::RGBA32I
// COMPRESSED_RG_RGTC2

constexpr std::array COPY_CLASS_64_BITS{
    PixelFormat::R16G16B16A16_FLOAT, PixelFormat::R16G16B16A16_UINT,
    PixelFormat::R16G16B16A16_UNORM, PixelFormat::R16G16B16A16_SNORM,
    PixelFormat::R16G16B16A16_SINT,  PixelFormat::R32G32_UINT,
    PixelFormat::R32G32_FLOAT,       PixelFormat::R32G32_SINT,
    PixelFormat::BC1_RGBA_UNORM,     PixelFormat::BC1_RGBA_SRGB,
};
// Missing formats:
// COMPRESSED_RGB_S3TC_DXT1_EXT
// COMPRESSED_SRGB_S3TC_DXT1_EXT
// COMPRESSED_RGBA_S3TC_DXT1_EXT
// COMPRESSED_SIGNED_RED_RGTC1

constexpr void Enable(Table& table, size_t format_a, size_t format_b) {
    table[format_a][format_b / 64] |= u64(1) << (format_b % 64);
    table[format_b][format_a / 64] |= u64(1) << (format_a % 64);
}

constexpr void Enable(Table& table, PixelFormat format_a, PixelFormat format_b) {
    Enable(table, static_cast<size_t>(format_a), static_cast<size_t>(format_b));
}

template <typename Range>
constexpr void EnableRange(Table& table, const Range& range) {
    for (auto it_a = range.begin(); it_a != range.end(); ++it_a) {
        for (auto it_b = it_a; it_b != range.end(); ++it_b) {
            Enable(table, *it_a, *it_b);
        }
    }
}

constexpr bool IsSupported(const Table& table, PixelFormat format_a, PixelFormat format_b) {
    const size_t a = static_cast<size_t>(format_a);
    const size_t b = static_cast<size_t>(format_b);
    return ((table[a][b / 64] >> (b % 64)) & 1) != 0;
}

constexpr Table MakeViewTable() {
    Table view{};
    for (size_t i = 0; i < MaxPixelFormat; ++i) {
        // Identity is allowed
        Enable(view, i, i);
    }
    EnableRange(view, VIEW_CLASS_128_BITS);
    EnableRange(view, VIEW_CLASS_96_BITS);
    EnableRange(view, VIEW_CLASS_64_BITS);
    EnableRange(view, VIEW_CLASS_16_BITS);
    EnableRange(view, VIEW_CLASS_8_BITS);
    EnableRange(view, VIEW_CLASS_RGTC1_RED);
    EnableRange(view, VIEW_CLASS_RGTC2_RG);
    EnableRange(view, VIEW_CLASS_BPTC_UNORM);
    EnableRange(view, VIEW_CLASS_BPTC_FLOAT);
    EnableRange(view, VIEW_CLASS_ASTC_4x4_RGBA);
    EnableRange(view, VIEW_CLASS_ASTC_5x4_RGBA);
    EnableRange(view, VIEW_CLASS_ASTC_5x5_RGBA);
    EnableRange(view, VIEW_CLASS_ASTC_6x5_RGBA);
    EnableRange(view, VIEW_CLASS_ASTC_6x6_RGBA);
    EnableRange(view, VIEW_CLASS_ASTC_8x5_RGBA);
    EnableRange(view, VIEW_CLASS_ASTC_8x8_RGBA);
    EnableRange(view, VIEW_CLASS_ASTC_10x6_RGBA);
    EnableRange(view, VIEW_CLASS_ASTC_10x8_RGBA);
    EnableRange(view, VIEW_CLASS_ASTC_10x10_RGBA);
    EnableRange(view, VIEW_CLASS_ASTC_12x12_RGBA);
    return view;
}

constexpr Table MakeCopyTable() {
    Table copy = MakeViewTable();
    EnableRange(copy, COPY_CLASS_128_BITS);
    EnableRange(copy, COPY_CLASS_64_BITS);
    return copy;
}

constexpr Table MakeNativeBgrViewTable() {
    Table copy = MakeViewTable();
    EnableRange(copy, VIEW_CLASS_32_BITS);
    return copy;
}

constexpr Table MakeNonNativeBgrViewTable() {
    Table copy = MakeViewTable();
    EnableRange(copy, VIEW_CLASS_32_BITS_NO_BGR);
    return copy;
}

constexpr Table MakeNativeBgrCopyTable() {
    Table copy = MakeCopyTable();
    EnableRange(copy, VIEW_CLASS_32_BITS);
    return copy;
}

constexpr Table MakeNonNativeBgrCopyTable() {
    Table copy = MakeCopyTable();
    EnableRange(copy, VIEW_CLASS_32_BITS);
    return copy;
}
} // Anonymous namespace

bool IsViewCompatible(PixelFormat format_a, PixelFormat format_b, bool broken_views,
                      bool native_bgr) {
    if (broken_views) {
        // If format views are broken, only accept formats that are identical.
        return format_a == format_b;
    }
    static constexpr Table BGR_TABLE = MakeNativeBgrViewTable();
    static constexpr Table NO_BGR_TABLE = MakeNonNativeBgrViewTable();
    return IsSupported(native_bgr ? BGR_TABLE : NO_BGR_TABLE, format_a, format_b);
}

bool IsCopyCompatible(PixelFormat format_a, PixelFormat format_b, bool native_bgr) {
    static constexpr Table BGR_TABLE = MakeNativeBgrCopyTable();
    static constexpr Table NO_BGR_TABLE = MakeNonNativeBgrCopyTable();
    return IsSupported(native_bgr ? BGR_TABLE : NO_BGR_TABLE, format_a, format_b);
}

} // namespace VideoCore::Surface

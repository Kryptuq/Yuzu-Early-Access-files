// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/common_types.h"
#include "common/logging/log.h"
#include "video_core/texture_cache/format_lookup_table.h"

namespace VideoCommon {

using Tegra::Texture::ComponentType;
using Tegra::Texture::TextureFormat;
using VideoCore::Surface::PixelFormat;

namespace {

constexpr auto SNORM = ComponentType::SNORM;
constexpr auto UNORM = ComponentType::UNORM;
constexpr auto SINT = ComponentType::SINT;
constexpr auto UINT = ComponentType::UINT;
constexpr auto FLOAT = ComponentType::FLOAT;
constexpr bool LINEAR = false;
constexpr bool SRGB = true;

constexpr u32 Hash(TextureFormat format, ComponentType red_component, ComponentType green_component,
                   ComponentType blue_component, ComponentType alpha_component, bool is_srgb) {
    u32 hash = is_srgb ? 1 : 0;
    hash |= static_cast<u32>(red_component) << 1;
    hash |= static_cast<u32>(green_component) << 4;
    hash |= static_cast<u32>(blue_component) << 7;
    hash |= static_cast<u32>(alpha_component) << 10;
    hash |= static_cast<u32>(format) << 13;
    return hash;
}

constexpr u32 Hash(TextureFormat format, ComponentType component, bool is_srgb = LINEAR) {
    return Hash(format, component, component, component, component, is_srgb);
}

} // Anonymous namespace

PixelFormat PixelFormatFromTextureInfo(TextureFormat format, ComponentType red, ComponentType green,
                                       ComponentType blue, ComponentType alpha,
                                       bool is_srgb) noexcept {
    switch (Hash(format, red, green, blue, alpha, is_srgb)) {
    case Hash(TextureFormat::A8R8G8B8, UNORM):
        return PixelFormat::A8B8G8R8_UNORM;
    case Hash(TextureFormat::A8R8G8B8, SNORM):
        return PixelFormat::A8B8G8R8_SNORM;
    case Hash(TextureFormat::A8R8G8B8, UINT):
        return PixelFormat::A8B8G8R8_UINT;
    case Hash(TextureFormat::A8R8G8B8, SINT):
        return PixelFormat::A8B8G8R8_SINT;
    case Hash(TextureFormat::A8R8G8B8, UNORM, SRGB):
        return PixelFormat::A8B8G8R8_SRGB;
    case Hash(TextureFormat::B5G6R5, UNORM):
        return PixelFormat::B5G6R5_UNORM;
    case Hash(TextureFormat::A2B10G10R10, UNORM):
        return PixelFormat::A2B10G10R10_UNORM;
    case Hash(TextureFormat::A2B10G10R10, UINT):
        return PixelFormat::A2B10G10R10_UINT;
    case Hash(TextureFormat::A1B5G5R5, UNORM):
        return PixelFormat::A1B5G5R5_UNORM;
    case Hash(TextureFormat::A4B4G4R4, UNORM):
        return PixelFormat::A4B4G4R4_UNORM;
    case Hash(TextureFormat::G4R4, UNORM):
        return PixelFormat::G4R4_UNORM;
    case Hash(TextureFormat::A5B5G5R1, UNORM):
        return PixelFormat::A5B5G5R1_UNORM;
    case Hash(TextureFormat::R8, UNORM):
        return PixelFormat::R8_UNORM;
    case Hash(TextureFormat::R8, SNORM):
        return PixelFormat::R8_SNORM;
    case Hash(TextureFormat::R8, UINT):
        return PixelFormat::R8_UINT;
    case Hash(TextureFormat::R8, SINT):
        return PixelFormat::R8_SINT;
    case Hash(TextureFormat::R8G8, UNORM):
        return PixelFormat::R8G8_UNORM;
    case Hash(TextureFormat::R8G8, SNORM):
        return PixelFormat::R8G8_SNORM;
    case Hash(TextureFormat::R8G8, UINT):
        return PixelFormat::R8G8_UINT;
    case Hash(TextureFormat::R8G8, SINT):
        return PixelFormat::R8G8_SINT;
    case Hash(TextureFormat::R16G16B16A16, FLOAT):
        return PixelFormat::R16G16B16A16_FLOAT;
    case Hash(TextureFormat::R16G16B16A16, UNORM):
        return PixelFormat::R16G16B16A16_UNORM;
    case Hash(TextureFormat::R16G16B16A16, SNORM):
        return PixelFormat::R16G16B16A16_SNORM;
    case Hash(TextureFormat::R16G16B16A16, UINT):
        return PixelFormat::R16G16B16A16_UINT;
    case Hash(TextureFormat::R16G16B16A16, SINT):
        return PixelFormat::R16G16B16A16_SINT;
    case Hash(TextureFormat::R16G16, FLOAT):
        return PixelFormat::R16G16_FLOAT;
    case Hash(TextureFormat::R16G16, UNORM):
        return PixelFormat::R16G16_UNORM;
    case Hash(TextureFormat::R16G16, SNORM):
        return PixelFormat::R16G16_SNORM;
    case Hash(TextureFormat::R16G16, UINT):
        return PixelFormat::R16G16_UINT;
    case Hash(TextureFormat::R16G16, SINT):
        return PixelFormat::R16G16_SINT;
    case Hash(TextureFormat::R16, FLOAT):
        return PixelFormat::R16_FLOAT;
    case Hash(TextureFormat::R16, UNORM):
        return PixelFormat::R16_UNORM;
    case Hash(TextureFormat::R16, SNORM):
        return PixelFormat::R16_SNORM;
    case Hash(TextureFormat::R16, UINT):
        return PixelFormat::R16_UINT;
    case Hash(TextureFormat::R16, SINT):
        return PixelFormat::R16_SINT;
    case Hash(TextureFormat::B10G11R11, FLOAT):
        return PixelFormat::B10G11R11_FLOAT;
    case Hash(TextureFormat::R32G32B32A32, FLOAT):
        return PixelFormat::R32G32B32A32_FLOAT;
    case Hash(TextureFormat::R32G32B32A32, UINT):
        return PixelFormat::R32G32B32A32_UINT;
    case Hash(TextureFormat::R32G32B32A32, SINT):
        return PixelFormat::R32G32B32A32_SINT;
    case Hash(TextureFormat::R32G32B32, FLOAT):
        return PixelFormat::R32G32B32_FLOAT;
    case Hash(TextureFormat::R32G32, FLOAT):
        return PixelFormat::R32G32_FLOAT;
    case Hash(TextureFormat::R32G32, UINT):
        return PixelFormat::R32G32_UINT;
    case Hash(TextureFormat::R32G32, SINT):
        return PixelFormat::R32G32_SINT;
    case Hash(TextureFormat::R32, FLOAT):
        return PixelFormat::R32_FLOAT;
    case Hash(TextureFormat::R32, UINT):
        return PixelFormat::R32_UINT;
    case Hash(TextureFormat::R32, SINT):
        return PixelFormat::R32_SINT;
    case Hash(TextureFormat::E5B9G9R9, FLOAT):
        return PixelFormat::E5B9G9R9_FLOAT;
    case Hash(TextureFormat::D32, FLOAT):
        return PixelFormat::D32_FLOAT;
    case Hash(TextureFormat::D16, UNORM):
        return PixelFormat::D16_UNORM;
    case Hash(TextureFormat::S8D24, UINT, UNORM, UNORM, UNORM, LINEAR):
        return PixelFormat::S8_UINT_D24_UNORM;
    case Hash(TextureFormat::S8D24, UINT, UNORM, UINT, UINT, LINEAR):
        return PixelFormat::S8_UINT_D24_UNORM;
    case Hash(TextureFormat::R8G24, UINT, UNORM, UNORM, UNORM, LINEAR):
        return PixelFormat::S8_UINT_D24_UNORM;
    case Hash(TextureFormat::D24S8, UNORM, UINT, UINT, UINT, LINEAR):
        return PixelFormat::D24_UNORM_S8_UINT;
    case Hash(TextureFormat::D32S8, FLOAT, UINT, UNORM, UNORM, LINEAR):
        return PixelFormat::D32_FLOAT_S8_UINT;
    case Hash(TextureFormat::BC1_RGBA, UNORM, LINEAR):
        return PixelFormat::BC1_RGBA_UNORM;
    case Hash(TextureFormat::BC1_RGBA, UNORM, SRGB):
        return PixelFormat::BC1_RGBA_SRGB;
    case Hash(TextureFormat::BC2, UNORM, LINEAR):
        return PixelFormat::BC2_UNORM;
    case Hash(TextureFormat::BC2, UNORM, SRGB):
        return PixelFormat::BC2_SRGB;
    case Hash(TextureFormat::BC3, UNORM, LINEAR):
        return PixelFormat::BC3_UNORM;
    case Hash(TextureFormat::BC3, UNORM, SRGB):
        return PixelFormat::BC3_SRGB;
    case Hash(TextureFormat::BC4, UNORM):
        return PixelFormat::BC4_UNORM;
    case Hash(TextureFormat::BC4, SNORM):
        return PixelFormat::BC4_SNORM;
    case Hash(TextureFormat::BC5, UNORM):
        return PixelFormat::BC5_UNORM;
    case Hash(TextureFormat::BC5, SNORM):
        return PixelFormat::BC5_SNORM;
    case Hash(TextureFormat::BC7, UNORM, LINEAR):
        return PixelFormat::BC7_UNORM;
    case Hash(TextureFormat::BC7, UNORM, SRGB):
        return PixelFormat::BC7_SRGB;
    case Hash(TextureFormat::BC6H_SFLOAT, FLOAT):
        return PixelFormat::BC6H_SFLOAT;
    case Hash(TextureFormat::BC6H_UFLOAT, FLOAT):
        return PixelFormat::BC6H_UFLOAT;
    case Hash(TextureFormat::ASTC_2D_4X4, UNORM, LINEAR):
        return PixelFormat::ASTC_2D_4X4_UNORM;
    case Hash(TextureFormat::ASTC_2D_4X4, UNORM, SRGB):
        return PixelFormat::ASTC_2D_4X4_SRGB;
    case Hash(TextureFormat::ASTC_2D_5X4, UNORM, LINEAR):
        return PixelFormat::ASTC_2D_5X4_UNORM;
    case Hash(TextureFormat::ASTC_2D_5X4, UNORM, SRGB):
        return PixelFormat::ASTC_2D_5X4_SRGB;
    case Hash(TextureFormat::ASTC_2D_5X5, UNORM, LINEAR):
        return PixelFormat::ASTC_2D_5X5_UNORM;
    case Hash(TextureFormat::ASTC_2D_5X5, UNORM, SRGB):
        return PixelFormat::ASTC_2D_5X5_SRGB;
    case Hash(TextureFormat::ASTC_2D_8X8, UNORM, LINEAR):
        return PixelFormat::ASTC_2D_8X8_UNORM;
    case Hash(TextureFormat::ASTC_2D_8X8, UNORM, SRGB):
        return PixelFormat::ASTC_2D_8X8_SRGB;
    case Hash(TextureFormat::ASTC_2D_8X5, UNORM, LINEAR):
        return PixelFormat::ASTC_2D_8X5_UNORM;
    case Hash(TextureFormat::ASTC_2D_8X5, UNORM, SRGB):
        return PixelFormat::ASTC_2D_8X5_SRGB;
    case Hash(TextureFormat::ASTC_2D_10X8, UNORM, LINEAR):
        return PixelFormat::ASTC_2D_10X8_UNORM;
    case Hash(TextureFormat::ASTC_2D_10X8, UNORM, SRGB):
        return PixelFormat::ASTC_2D_10X8_SRGB;
    case Hash(TextureFormat::ASTC_2D_6X6, UNORM, LINEAR):
        return PixelFormat::ASTC_2D_6X6_UNORM;
    case Hash(TextureFormat::ASTC_2D_6X6, UNORM, SRGB):
        return PixelFormat::ASTC_2D_6X6_SRGB;
    case Hash(TextureFormat::ASTC_2D_10X10, UNORM, LINEAR):
        return PixelFormat::ASTC_2D_10X10_UNORM;
    case Hash(TextureFormat::ASTC_2D_10X10, UNORM, SRGB):
        return PixelFormat::ASTC_2D_10X10_SRGB;
    case Hash(TextureFormat::ASTC_2D_12X12, UNORM, LINEAR):
        return PixelFormat::ASTC_2D_12X12_UNORM;
    case Hash(TextureFormat::ASTC_2D_12X12, UNORM, SRGB):
        return PixelFormat::ASTC_2D_12X12_SRGB;
    case Hash(TextureFormat::ASTC_2D_8X6, UNORM, LINEAR):
        return PixelFormat::ASTC_2D_8X6_UNORM;
    case Hash(TextureFormat::ASTC_2D_8X6, UNORM, SRGB):
        return PixelFormat::ASTC_2D_8X6_SRGB;
    case Hash(TextureFormat::ASTC_2D_6X5, UNORM, LINEAR):
        return PixelFormat::ASTC_2D_6X5_UNORM;
    case Hash(TextureFormat::ASTC_2D_6X5, UNORM, SRGB):
        return PixelFormat::ASTC_2D_6X5_SRGB;
    }
    UNIMPLEMENTED_MSG("texture format={} srgb={} components={{{} {} {} {}}}",
                      static_cast<int>(format), is_srgb, static_cast<int>(red),
                      static_cast<int>(green), static_cast<int>(blue), static_cast<int>(alpha));
    return PixelFormat::A8B8G8R8_UNORM;
}

} // namespace VideoCommon

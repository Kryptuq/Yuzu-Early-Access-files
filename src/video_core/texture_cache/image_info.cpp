// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <fmt/format.h>

#include "common/assert.h"
#include "video_core/surface.h"
#include "video_core/texture_cache/format_lookup_table.h"
#include "video_core/texture_cache/image_info.h"
#include "video_core/texture_cache/samples_helper.h"
#include "video_core/texture_cache/types.h"
#include "video_core/texture_cache/util.h"
#include "video_core/textures/texture.h"

namespace VideoCommon {

using Tegra::Engines::Fermi2D;
using Tegra::Engines::Maxwell3D;
using Tegra::Texture::TextureType;
using Tegra::Texture::TICEntry;
using VideoCore::Surface::PixelFormat;
using VideoCore::Surface::SurfaceType;

ImageInfo::ImageInfo(const TICEntry& config) noexcept {
    format = PixelFormatFromTextureInfo(config.format, config.r_type, config.g_type, config.b_type,
                                        config.a_type, config.srgb_conversion);
    num_samples = NumSamples(config.msaa_mode);
    resources.levels = config.max_mip_level + 1;
    if (config.IsPitchLinear()) {
        pitch = config.Pitch();
    } else if (config.IsBlockLinear()) {
        block = Extent3D{
            .width = config.block_width,
            .height = config.block_height,
            .depth = config.block_depth,
        };
    }
    rescaleable = false;
    tile_width_spacing = config.tile_width_spacing;
    if (config.texture_type != TextureType::Texture2D &&
        config.texture_type != TextureType::Texture2DNoMipmap) {
        ASSERT(!config.IsPitchLinear());
    }
    switch (config.texture_type) {
    case TextureType::Texture1D:
        ASSERT(config.BaseLayer() == 0);
        type = ImageType::e1D;
        size.width = config.Width();
        resources.layers = 1;
        break;
    case TextureType::Texture1DArray:
        UNIMPLEMENTED_IF(config.BaseLayer() != 0);
        type = ImageType::e1D;
        size.width = config.Width();
        resources.layers = config.Depth();
        break;
    case TextureType::Texture2D:
    case TextureType::Texture2DNoMipmap:
        ASSERT(config.Depth() == 1);
        type = config.IsPitchLinear() ? ImageType::Linear : ImageType::e2D;
        rescaleable = !config.IsPitchLinear();
        size.width = config.Width();
        size.height = config.Height();
        resources.layers = config.BaseLayer() + 1;
        break;
    case TextureType::Texture2DArray:
        type = ImageType::e2D;
        rescaleable = true;
        size.width = config.Width();
        size.height = config.Height();
        resources.layers = config.BaseLayer() + config.Depth();
        break;
    case TextureType::TextureCubemap:
        ASSERT(config.Depth() == 1);
        type = ImageType::e2D;
        size.width = config.Width();
        size.height = config.Height();
        resources.layers = config.BaseLayer() + 6;
        break;
    case TextureType::TextureCubeArray:
        UNIMPLEMENTED_IF(config.load_store_hint != 0);
        type = ImageType::e2D;
        size.width = config.Width();
        size.height = config.Height();
        resources.layers = config.BaseLayer() + config.Depth() * 6;
        break;
    case TextureType::Texture3D:
        ASSERT(config.BaseLayer() == 0);
        type = ImageType::e3D;
        size.width = config.Width();
        size.height = config.Height();
        size.depth = config.Depth();
        resources.layers = 1;
        break;
    case TextureType::Texture1DBuffer:
        type = ImageType::Buffer;
        size.width = config.Width();
        resources.layers = 1;
        break;
    default:
        ASSERT_MSG(false, "Invalid texture_type={}", static_cast<int>(config.texture_type.Value()));
        break;
    }
    if (num_samples > 1) {
        size.width *= NumSamplesX(config.msaa_mode);
        size.height *= NumSamplesY(config.msaa_mode);
    }
    if (type != ImageType::Linear) {
        // FIXME: Call this without passing *this
        layer_stride = CalculateLayerStride(*this);
        maybe_unaligned_layer_stride = CalculateLayerSize(*this);
        rescaleable &= (block.depth == 0) && resources.levels == 1;
        rescaleable &= size.height > 256 || GetFormatType(format) != SurfaceType::ColorTexture;
        downscaleable = size.height > 512;
    }
}

ImageInfo::ImageInfo(const Maxwell3D::Regs::RenderTargetConfig& ct,
                     Tegra::Texture::MsaaMode msaa_mode) noexcept {
    format = VideoCore::Surface::PixelFormatFromRenderTargetFormat(ct.format);
    rescaleable = false;
    if (ct.tile_mode.is_pitch_linear) {
        ASSERT(ct.tile_mode.dim_control ==
               Maxwell3D::Regs::TileMode::DimensionControl::DefineArraySize);
        type = ImageType::Linear;
        pitch = ct.width;
        size = Extent3D{
            .width = pitch / BytesPerBlock(format),
            .height = ct.height,
            .depth = 1,
        };
        return;
    }
    size.width = ct.width;
    size.height = ct.height;
    layer_stride = ct.array_pitch * 4;
    maybe_unaligned_layer_stride = layer_stride;
    num_samples = NumSamples(msaa_mode);
    block = Extent3D{
        .width = ct.tile_mode.block_width,
        .height = ct.tile_mode.block_height,
        .depth = ct.tile_mode.block_depth,
    };
    if (ct.tile_mode.dim_control == Maxwell3D::Regs::TileMode::DimensionControl::DefineDepthSize) {
        type = ImageType::e3D;
        size.depth = ct.depth;
    } else {
        rescaleable = block.depth == 0;
        rescaleable &= size.height > 256;
        downscaleable = size.height > 512;
        type = ImageType::e2D;
        resources.layers = ct.depth;
    }
}

ImageInfo::ImageInfo(const Maxwell3D::Regs::Zeta& zt, const Maxwell3D::Regs::ZetaSize& zt_size,
                     Tegra::Texture::MsaaMode msaa_mode) noexcept {
    format = VideoCore::Surface::PixelFormatFromDepthFormat(zt.format);
    size.width = zt_size.width;
    size.height = zt_size.height;
    rescaleable = false;
    resources.levels = 1;
    layer_stride = zt.array_pitch * 4;
    maybe_unaligned_layer_stride = layer_stride;
    num_samples = NumSamples(msaa_mode);
    block = Extent3D{
        .width = zt.tile_mode.block_width,
        .height = zt.tile_mode.block_height,
        .depth = zt.tile_mode.block_depth,
    };
    if (zt.tile_mode.is_pitch_linear) {
        ASSERT(zt.tile_mode.dim_control ==
               Maxwell3D::Regs::TileMode::DimensionControl::DefineArraySize);
        type = ImageType::Linear;
        pitch = size.width * BytesPerBlock(format);
    } else if (zt.tile_mode.dim_control ==
               Maxwell3D::Regs::TileMode::DimensionControl::DefineDepthSize) {
        ASSERT(zt_size.dim_control == Maxwell3D::Regs::ZetaSize::DimensionControl::ArraySizeIsOne);
        type = ImageType::e3D;
        size.depth = zt_size.depth;
    } else {
        rescaleable = block.depth == 0;
        downscaleable = size.height > 512;
        type = ImageType::e2D;
        switch (zt_size.dim_control) {
        case Maxwell3D::Regs::ZetaSize::DimensionControl::DefineArraySize:
            resources.layers = zt_size.depth;
            break;
        case Maxwell3D::Regs::ZetaSize::DimensionControl::ArraySizeIsOne:
            resources.layers = 1;
            break;
        }
    }
}

ImageInfo::ImageInfo(const Fermi2D::Surface& config) noexcept {
    UNIMPLEMENTED_IF_MSG(config.layer != 0, "Surface layer is not zero");
    format = VideoCore::Surface::PixelFormatFromRenderTargetFormat(config.format);
    rescaleable = false;
    if (config.linear == Fermi2D::MemoryLayout::Pitch) {
        type = ImageType::Linear;
        size = Extent3D{
            .width = config.pitch / VideoCore::Surface::BytesPerBlock(format),
            .height = config.height,
            .depth = 1,
        };
        pitch = config.pitch;
    } else {
        type = config.block_depth > 0 ? ImageType::e3D : ImageType::e2D;

        block = Extent3D{
            .width = config.block_width,
            .height = config.block_height,
            .depth = config.block_depth,
        };
        // 3D blits with more than once slice are not implemented for now
        // Render to individual slices
        size = Extent3D{
            .width = config.width,
            .height = config.height,
            .depth = 1,
        };
        rescaleable = block.depth == 0 && size.height > 256;
        downscaleable = size.height > 512;
    }
}

static PixelFormat ByteSizeToFormat(u32 bytes_per_pixel) {
    switch (bytes_per_pixel) {
    case 1:
        return PixelFormat::R8_UINT;
    case 2:
        return PixelFormat::R8G8_UINT;
    case 4:
        return PixelFormat::A8B8G8R8_UINT;
    case 8:
        return PixelFormat::R16G16B16A16_UINT;
    case 16:
        return PixelFormat::R32G32B32A32_UINT;
    default:
        UNIMPLEMENTED();
        return PixelFormat::Invalid;
    }
}

ImageInfo::ImageInfo(const Tegra::DMA::ImageOperand& config) noexcept {
    const u32 bytes_per_pixel = config.bytes_per_pixel;
    format = ByteSizeToFormat(bytes_per_pixel);
    type = config.params.block_size.depth > 0 ? ImageType::e3D : ImageType::e2D;
    num_samples = 1;
    block = Extent3D{
        .width = config.params.block_size.width,
        .height = config.params.block_size.height,
        .depth = config.params.block_size.depth,
    };
    size = Extent3D{
        .width = config.params.width,
        .height = config.params.height,
        .depth = config.params.depth,
    };
    tile_width_spacing = 0;
    resources.levels = 1;
    resources.layers = 1;
    layer_stride = CalculateLayerStride(*this);
    maybe_unaligned_layer_stride = CalculateLayerSize(*this);
    rescaleable = block.depth == 0 && size.height > 256;
    downscaleable = size.height > 512;
}

} // namespace VideoCommon

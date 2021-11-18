// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/common_types.h"
#include "common/math_util.h"
#include "video_core/surface.h"

namespace VideoCore::Surface {

SurfaceTarget SurfaceTargetFromTextureType(Tegra::Texture::TextureType texture_type) {
    switch (texture_type) {
    case Tegra::Texture::TextureType::Texture1D:
        return SurfaceTarget::Texture1D;
    case Tegra::Texture::TextureType::Texture1DBuffer:
        return SurfaceTarget::TextureBuffer;
    case Tegra::Texture::TextureType::Texture2D:
    case Tegra::Texture::TextureType::Texture2DNoMipmap:
        return SurfaceTarget::Texture2D;
    case Tegra::Texture::TextureType::Texture3D:
        return SurfaceTarget::Texture3D;
    case Tegra::Texture::TextureType::TextureCubemap:
        return SurfaceTarget::TextureCubemap;
    case Tegra::Texture::TextureType::TextureCubeArray:
        return SurfaceTarget::TextureCubeArray;
    case Tegra::Texture::TextureType::Texture1DArray:
        return SurfaceTarget::Texture1DArray;
    case Tegra::Texture::TextureType::Texture2DArray:
        return SurfaceTarget::Texture2DArray;
    default:
        LOG_CRITICAL(HW_GPU, "Unimplemented texture_type={}", texture_type);
        UNREACHABLE();
        return SurfaceTarget::Texture2D;
    }
}

bool SurfaceTargetIsLayered(SurfaceTarget target) {
    switch (target) {
    case SurfaceTarget::Texture1D:
    case SurfaceTarget::TextureBuffer:
    case SurfaceTarget::Texture2D:
    case SurfaceTarget::Texture3D:
        return false;
    case SurfaceTarget::Texture1DArray:
    case SurfaceTarget::Texture2DArray:
    case SurfaceTarget::TextureCubemap:
    case SurfaceTarget::TextureCubeArray:
        return true;
    default:
        LOG_CRITICAL(HW_GPU, "Unimplemented surface_target={}", target);
        UNREACHABLE();
        return false;
    }
}

bool SurfaceTargetIsArray(SurfaceTarget target) {
    switch (target) {
    case SurfaceTarget::Texture1D:
    case SurfaceTarget::TextureBuffer:
    case SurfaceTarget::Texture2D:
    case SurfaceTarget::Texture3D:
    case SurfaceTarget::TextureCubemap:
        return false;
    case SurfaceTarget::Texture1DArray:
    case SurfaceTarget::Texture2DArray:
    case SurfaceTarget::TextureCubeArray:
        return true;
    default:
        LOG_CRITICAL(HW_GPU, "Unimplemented surface_target={}", target);
        UNREACHABLE();
        return false;
    }
}

PixelFormat PixelFormatFromDepthFormat(Tegra::DepthFormat format) {
    switch (format) {
    case Tegra::DepthFormat::S8_UINT_Z24_UNORM:
        return PixelFormat::S8_UINT_D24_UNORM;
    case Tegra::DepthFormat::D24S8_UNORM:
        return PixelFormat::D24_UNORM_S8_UINT;
    case Tegra::DepthFormat::D32_FLOAT:
        return PixelFormat::D32_FLOAT;
    case Tegra::DepthFormat::D16_UNORM:
        return PixelFormat::D16_UNORM;
    case Tegra::DepthFormat::S8_UINT:
        return PixelFormat::S8_UINT;
    case Tegra::DepthFormat::D32_FLOAT_S8X24_UINT:
        return PixelFormat::D32_FLOAT_S8_UINT;
    default:
        UNIMPLEMENTED_MSG("Unimplemented format={}", format);
        return PixelFormat::S8_UINT_D24_UNORM;
    }
}

PixelFormat PixelFormatFromRenderTargetFormat(Tegra::RenderTargetFormat format) {
    switch (format) {
    case Tegra::RenderTargetFormat::R32B32G32A32_FLOAT:
        return PixelFormat::R32G32B32A32_FLOAT;
    case Tegra::RenderTargetFormat::R32G32B32A32_SINT:
        return PixelFormat::R32G32B32A32_SINT;
    case Tegra::RenderTargetFormat::R32G32B32A32_UINT:
        return PixelFormat::R32G32B32A32_UINT;
    case Tegra::RenderTargetFormat::R16G16B16A16_UNORM:
        return PixelFormat::R16G16B16A16_UNORM;
    case Tegra::RenderTargetFormat::R16G16B16A16_SNORM:
        return PixelFormat::R16G16B16A16_SNORM;
    case Tegra::RenderTargetFormat::R16G16B16A16_SINT:
        return PixelFormat::R16G16B16A16_SINT;
    case Tegra::RenderTargetFormat::R16G16B16A16_UINT:
        return PixelFormat::R16G16B16A16_UINT;
    case Tegra::RenderTargetFormat::R16G16B16A16_FLOAT:
        return PixelFormat::R16G16B16A16_FLOAT;
    case Tegra::RenderTargetFormat::R32G32_FLOAT:
        return PixelFormat::R32G32_FLOAT;
    case Tegra::RenderTargetFormat::R32G32_SINT:
        return PixelFormat::R32G32_SINT;
    case Tegra::RenderTargetFormat::R32G32_UINT:
        return PixelFormat::R32G32_UINT;
    case Tegra::RenderTargetFormat::R16G16B16X16_FLOAT:
        return PixelFormat::R16G16B16X16_FLOAT;
    case Tegra::RenderTargetFormat::B8G8R8A8_UNORM:
        return PixelFormat::B8G8R8A8_UNORM;
    case Tegra::RenderTargetFormat::B8G8R8A8_SRGB:
        return PixelFormat::B8G8R8A8_SRGB;
    case Tegra::RenderTargetFormat::A2B10G10R10_UNORM:
        return PixelFormat::A2B10G10R10_UNORM;
    case Tegra::RenderTargetFormat::A2B10G10R10_UINT:
        return PixelFormat::A2B10G10R10_UINT;
    case Tegra::RenderTargetFormat::A8B8G8R8_UNORM:
        return PixelFormat::A8B8G8R8_UNORM;
    case Tegra::RenderTargetFormat::A8B8G8R8_SRGB:
        return PixelFormat::A8B8G8R8_SRGB;
    case Tegra::RenderTargetFormat::A8B8G8R8_SNORM:
        return PixelFormat::A8B8G8R8_SNORM;
    case Tegra::RenderTargetFormat::A8B8G8R8_SINT:
        return PixelFormat::A8B8G8R8_SINT;
    case Tegra::RenderTargetFormat::A8B8G8R8_UINT:
        return PixelFormat::A8B8G8R8_UINT;
    case Tegra::RenderTargetFormat::R16G16_UNORM:
        return PixelFormat::R16G16_UNORM;
    case Tegra::RenderTargetFormat::R16G16_SNORM:
        return PixelFormat::R16G16_SNORM;
    case Tegra::RenderTargetFormat::R16G16_SINT:
        return PixelFormat::R16G16_SINT;
    case Tegra::RenderTargetFormat::R16G16_UINT:
        return PixelFormat::R16G16_UINT;
    case Tegra::RenderTargetFormat::R16G16_FLOAT:
        return PixelFormat::R16G16_FLOAT;
    case Tegra::RenderTargetFormat::B10G11R11_FLOAT:
        return PixelFormat::B10G11R11_FLOAT;
    case Tegra::RenderTargetFormat::R32_SINT:
        return PixelFormat::R32_SINT;
    case Tegra::RenderTargetFormat::R32_UINT:
        return PixelFormat::R32_UINT;
    case Tegra::RenderTargetFormat::R32_FLOAT:
        return PixelFormat::R32_FLOAT;
    case Tegra::RenderTargetFormat::R5G6B5_UNORM:
        return PixelFormat::R5G6B5_UNORM;
    case Tegra::RenderTargetFormat::A1R5G5B5_UNORM:
        return PixelFormat::A1R5G5B5_UNORM;
    case Tegra::RenderTargetFormat::R8G8_UNORM:
        return PixelFormat::R8G8_UNORM;
    case Tegra::RenderTargetFormat::R8G8_SNORM:
        return PixelFormat::R8G8_SNORM;
    case Tegra::RenderTargetFormat::R8G8_SINT:
        return PixelFormat::R8G8_SINT;
    case Tegra::RenderTargetFormat::R8G8_UINT:
        return PixelFormat::R8G8_UINT;
    case Tegra::RenderTargetFormat::R16_UNORM:
        return PixelFormat::R16_UNORM;
    case Tegra::RenderTargetFormat::R16_SNORM:
        return PixelFormat::R16_SNORM;
    case Tegra::RenderTargetFormat::R16_SINT:
        return PixelFormat::R16_SINT;
    case Tegra::RenderTargetFormat::R16_UINT:
        return PixelFormat::R16_UINT;
    case Tegra::RenderTargetFormat::R16_FLOAT:
        return PixelFormat::R16_FLOAT;
    case Tegra::RenderTargetFormat::R8_UNORM:
        return PixelFormat::R8_UNORM;
    case Tegra::RenderTargetFormat::R8_SNORM:
        return PixelFormat::R8_SNORM;
    case Tegra::RenderTargetFormat::R8_SINT:
        return PixelFormat::R8_SINT;
    case Tegra::RenderTargetFormat::R8_UINT:
        return PixelFormat::R8_UINT;
    default:
        UNIMPLEMENTED_MSG("Unimplemented format={}", format);
        return PixelFormat::A8B8G8R8_UNORM;
    }
}

PixelFormat PixelFormatFromGPUPixelFormat(Tegra::FramebufferConfig::PixelFormat format) {
    switch (format) {
    case Tegra::FramebufferConfig::PixelFormat::A8B8G8R8_UNORM:
        return PixelFormat::A8B8G8R8_UNORM;
    case Tegra::FramebufferConfig::PixelFormat::RGB565_UNORM:
        return PixelFormat::R5G6B5_UNORM;
    case Tegra::FramebufferConfig::PixelFormat::B8G8R8A8_UNORM:
        return PixelFormat::B8G8R8A8_UNORM;
    default:
        UNIMPLEMENTED_MSG("Unimplemented format={}", format);
        return PixelFormat::A8B8G8R8_UNORM;
    }
}

SurfaceType GetFormatType(PixelFormat pixel_format) {
    if (static_cast<std::size_t>(pixel_format) <
        static_cast<std::size_t>(PixelFormat::MaxColorFormat)) {
        return SurfaceType::ColorTexture;
    }

    if (static_cast<std::size_t>(pixel_format) <
        static_cast<std::size_t>(PixelFormat::MaxDepthFormat)) {
        return SurfaceType::Depth;
    }

    if (static_cast<std::size_t>(pixel_format) <
        static_cast<std::size_t>(PixelFormat::MaxStencilFormat)) {
        return SurfaceType::Stencil;
    }

    if (static_cast<std::size_t>(pixel_format) <
        static_cast<std::size_t>(PixelFormat::MaxDepthStencilFormat)) {
        return SurfaceType::DepthStencil;
    }

    // TODO(Subv): Implement the other formats
    ASSERT(false);

    return SurfaceType::Invalid;
}

bool IsPixelFormatASTC(PixelFormat format) {
    switch (format) {
    case PixelFormat::ASTC_2D_4X4_UNORM:
    case PixelFormat::ASTC_2D_5X4_UNORM:
    case PixelFormat::ASTC_2D_5X5_UNORM:
    case PixelFormat::ASTC_2D_8X8_UNORM:
    case PixelFormat::ASTC_2D_8X5_UNORM:
    case PixelFormat::ASTC_2D_4X4_SRGB:
    case PixelFormat::ASTC_2D_5X4_SRGB:
    case PixelFormat::ASTC_2D_5X5_SRGB:
    case PixelFormat::ASTC_2D_8X8_SRGB:
    case PixelFormat::ASTC_2D_8X5_SRGB:
    case PixelFormat::ASTC_2D_10X8_UNORM:
    case PixelFormat::ASTC_2D_10X8_SRGB:
    case PixelFormat::ASTC_2D_6X6_UNORM:
    case PixelFormat::ASTC_2D_6X6_SRGB:
    case PixelFormat::ASTC_2D_10X10_UNORM:
    case PixelFormat::ASTC_2D_10X10_SRGB:
    case PixelFormat::ASTC_2D_12X12_UNORM:
    case PixelFormat::ASTC_2D_12X12_SRGB:
    case PixelFormat::ASTC_2D_8X6_UNORM:
    case PixelFormat::ASTC_2D_8X6_SRGB:
    case PixelFormat::ASTC_2D_6X5_UNORM:
    case PixelFormat::ASTC_2D_6X5_SRGB:
        return true;
    default:
        return false;
    }
}

bool IsPixelFormatSRGB(PixelFormat format) {
    switch (format) {
    case PixelFormat::A8B8G8R8_SRGB:
    case PixelFormat::B8G8R8A8_SRGB:
    case PixelFormat::BC1_RGBA_SRGB:
    case PixelFormat::BC2_SRGB:
    case PixelFormat::BC3_SRGB:
    case PixelFormat::BC7_SRGB:
    case PixelFormat::ASTC_2D_4X4_SRGB:
    case PixelFormat::ASTC_2D_8X8_SRGB:
    case PixelFormat::ASTC_2D_8X5_SRGB:
    case PixelFormat::ASTC_2D_5X4_SRGB:
    case PixelFormat::ASTC_2D_5X5_SRGB:
    case PixelFormat::ASTC_2D_10X8_SRGB:
    case PixelFormat::ASTC_2D_6X6_SRGB:
    case PixelFormat::ASTC_2D_10X10_SRGB:
    case PixelFormat::ASTC_2D_12X12_SRGB:
    case PixelFormat::ASTC_2D_8X6_SRGB:
    case PixelFormat::ASTC_2D_6X5_SRGB:
        return true;
    default:
        return false;
    }
}

bool IsPixelFormatInteger(PixelFormat format) {
    switch (format) {
    case PixelFormat::A8B8G8R8_SINT:
    case PixelFormat::A8B8G8R8_UINT:
    case PixelFormat::A2B10G10R10_UINT:
    case PixelFormat::R8_SINT:
    case PixelFormat::R8_UINT:
    case PixelFormat::R16G16B16A16_SINT:
    case PixelFormat::R16G16B16A16_UINT:
    case PixelFormat::R32G32B32A32_UINT:
    case PixelFormat::R32G32B32A32_SINT:
    case PixelFormat::R32G32_SINT:
    case PixelFormat::R16_UINT:
    case PixelFormat::R16_SINT:
    case PixelFormat::R16G16_UINT:
    case PixelFormat::R16G16_SINT:
    case PixelFormat::R8G8_SINT:
    case PixelFormat::R8G8_UINT:
    case PixelFormat::R32G32_UINT:
    case PixelFormat::R32_UINT:
    case PixelFormat::R32_SINT:
        return true;
    default:
        return false;
    }
}

bool IsPixelFormatSignedInteger(PixelFormat format) {
    switch (format) {
    case PixelFormat::A8B8G8R8_SINT:
    case PixelFormat::R8_SINT:
    case PixelFormat::R16G16B16A16_SINT:
    case PixelFormat::R32G32B32A32_SINT:
    case PixelFormat::R32G32_SINT:
    case PixelFormat::R16_SINT:
    case PixelFormat::R16G16_SINT:
    case PixelFormat::R8G8_SINT:
    case PixelFormat::R32_SINT:
        return true;
    default:
        return false;
    }
}

size_t PixelComponentSizeBitsInteger(PixelFormat format) {
    switch (format) {
    case PixelFormat::A8B8G8R8_SINT:
    case PixelFormat::A8B8G8R8_UINT:
    case PixelFormat::R8_SINT:
    case PixelFormat::R8_UINT:
    case PixelFormat::R8G8_SINT:
    case PixelFormat::R8G8_UINT:
        return 8;
    case PixelFormat::A2B10G10R10_UINT:
        return 10;
    case PixelFormat::R16G16B16A16_SINT:
    case PixelFormat::R16G16B16A16_UINT:
    case PixelFormat::R16_UINT:
    case PixelFormat::R16_SINT:
    case PixelFormat::R16G16_UINT:
    case PixelFormat::R16G16_SINT:
        return 16;
    case PixelFormat::R32G32B32A32_UINT:
    case PixelFormat::R32G32B32A32_SINT:
    case PixelFormat::R32G32_SINT:
    case PixelFormat::R32G32_UINT:
    case PixelFormat::R32_UINT:
    case PixelFormat::R32_SINT:
        return 32;
    default:
        return 0;
    }
}

std::pair<u32, u32> GetASTCBlockSize(PixelFormat format) {
    return {DefaultBlockWidth(format), DefaultBlockHeight(format)};
}

u64 EstimatedDecompressedSize(u64 base_size, PixelFormat format) {
    constexpr u64 RGBA8_PIXEL_SIZE = 4;
    const u64 base_block_size = static_cast<u64>(DefaultBlockWidth(format)) *
                                static_cast<u64>(DefaultBlockHeight(format)) * RGBA8_PIXEL_SIZE;
    return (base_size * base_block_size) / BytesPerBlock(format);
}

} // namespace VideoCore::Surface

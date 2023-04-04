// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "video_core/engines/fermi_2d.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/maxwell_dma.h"
#include "video_core/surface.h"
#include "video_core/texture_cache/types.h"

namespace VideoCommon {

using Tegra::Texture::TICEntry;
using VideoCore::Surface::PixelFormat;

struct ImageInfo {
    ImageInfo() = default;
    explicit ImageInfo(const TICEntry& config) noexcept;
    explicit ImageInfo(const Tegra::Engines::Maxwell3D::Regs::RenderTargetConfig& ct,
                       Tegra::Texture::MsaaMode msaa_mode) noexcept;
    explicit ImageInfo(const Tegra::Engines::Maxwell3D::Regs::Zeta& zt,
                       const Tegra::Engines::Maxwell3D::Regs::ZetaSize& zt_size,
                       Tegra::Texture::MsaaMode msaa_mode) noexcept;
    explicit ImageInfo(const Tegra::Engines::Fermi2D::Surface& config) noexcept;
    explicit ImageInfo(const Tegra::DMA::ImageOperand& config) noexcept;

    PixelFormat format = PixelFormat::Invalid;
    ImageType type = ImageType::e1D;
    SubresourceExtent resources;
    Extent3D size{1, 1, 1};
    union {
        Extent3D block{0, 0, 0};
        u32 pitch;
    };
    u32 layer_stride = 0;
    u32 maybe_unaligned_layer_stride = 0;
    u32 num_samples = 1;
    u32 tile_width_spacing = 0;
    bool rescaleable = false;
    bool downscaleable = false;
};

} // namespace VideoCommon

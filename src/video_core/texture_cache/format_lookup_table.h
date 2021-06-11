// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "video_core/surface.h"
#include "video_core/textures/texture.h"

namespace VideoCommon {

VideoCore::Surface::PixelFormat PixelFormatFromTextureInfo(
    Tegra::Texture::TextureFormat format, Tegra::Texture::ComponentType red_component,
    Tegra::Texture::ComponentType green_component, Tegra::Texture::ComponentType blue_component,
    Tegra::Texture::ComponentType alpha_component, bool is_srgb) noexcept;

} // namespace VideoCommon

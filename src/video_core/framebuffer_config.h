// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

namespace Tegra {

/**
 * Struct describing framebuffer configuration
 */
struct FramebufferConfig {
    enum class PixelFormat : u32 {
        A8B8G8R8_UNORM = 1,
        RGB565_UNORM = 4,
        B8G8R8A8_UNORM = 5,
    };

    VAddr address{};
    u32 offset{};
    u32 width{};
    u32 height{};
    u32 stride{};
    PixelFormat pixel_format{};

    using TransformFlags = Service::NVFlinger::BufferQueue::BufferTransformFlags;
    TransformFlags transform_flags{};
    Common::Rectangle<int> crop_rect;
};

} // namespace Tegra

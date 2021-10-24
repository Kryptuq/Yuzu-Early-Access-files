// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "common/math_util.h"

namespace Layout {

namespace MinimumSize {
constexpr u32 Width = 640;
constexpr u32 Height = 360;
} // namespace MinimumSize

namespace ScreenUndocked {
constexpr u32 Width = 1280;
constexpr u32 Height = 720;
} // namespace ScreenUndocked

namespace ScreenDocked {
constexpr u32 Width = 1920;
constexpr u32 Height = 1080;
} // namespace ScreenDocked

enum class AspectRatio {
    Default,
    R4_3,
    R21_9,
    StretchToWindow,
};

/// Describes the layout of the window framebuffer
struct FramebufferLayout {
    u32 width{ScreenUndocked::Width};
    u32 height{ScreenUndocked::Height};
    bool is_srgb{};

    Common::Rectangle<u32> screen;

    /**
     * Returns the ration of pixel size of the screen, compared to the native size of the undocked
     * Switch screen.
     */
    float GetScalingRatio() const {
        return static_cast<float>(screen.GetWidth()) / ScreenUndocked::Width;
    }
};

/**
 * Factory method for constructing a default FramebufferLayout
 * @param width Window framebuffer width in pixels
 * @param height Window framebuffer height in pixels
 * @return Newly created FramebufferLayout object with default screen regions initialized
 */
FramebufferLayout DefaultFrameLayout(u32 width, u32 height);

/**
 * Convenience method to get frame layout by resolution scale
 * @param res_scale resolution scale factor
 */
FramebufferLayout FrameLayoutFromResolutionScale(f32 res_scale);

/**
 * Convenience method to determine emulation aspect ratio
 * @param aspect Represents the index of aspect ratio stored in Settings::values.aspect_ratio
 * @param window_aspect_ratio Current window aspect ratio
 * @return Emulation render window aspect ratio
 */
float EmulationAspectRatio(AspectRatio aspect, float window_aspect_ratio);

} // namespace Layout

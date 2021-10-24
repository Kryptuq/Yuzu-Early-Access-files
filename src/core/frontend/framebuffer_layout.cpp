// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cmath>

#include "common/assert.h"
#include "common/settings.h"
#include "core/frontend/framebuffer_layout.h"

namespace Layout {

// Finds the largest size subrectangle contained in window area that is confined to the aspect ratio
template <class T>
static Common::Rectangle<T> MaxRectangle(Common::Rectangle<T> window_area,
                                         float screen_aspect_ratio) {
    const float scale = std::min(static_cast<float>(window_area.GetWidth()),
                                 static_cast<float>(window_area.GetHeight()) / screen_aspect_ratio);
    return Common::Rectangle<T>{0, 0, static_cast<T>(std::round(scale)),
                                static_cast<T>(std::round(scale * screen_aspect_ratio))};
}

FramebufferLayout DefaultFrameLayout(u32 width, u32 height) {
    ASSERT(width > 0);
    ASSERT(height > 0);
    // The drawing code needs at least somewhat valid values for both screens
    // so just calculate them both even if the other isn't showing.
    FramebufferLayout res{width, height, false, {}};

    const float window_aspect_ratio = static_cast<float>(height) / static_cast<float>(width);
    const float emulation_aspect_ratio = EmulationAspectRatio(
        static_cast<AspectRatio>(Settings::values.aspect_ratio.GetValue()), window_aspect_ratio);

    const Common::Rectangle<u32> screen_window_area{0, 0, width, height};
    Common::Rectangle<u32> screen = MaxRectangle(screen_window_area, emulation_aspect_ratio);

    if (window_aspect_ratio < emulation_aspect_ratio) {
        screen = screen.TranslateX((screen_window_area.GetWidth() - screen.GetWidth()) / 2);
    } else {
        screen = screen.TranslateY((height - screen.GetHeight()) / 2);
    }

    res.screen = screen;
    return res;
}

FramebufferLayout FrameLayoutFromResolutionScale(f32 res_scale) {
    const bool is_docked = Settings::values.use_docked_mode.GetValue();
    const u32 screen_width = is_docked ? ScreenDocked::Width : ScreenUndocked::Width;
    const u32 screen_height = is_docked ? ScreenDocked::Height : ScreenUndocked::Height;

    const u32 width = static_cast<u32>(static_cast<f32>(screen_width) * res_scale);
    const u32 height = static_cast<u32>(static_cast<f32>(screen_height) * res_scale);

    return DefaultFrameLayout(width, height);
}

float EmulationAspectRatio(AspectRatio aspect, float window_aspect_ratio) {
    switch (aspect) {
    case AspectRatio::Default:
        return static_cast<float>(ScreenUndocked::Height) / ScreenUndocked::Width;
    case AspectRatio::R4_3:
        return 3.0f / 4.0f;
    case AspectRatio::R21_9:
        return 9.0f / 21.0f;
    case AspectRatio::StretchToWindow:
        return window_aspect_ratio;
    default:
        return static_cast<float>(ScreenUndocked::Height) / ScreenUndocked::Width;
    }
}

} // namespace Layout

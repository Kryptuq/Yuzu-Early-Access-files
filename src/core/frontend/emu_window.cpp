// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <mutex>
#include "core/frontend/emu_window.h"

namespace Core::Frontend {

GraphicsContext::~GraphicsContext() = default;

EmuWindow::EmuWindow() {
    // TODO: Find a better place to set this.
    config.min_client_area_size =
        std::make_pair(Layout::MinimumSize::Width, Layout::MinimumSize::Height);
    active_config = config;
}

EmuWindow::~EmuWindow() {}

std::pair<f32, f32> EmuWindow::MapToTouchScreen(u32 framebuffer_x, u32 framebuffer_y) const {
    std::tie(framebuffer_x, framebuffer_y) = ClipToTouchScreen(framebuffer_x, framebuffer_y);
    const float x =
        static_cast<float>(framebuffer_x - framebuffer_layout.screen.left) /
        static_cast<float>(framebuffer_layout.screen.right - framebuffer_layout.screen.left);
    const float y =
        static_cast<float>(framebuffer_y - framebuffer_layout.screen.top) /
        static_cast<float>(framebuffer_layout.screen.bottom - framebuffer_layout.screen.top);

    return std::make_pair(x, y);
}

std::pair<u32, u32> EmuWindow::ClipToTouchScreen(u32 new_x, u32 new_y) const {
    new_x = std::max(new_x, framebuffer_layout.screen.left);
    new_x = std::min(new_x, framebuffer_layout.screen.right - 1);

    new_y = std::max(new_y, framebuffer_layout.screen.top);
    new_y = std::min(new_y, framebuffer_layout.screen.bottom - 1);

    return std::make_pair(new_x, new_y);
}

void EmuWindow::UpdateCurrentFramebufferLayout(u32 width, u32 height) {
    NotifyFramebufferLayoutChanged(Layout::DefaultFrameLayout(width, height));
}

} // namespace Core::Frontend

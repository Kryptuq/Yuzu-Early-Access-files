// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cmath>
#include <mutex>
#include "common/settings.h"
#include "core/frontend/emu_window.h"
#include "core/frontend/input.h"

namespace Core::Frontend {

GraphicsContext::~GraphicsContext() = default;

class EmuWindow::TouchState : public Input::Factory<Input::TouchDevice>,
                              public std::enable_shared_from_this<TouchState> {
public:
    std::unique_ptr<Input::TouchDevice> Create(const Common::ParamPackage&) override {
        return std::make_unique<Device>(shared_from_this());
    }

    std::mutex mutex;

    Input::TouchStatus status;

private:
    class Device : public Input::TouchDevice {
    public:
        explicit Device(std::weak_ptr<TouchState>&& touch_state_) : touch_state(touch_state_) {}
        Input::TouchStatus GetStatus() const override {
            if (auto state = touch_state.lock()) {
                std::lock_guard guard{state->mutex};
                return state->status;
            }
            return {};
        }

    private:
        std::weak_ptr<TouchState> touch_state;
    };
};

EmuWindow::EmuWindow() {
    // TODO: Find a better place to set this.
    config.min_client_area_size =
        std::make_pair(Layout::MinimumSize::Width, Layout::MinimumSize::Height);
    active_config = config;
    touch_state = std::make_shared<TouchState>();
    Input::RegisterFactory<Input::TouchDevice>("emu_window", touch_state);
}

EmuWindow::~EmuWindow() {
    Input::UnregisterFactory<Input::TouchDevice>("emu_window");
}

/**
 * Check if the given x/y coordinates are within the touchpad specified by the framebuffer layout
 * @param layout FramebufferLayout object describing the framebuffer size and screen positions
 * @param framebuffer_x Framebuffer x-coordinate to check
 * @param framebuffer_y Framebuffer y-coordinate to check
 * @return True if the coordinates are within the touchpad, otherwise false
 */
static bool IsWithinTouchscreen(const Layout::FramebufferLayout& layout, u32 framebuffer_x,
                                u32 framebuffer_y) {
    return (framebuffer_y >= layout.screen.top && framebuffer_y < layout.screen.bottom &&
            framebuffer_x >= layout.screen.left && framebuffer_x < layout.screen.right);
}

std::pair<u32, u32> EmuWindow::ClipToTouchScreen(u32 new_x, u32 new_y) const {
    new_x = std::max(new_x, framebuffer_layout.screen.left);
    new_x = std::min(new_x, framebuffer_layout.screen.right - 1);

    new_y = std::max(new_y, framebuffer_layout.screen.top);
    new_y = std::min(new_y, framebuffer_layout.screen.bottom - 1);

    return std::make_pair(new_x, new_y);
}

void EmuWindow::TouchPressed(u32 framebuffer_x, u32 framebuffer_y, size_t id) {
    if (!IsWithinTouchscreen(framebuffer_layout, framebuffer_x, framebuffer_y)) {
        return;
    }
    if (id >= touch_state->status.size()) {
        return;
    }

    std::lock_guard guard{touch_state->mutex};
    const float x =
        static_cast<float>(framebuffer_x - framebuffer_layout.screen.left) /
        static_cast<float>(framebuffer_layout.screen.right - framebuffer_layout.screen.left);
    const float y =
        static_cast<float>(framebuffer_y - framebuffer_layout.screen.top) /
        static_cast<float>(framebuffer_layout.screen.bottom - framebuffer_layout.screen.top);

    touch_state->status[id] = std::make_tuple(x, y, true);
}

void EmuWindow::TouchReleased(size_t id) {
    if (id >= touch_state->status.size()) {
        return;
    }
    std::lock_guard guard{touch_state->mutex};
    touch_state->status[id] = std::make_tuple(0.0f, 0.0f, false);
}

void EmuWindow::TouchMoved(u32 framebuffer_x, u32 framebuffer_y, size_t id) {
    if (id >= touch_state->status.size()) {
        return;
    }

    if (!std::get<2>(touch_state->status[id])) {
        return;
    }

    if (!IsWithinTouchscreen(framebuffer_layout, framebuffer_x, framebuffer_y)) {
        std::tie(framebuffer_x, framebuffer_y) = ClipToTouchScreen(framebuffer_x, framebuffer_y);
    }

    TouchPressed(framebuffer_x, framebuffer_y, id);
}

void EmuWindow::UpdateCurrentFramebufferLayout(u32 width, u32 height) {
    NotifyFramebufferLayoutChanged(Layout::DefaultFrameLayout(width, height));
}

} // namespace Core::Frontend

// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/frontend/emu_window.h"
#include "video_core/gpu.h"
#include "video_core/rasterizer_interface.h"

namespace Core::Frontend {
class EmuWindow;
class GraphicsContext;
} // namespace Core::Frontend

namespace VideoCore {

struct RendererSettings {
    // Screenshot
    std::atomic<bool> screenshot_requested{false};
    void* screenshot_bits{};
    std::function<void(bool)> screenshot_complete_callback;
    Layout::FramebufferLayout screenshot_framebuffer_layout;
};

class RendererBase {
public:
    YUZU_NON_COPYABLE(RendererBase);
    YUZU_NON_MOVEABLE(RendererBase);

    explicit RendererBase(Core::Frontend::EmuWindow& window,
                          std::unique_ptr<Core::Frontend::GraphicsContext> context);
    virtual ~RendererBase();

    /// Finalize rendering the guest frame and draw into the presentation texture
    virtual void SwapBuffers(const Tegra::FramebufferConfig* framebuffer) = 0;

    [[nodiscard]] virtual RasterizerInterface* ReadRasterizer() = 0;

    [[nodiscard]] virtual std::string GetDeviceVendor() const = 0;

    // Getter/setter functions:
    // ------------------------

    [[nodiscard]] f32 GetCurrentFPS() const {
        return m_current_fps;
    }

    [[nodiscard]] int GetCurrentFrame() const {
        return m_current_frame;
    }

    [[nodiscard]] Core::Frontend::GraphicsContext& Context() {
        return *context;
    }

    [[nodiscard]] const Core::Frontend::GraphicsContext& Context() const {
        return *context;
    }

    [[nodiscard]] Core::Frontend::EmuWindow& GetRenderWindow() {
        return render_window;
    }

    [[nodiscard]] const Core::Frontend::EmuWindow& GetRenderWindow() const {
        return render_window;
    }

    [[nodiscard]] RendererSettings& Settings() {
        return renderer_settings;
    }

    [[nodiscard]] const RendererSettings& Settings() const {
        return renderer_settings;
    }

    /// Refreshes the settings common to all renderers
    void RefreshBaseSettings();

    /// Returns true if a screenshot is being processed
    bool IsScreenshotPending() const;

    /// Request a screenshot of the next frame
    void RequestScreenshot(void* data, std::function<void(bool)> callback,
                           const Layout::FramebufferLayout& layout);

protected:
    Core::Frontend::EmuWindow& render_window; ///< Reference to the render window handle.
    std::unique_ptr<Core::Frontend::GraphicsContext> context;
    f32 m_current_fps = 0.0f; ///< Current framerate, should be set by the renderer
    int m_current_frame = 0;  ///< Current frame, should be set by the renderer

    RendererSettings renderer_settings;

private:
    /// Updates the framebuffer layout of the contained render window handle.
    void UpdateCurrentFramebufferLayout();
};

} // namespace VideoCore

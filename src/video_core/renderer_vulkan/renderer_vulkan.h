// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string>

#include "common/dynamic_library.h"
#include "video_core/renderer_base.h"
#include "video_core/renderer_vulkan/vk_blit_screen.h"
#include "video_core/renderer_vulkan/vk_rasterizer.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_state_tracker.h"
#include "video_core/renderer_vulkan/vk_swapchain.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_memory_allocator.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Core {
class TelemetrySession;
}

namespace Core::Memory {
class Memory;
}

namespace Tegra {
class GPU;
}

namespace Vulkan {

class RendererVulkan final : public VideoCore::RendererBase {
public:
    explicit RendererVulkan(Core::TelemetrySession& telemtry_session,
                            Core::Frontend::EmuWindow& emu_window,
                            Core::Memory::Memory& cpu_memory_, Tegra::GPU& gpu_,
                            std::unique_ptr<Core::Frontend::GraphicsContext> context_);
    ~RendererVulkan() override;

    void SwapBuffers(const Tegra::FramebufferConfig* framebuffer) override;

    VideoCore::RasterizerInterface* ReadRasterizer() override {
        return &rasterizer;
    }

    [[nodiscard]] std::string GetDeviceVendor() const override {
        return device.GetDriverName();
    }

private:
    void Report() const;

    void RenderScreenshot(const Tegra::FramebufferConfig& framebuffer, bool use_accelerated);

    Core::TelemetrySession& telemetry_session;
    Core::Memory::Memory& cpu_memory;
    Tegra::GPU& gpu;

    Common::DynamicLibrary library;
    vk::InstanceDispatch dld;

    vk::Instance instance;
    vk::DebugUtilsMessenger debug_callback;
    vk::SurfaceKHR surface;

    VKScreenInfo screen_info;

    Device device;
    MemoryAllocator memory_allocator;
    StateTracker state_tracker;
    VKScheduler scheduler;
    VKSwapchain swapchain;
    VKBlitScreen blit_screen;
    RasterizerVulkan rasterizer;
};

} // namespace Vulkan

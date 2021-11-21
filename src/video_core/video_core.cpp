// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>

#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core.h"
#include "video_core/renderer_base.h"
#include "video_core/renderer_opengl/renderer_opengl.h"
#include "video_core/renderer_vulkan/renderer_vulkan.h"
#include "video_core/video_core.h"

namespace {

std::unique_ptr<VideoCore::RendererBase> CreateRenderer(
    Core::System& system, Core::Frontend::EmuWindow& emu_window, Tegra::GPU& gpu,
    std::unique_ptr<Core::Frontend::GraphicsContext> context) {
    auto& telemetry_session = system.TelemetrySession();
    auto& cpu_memory = system.Memory();

    switch (Settings::values.renderer_backend.GetValue()) {
    case Settings::RendererBackend::OpenGL:
        return std::make_unique<OpenGL::RendererOpenGL>(telemetry_session, emu_window, cpu_memory,
                                                        gpu, std::move(context));
    case Settings::RendererBackend::Vulkan:
        return std::make_unique<Vulkan::RendererVulkan>(telemetry_session, emu_window, cpu_memory,
                                                        gpu, std::move(context));
    default:
        return nullptr;
    }
}

} // Anonymous namespace

namespace VideoCore {

std::unique_ptr<Tegra::GPU> CreateGPU(Core::Frontend::EmuWindow& emu_window, Core::System& system) {
    Settings::UpdateRescalingInfo();

    const auto nvdec_value = Settings::values.nvdec_emulation.GetValue();
    const bool use_nvdec = nvdec_value != Settings::NvdecEmulation::Off;
    const bool use_async = Settings::values.use_asynchronous_gpu_emulation.GetValue();
    auto gpu = std::make_unique<Tegra::GPU>(system, use_async, use_nvdec);
    auto context = emu_window.CreateSharedContext();
    auto scope = context->Acquire();
    try {
        auto renderer = CreateRenderer(system, emu_window, *gpu, std::move(context));
        gpu->BindRenderer(std::move(renderer));
        return gpu;
    } catch (const std::runtime_error& exception) {
        LOG_ERROR(HW_GPU, "Failed to initialize GPU: {}", exception.what());
        return nullptr;
    }
}

} // namespace VideoCore

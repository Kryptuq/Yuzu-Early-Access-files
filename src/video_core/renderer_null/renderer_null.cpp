// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer_null/renderer_null.h"

namespace Null {

RendererNull::RendererNull(Core::Frontend::EmuWindow& emu_window, Core::Memory::Memory& cpu_memory,
                           Tegra::GPU& gpu,
                           std::unique_ptr<Core::Frontend::GraphicsContext> context_)
    : RendererBase(emu_window, std::move(context_)), m_gpu(gpu), m_rasterizer(cpu_memory, gpu) {}

RendererNull::~RendererNull() = default;

void RendererNull::SwapBuffers(const Tegra::FramebufferConfig* framebuffer) {
    if (!framebuffer) {
        return;
    }

    m_gpu.RendererFrameEndNotify();
    render_window.OnFrameDisplayed();
}

} // namespace Null

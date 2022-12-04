// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string>

#include "video_core/renderer_base.h"
#include "video_core/renderer_null/null_rasterizer.h"

namespace Null {

class RendererNull final : public VideoCore::RendererBase {
public:
    explicit RendererNull(Core::Frontend::EmuWindow& emu_window, Core::Memory::Memory& cpu_memory,
                          Tegra::GPU& gpu,
                          std::unique_ptr<Core::Frontend::GraphicsContext> context);
    ~RendererNull() override;

    void SwapBuffers(const Tegra::FramebufferConfig* framebuffer) override;

    VideoCore::RasterizerInterface* ReadRasterizer() override {
        return &m_rasterizer;
    }

    [[nodiscard]] std::string GetDeviceVendor() const override {
        return "NULL";
    }

private:
    Tegra::GPU& m_gpu;
    RasterizerNull m_rasterizer;
};

} // namespace Null

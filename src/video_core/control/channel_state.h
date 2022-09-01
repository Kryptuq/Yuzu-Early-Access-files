// SPDX-FileCopyrightText: 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include "common/common_types.h"

namespace Core {
class System;
}

namespace VideoCore {
class RasterizerInterface;
}

namespace Tegra {

class GPU;

namespace Engines {
class Puller;
class Fermi2D;
class Maxwell3D;
class MaxwellDMA;
class KeplerCompute;
class KeplerMemory;
} // namespace Engines

class MemoryManager;
class DmaPusher;

namespace Control {

struct ChannelState {
    explicit ChannelState(s32 bind_id);
    ChannelState(const ChannelState& state) = delete;
    ChannelState& operator=(const ChannelState&) = delete;
    ChannelState(ChannelState&& other) noexcept = default;
    ChannelState& operator=(ChannelState&& other) noexcept = default;

    void Init(Core::System& system, GPU& gpu);

    void BindRasterizer(VideoCore::RasterizerInterface* rasterizer);

    s32 bind_id = -1;
    /// 3D engine
    std::unique_ptr<Engines::Maxwell3D> maxwell_3d;
    /// 2D engine
    std::unique_ptr<Engines::Fermi2D> fermi_2d;
    /// Compute engine
    std::unique_ptr<Engines::KeplerCompute> kepler_compute;
    /// DMA engine
    std::unique_ptr<Engines::MaxwellDMA> maxwell_dma;
    /// Inline memory engine
    std::unique_ptr<Engines::KeplerMemory> kepler_memory;

    std::shared_ptr<MemoryManager> memory_manager;

    std::unique_ptr<DmaPusher> dma_pusher;

    bool initialized{};
};

} // namespace Control

} // namespace Tegra

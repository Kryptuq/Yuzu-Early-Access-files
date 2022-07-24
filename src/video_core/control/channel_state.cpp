// SPDX-FileCopyrightText: 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/assert.h"
#include "video_core/control/channel_state.h"
#include "video_core/dma_pusher.h"
#include "video_core/engines/fermi_2d.h"
#include "video_core/engines/kepler_compute.h"
#include "video_core/engines/kepler_memory.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/maxwell_dma.h"
#include "video_core/engines/puller.h"
#include "video_core/memory_manager.h"

namespace Tegra::Control {

ChannelState::ChannelState(s32 bind_id_) {
    bind_id = bind_id_;
    initiated = false;
}

void ChannelState::Init(Core::System& system, GPU& gpu) {
    ASSERT(memory_manager);
    dma_pusher = std::make_unique<Tegra::DmaPusher>(system, gpu, *memory_manager, *this);
    maxwell_3d = std::make_unique<Engines::Maxwell3D>(system, *memory_manager);
    fermi_2d = std::make_unique<Engines::Fermi2D>();
    kepler_compute = std::make_unique<Engines::KeplerCompute>(system, *memory_manager);
    maxwell_dma = std::make_unique<Engines::MaxwellDMA>(system, *memory_manager);
    kepler_memory = std::make_unique<Engines::KeplerMemory>(system, *memory_manager);
    initiated = true;
}

void ChannelState::BindRasterizer(VideoCore::RasterizerInterface* rasterizer) {
    dma_pusher->BindRasterizer(rasterizer);
    memory_manager->BindRasterizer(rasterizer);
    maxwell_3d->BindRasterizer(rasterizer);
    fermi_2d->BindRasterizer(rasterizer);
    kepler_memory->BindRasterizer(rasterizer);
    kepler_compute->BindRasterizer(rasterizer);
    maxwell_dma->BindRasterizer(rasterizer);
}

} // namespace Tegra::Control

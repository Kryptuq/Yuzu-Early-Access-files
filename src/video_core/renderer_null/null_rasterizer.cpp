// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/host1x/host1x.h"
#include "video_core/memory_manager.h"
#include "video_core/renderer_null/null_rasterizer.h"

namespace Null {

AccelerateDMA::AccelerateDMA() = default;

bool AccelerateDMA::BufferCopy(GPUVAddr start_address, GPUVAddr end_address, u64 amount) {
    return true;
}
bool AccelerateDMA::BufferClear(GPUVAddr src_address, u64 amount, u32 value) {
    return true;
}

RasterizerNull::RasterizerNull(Core::Memory::Memory& cpu_memory_, Tegra::GPU& gpu)
    : RasterizerAccelerated(cpu_memory_), m_gpu{gpu} {}
RasterizerNull::~RasterizerNull() = default;

void RasterizerNull::Draw(bool is_indexed, u32 instance_count) {}
void RasterizerNull::Clear(u32 layer_count) {}
void RasterizerNull::DispatchCompute() {}
void RasterizerNull::ResetCounter(VideoCore::QueryType type) {}
void RasterizerNull::Query(GPUVAddr gpu_addr, VideoCore::QueryType type,
                           std::optional<u64> timestamp) {
    if (!gpu_memory) {
        return;
    }

    gpu_memory->Write(gpu_addr, u64{0});
    if (timestamp) {
        gpu_memory->Write(gpu_addr + 8, *timestamp);
    }
}
void RasterizerNull::BindGraphicsUniformBuffer(size_t stage, u32 index, GPUVAddr gpu_addr,
                                               u32 size) {}
void RasterizerNull::DisableGraphicsUniformBuffer(size_t stage, u32 index) {}
void RasterizerNull::FlushAll() {}
void RasterizerNull::FlushRegion(VAddr addr, u64 size) {}
bool RasterizerNull::MustFlushRegion(VAddr addr, u64 size) {
    return false;
}
void RasterizerNull::InvalidateRegion(VAddr addr, u64 size) {}
void RasterizerNull::OnCPUWrite(VAddr addr, u64 size) {}
void RasterizerNull::InvalidateGPUCache() {}
void RasterizerNull::UnmapMemory(VAddr addr, u64 size) {}
void RasterizerNull::ModifyGPUMemory(size_t as_id, GPUVAddr addr, u64 size) {}
void RasterizerNull::SignalFence(std::function<void()>&& func) {
    func();
}
void RasterizerNull::SyncOperation(std::function<void()>&& func) {
    func();
}
void RasterizerNull::SignalSyncPoint(u32 value) {
    auto& syncpoint_manager = m_gpu.Host1x().GetSyncpointManager();
    syncpoint_manager.IncrementGuest(value);
    syncpoint_manager.IncrementHost(value);
}
void RasterizerNull::SignalReference() {}
void RasterizerNull::ReleaseFences() {}
void RasterizerNull::FlushAndInvalidateRegion(VAddr addr, u64 size) {}
void RasterizerNull::WaitForIdle() {}
void RasterizerNull::FragmentBarrier() {}
void RasterizerNull::TiledCacheBarrier() {}
void RasterizerNull::FlushCommands() {}
void RasterizerNull::TickFrame() {}
Tegra::Engines::AccelerateDMAInterface& RasterizerNull::AccessAccelerateDMA() {
    return m_accelerate_dma;
}
bool RasterizerNull::AccelerateSurfaceCopy(const Tegra::Engines::Fermi2D::Surface& src,
                                           const Tegra::Engines::Fermi2D::Surface& dst,
                                           const Tegra::Engines::Fermi2D::Config& copy_config) {
    return true;
}
void RasterizerNull::AccelerateInlineToMemory(GPUVAddr address, size_t copy_size,
                                              std::span<const u8> memory) {}
bool RasterizerNull::AccelerateDisplay(const Tegra::FramebufferConfig& config,
                                       VAddr framebuffer_addr, u32 pixel_stride) {
    return true;
}
void RasterizerNull::LoadDiskResources(u64 title_id, std::stop_token stop_loading,
                                       const VideoCore::DiskResourceLoadCallback& callback) {}
void RasterizerNull::InitializeChannel(Tegra::Control::ChannelState& channel) {}
void RasterizerNull::BindChannel(Tegra::Control::ChannelState& channel) {}
void RasterizerNull::ReleaseChannel(s32 channel_id) {}

} // namespace Null

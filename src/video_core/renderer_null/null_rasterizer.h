// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "video_core/control/channel_state_cache.h"
#include "video_core/engines/maxwell_dma.h"
#include "video_core/rasterizer_accelerated.h"
#include "video_core/rasterizer_interface.h"

namespace Core {
class System;
}

namespace Null {

class RasterizerNull;

class AccelerateDMA : public Tegra::Engines::AccelerateDMAInterface {
public:
    explicit AccelerateDMA();
    bool BufferCopy(GPUVAddr start_address, GPUVAddr end_address, u64 amount) override;
    bool BufferClear(GPUVAddr src_address, u64 amount, u32 value) override;
};

class RasterizerNull final : public VideoCore::RasterizerAccelerated,
                             protected VideoCommon::ChannelSetupCaches<VideoCommon::ChannelInfo> {
public:
    explicit RasterizerNull(Core::Memory::Memory& cpu_memory, Tegra::GPU& gpu);
    ~RasterizerNull() override;

    void Draw(bool is_indexed, u32 instance_count) override;
    void Clear(u32 layer_count) override;
    void DispatchCompute() override;
    void ResetCounter(VideoCore::QueryType type) override;
    void Query(GPUVAddr gpu_addr, VideoCore::QueryType type, std::optional<u64> timestamp) override;
    void BindGraphicsUniformBuffer(size_t stage, u32 index, GPUVAddr gpu_addr, u32 size) override;
    void DisableGraphicsUniformBuffer(size_t stage, u32 index) override;
    void FlushAll() override;
    void FlushRegion(VAddr addr, u64 size,
                     VideoCommon::CacheType which = VideoCommon::CacheType::All) override;
    bool MustFlushRegion(VAddr addr, u64 size,
                         VideoCommon::CacheType which = VideoCommon::CacheType::All) override;
    void InvalidateRegion(VAddr addr, u64 size,
                          VideoCommon::CacheType which = VideoCommon::CacheType::All) override;
    void OnCPUWrite(VAddr addr, u64 size) override;
    void InvalidateGPUCache() override;
    void UnmapMemory(VAddr addr, u64 size) override;
    void ModifyGPUMemory(size_t as_id, GPUVAddr addr, u64 size) override;
    void SignalFence(std::function<void()>&& func) override;
    void SyncOperation(std::function<void()>&& func) override;
    void SignalSyncPoint(u32 value) override;
    void SignalReference() override;
    void ReleaseFences() override;
    void FlushAndInvalidateRegion(
        VAddr addr, u64 size, VideoCommon::CacheType which = VideoCommon::CacheType::All) override;
    void WaitForIdle() override;
    void FragmentBarrier() override;
    void TiledCacheBarrier() override;
    void FlushCommands() override;
    void TickFrame() override;
    bool AccelerateSurfaceCopy(const Tegra::Engines::Fermi2D::Surface& src,
                               const Tegra::Engines::Fermi2D::Surface& dst,
                               const Tegra::Engines::Fermi2D::Config& copy_config) override;
    Tegra::Engines::AccelerateDMAInterface& AccessAccelerateDMA() override;
    void AccelerateInlineToMemory(GPUVAddr address, size_t copy_size,
                                  std::span<const u8> memory) override;
    bool AccelerateDisplay(const Tegra::FramebufferConfig& config, VAddr framebuffer_addr,
                           u32 pixel_stride) override;
    void LoadDiskResources(u64 title_id, std::stop_token stop_loading,
                           const VideoCore::DiskResourceLoadCallback& callback) override;
    void InitializeChannel(Tegra::Control::ChannelState& channel) override;
    void BindChannel(Tegra::Control::ChannelState& channel) override;
    void ReleaseChannel(s32 channel_id) override;

private:
    Tegra::GPU& m_gpu;
    AccelerateDMA m_accelerate_dma;
};

} // namespace Null

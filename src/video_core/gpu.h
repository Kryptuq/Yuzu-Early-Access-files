// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "common/bit_field.h"
#include "common/common_types.h"
#include "core/hle/service/nvdrv/nvdata.h"
#include "video_core/cdma_pusher.h"
#include "video_core/framebuffer_config.h"

namespace Core {
class System;
} // namespace Core

namespace VideoCore {
class RendererBase;
class ShaderNotify;
} // namespace VideoCore

namespace Tegra {
class DmaPusher;
struct CommandList;

enum class RenderTargetFormat : u32 {
    NONE = 0x0,
    R32B32G32A32_FLOAT = 0xC0,
    R32G32B32A32_SINT = 0xC1,
    R32G32B32A32_UINT = 0xC2,
    R16G16B16A16_UNORM = 0xC6,
    R16G16B16A16_SNORM = 0xC7,
    R16G16B16A16_SINT = 0xC8,
    R16G16B16A16_UINT = 0xC9,
    R16G16B16A16_FLOAT = 0xCA,
    R32G32_FLOAT = 0xCB,
    R32G32_SINT = 0xCC,
    R32G32_UINT = 0xCD,
    R16G16B16X16_FLOAT = 0xCE,
    B8G8R8A8_UNORM = 0xCF,
    B8G8R8A8_SRGB = 0xD0,
    A2B10G10R10_UNORM = 0xD1,
    A2B10G10R10_UINT = 0xD2,
    A8B8G8R8_UNORM = 0xD5,
    A8B8G8R8_SRGB = 0xD6,
    A8B8G8R8_SNORM = 0xD7,
    A8B8G8R8_SINT = 0xD8,
    A8B8G8R8_UINT = 0xD9,
    R16G16_UNORM = 0xDA,
    R16G16_SNORM = 0xDB,
    R16G16_SINT = 0xDC,
    R16G16_UINT = 0xDD,
    R16G16_FLOAT = 0xDE,
    B10G11R11_FLOAT = 0xE0,
    R32_SINT = 0xE3,
    R32_UINT = 0xE4,
    R32_FLOAT = 0xE5,
    R5G6B5_UNORM = 0xE8,
    A1R5G5B5_UNORM = 0xE9,
    R8G8_UNORM = 0xEA,
    R8G8_SNORM = 0xEB,
    R8G8_SINT = 0xEC,
    R8G8_UINT = 0xED,
    R16_UNORM = 0xEE,
    R16_SNORM = 0xEF,
    R16_SINT = 0xF0,
    R16_UINT = 0xF1,
    R16_FLOAT = 0xF2,
    R8_UNORM = 0xF3,
    R8_SNORM = 0xF4,
    R8_SINT = 0xF5,
    R8_UINT = 0xF6,
};

enum class DepthFormat : u32 {
    D32_FLOAT = 0xA,
    D16_UNORM = 0x13,
    S8_UINT_Z24_UNORM = 0x14,
    D24X8_UNORM = 0x15,
    D24S8_UNORM = 0x16,
    S8_UINT = 0x17,
    D24C8_UNORM = 0x18,
    D32_FLOAT_S8X24_UINT = 0x19,
};

namespace Engines {
class Maxwell3D;
class KeplerCompute;
} // namespace Engines

namespace Control {
struct ChannelState;
}

namespace Host1x {
class Host1x;
} // namespace Host1x

class MemoryManager;

class GPU final {
public:
    explicit GPU(Core::System& system, bool is_async, bool use_nvdec);
    ~GPU();

    /// Binds a renderer to the GPU.
    void BindRenderer(std::unique_ptr<VideoCore::RendererBase> renderer);

    /// Flush all current written commands into the host GPU for execution.
    void FlushCommands();
    /// Synchronizes CPU writes with Host GPU memory.
    void InvalidateGPUCache();
    /// Signal the ending of command list.
    void OnCommandListEnd();

    std::shared_ptr<Control::ChannelState> AllocateChannel();

    void InitChannel(Control::ChannelState& to_init);

    void BindChannel(s32 channel_id);

    void ReleaseChannel(Control::ChannelState& to_release);

    void InitAddressSpace(Tegra::MemoryManager& memory_manager);

    /// Request a host GPU memory flush from the CPU.
    [[nodiscard]] u64 RequestFlush(VAddr addr, std::size_t size);

    /// Obtains current flush request fence id.
    [[nodiscard]] u64 CurrentSyncRequestFence() const;

    void WaitForSyncOperation(u64 fence);

    /// Tick pending requests within the GPU.
    void TickWork();

    /// Gets a mutable reference to the Host1x interface
    [[nodiscard]] Host1x::Host1x& Host1x();

    /// Gets an immutable reference to the Host1x interface.
    [[nodiscard]] const Host1x::Host1x& Host1x() const;

    /// Returns a reference to the Maxwell3D GPU engine.
    [[nodiscard]] Engines::Maxwell3D& Maxwell3D();

    /// Returns a const reference to the Maxwell3D GPU engine.
    [[nodiscard]] const Engines::Maxwell3D& Maxwell3D() const;

    /// Returns a reference to the KeplerCompute GPU engine.
    [[nodiscard]] Engines::KeplerCompute& KeplerCompute();

    /// Returns a reference to the KeplerCompute GPU engine.
    [[nodiscard]] const Engines::KeplerCompute& KeplerCompute() const;

    /// Returns a reference to the GPU DMA pusher.
    [[nodiscard]] Tegra::DmaPusher& DmaPusher();

    /// Returns a const reference to the GPU DMA pusher.
    [[nodiscard]] const Tegra::DmaPusher& DmaPusher() const;

    /// Returns a reference to the underlying renderer.
    [[nodiscard]] VideoCore::RendererBase& Renderer();

    /// Returns a const reference to the underlying renderer.
    [[nodiscard]] const VideoCore::RendererBase& Renderer() const;

    /// Returns a reference to the shader notifier.
    [[nodiscard]] VideoCore::ShaderNotify& ShaderNotify();

    /// Returns a const reference to the shader notifier.
    [[nodiscard]] const VideoCore::ShaderNotify& ShaderNotify() const;

    [[nodiscard]] u64 GetTicks() const;

    [[nodiscard]] bool IsAsync() const;

    [[nodiscard]] bool UseNvdec() const;

    void RendererFrameEndNotify();

    void RequestSwapBuffers(const Tegra::FramebufferConfig* framebuffer,
                            std::array<Service::Nvidia::NvFence, 4>& fences, size_t num_fences);

    /// Performs any additional setup necessary in order to begin GPU emulation.
    /// This can be used to launch any necessary threads and register any necessary
    /// core timing events.
    void Start();

    /// Performs any additional necessary steps to shutdown GPU emulation.
    void NotifyShutdown();

    /// Obtain the CPU Context
    void ObtainContext();

    /// Release the CPU Context
    void ReleaseContext();

    /// Push GPU command entries to be processed
    void PushGPUEntries(s32 channel, Tegra::CommandList&& entries);

    /// Push GPU command buffer entries to be processed
    void PushCommandBuffer(u32 id, Tegra::ChCommandHeaderList& entries);

    /// Frees the CDMAPusher instance to free up resources
    void ClearCdmaInstance(u32 id);

    /// Swap buffers (render frame)
    void SwapBuffers(const Tegra::FramebufferConfig* framebuffer);

    /// Notify rasterizer that any caches of the specified region should be flushed to Switch memory
    void FlushRegion(VAddr addr, u64 size);

    /// Notify rasterizer that any caches of the specified region should be invalidated
    void InvalidateRegion(VAddr addr, u64 size);

    /// Notify rasterizer that any caches of the specified region should be flushed and invalidated
    void FlushAndInvalidateRegion(VAddr addr, u64 size);

private:
    struct Impl;
    mutable std::unique_ptr<Impl> impl;
};

} // namespace Tegra

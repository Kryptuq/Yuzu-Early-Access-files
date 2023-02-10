// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>

#include "common/assert.h"
#include "common/microprofile.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/frontend/emu_window.h"
#include "core/hle/service/nvdrv/nvdata.h"
#include "core/perf_stats.h"
#include "video_core/cdma_pusher.h"
#include "video_core/control/channel_state.h"
#include "video_core/control/scheduler.h"
#include "video_core/dma_pusher.h"
#include "video_core/engines/fermi_2d.h"
#include "video_core/engines/kepler_compute.h"
#include "video_core/engines/kepler_memory.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/maxwell_dma.h"
#include "video_core/gpu.h"
#include "video_core/gpu_thread.h"
#include "video_core/host1x/host1x.h"
#include "video_core/host1x/syncpoint_manager.h"
#include "video_core/memory_manager.h"
#include "video_core/renderer_base.h"
#include "video_core/shader_notify.h"

namespace Tegra {

struct GPU::Impl {
    explicit Impl(GPU& gpu_, Core::System& system_, bool is_async_, bool use_nvdec_)
        : gpu{gpu_}, system{system_}, host1x{system.Host1x()}, use_nvdec{use_nvdec_},
          shader_notify{std::make_unique<VideoCore::ShaderNotify>()}, is_async{is_async_},
          gpu_thread{system_, is_async_}, scheduler{std::make_unique<Control::Scheduler>(gpu)} {}

    ~Impl() = default;

    std::shared_ptr<Control::ChannelState> CreateChannel(s32 channel_id) {
        auto channel_state = std::make_shared<Tegra::Control::ChannelState>(channel_id);
        channels.emplace(channel_id, channel_state);
        scheduler->DeclareChannel(channel_state);
        return channel_state;
    }

    void BindChannel(s32 channel_id) {
        if (bound_channel == channel_id) {
            return;
        }
        auto it = channels.find(channel_id);
        ASSERT(it != channels.end());
        bound_channel = channel_id;
        current_channel = it->second.get();

        rasterizer->BindChannel(*current_channel);
    }

    std::shared_ptr<Control::ChannelState> AllocateChannel() {
        return CreateChannel(new_channel_id++);
    }

    void InitChannel(Control::ChannelState& to_init) {
        to_init.Init(system, gpu);
        to_init.BindRasterizer(rasterizer);
        rasterizer->InitializeChannel(to_init);
    }

    void InitAddressSpace(Tegra::MemoryManager& memory_manager) {
        memory_manager.BindRasterizer(rasterizer);
    }

    void ReleaseChannel(Control::ChannelState& to_release) {
        UNIMPLEMENTED();
    }

    /// Binds a renderer to the GPU.
    void BindRenderer(std::unique_ptr<VideoCore::RendererBase> renderer_) {
        renderer = std::move(renderer_);
        rasterizer = renderer->ReadRasterizer();
        host1x.MemoryManager().BindRasterizer(rasterizer);
    }

    /// Flush all current written commands into the host GPU for execution.
    void FlushCommands() {
        rasterizer->FlushCommands();
    }

    /// Synchronizes CPU writes with Host GPU memory.
    void InvalidateGPUCache() {
        rasterizer->InvalidateGPUCache();
    }

    /// Signal the ending of command list.
    void OnCommandListEnd() {
        rasterizer->ReleaseFences();
    }

    /// Request a host GPU memory flush from the CPU.
    template <typename Func>
    [[nodiscard]] u64 RequestSyncOperation(Func&& action) {
        std::unique_lock lck{sync_request_mutex};
        const u64 fence = ++last_sync_fence;
        sync_requests.emplace_back(action);
        return fence;
    }

    /// Obtains current flush request fence id.
    [[nodiscard]] u64 CurrentSyncRequestFence() const {
        return current_sync_fence.load(std::memory_order_relaxed);
    }

    void WaitForSyncOperation(const u64 fence) {
        std::unique_lock lck{sync_request_mutex};
        sync_request_cv.wait(lck, [this, fence] { return CurrentSyncRequestFence() >= fence; });
    }

    /// Tick pending requests within the GPU.
    void TickWork() {
        std::unique_lock lck{sync_request_mutex};
        while (!sync_requests.empty()) {
            auto request = std::move(sync_requests.front());
            sync_requests.pop_front();
            sync_request_mutex.unlock();
            request();
            current_sync_fence.fetch_add(1, std::memory_order_release);
            sync_request_mutex.lock();
            sync_request_cv.notify_all();
        }
    }

    /// Returns a reference to the Maxwell3D GPU engine.
    [[nodiscard]] Engines::Maxwell3D& Maxwell3D() {
        ASSERT(current_channel);
        return *current_channel->maxwell_3d;
    }

    /// Returns a const reference to the Maxwell3D GPU engine.
    [[nodiscard]] const Engines::Maxwell3D& Maxwell3D() const {
        ASSERT(current_channel);
        return *current_channel->maxwell_3d;
    }

    /// Returns a reference to the KeplerCompute GPU engine.
    [[nodiscard]] Engines::KeplerCompute& KeplerCompute() {
        ASSERT(current_channel);
        return *current_channel->kepler_compute;
    }

    /// Returns a reference to the KeplerCompute GPU engine.
    [[nodiscard]] const Engines::KeplerCompute& KeplerCompute() const {
        ASSERT(current_channel);
        return *current_channel->kepler_compute;
    }

    /// Returns a reference to the GPU DMA pusher.
    [[nodiscard]] Tegra::DmaPusher& DmaPusher() {
        ASSERT(current_channel);
        return *current_channel->dma_pusher;
    }

    /// Returns a const reference to the GPU DMA pusher.
    [[nodiscard]] const Tegra::DmaPusher& DmaPusher() const {
        ASSERT(current_channel);
        return *current_channel->dma_pusher;
    }

    /// Returns a reference to the underlying renderer.
    [[nodiscard]] VideoCore::RendererBase& Renderer() {
        return *renderer;
    }

    /// Returns a const reference to the underlying renderer.
    [[nodiscard]] const VideoCore::RendererBase& Renderer() const {
        return *renderer;
    }

    /// Returns a reference to the shader notifier.
    [[nodiscard]] VideoCore::ShaderNotify& ShaderNotify() {
        return *shader_notify;
    }

    /// Returns a const reference to the shader notifier.
    [[nodiscard]] const VideoCore::ShaderNotify& ShaderNotify() const {
        return *shader_notify;
    }

    [[nodiscard]] u64 GetTicks() const {
        // This values were reversed engineered by fincs from NVN
        // The gpu clock is reported in units of 385/625 nanoseconds
        constexpr u64 gpu_ticks_num = 384;
        constexpr u64 gpu_ticks_den = 625;

        u64 nanoseconds = system.CoreTiming().GetGlobalTimeNs().count();
        if (Settings::values.use_fast_gpu_time.GetValue()) {
            nanoseconds /= 256;
        }
        const u64 nanoseconds_num = nanoseconds / gpu_ticks_den;
        const u64 nanoseconds_rem = nanoseconds % gpu_ticks_den;
        return nanoseconds_num * gpu_ticks_num + (nanoseconds_rem * gpu_ticks_num) / gpu_ticks_den;
    }

    [[nodiscard]] bool IsAsync() const {
        return is_async;
    }

    [[nodiscard]] bool UseNvdec() const {
        return use_nvdec;
    }

    void RendererFrameEndNotify() {
        system.GetPerfStats().EndGameFrame();
    }

    /// Performs any additional setup necessary in order to begin GPU emulation.
    /// This can be used to launch any necessary threads and register any necessary
    /// core timing events.
    void Start() {
        gpu_thread.StartThread(*renderer, renderer->Context(), *scheduler);
    }

    void NotifyShutdown() {
        std::unique_lock lk{sync_mutex};
        shutting_down.store(true, std::memory_order::relaxed);
        sync_cv.notify_all();
    }

    /// Obtain the CPU Context
    void ObtainContext() {
        if (!cpu_context) {
            cpu_context = renderer->GetRenderWindow().CreateSharedContext();
        }
        cpu_context->MakeCurrent();
    }

    /// Release the CPU Context
    void ReleaseContext() {
        cpu_context->DoneCurrent();
    }

    /// Push GPU command entries to be processed
    void PushGPUEntries(s32 channel, Tegra::CommandList&& entries) {
        gpu_thread.SubmitList(channel, std::move(entries));
    }

    /// Push GPU command buffer entries to be processed
    void PushCommandBuffer(u32 id, Tegra::ChCommandHeaderList& entries) {
        if (!use_nvdec) {
            return;
        }

        if (!cdma_pushers.contains(id)) {
            cdma_pushers.insert_or_assign(id, std::make_unique<Tegra::CDmaPusher>(host1x));
        }

        // SubmitCommandBuffer would make the nvdec operations async, this is not currently working
        // TODO(ameerj): RE proper async nvdec operation
        // gpu_thread.SubmitCommandBuffer(std::move(entries));
        cdma_pushers[id]->ProcessEntries(std::move(entries));
    }

    /// Frees the CDMAPusher instance to free up resources
    void ClearCdmaInstance(u32 id) {
        const auto iter = cdma_pushers.find(id);
        if (iter != cdma_pushers.end()) {
            cdma_pushers.erase(iter);
        }
    }

    /// Swap buffers (render frame)
    void SwapBuffers(const Tegra::FramebufferConfig* framebuffer) {
        gpu_thread.SwapBuffers(framebuffer);
    }

    /// Notify rasterizer that any caches of the specified region should be flushed to Switch memory
    void FlushRegion(VAddr addr, u64 size) {
        gpu_thread.FlushRegion(addr, size);
    }

    /// Notify rasterizer that any caches of the specified region should be invalidated
    void InvalidateRegion(VAddr addr, u64 size) {
        gpu_thread.InvalidateRegion(addr, size);
    }

    /// Notify rasterizer that any caches of the specified region should be flushed and invalidated
    void FlushAndInvalidateRegion(VAddr addr, u64 size) {
        gpu_thread.FlushAndInvalidateRegion(addr, size);
    }

    void RequestSwapBuffers(const Tegra::FramebufferConfig* framebuffer,
                            std::array<Service::Nvidia::NvFence, 4>& fences, size_t num_fences) {
        size_t current_request_counter{};
        {
            std::unique_lock<std::mutex> lk(request_swap_mutex);
            if (free_swap_counters.empty()) {
                current_request_counter = request_swap_counters.size();
                request_swap_counters.emplace_back(num_fences);
            } else {
                current_request_counter = free_swap_counters.front();
                request_swap_counters[current_request_counter] = num_fences;
                free_swap_counters.pop_front();
            }
        }
        const auto wait_fence =
            RequestSyncOperation([this, current_request_counter, framebuffer, fences, num_fences] {
                auto& syncpoint_manager = host1x.GetSyncpointManager();
                if (num_fences == 0) {
                    renderer->SwapBuffers(framebuffer);
                }
                const auto executer = [this, current_request_counter,
                                       framebuffer_copy = *framebuffer]() {
                    {
                        std::unique_lock<std::mutex> lk(request_swap_mutex);
                        if (--request_swap_counters[current_request_counter] != 0) {
                            return;
                        }
                        free_swap_counters.push_back(current_request_counter);
                    }
                    renderer->SwapBuffers(&framebuffer_copy);
                };
                for (size_t i = 0; i < num_fences; i++) {
                    syncpoint_manager.RegisterGuestAction(fences[i].id, fences[i].value, executer);
                }
            });
        gpu_thread.TickGPU();
        WaitForSyncOperation(wait_fence);
    }

    GPU& gpu;
    Core::System& system;
    Host1x::Host1x& host1x;

    std::map<u32, std::unique_ptr<Tegra::CDmaPusher>> cdma_pushers;
    std::unique_ptr<VideoCore::RendererBase> renderer;
    VideoCore::RasterizerInterface* rasterizer = nullptr;
    const bool use_nvdec;

    s32 new_channel_id{1};
    /// Shader build notifier
    std::unique_ptr<VideoCore::ShaderNotify> shader_notify;
    /// When true, we are about to shut down emulation session, so terminate outstanding tasks
    std::atomic_bool shutting_down{};

    std::array<std::atomic<u32>, Service::Nvidia::MaxSyncPoints> syncpoints{};

    std::array<std::list<u32>, Service::Nvidia::MaxSyncPoints> syncpt_interrupts;

    std::mutex sync_mutex;
    std::mutex device_mutex;

    std::condition_variable sync_cv;

    std::list<std::function<void()>> sync_requests;
    std::atomic<u64> current_sync_fence{};
    u64 last_sync_fence{};
    std::mutex sync_request_mutex;
    std::condition_variable sync_request_cv;

    const bool is_async;

    VideoCommon::GPUThread::ThreadManager gpu_thread;
    std::unique_ptr<Core::Frontend::GraphicsContext> cpu_context;

    std::unique_ptr<Tegra::Control::Scheduler> scheduler;
    std::unordered_map<s32, std::shared_ptr<Tegra::Control::ChannelState>> channels;
    Tegra::Control::ChannelState* current_channel;
    s32 bound_channel{-1};

    std::deque<size_t> free_swap_counters;
    std::deque<size_t> request_swap_counters;
    std::mutex request_swap_mutex;
};

GPU::GPU(Core::System& system, bool is_async, bool use_nvdec)
    : impl{std::make_unique<Impl>(*this, system, is_async, use_nvdec)} {}

GPU::~GPU() = default;

std::shared_ptr<Control::ChannelState> GPU::AllocateChannel() {
    return impl->AllocateChannel();
}

void GPU::InitChannel(Control::ChannelState& to_init) {
    impl->InitChannel(to_init);
}

void GPU::BindChannel(s32 channel_id) {
    impl->BindChannel(channel_id);
}

void GPU::ReleaseChannel(Control::ChannelState& to_release) {
    impl->ReleaseChannel(to_release);
}

void GPU::InitAddressSpace(Tegra::MemoryManager& memory_manager) {
    impl->InitAddressSpace(memory_manager);
}

void GPU::BindRenderer(std::unique_ptr<VideoCore::RendererBase> renderer) {
    impl->BindRenderer(std::move(renderer));
}

void GPU::FlushCommands() {
    impl->FlushCommands();
}

void GPU::InvalidateGPUCache() {
    impl->InvalidateGPUCache();
}

void GPU::OnCommandListEnd() {
    impl->OnCommandListEnd();
}

u64 GPU::RequestFlush(VAddr addr, std::size_t size) {
    return impl->RequestSyncOperation(
        [this, addr, size]() { impl->rasterizer->FlushRegion(addr, size); });
}

u64 GPU::CurrentSyncRequestFence() const {
    return impl->CurrentSyncRequestFence();
}

void GPU::WaitForSyncOperation(u64 fence) {
    return impl->WaitForSyncOperation(fence);
}

void GPU::TickWork() {
    impl->TickWork();
}

/// Gets a mutable reference to the Host1x interface
Host1x::Host1x& GPU::Host1x() {
    return impl->host1x;
}

/// Gets an immutable reference to the Host1x interface.
const Host1x::Host1x& GPU::Host1x() const {
    return impl->host1x;
}

Engines::Maxwell3D& GPU::Maxwell3D() {
    return impl->Maxwell3D();
}

const Engines::Maxwell3D& GPU::Maxwell3D() const {
    return impl->Maxwell3D();
}

Engines::KeplerCompute& GPU::KeplerCompute() {
    return impl->KeplerCompute();
}

const Engines::KeplerCompute& GPU::KeplerCompute() const {
    return impl->KeplerCompute();
}

Tegra::DmaPusher& GPU::DmaPusher() {
    return impl->DmaPusher();
}

const Tegra::DmaPusher& GPU::DmaPusher() const {
    return impl->DmaPusher();
}

VideoCore::RendererBase& GPU::Renderer() {
    return impl->Renderer();
}

const VideoCore::RendererBase& GPU::Renderer() const {
    return impl->Renderer();
}

VideoCore::ShaderNotify& GPU::ShaderNotify() {
    return impl->ShaderNotify();
}

const VideoCore::ShaderNotify& GPU::ShaderNotify() const {
    return impl->ShaderNotify();
}

void GPU::RequestSwapBuffers(const Tegra::FramebufferConfig* framebuffer,
                             std::array<Service::Nvidia::NvFence, 4>& fences, size_t num_fences) {
    impl->RequestSwapBuffers(framebuffer, fences, num_fences);
}

u64 GPU::GetTicks() const {
    return impl->GetTicks();
}

bool GPU::IsAsync() const {
    return impl->IsAsync();
}

bool GPU::UseNvdec() const {
    return impl->UseNvdec();
}

void GPU::RendererFrameEndNotify() {
    impl->RendererFrameEndNotify();
}

void GPU::Start() {
    impl->Start();
}

void GPU::NotifyShutdown() {
    impl->NotifyShutdown();
}

void GPU::ObtainContext() {
    impl->ObtainContext();
}

void GPU::ReleaseContext() {
    impl->ReleaseContext();
}

void GPU::PushGPUEntries(s32 channel, Tegra::CommandList&& entries) {
    impl->PushGPUEntries(channel, std::move(entries));
}

void GPU::PushCommandBuffer(u32 id, Tegra::ChCommandHeaderList& entries) {
    impl->PushCommandBuffer(id, entries);
}

void GPU::ClearCdmaInstance(u32 id) {
    impl->ClearCdmaInstance(id);
}

void GPU::SwapBuffers(const Tegra::FramebufferConfig* framebuffer) {
    impl->SwapBuffers(framebuffer);
}

void GPU::FlushRegion(VAddr addr, u64 size) {
    impl->FlushRegion(addr, size);
}

void GPU::InvalidateRegion(VAddr addr, u64 size) {
    impl->InvalidateRegion(addr, size);
}

void GPU::FlushAndInvalidateRegion(VAddr addr, u64 size) {
    impl->FlushAndInvalidateRegion(addr, size);
}

} // namespace Tegra

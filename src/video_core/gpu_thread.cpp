// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/microprofile.h"
#include "common/scope_exit.h"
#include "common/settings.h"
#include "common/thread.h"
#include "core/core.h"
#include "core/frontend/emu_window.h"
#include "video_core/dma_pusher.h"
#include "video_core/gpu.h"
#include "video_core/gpu_thread.h"
#include "video_core/renderer_base.h"

namespace VideoCommon::GPUThread {

/// Runs the GPU thread
static void RunThread(Core::System& system, VideoCore::RendererBase& renderer,
                      Core::Frontend::GraphicsContext& context, Tegra::DmaPusher& dma_pusher,
                      SynchState& state) {
    std::string name = "yuzu:GPU";
    MicroProfileOnThreadCreate(name.c_str());
    SCOPE_EXIT({ MicroProfileOnThreadExit(); });

    Common::SetCurrentThreadName(name.c_str());
    Common::SetCurrentThreadPriority(Common::ThreadPriority::High);
    system.RegisterHostThread();

    // Wait for first GPU command before acquiring the window context
    state.queue.Wait();

    // If emulation was stopped during disk shader loading, abort before trying to acquire context
    if (!state.is_running) {
        return;
    }

    auto current_context = context.Acquire();
    VideoCore::RasterizerInterface* const rasterizer = renderer.ReadRasterizer();

    CommandDataContainer next;
    while (state.is_running) {
        next = state.queue.PopWait();
        if (auto* submit_list = std::get_if<SubmitListCommand>(&next.data)) {
            dma_pusher.Push(std::move(submit_list->entries));
            dma_pusher.DispatchCalls();
        } else if (const auto* data = std::get_if<SwapBuffersCommand>(&next.data)) {
            renderer.SwapBuffers(data->framebuffer ? &*data->framebuffer : nullptr);
        } else if (std::holds_alternative<OnCommandListEndCommand>(next.data)) {
            rasterizer->ReleaseFences();
        } else if (std::holds_alternative<GPUTickCommand>(next.data)) {
            system.GPU().TickWork();
        } else if (const auto* flush = std::get_if<FlushRegionCommand>(&next.data)) {
            rasterizer->FlushRegion(flush->addr, flush->size);
        } else if (const auto* invalidate = std::get_if<InvalidateRegionCommand>(&next.data)) {
            rasterizer->OnCPUWrite(invalidate->addr, invalidate->size);
        } else if (std::holds_alternative<EndProcessingCommand>(next.data)) {
            ASSERT(state.is_running == false);
        } else {
            UNREACHABLE();
        }
        state.signaled_fence.store(next.fence);
        if (next.block) {
            // We have to lock the write_lock to ensure that the condition_variable wait not get a
            // race between the check and the lock itself.
            std::lock_guard lk(state.write_lock);
            state.cv.notify_all();
        }
    }
}

ThreadManager::ThreadManager(Core::System& system_, bool is_async_)
    : system{system_}, is_async{is_async_} {}

ThreadManager::~ThreadManager() {
    ShutDown();
}

void ThreadManager::StartThread(VideoCore::RendererBase& renderer,
                                Core::Frontend::GraphicsContext& context,
                                Tegra::DmaPusher& dma_pusher) {
    rasterizer = renderer.ReadRasterizer();
    thread = std::thread(RunThread, std::ref(system), std::ref(renderer), std::ref(context),
                         std::ref(dma_pusher), std::ref(state));
}

void ThreadManager::SubmitList(Tegra::CommandList&& entries) {
    PushCommand(SubmitListCommand(std::move(entries)));
}

void ThreadManager::SwapBuffers(const Tegra::FramebufferConfig* framebuffer) {
    PushCommand(SwapBuffersCommand(framebuffer ? std::make_optional(*framebuffer) : std::nullopt));
}

void ThreadManager::FlushRegion(VAddr addr, u64 size) {
    if (!is_async) {
        // Always flush with synchronous GPU mode
        PushCommand(FlushRegionCommand(addr, size));
        return;
    }
    if (!Settings::IsGPULevelExtreme()) {
        return;
    }
    auto& gpu = system.GPU();
    u64 fence = gpu.RequestFlush(addr, size);
    PushCommand(GPUTickCommand(), true);
    ASSERT(fence <= gpu.CurrentWorkRequestFence());
}

void ThreadManager::InvalidateRegion(VAddr addr, u64 size) {
    rasterizer->OnCPUWrite(addr, size);
}

void ThreadManager::FlushAndInvalidateRegion(VAddr addr, u64 size) {
    // Skip flush on asynch mode, as FlushAndInvalidateRegion is not used for anything too important
    rasterizer->OnCPUWrite(addr, size);
}

void ThreadManager::ShutDown() {
    if (!state.is_running) {
        return;
    }

    {
        std::lock_guard lk(state.write_lock);
        state.is_running = false;
        state.cv.notify_all();
    }

    if (!thread.joinable()) {
        return;
    }

    // Notify GPU thread that a shutdown is pending
    PushCommand(EndProcessingCommand());
    thread.join();
}

void ThreadManager::OnCommandListEnd() {
    PushCommand(OnCommandListEndCommand());
}

u64 ThreadManager::PushCommand(CommandData&& command_data, bool block) {
    if (!is_async) {
        // In synchronous GPU mode, block the caller until the command has executed
        block = true;
    }

    std::unique_lock lk(state.write_lock);
    const u64 fence{++state.last_fence};
    state.queue.Push(CommandDataContainer(std::move(command_data), fence, block));

    if (block) {
        state.cv.wait(lk, [this, fence] {
            return fence <= state.signaled_fence.load(std::memory_order_relaxed) ||
                   !state.is_running;
        });
    }

    return fence;
}

} // namespace VideoCommon::GPUThread

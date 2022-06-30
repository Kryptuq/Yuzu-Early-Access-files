// SPDX-FileCopyrightText: 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core.h"
#include "video_core/control/channel_state.h"
#include "video_core/dma_pusher.h"
#include "video_core/engines/fermi_2d.h"
#include "video_core/engines/kepler_compute.h"
#include "video_core/engines/kepler_memory.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/maxwell_dma.h"
#include "video_core/engines/puller.h"
#include "video_core/gpu.h"
#include "video_core/memory_manager.h"
#include "video_core/rasterizer_interface.h"

namespace Tegra::Engines {

Puller::Puller(GPU& gpu_, MemoryManager& memory_manager_, DmaPusher& dma_pusher_,
               Control::ChannelState& channel_state_)
    : gpu{gpu_}, memory_manager{memory_manager_}, dma_pusher{dma_pusher_}, channel_state{
                                                                               channel_state_} {}

Puller::~Puller() = default;

void Puller::ProcessBindMethod(const MethodCall& method_call) {
    // Bind the current subchannel to the desired engine id.
    LOG_DEBUG(HW_GPU, "Binding subchannel {} to engine {}", method_call.subchannel,
              method_call.argument);
    const auto engine_id = static_cast<EngineID>(method_call.argument);
    bound_engines[method_call.subchannel] = static_cast<EngineID>(engine_id);
    switch (engine_id) {
    case EngineID::FERMI_TWOD_A:
        dma_pusher.BindSubchannel(channel_state.fermi_2d.get(), method_call.subchannel);
        break;
    case EngineID::MAXWELL_B:
        dma_pusher.BindSubchannel(channel_state.maxwell_3d.get(), method_call.subchannel);
        break;
    case EngineID::KEPLER_COMPUTE_B:
        dma_pusher.BindSubchannel(channel_state.kepler_compute.get(), method_call.subchannel);
        break;
    case EngineID::MAXWELL_DMA_COPY_A:
        dma_pusher.BindSubchannel(channel_state.maxwell_dma.get(), method_call.subchannel);
        break;
    case EngineID::KEPLER_INLINE_TO_MEMORY_B:
        dma_pusher.BindSubchannel(channel_state.kepler_memory.get(), method_call.subchannel);
        break;
    default:
        UNIMPLEMENTED_MSG("Unimplemented engine {:04X}", engine_id);
    }
}

void Puller::ProcessFenceActionMethod() {
    switch (regs.fence_action.op) {
    case Puller::FenceOperation::Acquire:
        // UNIMPLEMENTED_MSG("Channel Scheduling pending.");
        // WaitFence(regs.fence_action.syncpoint_id, regs.fence_value);
        rasterizer->ReleaseFences();
        break;
    case Puller::FenceOperation::Increment:
        rasterizer->SignalSyncPoint(regs.fence_action.syncpoint_id);
        break;
    default:
        UNIMPLEMENTED_MSG("Unimplemented operation {}", regs.fence_action.op.Value());
    }
}

void Puller::ProcessSemaphoreTriggerMethod() {
    const auto semaphoreOperationMask = 0xF;
    const auto op =
        static_cast<GpuSemaphoreOperation>(regs.semaphore_trigger & semaphoreOperationMask);
    if (op == GpuSemaphoreOperation::WriteLong) {
        struct Block {
            u32 sequence;
            u32 zeros = 0;
            u64 timestamp;
        };

        const GPUVAddr sequence_address{regs.semaphore_address.SemaphoreAddress()};
        const u32 payload = regs.semaphore_sequence;
        std::function<void()> operation([this, sequence_address, payload] {
            Block block{};
            block.sequence = payload;
            block.timestamp = gpu.GetTicks();
            memory_manager.WriteBlockUnsafe(sequence_address, &block, sizeof(block));
        });
        rasterizer->SyncOperation(std::move(operation));
    } else {
        do {
            const u32 word{memory_manager.Read<u32>(regs.semaphore_address.SemaphoreAddress())};
            regs.acquire_source = true;
            regs.acquire_value = regs.semaphore_sequence;
            if (op == GpuSemaphoreOperation::AcquireEqual) {
                regs.acquire_active = true;
                regs.acquire_mode = false;
                if (word != regs.acquire_value) {
                    rasterizer->ReleaseFences();
                    continue;
                }
            } else if (op == GpuSemaphoreOperation::AcquireGequal) {
                regs.acquire_active = true;
                regs.acquire_mode = true;
                if (word < regs.acquire_value) {
                    rasterizer->ReleaseFences();
                    continue;
                }
            } else if (op == GpuSemaphoreOperation::AcquireMask) {
                if (word && regs.semaphore_sequence == 0) {
                    rasterizer->ReleaseFences();
                    continue;
                }
            } else {
                LOG_ERROR(HW_GPU, "Invalid semaphore operation");
            }
        } while (false);
    }
}

void Puller::ProcessSemaphoreRelease() {
    const GPUVAddr sequence_address{regs.semaphore_address.SemaphoreAddress()};
    const u32 payload = regs.semaphore_release;
    std::function<void()> operation([this, sequence_address, payload] {
        memory_manager.Write<u32>(sequence_address, payload);
    });
    rasterizer->SyncOperation(std::move(operation));
}

void Puller::ProcessSemaphoreAcquire() {
    u32 word = memory_manager.Read<u32>(regs.semaphore_address.SemaphoreAddress());
    const auto value = regs.semaphore_acquire;
    while (word != value) {
        regs.acquire_active = true;
        regs.acquire_value = value;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        rasterizer->ReleaseFences();
        word = memory_manager.Read<u32>(regs.semaphore_address.SemaphoreAddress());
        // TODO(kemathe73) figure out how to do the acquire_timeout
        regs.acquire_mode = false;
        regs.acquire_source = false;
    }
}

/// Calls a GPU puller method.
void Puller::CallPullerMethod(const MethodCall& method_call) {
    regs.reg_array[method_call.method] = method_call.argument;
    const auto method = static_cast<BufferMethods>(method_call.method);

    switch (method) {
    case BufferMethods::BindObject: {
        ProcessBindMethod(method_call);
        break;
    }
    case BufferMethods::Nop:
    case BufferMethods::SemaphoreAddressHigh:
    case BufferMethods::SemaphoreAddressLow:
    case BufferMethods::SemaphoreSequencePayload:
    case BufferMethods::SyncpointPayload:
        break;
    case BufferMethods::WrcacheFlush:
    case BufferMethods::RefCnt:
        rasterizer->SignalReference();
        break;
    case BufferMethods::SyncpointOperation:
        ProcessFenceActionMethod();
        break;
    case BufferMethods::WaitForIdle:
        rasterizer->WaitForIdle();
        break;
    case BufferMethods::SemaphoreOperation: {
        ProcessSemaphoreTriggerMethod();
        break;
    }
    case BufferMethods::NonStallInterrupt: {
        LOG_ERROR(HW_GPU, "Special puller engine method NonStallInterrupt not implemented");
        break;
    }
    case BufferMethods::MemOpA: {
        LOG_ERROR(HW_GPU, "Memory Operation A");
        break;
    }
    case BufferMethods::MemOpB: {
        // Implement this better.
        rasterizer->InvalidateGPUCache();
        break;
    }
    case BufferMethods::MemOpC:
    case BufferMethods::MemOpD: {
        LOG_ERROR(HW_GPU, "Memory Operation C,D");
        break;
    }
    case BufferMethods::SemaphoreAcquire: {
        ProcessSemaphoreAcquire();
        break;
    }
    case BufferMethods::SemaphoreRelease: {
        ProcessSemaphoreRelease();
        break;
    }
    case BufferMethods::Yield: {
        // TODO(Kmather73): Research and implement this method.
        LOG_ERROR(HW_GPU, "Special puller engine method Yield not implemented");
        break;
    }
    default:
        LOG_ERROR(HW_GPU, "Special puller engine method {:X} not implemented", method);
        break;
    }
}

/// Calls a GPU engine method.
void Puller::CallEngineMethod(const MethodCall& method_call) {
    const EngineID engine = bound_engines[method_call.subchannel];

    switch (engine) {
    case EngineID::FERMI_TWOD_A:
        channel_state.fermi_2d->CallMethod(method_call.method, method_call.argument,
                                           method_call.IsLastCall());
        break;
    case EngineID::MAXWELL_B:
        channel_state.maxwell_3d->CallMethod(method_call.method, method_call.argument,
                                             method_call.IsLastCall());
        break;
    case EngineID::KEPLER_COMPUTE_B:
        channel_state.kepler_compute->CallMethod(method_call.method, method_call.argument,
                                                 method_call.IsLastCall());
        break;
    case EngineID::MAXWELL_DMA_COPY_A:
        channel_state.maxwell_dma->CallMethod(method_call.method, method_call.argument,
                                              method_call.IsLastCall());
        break;
    case EngineID::KEPLER_INLINE_TO_MEMORY_B:
        channel_state.kepler_memory->CallMethod(method_call.method, method_call.argument,
                                                method_call.IsLastCall());
        break;
    default:
        UNIMPLEMENTED_MSG("Unimplemented engine");
    }
}

/// Calls a GPU engine multivalue method.
void Puller::CallEngineMultiMethod(u32 method, u32 subchannel, const u32* base_start, u32 amount,
                                   u32 methods_pending) {
    const EngineID engine = bound_engines[subchannel];

    switch (engine) {
    case EngineID::FERMI_TWOD_A:
        channel_state.fermi_2d->CallMultiMethod(method, base_start, amount, methods_pending);
        break;
    case EngineID::MAXWELL_B:
        channel_state.maxwell_3d->CallMultiMethod(method, base_start, amount, methods_pending);
        break;
    case EngineID::KEPLER_COMPUTE_B:
        channel_state.kepler_compute->CallMultiMethod(method, base_start, amount, methods_pending);
        break;
    case EngineID::MAXWELL_DMA_COPY_A:
        channel_state.maxwell_dma->CallMultiMethod(method, base_start, amount, methods_pending);
        break;
    case EngineID::KEPLER_INLINE_TO_MEMORY_B:
        channel_state.kepler_memory->CallMultiMethod(method, base_start, amount, methods_pending);
        break;
    default:
        UNIMPLEMENTED_MSG("Unimplemented engine");
    }
}

/// Calls a GPU method.
void Puller::CallMethod(const MethodCall& method_call) {
    LOG_TRACE(HW_GPU, "Processing method {:08X} on subchannel {}", method_call.method,
              method_call.subchannel);

    ASSERT(method_call.subchannel < bound_engines.size());

    if (ExecuteMethodOnEngine(method_call.method)) {
        CallEngineMethod(method_call);
    } else {
        CallPullerMethod(method_call);
    }
}

/// Calls a GPU multivalue method.
void Puller::CallMultiMethod(u32 method, u32 subchannel, const u32* base_start, u32 amount,
                             u32 methods_pending) {
    LOG_TRACE(HW_GPU, "Processing method {:08X} on subchannel {}", method, subchannel);

    ASSERT(subchannel < bound_engines.size());

    if (ExecuteMethodOnEngine(method)) {
        CallEngineMultiMethod(method, subchannel, base_start, amount, methods_pending);
    } else {
        for (std::size_t i = 0; i < amount; i++) {
            CallPullerMethod(MethodCall{
                method,
                base_start[i],
                subchannel,
                methods_pending - static_cast<u32>(i),
            });
        }
    }
}

void Puller::BindRasterizer(VideoCore::RasterizerInterface* rasterizer_) {
    rasterizer = rasterizer_;
}

/// Determines where the method should be executed.
[[nodiscard]] bool Puller::ExecuteMethodOnEngine(u32 method) {
    const auto buffer_method = static_cast<BufferMethods>(method);
    return buffer_method >= BufferMethods::NonPullerMethods;
}

} // namespace Tegra::Engines

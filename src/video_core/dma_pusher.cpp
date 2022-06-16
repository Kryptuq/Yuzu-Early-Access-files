// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/cityhash.h"
#include "common/microprofile.h"
#include "common/settings.h"
#include "core/core.h"
#include "video_core/dma_pusher.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/gpu.h"
#include "video_core/memory_manager.h"

namespace Tegra {

DmaPusher::DmaPusher(Core::System& system_, GPU& gpu_, MemoryManager& memory_manager_,
                     Control::ChannelState& channel_state_)
    : gpu{gpu_}, system{system_}, memory_manager{memory_manager_}, puller{gpu_, memory_manager_,
                                                                          *this, channel_state_} {}

DmaPusher::~DmaPusher() = default;

MICROPROFILE_DEFINE(DispatchCalls, "GPU", "Execute command buffer", MP_RGB(128, 128, 192));

void DmaPusher::DispatchCalls() {
    MICROPROFILE_SCOPE(DispatchCalls);

    dma_pushbuffer_subindex = 0;

    dma_state.is_last_call = true;

    while (system.IsPoweredOn()) {
        if (!Step()) {
            break;
        }
    }
    gpu.FlushCommands();
    gpu.OnCommandListEnd();
}

bool DmaPusher::Step() {
    if (!ib_enable || dma_pushbuffer.empty()) {
        // pushbuffer empty and IB empty or nonexistent - nothing to do
        return false;
    }

    CommandList& command_list{dma_pushbuffer.front()};

    ASSERT_OR_EXECUTE(
        command_list.command_lists.size() || command_list.prefetch_command_list.size(), {
            // Somehow the command_list is empty, in order to avoid a crash
            // We ignore it and assume its size is 0.
            dma_pushbuffer.pop();
            dma_pushbuffer_subindex = 0;
            return true;
        });

    if (command_list.prefetch_command_list.size()) {
        // Prefetched command list from nvdrv, used for things like synchronization
        command_headers = std::move(command_list.prefetch_command_list);
        dma_pushbuffer.pop();
    } else {
        const CommandListHeader command_list_header{
            command_list.command_lists[dma_pushbuffer_subindex++]};
        const GPUVAddr dma_get = command_list_header.addr;

        if (dma_pushbuffer_subindex >= command_list.command_lists.size()) {
            // We've gone through the current list, remove it from the queue
            dma_pushbuffer.pop();
            dma_pushbuffer_subindex = 0;
        }

        if (command_list_header.size == 0) {
            return true;
        }

        // Push buffer non-empty, read a word
        command_headers.resize(command_list_header.size);
        if (Settings::IsGPULevelHigh()) {
            memory_manager.ReadBlock(dma_get, command_headers.data(),
                                     command_list_header.size * sizeof(u32));
        } else {
            memory_manager.ReadBlockUnsafe(dma_get, command_headers.data(),
                                           command_list_header.size * sizeof(u32));
        }
    }
    for (std::size_t index = 0; index < command_headers.size();) {
        const CommandHeader& command_header = command_headers[index];

        if (dma_state.method_count) {
            // Data word of methods command
            if (dma_state.non_incrementing) {
                const u32 max_write = static_cast<u32>(
                    std::min<std::size_t>(index + dma_state.method_count, command_headers.size()) -
                    index);
                CallMultiMethod(&command_header.argument, max_write);
                dma_state.method_count -= max_write;
                dma_state.is_last_call = true;
                index += max_write;
                continue;
            } else {
                dma_state.is_last_call = dma_state.method_count <= 1;
                CallMethod(command_header.argument);
            }

            if (!dma_state.non_incrementing) {
                dma_state.method++;
            }

            if (dma_increment_once) {
                dma_state.non_incrementing = true;
            }

            dma_state.method_count--;
        } else {
            // No command active - this is the first word of a new one
            switch (command_header.mode) {
            case SubmissionMode::Increasing:
                SetState(command_header);
                dma_state.non_incrementing = false;
                dma_increment_once = false;
                break;
            case SubmissionMode::NonIncreasing:
                SetState(command_header);
                dma_state.non_incrementing = true;
                dma_increment_once = false;
                break;
            case SubmissionMode::Inline:
                dma_state.method = command_header.method;
                dma_state.subchannel = command_header.subchannel;
                CallMethod(command_header.arg_count);
                dma_state.non_incrementing = true;
                dma_increment_once = false;
                break;
            case SubmissionMode::IncreaseOnce:
                SetState(command_header);
                dma_state.non_incrementing = false;
                dma_increment_once = true;
                break;
            default:
                break;
            }
        }
        index++;
    }

    return true;
}

void DmaPusher::SetState(const CommandHeader& command_header) {
    dma_state.method = command_header.method;
    dma_state.subchannel = command_header.subchannel;
    dma_state.method_count = command_header.method_count;
}

void DmaPusher::CallMethod(u32 argument) const {
    if (dma_state.method < non_puller_methods) {
        puller.CallPullerMethod(Engines::Puller::MethodCall{
            dma_state.method,
            argument,
            dma_state.subchannel,
            dma_state.method_count,
        });
    } else {
        subchannels[dma_state.subchannel]->CallMethod(dma_state.method, argument,
                                                      dma_state.is_last_call);
    }
}

void DmaPusher::CallMultiMethod(const u32* base_start, u32 num_methods) const {
    if (dma_state.method < non_puller_methods) {
        puller.CallMultiMethod(dma_state.method, dma_state.subchannel, base_start, num_methods,
                               dma_state.method_count);
    } else {
        subchannels[dma_state.subchannel]->CallMultiMethod(dma_state.method, base_start,
                                                           num_methods, dma_state.method_count);
    }
}

void DmaPusher::BindRasterizer(VideoCore::RasterizerInterface* rasterizer) {
    puller.BindRasterizer(rasterizer);
}

} // namespace Tegra

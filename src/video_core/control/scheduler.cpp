// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv3 or any later version
// Refer to the license.txt file included.

#include <memory>

#include "video_core/control/channel_state.h"
#include "video_core/control/scheduler.h"
#include "video_core/gpu.h"

namespace Tegra::Control {
Scheduler::Scheduler(GPU& gpu_) : gpu{gpu_} {}

Scheduler::~Scheduler() = default;

void Scheduler::Push(s32 channel, CommandList&& entries) {
    std::unique_lock<std::mutex> lk(scheduling_guard);
    auto it = channels.find(channel);
    auto channel_state = it->second;
    gpu.BindChannel(channel_state->bind_id);
    channel_state->dma_pusher->Push(std::move(entries));
    channel_state->dma_pusher->DispatchCalls();
}

void Scheduler::DeclareChannel(std::shared_ptr<ChannelState> new_channel) {
    s32 channel = new_channel->bind_id;
    std::unique_lock<std::mutex> lk(scheduling_guard);
    channels.emplace(channel, new_channel);
}

} // namespace Tegra::Control

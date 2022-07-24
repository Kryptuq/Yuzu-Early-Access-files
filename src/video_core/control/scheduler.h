// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

#include "video_core/dma_pusher.h"

namespace Tegra {

class GPU;

namespace Control {

struct ChannelState;

class Scheduler {
public:
    Scheduler(GPU& gpu_);
    ~Scheduler();

    void Push(s32 channel, CommandList&& entries);

    void DeclareChannel(std::shared_ptr<ChannelState> new_channel);

private:
    std::unordered_map<s32, std::shared_ptr<ChannelState>> channels;
    std::mutex scheduling_guard;
    GPU& gpu;
};

} // namespace Control

} // namespace Tegra

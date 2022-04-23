// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "video_core/renderer_vulkan/vk_buffer_cache.h"
#include "video_core/renderer_vulkan/vk_fence_manager.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_texture_cache.h"
#include "video_core/vulkan_common/vulkan_device.h"

namespace Vulkan {

InnerFence::InnerFence(VKScheduler& scheduler_, u32 payload_, bool is_stubbed_)
    : FenceBase{payload_, is_stubbed_}, scheduler{scheduler_} {}

InnerFence::InnerFence(VKScheduler& scheduler_, GPUVAddr address_, u32 payload_, bool is_stubbed_)
    : FenceBase{address_, payload_, is_stubbed_}, scheduler{scheduler_} {}

InnerFence::~InnerFence() = default;

void InnerFence::Queue() {
    if (is_stubbed) {
        return;
    }
    // Get the current tick so we can wait for it
    wait_tick = scheduler.CurrentTick();
    scheduler.Flush();
}

bool InnerFence::IsSignaled() const {
    if (is_stubbed) {
        return true;
    }
    return scheduler.IsFree(wait_tick);
}

void InnerFence::Wait() {
    if (is_stubbed) {
        return;
    }
    scheduler.Wait(wait_tick);
}

VKFenceManager::VKFenceManager(VideoCore::RasterizerInterface& rasterizer_, Tegra::GPU& gpu_,
                               TextureCache& texture_cache_, BufferCache& buffer_cache_,
                               VKQueryCache& query_cache_, const Device& device_,
                               VKScheduler& scheduler_)
    : GenericFenceManager{rasterizer_, gpu_, texture_cache_, buffer_cache_, query_cache_},
      scheduler{scheduler_} {}

Fence VKFenceManager::CreateFence(u32 value, bool is_stubbed) {
    return std::make_shared<InnerFence>(scheduler, value, is_stubbed);
}

Fence VKFenceManager::CreateFence(GPUVAddr addr, u32 value, bool is_stubbed) {
    return std::make_shared<InnerFence>(scheduler, addr, value, is_stubbed);
}

void VKFenceManager::QueueFence(Fence& fence) {
    fence->Queue();
}

bool VKFenceManager::IsFenceSignaled(Fence& fence) const {
    return fence->IsSignaled();
}

void VKFenceManager::WaitFence(Fence& fence) {
    fence->Wait();
}

} // namespace Vulkan

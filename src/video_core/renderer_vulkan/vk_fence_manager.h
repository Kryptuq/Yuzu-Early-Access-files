// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "video_core/fence_manager.h"
#include "video_core/renderer_vulkan/vk_buffer_cache.h"
#include "video_core/renderer_vulkan/vk_texture_cache.h"

namespace Core {
class System;
}

namespace VideoCore {
class RasterizerInterface;
}

namespace Vulkan {

class Device;
class VKQueryCache;
class VKScheduler;

class InnerFence : public VideoCommon::FenceBase {
public:
    explicit InnerFence(VKScheduler& scheduler_, bool is_stubbed_);
    ~InnerFence();

    void Queue();

    bool IsSignaled() const;

    void Wait();

private:
    VKScheduler& scheduler;
    u64 wait_tick = 0;
};
using Fence = std::shared_ptr<InnerFence>;

using GenericFenceManager =
    VideoCommon::FenceManager<Fence, TextureCache, BufferCache, VKQueryCache>;

class VKFenceManager final : public GenericFenceManager {
public:
    explicit VKFenceManager(VideoCore::RasterizerInterface& rasterizer, Tegra::GPU& gpu,
                            TextureCache& texture_cache, BufferCache& buffer_cache,
                            VKQueryCache& query_cache, const Device& device,
                            VKScheduler& scheduler);

protected:
    Fence CreateFence(bool is_stubbed) override;
    void QueueFence(Fence& fence) override;
    bool IsFenceSignaled(Fence& fence) const override;
    void WaitFence(Fence& fence) override;

private:
    VKScheduler& scheduler;
};

} // namespace Vulkan

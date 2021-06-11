// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "common/common_types.h"
#include "video_core/query_cache.h"
#include "video_core/renderer_vulkan/vk_resource_pool.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace VideoCore {
class RasterizerInterface;
}

namespace Vulkan {

class CachedQuery;
class Device;
class HostCounter;
class VKQueryCache;
class VKScheduler;

using CounterStream = VideoCommon::CounterStreamBase<VKQueryCache, HostCounter>;

class QueryPool final : public ResourcePool {
public:
    explicit QueryPool(const Device& device, VKScheduler& scheduler, VideoCore::QueryType type);
    ~QueryPool() override;

    std::pair<VkQueryPool, u32> Commit();

    void Reserve(std::pair<VkQueryPool, u32> query);

protected:
    void Allocate(std::size_t begin, std::size_t end) override;

private:
    static constexpr std::size_t GROW_STEP = 512;

    const Device& device;
    const VideoCore::QueryType type;

    std::vector<vk::QueryPool> pools;
    std::vector<bool> usage;
};

class VKQueryCache final
    : public VideoCommon::QueryCacheBase<VKQueryCache, CachedQuery, CounterStream, HostCounter> {
public:
    explicit VKQueryCache(VideoCore::RasterizerInterface& rasterizer_,
                          Tegra::Engines::Maxwell3D& maxwell3d_, Tegra::MemoryManager& gpu_memory_,
                          const Device& device_, VKScheduler& scheduler_);
    ~VKQueryCache();

    std::pair<VkQueryPool, u32> AllocateQuery(VideoCore::QueryType type);

    void Reserve(VideoCore::QueryType type, std::pair<VkQueryPool, u32> query);

    const Device& GetDevice() const noexcept {
        return device;
    }

    VKScheduler& GetScheduler() const noexcept {
        return scheduler;
    }

private:
    const Device& device;
    VKScheduler& scheduler;
    std::array<QueryPool, VideoCore::NumQueryTypes> query_pools;
};

class HostCounter final : public VideoCommon::HostCounterBase<VKQueryCache, HostCounter> {
public:
    explicit HostCounter(VKQueryCache& cache_, std::shared_ptr<HostCounter> dependency_,
                         VideoCore::QueryType type_);
    ~HostCounter();

    void EndQuery();

private:
    u64 BlockingQuery() const override;

    VKQueryCache& cache;
    const VideoCore::QueryType type;
    const std::pair<VkQueryPool, u32> query;
    const u64 tick;
};

class CachedQuery : public VideoCommon::CachedQueryBase<HostCounter> {
public:
    explicit CachedQuery(VKQueryCache&, VideoCore::QueryType, VAddr cpu_addr_, u8* host_ptr_)
        : CachedQueryBase{cpu_addr_, host_ptr_} {}
};

} // namespace Vulkan

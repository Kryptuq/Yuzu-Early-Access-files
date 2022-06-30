// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
class QueryCache;
class Scheduler;

using CounterStream = VideoCommon::CounterStreamBase<QueryCache, HostCounter>;

class QueryPool final : public ResourcePool {
public:
    explicit QueryPool(const Device& device, Scheduler& scheduler, VideoCore::QueryType type);
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

class QueryCache final
    : public VideoCommon::QueryCacheBase<QueryCache, CachedQuery, CounterStream, HostCounter> {
public:
    explicit QueryCache(VideoCore::RasterizerInterface& rasterizer_, const Device& device_,
                        Scheduler& scheduler_);
    ~QueryCache();

    std::pair<VkQueryPool, u32> AllocateQuery(VideoCore::QueryType type);

    void Reserve(VideoCore::QueryType type, std::pair<VkQueryPool, u32> query);

    const Device& GetDevice() const noexcept {
        return device;
    }

    Scheduler& GetScheduler() const noexcept {
        return scheduler;
    }

private:
    const Device& device;
    Scheduler& scheduler;
    std::array<QueryPool, VideoCore::NumQueryTypes> query_pools;
};

class HostCounter final : public VideoCommon::HostCounterBase<QueryCache, HostCounter> {
public:
    explicit HostCounter(QueryCache& cache_, std::shared_ptr<HostCounter> dependency_,
                         VideoCore::QueryType type_);
    ~HostCounter();

    void EndQuery();

private:
    u64 BlockingQuery() const override;

    QueryCache& cache;
    const VideoCore::QueryType type;
    const std::pair<VkQueryPool, u32> query;
    const u64 tick;
};

class CachedQuery : public VideoCommon::CachedQueryBase<HostCounter> {
public:
    explicit CachedQuery(QueryCache&, VideoCore::QueryType, VAddr cpu_addr_, u8* host_ptr_)
        : CachedQueryBase{cpu_addr_, host_ptr_} {}
};

} // namespace Vulkan

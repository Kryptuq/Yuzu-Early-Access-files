// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/bit_util.h"
#include "common/common_types.h"
#include "common/literals.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_staging_buffer_pool.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {
namespace {

using namespace Common::Literals;

// Maximum potential alignment of a Vulkan buffer
constexpr VkDeviceSize MAX_ALIGNMENT = 256;
// Maximum size to put elements in the stream buffer
constexpr VkDeviceSize MAX_STREAM_BUFFER_REQUEST_SIZE = 8_MiB;

constexpr VkMemoryPropertyFlags HOST_FLAGS =
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
constexpr VkMemoryPropertyFlags STREAM_FLAGS = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | HOST_FLAGS;

static bool IsStreamHeap(VkMemoryHeap heap, size_t staging_buffer_size) noexcept {
    return staging_buffer_size < (heap.size * 2) / 3;
}

static bool HasLargeDeviceLocalHostVisibleMemory(const VkPhysicalDeviceMemoryProperties& props) {
    const auto flags{VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT};

    for (u32 type_index = 0; type_index < props.memoryTypeCount; ++type_index) {
        const auto& memory_type{props.memoryTypes[type_index]};

        if ((memory_type.propertyFlags & flags) != flags) {
            // Memory must be device local and host visible
            continue;
        }

        const auto& heap{props.memoryHeaps[memory_type.heapIndex]};
        if (heap.size >= 7168_MiB) {
            // This is the right type of memory
            return true;
        }
    }

    return false;
}

std::optional<u32> FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& props, u32 type_mask,
                                       VkMemoryPropertyFlags flags,
                                       size_t staging_buffer_size) noexcept {
    for (u32 type_index = 0; type_index < props.memoryTypeCount; ++type_index) {
        if (((type_mask >> type_index) & 1) == 0) {
            // Memory type is incompatible
            continue;
        }
        const VkMemoryType& memory_type = props.memoryTypes[type_index];
        if ((memory_type.propertyFlags & flags) != flags) {
            // Memory type doesn't have the flags we want
            continue;
        }
        if (!IsStreamHeap(props.memoryHeaps[memory_type.heapIndex], staging_buffer_size)) {
            // Memory heap is not suitable for streaming
            continue;
        }
        // Success!
        return type_index;
    }
    return std::nullopt;
}

u32 FindMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& props, u32 type_mask,
                        bool try_device_local, size_t staging_buffer_size) {
    std::optional<u32> type;
    if (try_device_local) {
        // Try to find a DEVICE_LOCAL_BIT type, Nvidia and AMD have a dedicated heap for this
        type = FindMemoryTypeIndex(props, type_mask, STREAM_FLAGS, staging_buffer_size);
        if (type) {
            return *type;
        }
    }
    // Otherwise try without the DEVICE_LOCAL_BIT
    type = FindMemoryTypeIndex(props, type_mask, HOST_FLAGS, staging_buffer_size);
    if (type) {
        return *type;
    }
    // This should never happen, and in case it does, signal it as an out of memory situation
    throw vk::Exception(VK_ERROR_OUT_OF_DEVICE_MEMORY);
}

size_t Region(size_t iterator, size_t region_size) noexcept {
    return iterator / region_size;
}
} // Anonymous namespace

StagingBufferPool::StagingBufferPool(const Device& device_, MemoryAllocator& memory_allocator_,
                                     Scheduler& scheduler_)
    : device{device_}, memory_allocator{memory_allocator_}, scheduler{scheduler_} {

    const auto memory_properties{device.GetPhysical().GetMemoryProperties().memoryProperties};
    if (HasLargeDeviceLocalHostVisibleMemory(memory_properties)) {
        // Possible on many integrated and newer discrete cards
        staging_buffer_size = 1_GiB;
    } else {
        // Well-supported default size used by most Vulkan PC games
        staging_buffer_size = 256_MiB;
    }

    region_size = staging_buffer_size / StagingBufferPool::NUM_SYNCS;

    const vk::Device& dev = device.GetLogical();
    stream_buffer = dev.CreateBuffer(VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = staging_buffer_size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    });
    if (device.HasDebuggingToolAttached()) {
        stream_buffer.SetObjectNameEXT("Stream Buffer");
    }
    VkMemoryDedicatedRequirements dedicated_reqs{
        .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS,
        .pNext = nullptr,
        .prefersDedicatedAllocation = VK_FALSE,
        .requiresDedicatedAllocation = VK_FALSE,
    };
    const auto requirements = dev.GetBufferMemoryRequirements(*stream_buffer, &dedicated_reqs);
    const bool make_dedicated = dedicated_reqs.prefersDedicatedAllocation == VK_TRUE ||
                                dedicated_reqs.requiresDedicatedAllocation == VK_TRUE;
    const VkMemoryDedicatedAllocateInfo dedicated_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
        .pNext = nullptr,
        .image = nullptr,
        .buffer = *stream_buffer,
    };
    VkMemoryAllocateInfo stream_memory_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = make_dedicated ? &dedicated_info : nullptr,
        .allocationSize = requirements.size,
        .memoryTypeIndex = FindMemoryTypeIndex(memory_properties, requirements.memoryTypeBits, true,
                                               staging_buffer_size),
    };
    stream_memory = dev.TryAllocateMemory(stream_memory_info);
    if (!stream_memory) {
        LOG_INFO(Render_Vulkan, "Dynamic memory allocation failed, trying with system memory");
        stream_memory_info.memoryTypeIndex = FindMemoryTypeIndex(
            memory_properties, requirements.memoryTypeBits, false, staging_buffer_size);
        stream_memory = dev.AllocateMemory(stream_memory_info);
    }

    if (device.HasDebuggingToolAttached()) {
        stream_memory.SetObjectNameEXT("Stream Buffer Memory");
    }
    stream_buffer.BindMemory(*stream_memory, 0);
    stream_pointer = stream_memory.Map(0, staging_buffer_size);
}

StagingBufferPool::~StagingBufferPool() = default;

StagingBufferRef StagingBufferPool::Request(size_t size, MemoryUsage usage) {
    if (usage == MemoryUsage::Upload && size <= MAX_STREAM_BUFFER_REQUEST_SIZE) {
        return GetStreamBuffer(size);
    }
    return GetStagingBuffer(size, usage);
}

void StagingBufferPool::TickFrame() {
    current_delete_level = (current_delete_level + 1) % NUM_LEVELS;

    ReleaseCache(MemoryUsage::DeviceLocal);
    ReleaseCache(MemoryUsage::Upload);
    ReleaseCache(MemoryUsage::Download);
}

StagingBufferRef StagingBufferPool::GetStreamBuffer(size_t size) {
    if (AreRegionsActive(Region(free_iterator, region_size) + 1,
                         std::min(Region(iterator + size, region_size) + 1, NUM_SYNCS))) {
        // Avoid waiting for the previous usages to be free
        return GetStagingBuffer(size, MemoryUsage::Upload);
    }
    const u64 current_tick = scheduler.CurrentTick();
    std::fill(sync_ticks.begin() + Region(used_iterator, region_size),
              sync_ticks.begin() + Region(iterator, region_size), current_tick);
    used_iterator = iterator;
    free_iterator = std::max(free_iterator, iterator + size);

    if (iterator + size >= staging_buffer_size) {
        std::fill(sync_ticks.begin() + Region(used_iterator, region_size),
                  sync_ticks.begin() + NUM_SYNCS, current_tick);
        used_iterator = 0;
        iterator = 0;
        free_iterator = size;

        if (AreRegionsActive(0, Region(size, region_size) + 1)) {
            // Avoid waiting for the previous usages to be free
            return GetStagingBuffer(size, MemoryUsage::Upload);
        }
    }
    const size_t offset = iterator;
    iterator = Common::AlignUp(iterator + size, MAX_ALIGNMENT);
    return StagingBufferRef{
        .buffer = *stream_buffer,
        .offset = static_cast<VkDeviceSize>(offset),
        .mapped_span = std::span<u8>(stream_pointer + offset, size),
    };
}

bool StagingBufferPool::AreRegionsActive(size_t region_begin, size_t region_end) const {
    const u64 gpu_tick = scheduler.GetMasterSemaphore().KnownGpuTick();
    return std::any_of(sync_ticks.begin() + region_begin, sync_ticks.begin() + region_end,
                       [gpu_tick](u64 sync_tick) { return gpu_tick < sync_tick; });
};

StagingBufferRef StagingBufferPool::GetStagingBuffer(size_t size, MemoryUsage usage) {
    if (const std::optional<StagingBufferRef> ref = TryGetReservedBuffer(size, usage)) {
        return *ref;
    }
    return CreateStagingBuffer(size, usage);
}

std::optional<StagingBufferRef> StagingBufferPool::TryGetReservedBuffer(size_t size,
                                                                        MemoryUsage usage) {
    StagingBuffers& cache_level = GetCache(usage)[Common::Log2Ceil64(size)];

    const auto is_free = [this](const StagingBuffer& entry) {
        return scheduler.IsFree(entry.tick);
    };
    auto& entries = cache_level.entries;
    const auto hint_it = entries.begin() + cache_level.iterate_index;
    auto it = std::find_if(entries.begin() + cache_level.iterate_index, entries.end(), is_free);
    if (it == entries.end()) {
        it = std::find_if(entries.begin(), hint_it, is_free);
        if (it == hint_it) {
            return std::nullopt;
        }
    }
    cache_level.iterate_index = std::distance(entries.begin(), it) + 1;
    it->tick = scheduler.CurrentTick();
    return it->Ref();
}

StagingBufferRef StagingBufferPool::CreateStagingBuffer(size_t size, MemoryUsage usage) {
    const u32 log2 = Common::Log2Ceil64(size);
    vk::Buffer buffer = device.GetLogical().CreateBuffer({
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = 1ULL << log2,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    });
    if (device.HasDebuggingToolAttached()) {
        ++buffer_index;
        buffer.SetObjectNameEXT(fmt::format("Staging Buffer {}", buffer_index).c_str());
    }
    MemoryCommit commit = memory_allocator.Commit(buffer, usage);
    const std::span<u8> mapped_span = IsHostVisible(usage) ? commit.Map() : std::span<u8>{};

    StagingBuffer& entry = GetCache(usage)[log2].entries.emplace_back(StagingBuffer{
        .buffer = std::move(buffer),
        .commit = std::move(commit),
        .mapped_span = mapped_span,
        .tick = scheduler.CurrentTick(),
    });
    return entry.Ref();
}

StagingBufferPool::StagingBuffersCache& StagingBufferPool::GetCache(MemoryUsage usage) {
    switch (usage) {
    case MemoryUsage::DeviceLocal:
        return device_local_cache;
    case MemoryUsage::Upload:
        return upload_cache;
    case MemoryUsage::Download:
        return download_cache;
    default:
        ASSERT_MSG(false, "Invalid memory usage={}", usage);
        return upload_cache;
    }
}

void StagingBufferPool::ReleaseCache(MemoryUsage usage) {
    ReleaseLevel(GetCache(usage), current_delete_level);
}

void StagingBufferPool::ReleaseLevel(StagingBuffersCache& cache, size_t log2) {
    constexpr size_t deletions_per_tick = 16;
    auto& staging = cache[log2];
    auto& entries = staging.entries;
    const size_t old_size = entries.size();

    const auto is_deleteable = [this](const StagingBuffer& entry) {
        return scheduler.IsFree(entry.tick);
    };
    const size_t begin_offset = staging.delete_index;
    const size_t end_offset = std::min(begin_offset + deletions_per_tick, old_size);
    const auto begin = entries.begin() + begin_offset;
    const auto end = entries.begin() + end_offset;
    entries.erase(std::remove_if(begin, end, is_deleteable), end);

    const size_t new_size = entries.size();
    staging.delete_index += deletions_per_tick;
    if (staging.delete_index >= new_size) {
        staging.delete_index = 0;
    }
    if (staging.iterate_index > new_size) {
        staging.iterate_index = 0;
    }
}

} // namespace Vulkan

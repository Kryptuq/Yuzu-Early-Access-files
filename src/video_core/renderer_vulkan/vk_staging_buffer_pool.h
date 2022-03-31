// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <climits>
#include <vector>

#include "common/common_types.h"

#include "video_core/vulkan_common/vulkan_memory_allocator.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

class Device;
class VKScheduler;

struct StagingBufferRef {
    VkBuffer buffer;
    VkDeviceSize offset;
    std::span<u8> mapped_span;
};

class StagingBufferPool {
public:
    static constexpr size_t NUM_STREAM_REGIONS = 0x8000;

    explicit StagingBufferPool(const Device& device, MemoryAllocator& memory_allocator,
                               VKScheduler& scheduler);
    ~StagingBufferPool();

    StagingBufferRef Request(size_t size, MemoryUsage usage);

    void TickFrame();

private:
    struct StreamBufferCommit {
        size_t upper_bound;
        u64 tick;
    };

    struct StagingBuffer {
        vk::Buffer buffer;
        MemoryCommit commit;
        std::span<u8> mapped_span;
        u64 tick = 0;

        StagingBufferRef Ref() const noexcept {
            return {
                .buffer = *buffer,
                .offset = 0,
                .mapped_span = mapped_span,
            };
        }
    };

    struct StagingBuffers {
        std::vector<StagingBuffer> entries;
        size_t delete_index = 0;
        size_t iterate_index = 0;
    };

    static constexpr size_t NUM_LEVELS = sizeof(size_t) * CHAR_BIT;
    using StagingBuffersCache = std::array<StagingBuffers, NUM_LEVELS>;

    StagingBufferRef GetStreamBuffer(size_t size);

    std::optional<size_t> NextAvailableStreamIndex(size_t num_regions) const;

    StagingBufferRef GetStagingBuffer(size_t size, MemoryUsage usage);

    std::optional<StagingBufferRef> TryGetReservedBuffer(size_t size, MemoryUsage usage);

    StagingBufferRef CreateStagingBuffer(size_t size, MemoryUsage usage);

    StagingBuffersCache& GetCache(MemoryUsage usage);

    void ReleaseCache(MemoryUsage usage);

    void ReleaseLevel(StagingBuffersCache& cache, size_t log2);

    const Device& device;
    MemoryAllocator& memory_allocator;
    VKScheduler& scheduler;

    vk::Buffer stream_buffer;
    vk::DeviceMemory stream_memory;
    u8* stream_pointer = nullptr;

    size_t next_index = 0;
    std::array<u64, NUM_STREAM_REGIONS> sync_ticks{};

    StagingBuffersCache device_local_cache;
    StagingBuffersCache upload_cache;
    StagingBuffersCache download_cache;

    size_t current_delete_level = 0;
    u64 buffer_index = 0;
};

} // namespace Vulkan

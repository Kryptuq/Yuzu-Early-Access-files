// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <limits>
#include <optional>
#include <tuple>
#include <vector>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/literals.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_stream_buffer.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

namespace {

using namespace Common::Literals;

constexpr VkBufferUsageFlags BUFFER_USAGE =
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

constexpr u64 WATCHES_INITIAL_RESERVE = 0x4000;
constexpr u64 WATCHES_RESERVE_CHUNK = 0x1000;

constexpr u64 PREFERRED_STREAM_BUFFER_SIZE = 256_MiB;

/// Find a memory type with the passed requirements
std::optional<u32> FindMemoryType(const VkPhysicalDeviceMemoryProperties& properties,
                                  VkMemoryPropertyFlags wanted,
                                  u32 filter = std::numeric_limits<u32>::max()) {
    for (u32 i = 0; i < properties.memoryTypeCount; ++i) {
        const auto flags = properties.memoryTypes[i].propertyFlags;
        if ((flags & wanted) == wanted && (filter & (1U << i)) != 0) {
            return i;
        }
    }
    return std::nullopt;
}

/// Get the preferred host visible memory type.
u32 GetMemoryType(const VkPhysicalDeviceMemoryProperties& properties,
                  u32 filter = std::numeric_limits<u32>::max()) {
    // Prefer device local host visible allocations. Both AMD and Nvidia now provide one.
    // Otherwise search for a host visible allocation.
    static constexpr auto HOST_MEMORY =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    static constexpr auto DYNAMIC_MEMORY = HOST_MEMORY | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    std::optional preferred_type = FindMemoryType(properties, DYNAMIC_MEMORY);
    if (!preferred_type) {
        preferred_type = FindMemoryType(properties, HOST_MEMORY);
        ASSERT_MSG(preferred_type, "No host visible and coherent memory type found");
    }
    return preferred_type.value_or(0);
}

} // Anonymous namespace

VKStreamBuffer::VKStreamBuffer(const Device& device_, VKScheduler& scheduler_)
    : device{device_}, scheduler{scheduler_} {
    CreateBuffers();
    ReserveWatches(current_watches, WATCHES_INITIAL_RESERVE);
    ReserveWatches(previous_watches, WATCHES_INITIAL_RESERVE);
}

VKStreamBuffer::~VKStreamBuffer() = default;

std::pair<u8*, u64> VKStreamBuffer::Map(u64 size, u64 alignment) {
    ASSERT(size <= stream_buffer_size);
    mapped_size = size;

    if (alignment > 0) {
        offset = Common::AlignUp(offset, alignment);
    }

    WaitPendingOperations(offset);

    if (offset + size > stream_buffer_size) {
        // The buffer would overflow, save the amount of used watches and reset the state.
        invalidation_mark = current_watch_cursor;
        current_watch_cursor = 0;
        offset = 0;

        // Swap watches and reset waiting cursors.
        std::swap(previous_watches, current_watches);
        wait_cursor = 0;
        wait_bound = 0;

        // Ensure that we don't wait for uncommitted fences.
        scheduler.Flush();
    }

    return std::make_pair(memory.Map(offset, size), offset);
}

void VKStreamBuffer::Unmap(u64 size) {
    ASSERT_MSG(size <= mapped_size, "Reserved size is too small");

    memory.Unmap();

    offset += size;

    if (current_watch_cursor + 1 >= current_watches.size()) {
        // Ensure that there are enough watches.
        ReserveWatches(current_watches, WATCHES_RESERVE_CHUNK);
    }
    auto& watch = current_watches[current_watch_cursor++];
    watch.upper_bound = offset;
    watch.tick = scheduler.CurrentTick();
}

void VKStreamBuffer::CreateBuffers() {
    const auto memory_properties = device.GetPhysical().GetMemoryProperties();
    const u32 preferred_type = GetMemoryType(memory_properties);
    const u32 preferred_heap = memory_properties.memoryTypes[preferred_type].heapIndex;

    // Substract from the preferred heap size some bytes to avoid getting out of memory.
    const VkDeviceSize heap_size = memory_properties.memoryHeaps[preferred_heap].size;
    // As per DXVK's example, using `heap_size / 2`
    const VkDeviceSize allocable_size = heap_size / 2;
    buffer = device.GetLogical().CreateBuffer({
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = std::min(PREFERRED_STREAM_BUFFER_SIZE, allocable_size),
        .usage = BUFFER_USAGE,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    });

    const auto requirements = device.GetLogical().GetBufferMemoryRequirements(*buffer);
    const u32 required_flags = requirements.memoryTypeBits;
    stream_buffer_size = static_cast<u64>(requirements.size);

    memory = device.GetLogical().AllocateMemory({
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = requirements.size,
        .memoryTypeIndex = GetMemoryType(memory_properties, required_flags),
    });
    buffer.BindMemory(*memory, 0);
}

void VKStreamBuffer::ReserveWatches(std::vector<Watch>& watches, std::size_t grow_size) {
    watches.resize(watches.size() + grow_size);
}

void VKStreamBuffer::WaitPendingOperations(u64 requested_upper_bound) {
    if (!invalidation_mark) {
        return;
    }
    while (requested_upper_bound < wait_bound && wait_cursor < *invalidation_mark) {
        auto& watch = previous_watches[wait_cursor];
        wait_bound = watch.upper_bound;
        scheduler.Wait(watch.tick);
        ++wait_cursor;
    }
}

} // namespace Vulkan

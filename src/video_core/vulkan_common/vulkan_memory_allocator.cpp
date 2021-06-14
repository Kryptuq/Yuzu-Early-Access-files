// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <bit>
#include <optional>
#include <vector>

#include <glad/glad.h>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_memory_allocator.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {
namespace {
struct Range {
    u64 begin;
    u64 end;

    [[nodiscard]] bool Contains(u64 iterator, u64 size) const noexcept {
        return iterator < end && begin < iterator + size;
    }
};

[[nodiscard]] u64 AllocationChunkSize(u64 required_size) {
    static constexpr std::array sizes{
        0x1000ULL << 10,  0x1400ULL << 10,  0x1800ULL << 10,  0x1c00ULL << 10, 0x2000ULL << 10,
        0x3200ULL << 10,  0x4000ULL << 10,  0x6000ULL << 10,  0x8000ULL << 10, 0xA000ULL << 10,
        0x10000ULL << 10, 0x18000ULL << 10, 0x20000ULL << 10,
    };
    static_assert(std::is_sorted(sizes.begin(), sizes.end()));

    const auto it = std::ranges::lower_bound(sizes, required_size);
    return it != sizes.end() ? *it : Common::AlignUp(required_size, 4ULL << 20);
}

[[nodiscard]] VkMemoryPropertyFlags MemoryUsagePropertyFlags(MemoryUsage usage) {
    switch (usage) {
    case MemoryUsage::DeviceLocal:
        return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    case MemoryUsage::Upload:
        return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    case MemoryUsage::Download:
        return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
               VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    }
    UNREACHABLE_MSG("Invalid memory usage={}", usage);
    return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
}

constexpr VkExportMemoryAllocateInfo EXPORT_ALLOCATE_INFO{
    .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
    .pNext = nullptr,
#ifdef _WIN32
    .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT,
#elif __unix__
    .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
#else
    .handleTypes = 0,
#endif
};
} // Anonymous namespace

class MemoryAllocation {
public:
    explicit MemoryAllocation(MemoryAllocator* const allocator_, vk::DeviceMemory memory_,
                              VkMemoryPropertyFlags properties, u64 allocation_size_, u32 type)
        : allocator{allocator_}, memory{std::move(memory_)}, allocation_size{allocation_size_},
          property_flags{properties}, shifted_memory_type{1U << type} {}

#if defined(_WIN32) || defined(__unix__)
    ~MemoryAllocation() {
        if (owning_opengl_handle != 0) {
            glDeleteMemoryObjectsEXT(1, &owning_opengl_handle);
        }
    }
#endif

    MemoryAllocation& operator=(const MemoryAllocation&) = delete;
    MemoryAllocation(const MemoryAllocation&) = delete;

    MemoryAllocation& operator=(MemoryAllocation&&) = delete;
    MemoryAllocation(MemoryAllocation&&) = delete;

    [[nodiscard]] std::optional<MemoryCommit> Commit(VkDeviceSize size, VkDeviceSize alignment) {
        const std::optional<u64> alloc = FindFreeRegion(size, alignment);
        if (!alloc) {
            // Signal out of memory, it'll try to do more allocations.
            return std::nullopt;
        }
        const Range range{
            .begin = *alloc,
            .end = *alloc + size,
        };
        commits.insert(std::ranges::upper_bound(commits, *alloc, {}, &Range::begin), range);
        return std::make_optional<MemoryCommit>(this, *memory, *alloc, *alloc + size);
    }

    void Free(u64 begin) {
        const auto it = std::ranges::find(commits, begin, &Range::begin);
        ASSERT_MSG(it != commits.end(), "Invalid commit");
        commits.erase(it);
        if (commits.empty()) {
            // Do not call any code involving 'this' after this call, the object will be destroyed
            allocator->ReleaseMemory(this);
        }
    }

    [[nodiscard]] std::span<u8> Map() {
        if (memory_mapped_span.empty()) {
            u8* const raw_pointer = memory.Map(0, allocation_size);
            memory_mapped_span = std::span<u8>(raw_pointer, allocation_size);
        }
        return memory_mapped_span;
    }

#ifdef _WIN32
    [[nodiscard]] u32 ExportOpenGLHandle() {
        if (!owning_opengl_handle) {
            glCreateMemoryObjectsEXT(1, &owning_opengl_handle);
            glImportMemoryWin32HandleEXT(owning_opengl_handle, allocation_size,
                                         GL_HANDLE_TYPE_OPAQUE_WIN32_EXT,
                                         memory.GetMemoryWin32HandleKHR());
        }
        return owning_opengl_handle;
    }
#elif __unix__
    [[nodiscard]] u32 ExportOpenGLHandle() {
        if (!owning_opengl_handle) {
            glCreateMemoryObjectsEXT(1, &owning_opengl_handle);
            glImportMemoryFdEXT(owning_opengl_handle, allocation_size, GL_HANDLE_TYPE_OPAQUE_FD_EXT,
                                memory.GetMemoryFdKHR());
        }
        return owning_opengl_handle;
    }
#else
    [[nodiscard]] u32 ExportOpenGLHandle() {
        return 0;
    }
#endif

    /// Returns whether this allocation is compatible with the arguments.
    [[nodiscard]] bool IsCompatible(VkMemoryPropertyFlags flags, u32 type_mask) const {
        return (flags & property_flags) == property_flags && (type_mask & shifted_memory_type) != 0;
    }

private:
    [[nodiscard]] static constexpr u32 ShiftType(u32 type) {
        return 1U << type;
    }

    [[nodiscard]] std::optional<u64> FindFreeRegion(u64 size, u64 alignment) noexcept {
        ASSERT(std::has_single_bit(alignment));
        const u64 alignment_log2 = std::countr_zero(alignment);
        std::optional<u64> candidate;
        u64 iterator = 0;
        auto commit = commits.begin();
        while (iterator + size <= allocation_size) {
            candidate = candidate.value_or(iterator);
            if (commit == commits.end()) {
                break;
            }
            if (commit->Contains(*candidate, size)) {
                candidate = std::nullopt;
            }
            iterator = Common::AlignUpLog2(commit->end, alignment_log2);
            ++commit;
        }
        return candidate;
    }

    MemoryAllocator* const allocator;           ///< Parent memory allocation.
    const vk::DeviceMemory memory;              ///< Vulkan memory allocation handler.
    const u64 allocation_size;                  ///< Size of this allocation.
    const VkMemoryPropertyFlags property_flags; ///< Vulkan memory property flags.
    const u32 shifted_memory_type;              ///< Shifted Vulkan memory type.
    std::vector<Range> commits;                 ///< All commit ranges done from this allocation.
    std::span<u8> memory_mapped_span; ///< Memory mapped span. Empty if not queried before.
#if defined(_WIN32) || defined(__unix__)
    u32 owning_opengl_handle{}; ///< Owning OpenGL memory object handle.
#endif
};

MemoryCommit::MemoryCommit(MemoryAllocation* allocation_, VkDeviceMemory memory_, u64 begin_,
                           u64 end_) noexcept
    : allocation{allocation_}, memory{memory_}, begin{begin_}, end{end_} {}

MemoryCommit::~MemoryCommit() {
    Release();
}

MemoryCommit& MemoryCommit::operator=(MemoryCommit&& rhs) noexcept {
    Release();
    allocation = std::exchange(rhs.allocation, nullptr);
    memory = rhs.memory;
    begin = rhs.begin;
    end = rhs.end;
    span = std::exchange(rhs.span, std::span<u8>{});
    return *this;
}

MemoryCommit::MemoryCommit(MemoryCommit&& rhs) noexcept
    : allocation{std::exchange(rhs.allocation, nullptr)}, memory{rhs.memory}, begin{rhs.begin},
      end{rhs.end}, span{std::exchange(rhs.span, std::span<u8>{})} {}

std::span<u8> MemoryCommit::Map() {
    if (span.empty()) {
        span = allocation->Map().subspan(begin, end - begin);
    }
    return span;
}

u32 MemoryCommit::ExportOpenGLHandle() const {
    return allocation->ExportOpenGLHandle();
}

void MemoryCommit::Release() {
    if (allocation) {
        allocation->Free(begin);
    }
}

MemoryAllocator::MemoryAllocator(const Device& device_, bool export_allocations_)
    : device{device_}, properties{device_.GetPhysical().GetMemoryProperties()},
      export_allocations{export_allocations_} {}

MemoryAllocator::~MemoryAllocator() = default;

MemoryCommit MemoryAllocator::Commit(const VkMemoryRequirements& requirements, MemoryUsage usage) {
    // Find the fastest memory flags we can afford with the current requirements
    const u32 type_mask = requirements.memoryTypeBits;
    const VkMemoryPropertyFlags usage_flags = MemoryUsagePropertyFlags(usage);
    const VkMemoryPropertyFlags flags = MemoryPropertyFlags(type_mask, usage_flags);
    if (std::optional<MemoryCommit> commit = TryCommit(requirements, flags)) {
        return std::move(*commit);
    }
    // Commit has failed, allocate more memory.
    const u64 chunk_size = AllocationChunkSize(requirements.size);
    if (!TryAllocMemory(flags, type_mask, chunk_size)) {
        // TODO(Rodrigo): Handle out of memory situations in some way like flushing to guest memory.
        throw vk::Exception(VK_ERROR_OUT_OF_DEVICE_MEMORY);
    }
    // Commit again, this time it won't fail since there's a fresh allocation above.
    // If it does, there's a bug.
    return TryCommit(requirements, flags).value();
}

MemoryCommit MemoryAllocator::Commit(const vk::Buffer& buffer, MemoryUsage usage) {
    auto commit = Commit(device.GetLogical().GetBufferMemoryRequirements(*buffer), usage);
    buffer.BindMemory(commit.Memory(), commit.Offset());
    return commit;
}

MemoryCommit MemoryAllocator::Commit(const vk::Image& image, MemoryUsage usage) {
    auto commit = Commit(device.GetLogical().GetImageMemoryRequirements(*image), usage);
    image.BindMemory(commit.Memory(), commit.Offset());
    return commit;
}

bool MemoryAllocator::TryAllocMemory(VkMemoryPropertyFlags flags, u32 type_mask, u64 size) {
    const u32 type = FindType(flags, type_mask).value();
    vk::DeviceMemory memory = device.GetLogical().TryAllocateMemory({
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = export_allocations ? &EXPORT_ALLOCATE_INFO : nullptr,
        .allocationSize = size,
        .memoryTypeIndex = type,
    });
    if (!memory) {
        if ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
            // Try to allocate non device local memory
            return TryAllocMemory(flags & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, type_mask, size);
        } else {
            // RIP
            return false;
        }
    }
    allocations.push_back(
        std::make_unique<MemoryAllocation>(this, std::move(memory), flags, size, type));
    return true;
}

void MemoryAllocator::ReleaseMemory(MemoryAllocation* alloc) {
    const auto it = std::ranges::find(allocations, alloc, &std::unique_ptr<MemoryAllocation>::get);
    ASSERT(it != allocations.end());
    allocations.erase(it);
}

std::optional<MemoryCommit> MemoryAllocator::TryCommit(const VkMemoryRequirements& requirements,
                                                       VkMemoryPropertyFlags flags) {
    for (auto& allocation : allocations) {
        if (!allocation->IsCompatible(flags, requirements.memoryTypeBits)) {
            continue;
        }
        if (auto commit = allocation->Commit(requirements.size, requirements.alignment)) {
            return commit;
        }
    }
    if ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
        // Look for non device local commits on failure
        return TryCommit(requirements, flags & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
    return std::nullopt;
}

VkMemoryPropertyFlags MemoryAllocator::MemoryPropertyFlags(u32 type_mask,
                                                           VkMemoryPropertyFlags flags) const {
    if (FindType(flags, type_mask)) {
        // Found a memory type with those requirements
        return flags;
    }
    if ((flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0) {
        // Remove host cached bit in case it's not supported
        return MemoryPropertyFlags(type_mask, flags & ~VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    }
    if ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
        // Remove device local, if it's not supported by the requested resource
        return MemoryPropertyFlags(type_mask, flags & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
    UNREACHABLE_MSG("No compatible memory types found");
    return 0;
}

std::optional<u32> MemoryAllocator::FindType(VkMemoryPropertyFlags flags, u32 type_mask) const {
    for (u32 type_index = 0; type_index < properties.memoryTypeCount; ++type_index) {
        const VkMemoryPropertyFlags type_flags = properties.memoryTypes[type_index].propertyFlags;
        if ((type_mask & (1U << type_index)) != 0 && (type_flags & flags) == flags) {
            // The type matches in type and in the wanted properties.
            return type_index;
        }
    }
    // Failed to find index
    return std::nullopt;
}

bool IsHostVisible(MemoryUsage usage) noexcept {
    switch (usage) {
    case MemoryUsage::DeviceLocal:
        return false;
    case MemoryUsage::Upload:
    case MemoryUsage::Download:
        return true;
    }
    UNREACHABLE_MSG("Invalid memory usage={}", usage);
    return false;
}

} // namespace Vulkan

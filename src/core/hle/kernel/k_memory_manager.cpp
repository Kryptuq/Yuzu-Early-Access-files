// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/common_types.h"
#include "common/scope_exit.h"
#include "core/core.h"
#include "core/device_memory.h"
#include "core/hle/kernel/k_memory_manager.h"
#include "core/hle/kernel/k_page_linked_list.h"
#include "core/hle/kernel/svc_results.h"

namespace Kernel {

KMemoryManager::KMemoryManager(Core::System& system_) : system{system_} {}

std::size_t KMemoryManager::Impl::Initialize(Pool new_pool, u64 start_address, u64 end_address) {
    const auto size{end_address - start_address};

    // Calculate metadata sizes
    const auto ref_count_size{(size / PageSize) * sizeof(u16)};
    const auto optimize_map_size{(Common::AlignUp((size / PageSize), 64) / 64) * sizeof(u64)};
    const auto manager_size{Common::AlignUp(optimize_map_size + ref_count_size, PageSize)};
    const auto page_heap_size{KPageHeap::CalculateManagementOverheadSize(size)};
    const auto total_metadata_size{manager_size + page_heap_size};
    ASSERT(manager_size <= total_metadata_size);
    ASSERT(Common::IsAligned(total_metadata_size, PageSize));

    // Setup region
    pool = new_pool;

    // Initialize the manager's KPageHeap
    heap.Initialize(start_address, size, page_heap_size);

    // Free the memory to the heap
    heap.Free(start_address, size / PageSize);

    // Update the heap's used size
    heap.UpdateUsedSize();

    return total_metadata_size;
}

void KMemoryManager::InitializeManager(Pool pool, u64 start_address, u64 end_address) {
    ASSERT(pool < Pool::Count);
    managers[static_cast<std::size_t>(pool)].Initialize(pool, start_address, end_address);
}

VAddr KMemoryManager::AllocateAndOpenContinuous(std::size_t num_pages, std::size_t align_pages,
                                                u32 option) {
    // Early return if we're allocating no pages
    if (num_pages == 0) {
        return {};
    }

    // Lock the pool that we're allocating from
    const auto [pool, dir] = DecodeOption(option);
    const auto pool_index{static_cast<std::size_t>(pool)};
    std::lock_guard lock{pool_locks[pool_index]};

    // Choose a heap based on our page size request
    const s32 heap_index{KPageHeap::GetAlignedBlockIndex(num_pages, align_pages)};

    // Loop, trying to iterate from each block
    // TODO (bunnei): Support multiple managers
    Impl& chosen_manager{managers[pool_index]};
    VAddr allocated_block{chosen_manager.AllocateBlock(heap_index, false)};

    // If we failed to allocate, quit now
    if (!allocated_block) {
        return {};
    }

    // If we allocated more than we need, free some
    const auto allocated_pages{KPageHeap::GetBlockNumPages(heap_index)};
    if (allocated_pages > num_pages) {
        chosen_manager.Free(allocated_block + num_pages * PageSize, allocated_pages - num_pages);
    }

    return allocated_block;
}

ResultCode KMemoryManager::Allocate(KPageLinkedList& page_list, std::size_t num_pages, Pool pool,
                                    Direction dir, u32 heap_fill_value) {
    ASSERT(page_list.GetNumPages() == 0);

    // Early return if we're allocating no pages
    if (num_pages == 0) {
        return ResultSuccess;
    }

    // Lock the pool that we're allocating from
    const auto pool_index{static_cast<std::size_t>(pool)};
    std::lock_guard lock{pool_locks[pool_index]};

    // Choose a heap based on our page size request
    const s32 heap_index{KPageHeap::GetBlockIndex(num_pages)};
    if (heap_index < 0) {
        return ResultOutOfMemory;
    }

    // TODO (bunnei): Support multiple managers
    Impl& chosen_manager{managers[pool_index]};

    // Ensure that we don't leave anything un-freed
    auto group_guard = detail::ScopeExit([&] {
        for (const auto& it : page_list.Nodes()) {
            const auto min_num_pages{std::min<size_t>(
                it.GetNumPages(), (chosen_manager.GetEndAddress() - it.GetAddress()) / PageSize)};
            chosen_manager.Free(it.GetAddress(), min_num_pages);
        }
    });

    // Keep allocating until we've allocated all our pages
    for (s32 index{heap_index}; index >= 0 && num_pages > 0; index--) {
        const auto pages_per_alloc{KPageHeap::GetBlockNumPages(index)};

        while (num_pages >= pages_per_alloc) {
            // Allocate a block
            VAddr allocated_block{chosen_manager.AllocateBlock(index, false)};
            if (!allocated_block) {
                break;
            }

            // Safely add it to our group
            {
                auto block_guard = detail::ScopeExit(
                    [&] { chosen_manager.Free(allocated_block, pages_per_alloc); });

                if (const ResultCode result{page_list.AddBlock(allocated_block, pages_per_alloc)};
                    result.IsError()) {
                    return result;
                }

                block_guard.Cancel();
            }

            num_pages -= pages_per_alloc;
        }
    }

    // Clear allocated memory.
    for (const auto& it : page_list.Nodes()) {
        std::memset(system.DeviceMemory().GetPointer(it.GetAddress()), heap_fill_value,
                    it.GetSize());
    }

    // Only succeed if we allocated as many pages as we wanted
    if (num_pages) {
        return ResultOutOfMemory;
    }

    // We succeeded!
    group_guard.Cancel();

    return ResultSuccess;
}

ResultCode KMemoryManager::Free(KPageLinkedList& page_list, std::size_t num_pages, Pool pool,
                                Direction dir, u32 heap_fill_value) {
    // Early return if we're freeing no pages
    if (!num_pages) {
        return ResultSuccess;
    }

    // Lock the pool that we're freeing from
    const auto pool_index{static_cast<std::size_t>(pool)};
    std::lock_guard lock{pool_locks[pool_index]};

    // TODO (bunnei): Support multiple managers
    Impl& chosen_manager{managers[pool_index]};

    // Free all of the pages
    for (const auto& it : page_list.Nodes()) {
        const auto min_num_pages{std::min<size_t>(
            it.GetNumPages(), (chosen_manager.GetEndAddress() - it.GetAddress()) / PageSize)};
        chosen_manager.Free(it.GetAddress(), min_num_pages);
    }

    return ResultSuccess;
}

std::size_t KMemoryManager::Impl::CalculateManagementOverheadSize(std::size_t region_size) {
    const std::size_t ref_count_size = (region_size / PageSize) * sizeof(u16);
    const std::size_t optimize_map_size =
        (Common::AlignUp((region_size / PageSize), Common::BitSize<u64>()) /
         Common::BitSize<u64>()) *
        sizeof(u64);
    const std::size_t manager_meta_size =
        Common::AlignUp(optimize_map_size + ref_count_size, PageSize);
    const std::size_t page_heap_size = KPageHeap::CalculateManagementOverheadSize(region_size);
    return manager_meta_size + page_heap_size;
}

} // namespace Kernel

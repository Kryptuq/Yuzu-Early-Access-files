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
#include "core/hle/kernel/initial_process.h"
#include "core/hle/kernel/k_memory_manager.h"
#include "core/hle/kernel/k_page_linked_list.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_results.h"
#include "core/memory.h"

namespace Kernel {

namespace {

constexpr KMemoryManager::Pool GetPoolFromMemoryRegionType(u32 type) {
    if ((type | KMemoryRegionType_DramApplicationPool) == type) {
        return KMemoryManager::Pool::Application;
    } else if ((type | KMemoryRegionType_DramAppletPool) == type) {
        return KMemoryManager::Pool::Applet;
    } else if ((type | KMemoryRegionType_DramSystemPool) == type) {
        return KMemoryManager::Pool::System;
    } else if ((type | KMemoryRegionType_DramSystemNonSecurePool) == type) {
        return KMemoryManager::Pool::SystemNonSecure;
    } else {
        ASSERT_MSG("InvalidMemoryRegionType for conversion to Pool");
        return {};
    }
}

} // namespace

KMemoryManager::KMemoryManager(Core::System& system_)
    : system{system_}, pool_locks{
                           KLightLock{system_.Kernel()},
                           KLightLock{system_.Kernel()},
                           KLightLock{system_.Kernel()},
                           KLightLock{system_.Kernel()},
                       } {}

void KMemoryManager::Initialize(VAddr management_region, size_t management_region_size) {

    // Clear the management region to zero.
    const VAddr management_region_end = management_region + management_region_size;

    // Reset our manager count.
    num_managers = 0;

    // Traverse the virtual memory layout tree, initializing each manager as appropriate.
    while (num_managers != MaxManagerCount) {
        // Locate the region that should initialize the current manager.
        PAddr region_address = 0;
        size_t region_size = 0;
        Pool region_pool = Pool::Count;
        for (const auto& it : system.Kernel().MemoryLayout().GetPhysicalMemoryRegionTree()) {
            // We only care about regions that we need to create managers for.
            if (!it.IsDerivedFrom(KMemoryRegionType_DramUserPool)) {
                continue;
            }

            // We want to initialize the managers in order.
            if (it.GetAttributes() != num_managers) {
                continue;
            }

            const PAddr cur_start = it.GetAddress();
            const PAddr cur_end = it.GetEndAddress();

            // Validate the region.
            ASSERT(cur_end != 0);
            ASSERT(cur_start != 0);
            ASSERT(it.GetSize() > 0);

            // Update the region's extents.
            if (region_address == 0) {
                region_address = cur_start;
                region_size = it.GetSize();
                region_pool = GetPoolFromMemoryRegionType(it.GetType());
            } else {
                ASSERT(cur_start == region_address + region_size);

                // Update the size.
                region_size = cur_end - region_address;
                ASSERT(GetPoolFromMemoryRegionType(it.GetType()) == region_pool);
            }
        }

        // If we didn't find a region, we're done.
        if (region_size == 0) {
            break;
        }

        // Initialize a new manager for the region.
        Impl* manager = std::addressof(managers[num_managers++]);
        ASSERT(num_managers <= managers.size());

        const size_t cur_size = manager->Initialize(region_address, region_size, management_region,
                                                    management_region_end, region_pool);
        management_region += cur_size;
        ASSERT(management_region <= management_region_end);

        // Insert the manager into the pool list.
        const auto region_pool_index = static_cast<u32>(region_pool);
        if (pool_managers_tail[region_pool_index] == nullptr) {
            pool_managers_head[region_pool_index] = manager;
        } else {
            pool_managers_tail[region_pool_index]->SetNext(manager);
            manager->SetPrev(pool_managers_tail[region_pool_index]);
        }
        pool_managers_tail[region_pool_index] = manager;
    }

    // Free each region to its corresponding heap.
    size_t reserved_sizes[MaxManagerCount] = {};
    const PAddr ini_start = GetInitialProcessBinaryPhysicalAddress();
    const PAddr ini_end = ini_start + InitialProcessBinarySizeMax;
    const PAddr ini_last = ini_end - 1;
    for (const auto& it : system.Kernel().MemoryLayout().GetPhysicalMemoryRegionTree()) {
        if (it.IsDerivedFrom(KMemoryRegionType_DramUserPool)) {
            // Get the manager for the region.
            auto index = it.GetAttributes();
            auto& manager = managers[index];

            const PAddr cur_start = it.GetAddress();
            const PAddr cur_last = it.GetLastAddress();
            const PAddr cur_end = it.GetEndAddress();

            if (cur_start <= ini_start && ini_last <= cur_last) {
                // Free memory before the ini to the heap.
                if (cur_start != ini_start) {
                    manager.Free(cur_start, (ini_start - cur_start) / PageSize);
                }

                // Open/reserve the ini memory.
                manager.OpenFirst(ini_start, InitialProcessBinarySizeMax / PageSize);
                reserved_sizes[it.GetAttributes()] += InitialProcessBinarySizeMax;

                // Free memory after the ini to the heap.
                if (ini_last != cur_last) {
                    ASSERT(cur_end != 0);
                    manager.Free(ini_end, cur_end - ini_end);
                }
            } else {
                // Ensure there's no partial overlap with the ini image.
                if (cur_start <= ini_last) {
                    ASSERT(cur_last < ini_start);
                } else {
                    // Otherwise, check the region for general validity.
                    ASSERT(cur_end != 0);
                }

                // Free the memory to the heap.
                manager.Free(cur_start, it.GetSize() / PageSize);
            }
        }
    }

    // Update the used size for all managers.
    for (size_t i = 0; i < num_managers; ++i) {
        managers[i].SetInitialUsedHeapSize(reserved_sizes[i]);
    }
}

PAddr KMemoryManager::AllocateAndOpenContinuous(size_t num_pages, size_t align_pages, u32 option) {
    // Early return if we're allocating no pages.
    if (num_pages == 0) {
        return 0;
    }

    // Lock the pool that we're allocating from.
    const auto [pool, dir] = DecodeOption(option);
    KScopedLightLock lk(pool_locks[static_cast<std::size_t>(pool)]);

    // Choose a heap based on our page size request.
    const s32 heap_index = KPageHeap::GetAlignedBlockIndex(num_pages, align_pages);

    // Loop, trying to iterate from each block.
    Impl* chosen_manager = nullptr;
    PAddr allocated_block = 0;
    for (chosen_manager = this->GetFirstManager(pool, dir); chosen_manager != nullptr;
         chosen_manager = this->GetNextManager(chosen_manager, dir)) {
        allocated_block = chosen_manager->AllocateBlock(heap_index, true);
        if (allocated_block != 0) {
            break;
        }
    }

    // If we failed to allocate, quit now.
    if (allocated_block == 0) {
        return 0;
    }

    // If we allocated more than we need, free some.
    const size_t allocated_pages = KPageHeap::GetBlockNumPages(heap_index);
    if (allocated_pages > num_pages) {
        chosen_manager->Free(allocated_block + num_pages * PageSize, allocated_pages - num_pages);
    }

    // Open the first reference to the pages.
    chosen_manager->OpenFirst(allocated_block, num_pages);

    return allocated_block;
}

ResultCode KMemoryManager::AllocatePageGroupImpl(KPageLinkedList* out, size_t num_pages, Pool pool,
                                                 Direction dir, bool random) {
    // Choose a heap based on our page size request.
    const s32 heap_index = KPageHeap::GetBlockIndex(num_pages);
    R_UNLESS(0 <= heap_index, ResultOutOfMemory);

    // Ensure that we don't leave anything un-freed.
    auto group_guard = SCOPE_GUARD({
        for (const auto& it : out->Nodes()) {
            auto& manager = this->GetManager(system.Kernel().MemoryLayout(), it.GetAddress());
            const size_t num_pages_to_free =
                std::min(it.GetNumPages(), (manager.GetEndAddress() - it.GetAddress()) / PageSize);
            manager.Free(it.GetAddress(), num_pages_to_free);
        }
    });

    // Keep allocating until we've allocated all our pages.
    for (s32 index = heap_index; index >= 0 && num_pages > 0; index--) {
        const size_t pages_per_alloc = KPageHeap::GetBlockNumPages(index);
        for (Impl* cur_manager = this->GetFirstManager(pool, dir); cur_manager != nullptr;
             cur_manager = this->GetNextManager(cur_manager, dir)) {
            while (num_pages >= pages_per_alloc) {
                // Allocate a block.
                PAddr allocated_block = cur_manager->AllocateBlock(index, random);
                if (allocated_block == 0) {
                    break;
                }

                // Safely add it to our group.
                {
                    auto block_guard =
                        SCOPE_GUARD({ cur_manager->Free(allocated_block, pages_per_alloc); });
                    R_TRY(out->AddBlock(allocated_block, pages_per_alloc));
                    block_guard.Cancel();
                }

                num_pages -= pages_per_alloc;
            }
        }
    }

    // Only succeed if we allocated as many pages as we wanted.
    R_UNLESS(num_pages == 0, ResultOutOfMemory);

    // We succeeded!
    group_guard.Cancel();
    return ResultSuccess;
}

ResultCode KMemoryManager::AllocateAndOpen(KPageLinkedList* out, size_t num_pages, u32 option) {
    ASSERT(out != nullptr);
    ASSERT(out->GetNumPages() == 0);

    // Early return if we're allocating no pages.
    R_SUCCEED_IF(num_pages == 0);

    // Lock the pool that we're allocating from.
    const auto [pool, dir] = DecodeOption(option);
    KScopedLightLock lk(pool_locks[static_cast<size_t>(pool)]);

    // Allocate the page group.
    R_TRY(this->AllocatePageGroupImpl(out, num_pages, pool, dir, false));

    // Open the first reference to the pages.
    for (const auto& block : out->Nodes()) {
        PAddr cur_address = block.GetAddress();
        size_t remaining_pages = block.GetNumPages();
        while (remaining_pages > 0) {
            // Get the manager for the current address.
            auto& manager = this->GetManager(system.Kernel().MemoryLayout(), cur_address);

            // Process part or all of the block.
            const size_t cur_pages =
                std::min(remaining_pages, manager.GetPageOffsetToEnd(cur_address));
            manager.OpenFirst(cur_address, cur_pages);

            // Advance.
            cur_address += cur_pages * PageSize;
            remaining_pages -= cur_pages;
        }
    }

    return ResultSuccess;
}

ResultCode KMemoryManager::AllocateAndOpenForProcess(KPageLinkedList* out, size_t num_pages,
                                                     u32 option, u64 process_id, u8 fill_pattern) {
    ASSERT(out != nullptr);
    ASSERT(out->GetNumPages() == 0);

    // Decode the option.
    const auto [pool, dir] = DecodeOption(option);

    // Allocate the memory.
    {
        // Lock the pool that we're allocating from.
        KScopedLightLock lk(pool_locks[static_cast<size_t>(pool)]);

        // Allocate the page group.
        R_TRY(this->AllocatePageGroupImpl(out, num_pages, pool, dir, false));

        // Open the first reference to the pages.
        for (const auto& block : out->Nodes()) {
            PAddr cur_address = block.GetAddress();
            size_t remaining_pages = block.GetNumPages();
            while (remaining_pages > 0) {
                // Get the manager for the current address.
                auto& manager = this->GetManager(system.Kernel().MemoryLayout(), cur_address);

                // Process part or all of the block.
                const size_t cur_pages =
                    std::min(remaining_pages, manager.GetPageOffsetToEnd(cur_address));
                manager.OpenFirst(cur_address, cur_pages);

                // Advance.
                cur_address += cur_pages * PageSize;
                remaining_pages -= cur_pages;
            }
        }
    }

    // Set all the allocated memory.
    for (const auto& block : out->Nodes()) {
        std::memset(system.DeviceMemory().GetPointer(block.GetAddress()), fill_pattern,
                    block.GetSize());
    }

    return ResultSuccess;
}

void KMemoryManager::Open(PAddr address, size_t num_pages) {
    // Repeatedly open references until we've done so for all pages.
    while (num_pages) {
        auto& manager = this->GetManager(system.Kernel().MemoryLayout(), address);
        const size_t cur_pages = std::min(num_pages, manager.GetPageOffsetToEnd(address));

        {
            KScopedLightLock lk(pool_locks[static_cast<size_t>(manager.GetPool())]);
            manager.Open(address, cur_pages);
        }

        num_pages -= cur_pages;
        address += cur_pages * PageSize;
    }
}

void KMemoryManager::Close(PAddr address, size_t num_pages) {
    // Repeatedly close references until we've done so for all pages.
    while (num_pages) {
        auto& manager = this->GetManager(system.Kernel().MemoryLayout(), address);
        const size_t cur_pages = std::min(num_pages, manager.GetPageOffsetToEnd(address));

        {
            KScopedLightLock lk(pool_locks[static_cast<size_t>(manager.GetPool())]);
            manager.Close(address, cur_pages);
        }

        num_pages -= cur_pages;
        address += cur_pages * PageSize;
    }
}

void KMemoryManager::Close(const KPageLinkedList& pg) {
    for (const auto& node : pg.Nodes()) {
        Close(node.GetAddress(), node.GetNumPages());
    }
}
void KMemoryManager::Open(const KPageLinkedList& pg) {
    for (const auto& node : pg.Nodes()) {
        Open(node.GetAddress(), node.GetNumPages());
    }
}

size_t KMemoryManager::Impl::Initialize(PAddr address, size_t size, VAddr management,
                                        VAddr management_end, Pool p) {
    // Calculate management sizes.
    const size_t ref_count_size = (size / PageSize) * sizeof(u16);
    const size_t optimize_map_size = CalculateOptimizedProcessOverheadSize(size);
    const size_t manager_size = Common::AlignUp(optimize_map_size + ref_count_size, PageSize);
    const size_t page_heap_size = KPageHeap::CalculateManagementOverheadSize(size);
    const size_t total_management_size = manager_size + page_heap_size;
    ASSERT(manager_size <= total_management_size);
    ASSERT(management + total_management_size <= management_end);
    ASSERT(Common::IsAligned(total_management_size, PageSize));

    // Setup region.
    pool = p;
    management_region = management;
    page_reference_counts.resize(
        Kernel::Board::Nintendo::Nx::KSystemControl::Init::GetIntendedMemorySize() / PageSize);
    ASSERT(Common::IsAligned(management_region, PageSize));

    // Initialize the manager's KPageHeap.
    heap.Initialize(address, size, management + manager_size, page_heap_size);

    return total_management_size;
}

size_t KMemoryManager::Impl::CalculateManagementOverheadSize(size_t region_size) {
    const size_t ref_count_size = (region_size / PageSize) * sizeof(u16);
    const size_t optimize_map_size =
        (Common::AlignUp((region_size / PageSize), Common::BitSize<u64>()) /
         Common::BitSize<u64>()) *
        sizeof(u64);
    const size_t manager_meta_size = Common::AlignUp(optimize_map_size + ref_count_size, PageSize);
    const size_t page_heap_size = KPageHeap::CalculateManagementOverheadSize(region_size);
    return manager_meta_size + page_heap_size;
}

} // namespace Kernel

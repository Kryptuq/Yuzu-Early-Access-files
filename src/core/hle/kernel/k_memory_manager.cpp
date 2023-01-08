// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/common_types.h"
#include "common/scope_exit.h"
#include "core/core.h"
#include "core/device_memory.h"
#include "core/hle/kernel/initial_process.h"
#include "core/hle/kernel/k_memory_manager.h"
#include "core/hle/kernel/k_page_group.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_results.h"

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
        UNREACHABLE_MSG("InvalidMemoryRegionType for conversion to Pool");
    }
}

} // namespace

KMemoryManager::KMemoryManager(Core::System& system)
    : m_system{system}, m_memory_layout{system.Kernel().MemoryLayout()},
      m_pool_locks{
          KLightLock{system.Kernel()},
          KLightLock{system.Kernel()},
          KLightLock{system.Kernel()},
          KLightLock{system.Kernel()},
      } {}

void KMemoryManager::Initialize(VAddr management_region, size_t management_region_size) {

    // Clear the management region to zero.
    const VAddr management_region_end = management_region + management_region_size;
    // std::memset(GetVoidPointer(management_region), 0, management_region_size);

    // Reset our manager count.
    m_num_managers = 0;

    // Traverse the virtual memory layout tree, initializing each manager as appropriate.
    while (m_num_managers != MaxManagerCount) {
        // Locate the region that should initialize the current manager.
        PAddr region_address = 0;
        size_t region_size = 0;
        Pool region_pool = Pool::Count;
        for (const auto& it : m_system.Kernel().MemoryLayout().GetPhysicalMemoryRegionTree()) {
            // We only care about regions that we need to create managers for.
            if (!it.IsDerivedFrom(KMemoryRegionType_DramUserPool)) {
                continue;
            }

            // We want to initialize the managers in order.
            if (it.GetAttributes() != m_num_managers) {
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
        Impl* manager = std::addressof(m_managers[m_num_managers++]);
        ASSERT(m_num_managers <= m_managers.size());

        const size_t cur_size = manager->Initialize(region_address, region_size, management_region,
                                                    management_region_end, region_pool);
        management_region += cur_size;
        ASSERT(management_region <= management_region_end);

        // Insert the manager into the pool list.
        const auto region_pool_index = static_cast<u32>(region_pool);
        if (m_pool_managers_tail[region_pool_index] == nullptr) {
            m_pool_managers_head[region_pool_index] = manager;
        } else {
            m_pool_managers_tail[region_pool_index]->SetNext(manager);
            manager->SetPrev(m_pool_managers_tail[region_pool_index]);
        }
        m_pool_managers_tail[region_pool_index] = manager;
    }

    // Free each region to its corresponding heap.
    size_t reserved_sizes[MaxManagerCount] = {};
    const PAddr ini_start = GetInitialProcessBinaryPhysicalAddress();
    const PAddr ini_end = ini_start + InitialProcessBinarySizeMax;
    const PAddr ini_last = ini_end - 1;
    for (const auto& it : m_system.Kernel().MemoryLayout().GetPhysicalMemoryRegionTree()) {
        if (it.IsDerivedFrom(KMemoryRegionType_DramUserPool)) {
            // Get the manager for the region.
            auto& manager = m_managers[it.GetAttributes()];

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
    for (size_t i = 0; i < m_num_managers; ++i) {
        m_managers[i].SetInitialUsedHeapSize(reserved_sizes[i]);
    }
}

Result KMemoryManager::InitializeOptimizedMemory(u64 process_id, Pool pool) {
    UNREACHABLE();
}

void KMemoryManager::FinalizeOptimizedMemory(u64 process_id, Pool pool) {
    UNREACHABLE();
}

PAddr KMemoryManager::AllocateAndOpenContinuous(size_t num_pages, size_t align_pages, u32 option) {
    // Early return if we're allocating no pages.
    if (num_pages == 0) {
        return 0;
    }

    // Lock the pool that we're allocating from.
    const auto [pool, dir] = DecodeOption(option);
    KScopedLightLock lk(m_pool_locks[static_cast<std::size_t>(pool)]);

    // Choose a heap based on our page size request.
    const s32 heap_index = KPageHeap::GetAlignedBlockIndex(num_pages, align_pages);

    // Loop, trying to iterate from each block.
    Impl* chosen_manager = nullptr;
    PAddr allocated_block = 0;
    for (chosen_manager = this->GetFirstManager(pool, dir); chosen_manager != nullptr;
         chosen_manager = this->GetNextManager(chosen_manager, dir)) {
        allocated_block = chosen_manager->AllocateAligned(heap_index, num_pages, align_pages);
        if (allocated_block != 0) {
            break;
        }
    }

    // If we failed to allocate, quit now.
    if (allocated_block == 0) {
        return 0;
    }

    // Maintain the optimized memory bitmap, if we should.
    if (m_has_optimized_process[static_cast<size_t>(pool)]) {
        UNIMPLEMENTED();
    }

    // Open the first reference to the pages.
    chosen_manager->OpenFirst(allocated_block, num_pages);

    return allocated_block;
}

Result KMemoryManager::AllocatePageGroupImpl(KPageGroup* out, size_t num_pages, Pool pool,
                                             Direction dir, bool unoptimized, bool random) {
    // Choose a heap based on our page size request.
    const s32 heap_index = KPageHeap::GetBlockIndex(num_pages);
    R_UNLESS(0 <= heap_index, ResultOutOfMemory);

    // Ensure that we don't leave anything un-freed.
    ON_RESULT_FAILURE {
        for (const auto& it : *out) {
            auto& manager = this->GetManager(it.GetAddress());
            const size_t node_num_pages = std::min<u64>(
                it.GetNumPages(), (manager.GetEndAddress() - it.GetAddress()) / PageSize);
            manager.Free(it.GetAddress(), node_num_pages);
        }
        out->Finalize();
    };

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

                // Ensure we don't leak the block if we fail.
                ON_RESULT_FAILURE_2 {
                    cur_manager->Free(allocated_block, pages_per_alloc);
                };

                // Add the block to our group.
                R_TRY(out->AddBlock(allocated_block, pages_per_alloc));

                // Maintain the optimized memory bitmap, if we should.
                if (unoptimized) {
                    UNIMPLEMENTED();
                }

                num_pages -= pages_per_alloc;
            }
        }
    }

    // Only succeed if we allocated as many pages as we wanted.
    R_UNLESS(num_pages == 0, ResultOutOfMemory);

    // We succeeded!
    R_SUCCEED();
}

Result KMemoryManager::AllocateAndOpen(KPageGroup* out, size_t num_pages, u32 option) {
    ASSERT(out != nullptr);
    ASSERT(out->GetNumPages() == 0);

    // Early return if we're allocating no pages.
    R_SUCCEED_IF(num_pages == 0);

    // Lock the pool that we're allocating from.
    const auto [pool, dir] = DecodeOption(option);
    KScopedLightLock lk(m_pool_locks[static_cast<size_t>(pool)]);

    // Allocate the page group.
    R_TRY(this->AllocatePageGroupImpl(out, num_pages, pool, dir,
                                      m_has_optimized_process[static_cast<size_t>(pool)], true));

    // Open the first reference to the pages.
    for (const auto& block : *out) {
        PAddr cur_address = block.GetAddress();
        size_t remaining_pages = block.GetNumPages();
        while (remaining_pages > 0) {
            // Get the manager for the current address.
            auto& manager = this->GetManager(cur_address);

            // Process part or all of the block.
            const size_t cur_pages =
                std::min(remaining_pages, manager.GetPageOffsetToEnd(cur_address));
            manager.OpenFirst(cur_address, cur_pages);

            // Advance.
            cur_address += cur_pages * PageSize;
            remaining_pages -= cur_pages;
        }
    }

    R_SUCCEED();
}

Result KMemoryManager::AllocateForProcess(KPageGroup* out, size_t num_pages, u32 option,
                                          u64 process_id, u8 fill_pattern) {
    ASSERT(out != nullptr);
    ASSERT(out->GetNumPages() == 0);

    // Decode the option.
    const auto [pool, dir] = DecodeOption(option);

    // Allocate the memory.
    bool optimized;
    {
        // Lock the pool that we're allocating from.
        KScopedLightLock lk(m_pool_locks[static_cast<size_t>(pool)]);

        // Check if we have an optimized process.
        const bool has_optimized = m_has_optimized_process[static_cast<size_t>(pool)];
        const bool is_optimized = m_optimized_process_ids[static_cast<size_t>(pool)] == process_id;

        // Allocate the page group.
        R_TRY(this->AllocatePageGroupImpl(out, num_pages, pool, dir, has_optimized && !is_optimized,
                                          false));

        // Set whether we should optimize.
        optimized = has_optimized && is_optimized;
    }

    // Perform optimized memory tracking, if we should.
    if (optimized) {
        // Iterate over the allocated blocks.
        for (const auto& block : *out) {
            // Get the block extents.
            const PAddr block_address = block.GetAddress();
            const size_t block_pages = block.GetNumPages();

            // If it has no pages, we don't need to do anything.
            if (block_pages == 0) {
                continue;
            }

            // Fill all the pages that we need to fill.
            bool any_new = false;
            {
                PAddr cur_address = block_address;
                size_t remaining_pages = block_pages;
                while (remaining_pages > 0) {
                    // Get the manager for the current address.
                    auto& manager = this->GetManager(cur_address);

                    // Process part or all of the block.
                    const size_t cur_pages =
                        std::min(remaining_pages, manager.GetPageOffsetToEnd(cur_address));
                    any_new =
                        manager.ProcessOptimizedAllocation(cur_address, cur_pages, fill_pattern);

                    // Advance.
                    cur_address += cur_pages * PageSize;
                    remaining_pages -= cur_pages;
                }
            }

            // If there are new pages, update tracking for the allocation.
            if (any_new) {
                // Update tracking for the allocation.
                PAddr cur_address = block_address;
                size_t remaining_pages = block_pages;
                while (remaining_pages > 0) {
                    // Get the manager for the current address.
                    auto& manager = this->GetManager(cur_address);

                    // Lock the pool for the manager.
                    KScopedLightLock lk(m_pool_locks[static_cast<size_t>(manager.GetPool())]);

                    // Track some or all of the current pages.
                    const size_t cur_pages =
                        std::min(remaining_pages, manager.GetPageOffsetToEnd(cur_address));
                    manager.TrackOptimizedAllocation(cur_address, cur_pages);

                    // Advance.
                    cur_address += cur_pages * PageSize;
                    remaining_pages -= cur_pages;
                }
            }
        }
    } else {
        // Set all the allocated memory.
        for (const auto& block : *out) {
            std::memset(m_system.DeviceMemory().GetPointer<void>(block.GetAddress()), fill_pattern,
                        block.GetSize());
        }
    }

    R_SUCCEED();
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
    m_pool = p;
    m_management_region = management;
    m_page_reference_counts.resize(
        Kernel::Board::Nintendo::Nx::KSystemControl::Init::GetIntendedMemorySize() / PageSize);
    ASSERT(Common::IsAligned(m_management_region, PageSize));

    // Initialize the manager's KPageHeap.
    m_heap.Initialize(address, size, management + manager_size, page_heap_size);

    return total_management_size;
}

void KMemoryManager::Impl::TrackUnoptimizedAllocation(PAddr block, size_t num_pages) {
    UNREACHABLE();
}

void KMemoryManager::Impl::TrackOptimizedAllocation(PAddr block, size_t num_pages) {
    UNREACHABLE();
}

bool KMemoryManager::Impl::ProcessOptimizedAllocation(PAddr block, size_t num_pages,
                                                      u8 fill_pattern) {
    UNREACHABLE();
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

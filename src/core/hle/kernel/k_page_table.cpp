// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/assert.h"
#include "common/literals.h"
#include "common/scope_exit.h"
#include "core/core.h"
#include "core/hle/kernel/k_address_space_info.h"
#include "core/hle/kernel/k_memory_block.h"
#include "core/hle/kernel/k_memory_block_manager.h"
#include "core/hle/kernel/k_page_linked_list.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/k_scoped_resource_reservation.h"
#include "core/hle/kernel/k_system_control.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_results.h"
#include "core/memory.h"

namespace Kernel {

namespace {

using namespace Common::Literals;

constexpr std::size_t GetAddressSpaceWidthFromType(FileSys::ProgramAddressSpaceType as_type) {
    switch (as_type) {
    case FileSys::ProgramAddressSpaceType::Is32Bit:
    case FileSys::ProgramAddressSpaceType::Is32BitNoMap:
        return 32;
    case FileSys::ProgramAddressSpaceType::Is36Bit:
        return 36;
    case FileSys::ProgramAddressSpaceType::Is39Bit:
        return 39;
    default:
        ASSERT(false);
        return {};
    }
}

} // namespace

KPageTable::KPageTable(Core::System& system_)
    : general_lock{system_.Kernel()}, map_physical_memory_lock{system_.Kernel()}, system{system_} {}

KPageTable::~KPageTable() = default;

ResultCode KPageTable::InitializeForProcess(FileSys::ProgramAddressSpaceType as_type,
                                            bool enable_aslr, VAddr code_addr,
                                            std::size_t code_size, KMemoryManager::Pool pool) {

    const auto GetSpaceStart = [this](KAddressSpaceInfo::Type type) {
        return KAddressSpaceInfo::GetAddressSpaceStart(address_space_width, type);
    };
    const auto GetSpaceSize = [this](KAddressSpaceInfo::Type type) {
        return KAddressSpaceInfo::GetAddressSpaceSize(address_space_width, type);
    };

    //  Set our width and heap/alias sizes
    address_space_width = GetAddressSpaceWidthFromType(as_type);
    const VAddr start = 0;
    const VAddr end{1ULL << address_space_width};
    std::size_t alias_region_size{GetSpaceSize(KAddressSpaceInfo::Type::Alias)};
    std::size_t heap_region_size{GetSpaceSize(KAddressSpaceInfo::Type::Heap)};

    ASSERT(code_addr < code_addr + code_size);
    ASSERT(code_addr + code_size - 1 <= end - 1);

    // Adjust heap/alias size if we don't have an alias region
    if (as_type == FileSys::ProgramAddressSpaceType::Is32BitNoMap) {
        heap_region_size += alias_region_size;
        alias_region_size = 0;
    }

    // Set code regions and determine remaining
    constexpr std::size_t RegionAlignment{2_MiB};
    VAddr process_code_start{};
    VAddr process_code_end{};
    std::size_t stack_region_size{};
    std::size_t kernel_map_region_size{};

    if (address_space_width == 39) {
        alias_region_size = GetSpaceSize(KAddressSpaceInfo::Type::Alias);
        heap_region_size = GetSpaceSize(KAddressSpaceInfo::Type::Heap);
        stack_region_size = GetSpaceSize(KAddressSpaceInfo::Type::Stack);
        kernel_map_region_size = GetSpaceSize(KAddressSpaceInfo::Type::MapSmall);
        code_region_start = GetSpaceStart(KAddressSpaceInfo::Type::Map39Bit);
        code_region_end = code_region_start + GetSpaceSize(KAddressSpaceInfo::Type::Map39Bit);
        alias_code_region_start = code_region_start;
        alias_code_region_end = code_region_end;
        process_code_start = Common::AlignDown(code_addr, RegionAlignment);
        process_code_end = Common::AlignUp(code_addr + code_size, RegionAlignment);
    } else {
        stack_region_size = 0;
        kernel_map_region_size = 0;
        code_region_start = GetSpaceStart(KAddressSpaceInfo::Type::MapSmall);
        code_region_end = code_region_start + GetSpaceSize(KAddressSpaceInfo::Type::MapSmall);
        stack_region_start = code_region_start;
        alias_code_region_start = code_region_start;
        alias_code_region_end = GetSpaceStart(KAddressSpaceInfo::Type::MapLarge) +
                                GetSpaceSize(KAddressSpaceInfo::Type::MapLarge);
        stack_region_end = code_region_end;
        kernel_map_region_start = code_region_start;
        kernel_map_region_end = code_region_end;
        process_code_start = code_region_start;
        process_code_end = code_region_end;
    }

    // Set other basic fields
    is_aslr_enabled = enable_aslr;
    address_space_start = start;
    address_space_end = end;
    is_kernel = false;

    // Determine the region we can place our undetermineds in
    VAddr alloc_start{};
    std::size_t alloc_size{};
    if ((process_code_start - code_region_start) >= (end - process_code_end)) {
        alloc_start = code_region_start;
        alloc_size = process_code_start - code_region_start;
    } else {
        alloc_start = process_code_end;
        alloc_size = end - process_code_end;
    }
    const std::size_t needed_size{
        (alias_region_size + heap_region_size + stack_region_size + kernel_map_region_size)};
    if (alloc_size < needed_size) {
        ASSERT(false);
        return ResultOutOfMemory;
    }

    const std::size_t remaining_size{alloc_size - needed_size};

    // Determine random placements for each region
    std::size_t alias_rnd{}, heap_rnd{}, stack_rnd{}, kmap_rnd{};
    if (enable_aslr) {
        alias_rnd = KSystemControl::GenerateRandomRange(0, remaining_size / RegionAlignment) *
                    RegionAlignment;
        heap_rnd = KSystemControl::GenerateRandomRange(0, remaining_size / RegionAlignment) *
                   RegionAlignment;
        stack_rnd = KSystemControl::GenerateRandomRange(0, remaining_size / RegionAlignment) *
                    RegionAlignment;
        kmap_rnd = KSystemControl::GenerateRandomRange(0, remaining_size / RegionAlignment) *
                   RegionAlignment;
    }

    // Setup heap and alias regions
    alias_region_start = alloc_start + alias_rnd;
    alias_region_end = alias_region_start + alias_region_size;
    heap_region_start = alloc_start + heap_rnd;
    heap_region_end = heap_region_start + heap_region_size;

    if (alias_rnd <= heap_rnd) {
        heap_region_start += alias_region_size;
        heap_region_end += alias_region_size;
    } else {
        alias_region_start += heap_region_size;
        alias_region_end += heap_region_size;
    }

    // Setup stack region
    if (stack_region_size) {
        stack_region_start = alloc_start + stack_rnd;
        stack_region_end = stack_region_start + stack_region_size;

        if (alias_rnd < stack_rnd) {
            stack_region_start += alias_region_size;
            stack_region_end += alias_region_size;
        } else {
            alias_region_start += stack_region_size;
            alias_region_end += stack_region_size;
        }

        if (heap_rnd < stack_rnd) {
            stack_region_start += heap_region_size;
            stack_region_end += heap_region_size;
        } else {
            heap_region_start += stack_region_size;
            heap_region_end += stack_region_size;
        }
    }

    // Setup kernel map region
    if (kernel_map_region_size) {
        kernel_map_region_start = alloc_start + kmap_rnd;
        kernel_map_region_end = kernel_map_region_start + kernel_map_region_size;

        if (alias_rnd < kmap_rnd) {
            kernel_map_region_start += alias_region_size;
            kernel_map_region_end += alias_region_size;
        } else {
            alias_region_start += kernel_map_region_size;
            alias_region_end += kernel_map_region_size;
        }

        if (heap_rnd < kmap_rnd) {
            kernel_map_region_start += heap_region_size;
            kernel_map_region_end += heap_region_size;
        } else {
            heap_region_start += kernel_map_region_size;
            heap_region_end += kernel_map_region_size;
        }

        if (stack_region_size) {
            if (stack_rnd < kmap_rnd) {
                kernel_map_region_start += stack_region_size;
                kernel_map_region_end += stack_region_size;
            } else {
                stack_region_start += kernel_map_region_size;
                stack_region_end += kernel_map_region_size;
            }
        }
    }

    // Set heap members
    current_heap_end = heap_region_start;
    max_heap_size = 0;
    max_physical_memory_size = 0;

    // Ensure that we regions inside our address space
    auto IsInAddressSpace = [&](VAddr addr) {
        return address_space_start <= addr && addr <= address_space_end;
    };
    ASSERT(IsInAddressSpace(alias_region_start));
    ASSERT(IsInAddressSpace(alias_region_end));
    ASSERT(IsInAddressSpace(heap_region_start));
    ASSERT(IsInAddressSpace(heap_region_end));
    ASSERT(IsInAddressSpace(stack_region_start));
    ASSERT(IsInAddressSpace(stack_region_end));
    ASSERT(IsInAddressSpace(kernel_map_region_start));
    ASSERT(IsInAddressSpace(kernel_map_region_end));

    // Ensure that we selected regions that don't overlap
    const VAddr alias_start{alias_region_start};
    const VAddr alias_last{alias_region_end - 1};
    const VAddr heap_start{heap_region_start};
    const VAddr heap_last{heap_region_end - 1};
    const VAddr stack_start{stack_region_start};
    const VAddr stack_last{stack_region_end - 1};
    const VAddr kmap_start{kernel_map_region_start};
    const VAddr kmap_last{kernel_map_region_end - 1};
    ASSERT(alias_last < heap_start || heap_last < alias_start);
    ASSERT(alias_last < stack_start || stack_last < alias_start);
    ASSERT(alias_last < kmap_start || kmap_last < alias_start);
    ASSERT(heap_last < stack_start || stack_last < heap_start);
    ASSERT(heap_last < kmap_start || kmap_last < heap_start);

    current_heap_end = heap_region_start;
    max_heap_size = 0;
    mapped_physical_memory_size = 0;
    memory_pool = pool;

    page_table_impl.Resize(address_space_width, PageBits);

    return InitializeMemoryLayout(start, end);
}

ResultCode KPageTable::MapProcessCode(VAddr addr, std::size_t num_pages, KMemoryState state,
                                      KMemoryPermission perm) {
    const u64 size{num_pages * PageSize};

    // Validate the mapping request.
    R_UNLESS(this->CanContain(addr, size, state), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Verify that the destination memory is unmapped.
    R_TRY(this->CheckMemoryState(addr, size, KMemoryState::All, KMemoryState::Free,
                                 KMemoryPermission::None, KMemoryPermission::None,
                                 KMemoryAttribute::None, KMemoryAttribute::None));
    KPageLinkedList pg;
    R_TRY(system.Kernel().MemoryManager().AllocateAndOpen(
        &pg, num_pages,
        KMemoryManager::EncodeOption(KMemoryManager::Pool::Application, allocation_option)));

    R_TRY(Operate(addr, num_pages, pg, OperationType::MapGroup));

    block_manager->Update(addr, num_pages, state, perm);

    return ResultSuccess;
}

ResultCode KPageTable::MapCodeMemory(VAddr dst_address, VAddr src_address, std::size_t size) {
    // Validate the mapping request.
    R_UNLESS(this->CanContain(dst_address, size, KMemoryState::AliasCode),
             ResultInvalidMemoryRegion);

    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Verify that the source memory is normal heap.
    KMemoryState src_state{};
    KMemoryPermission src_perm{};
    std::size_t num_src_allocator_blocks{};
    R_TRY(this->CheckMemoryState(&src_state, &src_perm, nullptr, &num_src_allocator_blocks,
                                 src_address, size, KMemoryState::All, KMemoryState::Normal,
                                 KMemoryPermission::All, KMemoryPermission::UserReadWrite,
                                 KMemoryAttribute::All, KMemoryAttribute::None));

    // Verify that the destination memory is unmapped.
    std::size_t num_dst_allocator_blocks{};
    R_TRY(this->CheckMemoryState(&num_dst_allocator_blocks, dst_address, size, KMemoryState::All,
                                 KMemoryState::Free, KMemoryPermission::None,
                                 KMemoryPermission::None, KMemoryAttribute::None,
                                 KMemoryAttribute::None));

    // Map the code memory.
    {
        // Determine the number of pages being operated on.
        const std::size_t num_pages = size / PageSize;

        // Create page groups for the memory being mapped.
        KPageLinkedList pg;
        AddRegionToPages(src_address, num_pages, pg);

        // Reprotect the source as kernel-read/not mapped.
        const auto new_perm = static_cast<KMemoryPermission>(KMemoryPermission::KernelRead |
                                                             KMemoryPermission::NotMapped);
        R_TRY(Operate(src_address, num_pages, new_perm, OperationType::ChangePermissions));

        // Ensure that we unprotect the source pages on failure.
        auto unprot_guard = SCOPE_GUARD({
            ASSERT(this->Operate(src_address, num_pages, src_perm, OperationType::ChangePermissions)
                       .IsSuccess());
        });

        // Map the alias pages.
        R_TRY(MapPages(dst_address, pg, new_perm));

        // We successfully mapped the alias pages, so we don't need to unprotect the src pages on
        // failure.
        unprot_guard.Cancel();

        // Apply the memory block updates.
        block_manager->Update(src_address, num_pages, src_state, new_perm,
                              KMemoryAttribute::Locked);
        block_manager->Update(dst_address, num_pages, KMemoryState::AliasCode, new_perm,
                              KMemoryAttribute::None);
    }

    return ResultSuccess;
}

ResultCode KPageTable::UnmapCodeMemory(VAddr dst_address, VAddr src_address, std::size_t size,
                                       ICacheInvalidationStrategy icache_invalidation_strategy) {
    // Validate the mapping request.
    R_UNLESS(this->CanContain(dst_address, size, KMemoryState::AliasCode),
             ResultInvalidMemoryRegion);

    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Verify that the source memory is locked normal heap.
    std::size_t num_src_allocator_blocks{};
    R_TRY(this->CheckMemoryState(std::addressof(num_src_allocator_blocks), src_address, size,
                                 KMemoryState::All, KMemoryState::Normal, KMemoryPermission::None,
                                 KMemoryPermission::None, KMemoryAttribute::All,
                                 KMemoryAttribute::Locked));

    // Verify that the destination memory is aliasable code.
    std::size_t num_dst_allocator_blocks{};
    R_TRY(this->CheckMemoryStateContiguous(
        std::addressof(num_dst_allocator_blocks), dst_address, size, KMemoryState::FlagCanCodeAlias,
        KMemoryState::FlagCanCodeAlias, KMemoryPermission::None, KMemoryPermission::None,
        KMemoryAttribute::All, KMemoryAttribute::None));

    // Determine whether any pages being unmapped are code.
    bool any_code_pages = false;
    {
        KMemoryBlockManager::const_iterator it = block_manager->FindIterator(dst_address);
        while (true) {
            // Get the memory info.
            const KMemoryInfo info = it->GetMemoryInfo();

            // Check if the memory has code flag.
            if ((info.GetState() & KMemoryState::FlagCode) != KMemoryState::None) {
                any_code_pages = true;
                break;
            }

            // Check if we're done.
            if (dst_address + size - 1 <= info.GetLastAddress()) {
                break;
            }

            // Advance.
            ++it;
        }
    }

    // Ensure that we maintain the instruction cache.
    bool reprotected_pages = false;
    SCOPE_EXIT({
        if (reprotected_pages && any_code_pages) {
            if (icache_invalidation_strategy == ICacheInvalidationStrategy::InvalidateRange) {
                system.InvalidateCpuInstructionCacheRange(dst_address, size);
            } else {
                system.InvalidateCpuInstructionCaches();
            }
        }
    });

    // Unmap.
    {
        // Determine the number of pages being operated on.
        const std::size_t num_pages = size / PageSize;

        // Unmap the aliased copy of the pages.
        R_TRY(Operate(dst_address, num_pages, KMemoryPermission::None, OperationType::Unmap));

        // Try to set the permissions for the source pages back to what they should be.
        R_TRY(Operate(src_address, num_pages, KMemoryPermission::UserReadWrite,
                      OperationType::ChangePermissions));

        // Apply the memory block updates.
        block_manager->Update(dst_address, num_pages, KMemoryState::None);
        block_manager->Update(src_address, num_pages, KMemoryState::Normal,
                              KMemoryPermission::UserReadWrite);

        // Note that we reprotected pages.
        reprotected_pages = true;
    }

    return ResultSuccess;
}

VAddr KPageTable::FindFreeArea(VAddr region_start, std::size_t region_num_pages,
                               std::size_t num_pages, std::size_t alignment, std::size_t offset,
                               std::size_t guard_pages) {
    VAddr address = 0;

    if (num_pages <= region_num_pages) {
        if (this->IsAslrEnabled()) {
            // Try to directly find a free area up to 8 times.
            for (std::size_t i = 0; i < 8; i++) {
                const std::size_t random_offset =
                    KSystemControl::GenerateRandomRange(
                        0, (region_num_pages - num_pages - guard_pages) * PageSize / alignment) *
                    alignment;
                const VAddr candidate =
                    Common::AlignDown((region_start + random_offset), alignment) + offset;

                KMemoryInfo info = this->QueryInfoImpl(candidate);

                if (info.state != KMemoryState::Free) {
                    continue;
                }
                if (region_start > candidate) {
                    continue;
                }
                if (info.GetAddress() + guard_pages * PageSize > candidate) {
                    continue;
                }

                const VAddr candidate_end = candidate + (num_pages + guard_pages) * PageSize - 1;
                if (candidate_end > info.GetLastAddress()) {
                    continue;
                }
                if (candidate_end > region_start + region_num_pages * PageSize - 1) {
                    continue;
                }

                address = candidate;
                break;
            }
            // Fall back to finding the first free area with a random offset.
            if (address == 0) {
                // NOTE: Nintendo does not account for guard pages here.
                // This may theoretically cause an offset to be chosen that cannot be mapped. We
                // will account for guard pages.
                const std::size_t offset_pages = KSystemControl::GenerateRandomRange(
                    0, region_num_pages - num_pages - guard_pages);
                address = block_manager->FindFreeArea(region_start + offset_pages * PageSize,
                                                      region_num_pages - offset_pages, num_pages,
                                                      alignment, offset, guard_pages);
            }
        }

        // Find the first free area.
        if (address == 0) {
            address = block_manager->FindFreeArea(region_start, region_num_pages, num_pages,
                                                  alignment, offset, guard_pages);
        }
    }

    return address;
}

ResultCode KPageTable::MakePageGroup(KPageLinkedList& pg, VAddr addr, size_t num_pages) {
    ASSERT(this->IsLockedByCurrentThread());

    const size_t size = num_pages * PageSize;

    // We're making a new group, not adding to an existing one.
    R_UNLESS(pg.Empty(), ResultInvalidCurrentMemory);

    // Begin traversal.
    Common::PageTable::TraversalContext context;
    Common::PageTable::TraversalEntry next_entry;
    R_UNLESS(page_table_impl.BeginTraversal(next_entry, context, addr), ResultInvalidCurrentMemory);

    // Prepare tracking variables.
    PAddr cur_addr = next_entry.phys_addr;
    size_t cur_size = next_entry.block_size - (cur_addr & (next_entry.block_size - 1));
    size_t tot_size = cur_size;

    // Iterate, adding to group as we go.
    const auto& memory_layout = system.Kernel().MemoryLayout();
    while (tot_size < size) {
        R_UNLESS(page_table_impl.ContinueTraversal(next_entry, context),
                 ResultInvalidCurrentMemory);

        if (next_entry.phys_addr != (cur_addr + cur_size)) {
            const size_t cur_pages = cur_size / PageSize;

            R_UNLESS(IsHeapPhysicalAddress(memory_layout, cur_addr), ResultInvalidCurrentMemory);
            R_TRY(pg.AddBlock(cur_addr, cur_pages));

            cur_addr = next_entry.phys_addr;
            cur_size = next_entry.block_size;
        } else {
            cur_size += next_entry.block_size;
        }

        tot_size += next_entry.block_size;
    }

    // Ensure we add the right amount for the last block.
    if (tot_size > size) {
        cur_size -= (tot_size - size);
    }

    // Add the last block.
    const size_t cur_pages = cur_size / PageSize;
    R_UNLESS(IsHeapPhysicalAddress(memory_layout, cur_addr), ResultInvalidCurrentMemory);
    R_TRY(pg.AddBlock(cur_addr, cur_pages));

    return ResultSuccess;
}

bool KPageTable::IsValidPageGroup(const KPageLinkedList& pg_ll, VAddr addr, size_t num_pages) {
    ASSERT(this->IsLockedByCurrentThread());

    const size_t size = num_pages * PageSize;
    const auto& pg = pg_ll.Nodes();
    const auto& memory_layout = system.Kernel().MemoryLayout();

    // Empty groups are necessarily invalid.
    if (pg.empty()) {
        return false;
    }

    // We're going to validate that the group we'd expect is the group we see.
    auto cur_it = pg.begin();
    PAddr cur_block_address = cur_it->GetAddress();
    size_t cur_block_pages = cur_it->GetNumPages();

    auto UpdateCurrentIterator = [&]() {
        if (cur_block_pages == 0) {
            if ((++cur_it) == pg.end()) {
                return false;
            }

            cur_block_address = cur_it->GetAddress();
            cur_block_pages = cur_it->GetNumPages();
        }
        return true;
    };

    // Begin traversal.
    Common::PageTable::TraversalContext context;
    Common::PageTable::TraversalEntry next_entry;
    if (!page_table_impl.BeginTraversal(next_entry, context, addr)) {
        return false;
    }

    // Prepare tracking variables.
    PAddr cur_addr = next_entry.phys_addr;
    size_t cur_size = next_entry.block_size - (cur_addr & (next_entry.block_size - 1));
    size_t tot_size = cur_size;

    // Iterate, comparing expected to actual.
    while (tot_size < size) {
        if (!page_table_impl.ContinueTraversal(next_entry, context)) {
            return false;
        }

        if (next_entry.phys_addr != (cur_addr + cur_size)) {
            const size_t cur_pages = cur_size / PageSize;

            if (!IsHeapPhysicalAddress(memory_layout, cur_addr)) {
                return false;
            }

            if (!UpdateCurrentIterator()) {
                return false;
            }

            if (cur_block_address != cur_addr || cur_block_pages < cur_pages) {
                return false;
            }

            cur_block_address += cur_size;
            cur_block_pages -= cur_pages;
            cur_addr = next_entry.phys_addr;
            cur_size = next_entry.block_size;
        } else {
            cur_size += next_entry.block_size;
        }

        tot_size += next_entry.block_size;
    }

    // Ensure we compare the right amount for the last block.
    if (tot_size > size) {
        cur_size -= (tot_size - size);
    }

    if (!IsHeapPhysicalAddress(memory_layout, cur_addr)) {
        return false;
    }

    if (!UpdateCurrentIterator()) {
        return false;
    }

    return cur_block_address == cur_addr && cur_block_pages == (cur_size / PageSize);
}

ResultCode KPageTable::UnmapProcessMemory(VAddr dst_addr, std::size_t size,
                                          KPageTable& src_page_table, VAddr src_addr) {
    KScopedLightLock lk(general_lock);

    const std::size_t num_pages{size / PageSize};

    // Check that the memory is mapped in the destination process.
    size_t num_allocator_blocks;
    R_TRY(CheckMemoryState(&num_allocator_blocks, dst_addr, size, KMemoryState::All,
                           KMemoryState::SharedCode, KMemoryPermission::UserReadWrite,
                           KMemoryPermission::UserReadWrite, KMemoryAttribute::All,
                           KMemoryAttribute::None));

    // Check that the memory is mapped in the source process.
    R_TRY(src_page_table.CheckMemoryState(src_addr, size, KMemoryState::FlagCanMapProcess,
                                          KMemoryState::FlagCanMapProcess, KMemoryPermission::None,
                                          KMemoryPermission::None, KMemoryAttribute::All,
                                          KMemoryAttribute::None));

    CASCADE_CODE(Operate(dst_addr, num_pages, KMemoryPermission::None, OperationType::Unmap));

    // Apply the memory block update.
    block_manager->Update(dst_addr, num_pages, KMemoryState::Free, KMemoryPermission::None,
                          KMemoryAttribute::None);

    system.InvalidateCpuInstructionCaches();

    return ResultSuccess;
}

ResultCode KPageTable::MapPhysicalMemory(VAddr address, std::size_t size) {
    // Lock the physical memory lock.
    KScopedLightLock map_phys_mem_lk(map_physical_memory_lock);

    // Calculate the last address for convenience.
    const VAddr last_address = address + size - 1;

    // Define iteration variables.
    VAddr cur_address;
    std::size_t mapped_size;

    // The entire mapping process can be retried.
    while (true) {
        // Check if the memory is already mapped.
        {
            // Lock the table.
            KScopedLightLock lk(general_lock);

            // Iterate over the memory.
            cur_address = address;
            mapped_size = 0;

            auto it = block_manager->FindIterator(cur_address);
            while (true) {
                // Check that the iterator is valid.
                ASSERT(it != block_manager->end());

                // Get the memory info.
                const KMemoryInfo info = it->GetMemoryInfo();

                // Check if we're done.
                if (last_address <= info.GetLastAddress()) {
                    if (info.GetState() != KMemoryState::Free) {
                        mapped_size += (last_address + 1 - cur_address);
                    }
                    break;
                }

                // Track the memory if it's mapped.
                if (info.GetState() != KMemoryState::Free) {
                    mapped_size += VAddr(info.GetEndAddress()) - cur_address;
                }

                // Advance.
                cur_address = info.GetEndAddress();
                ++it;
            }

            // If the size mapped is the size requested, we've nothing to do.
            R_SUCCEED_IF(size == mapped_size);
        }

        // Allocate and map the memory.
        {
            // Reserve the memory from the process resource limit.
            KScopedResourceReservation memory_reservation(
                system.Kernel().CurrentProcess()->GetResourceLimit(),
                LimitableResource::PhysicalMemory, size - mapped_size);
            R_UNLESS(memory_reservation.Succeeded(), ResultLimitReached);

            // Allocate pages for the new memory.
            KPageLinkedList pg;
            R_TRY(system.Kernel().MemoryManager().AllocateAndOpenForProcess(
                &pg, (size - mapped_size) / PageSize,
                KMemoryManager::EncodeOption(memory_pool, allocation_option), 0, 0));

            // Map the memory.
            {
                // Lock the table.
                KScopedLightLock lk(general_lock);

                size_t num_allocator_blocks = 0;

                // Verify that nobody has mapped memory since we first checked.
                {
                    // Iterate over the memory.
                    size_t checked_mapped_size = 0;
                    cur_address = address;

                    auto it = block_manager->FindIterator(cur_address);
                    while (true) {
                        // Check that the iterator is valid.
                        ASSERT(it != block_manager->end());

                        // Get the memory info.
                        const KMemoryInfo info = it->GetMemoryInfo();

                        const bool is_free = info.GetState() == KMemoryState::Free;
                        if (is_free) {
                            if (info.GetAddress() < address) {
                                ++num_allocator_blocks;
                            }
                            if (last_address < info.GetLastAddress()) {
                                ++num_allocator_blocks;
                            }
                        }

                        // Check if we're done.
                        if (last_address <= info.GetLastAddress()) {
                            if (!is_free) {
                                checked_mapped_size += (last_address + 1 - cur_address);
                            }
                            break;
                        }

                        // Track the memory if it's mapped.
                        if (!is_free) {
                            checked_mapped_size += VAddr(info.GetEndAddress()) - cur_address;
                        }

                        // Advance.
                        cur_address = info.GetEndAddress();
                        ++it;
                    }

                    // If the size now isn't what it was before, somebody mapped or unmapped
                    // concurrently. If this happened, retry.
                    if (mapped_size != checked_mapped_size) {
                        continue;
                    }
                }

                // Reset the current tracking address, and make sure we clean up on failure.
                cur_address = address;
                auto unmap_guard = detail::ScopeExit([&] {
                    if (cur_address > address) {
                        const VAddr last_unmap_address = cur_address - 1;

                        // Iterate, unmapping the pages.
                        cur_address = address;

                        auto it = block_manager->FindIterator(cur_address);
                        while (true) {
                            // Check that the iterator is valid.
                            ASSERT(it != block_manager->end());

                            // Get the memory info.
                            const KMemoryInfo info = it->GetMemoryInfo();

                            // If the memory state is free, we mapped it and need to unmap it.
                            if (info.GetState() == KMemoryState::Free) {
                                // Determine the range to unmap.
                                const size_t cur_pages =
                                    std::min(VAddr(info.GetEndAddress()) - cur_address,
                                             last_unmap_address + 1 - cur_address) /
                                    PageSize;

                                // Unmap.
                                ASSERT(Operate(cur_address, cur_pages, KMemoryPermission::None,
                                               OperationType::Unmap)
                                           .IsSuccess());
                            }

                            // Check if we're done.
                            if (last_unmap_address <= info.GetLastAddress()) {
                                break;
                            }

                            // Advance.
                            cur_address = info.GetEndAddress();
                            ++it;
                        }
                    }
                });

                // Iterate over the memory.
                auto pg_it = pg.Nodes().begin();
                PAddr pg_phys_addr = pg_it->GetAddress();
                size_t pg_pages = pg_it->GetNumPages();

                auto it = block_manager->FindIterator(cur_address);
                while (true) {
                    // Check that the iterator is valid.
                    ASSERT(it != block_manager->end());

                    // Get the memory info.
                    const KMemoryInfo info = it->GetMemoryInfo();

                    // If it's unmapped, we need to map it.
                    if (info.GetState() == KMemoryState::Free) {
                        // Determine the range to map.
                        size_t map_pages = std::min(VAddr(info.GetEndAddress()) - cur_address,
                                                    last_address + 1 - cur_address) /
                                           PageSize;

                        // While we have pages to map, map them.
                        while (map_pages > 0) {
                            // Check if we're at the end of the physical block.
                            if (pg_pages == 0) {
                                // Ensure there are more pages to map.
                                ASSERT(pg_it != pg.Nodes().end());

                                // Advance our physical block.
                                ++pg_it;
                                pg_phys_addr = pg_it->GetAddress();
                                pg_pages = pg_it->GetNumPages();
                            }

                            // Map whatever we can.
                            const size_t cur_pages = std::min(pg_pages, map_pages);
                            R_TRY(Operate(cur_address, cur_pages, KMemoryPermission::UserReadWrite,
                                          OperationType::Map, pg_phys_addr));

                            // Advance.
                            cur_address += cur_pages * PageSize;
                            map_pages -= cur_pages;

                            pg_phys_addr += cur_pages * PageSize;
                            pg_pages -= cur_pages;
                        }
                    }

                    // Check if we're done.
                    if (last_address <= info.GetLastAddress()) {
                        break;
                    }

                    // Advance.
                    cur_address = info.GetEndAddress();
                    ++it;
                }

                // We succeeded, so commit the memory reservation.
                memory_reservation.Commit();

                // Increase our tracked mapped size.
                mapped_physical_memory_size += (size - mapped_size);

                // Update the relevant memory blocks.
                block_manager->Update(address, size / PageSize, KMemoryState::Free,
                                      KMemoryPermission::None, KMemoryAttribute::None,
                                      KMemoryState::Normal, KMemoryPermission::UserReadWrite,
                                      KMemoryAttribute::None);

                // Cancel our guard.
                unmap_guard.Cancel();

                return ResultSuccess;
            }
        }
    }
}

ResultCode KPageTable::UnmapPhysicalMemory(VAddr address, std::size_t size) {
    // Lock the physical memory lock.
    KScopedLightLock map_phys_mem_lk(map_physical_memory_lock);

    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Calculate the last address for convenience.
    const VAddr last_address = address + size - 1;

    // Define iteration variables.
    VAddr cur_address = 0;
    std::size_t mapped_size = 0;
    std::size_t num_allocator_blocks = 0;

    // Check if the memory is mapped.
    {
        // Iterate over the memory.
        cur_address = address;
        mapped_size = 0;

        auto it = block_manager->FindIterator(cur_address);
        while (true) {
            // Check that the iterator is valid.
            ASSERT(it != block_manager->end());

            // Get the memory info.
            const KMemoryInfo info = it->GetMemoryInfo();

            // Verify the memory's state.
            const bool is_normal = info.GetState() == KMemoryState::Normal &&
                                   info.GetAttribute() == KMemoryAttribute::None;
            const bool is_free = info.GetState() == KMemoryState::Free;
            R_UNLESS(is_normal || is_free, ResultInvalidCurrentMemory);

            if (is_normal) {
                R_UNLESS(info.GetAttribute() == KMemoryAttribute::None, ResultInvalidCurrentMemory);

                if (info.GetAddress() < address) {
                    ++num_allocator_blocks;
                }
                if (last_address < info.GetLastAddress()) {
                    ++num_allocator_blocks;
                }
            }

            // Check if we're done.
            if (last_address <= info.GetLastAddress()) {
                if (is_normal) {
                    mapped_size += (last_address + 1 - cur_address);
                }
                break;
            }

            // Track the memory if it's mapped.
            if (is_normal) {
                mapped_size += VAddr(info.GetEndAddress()) - cur_address;
            }

            // Advance.
            cur_address = info.GetEndAddress();
            ++it;
        }

        // If there's nothing mapped, we've nothing to do.
        R_SUCCEED_IF(mapped_size == 0);
    }

    // Make a page group for the unmap region.
    KPageLinkedList pg;
    {
        auto& impl = this->PageTableImpl();

        // Begin traversal.
        Common::PageTable::TraversalContext context;
        Common::PageTable::TraversalEntry cur_entry = {.phys_addr = 0, .block_size = 0};
        bool cur_valid = false;
        Common::PageTable::TraversalEntry next_entry;
        bool next_valid = false;
        size_t tot_size = 0;

        cur_address = address;
        next_valid = impl.BeginTraversal(next_entry, context, cur_address);
        next_entry.block_size =
            (next_entry.block_size - (next_entry.phys_addr & (next_entry.block_size - 1)));

        // Iterate, building the group.
        while (true) {
            if ((!next_valid && !cur_valid) ||
                (next_valid && cur_valid &&
                 next_entry.phys_addr == cur_entry.phys_addr + cur_entry.block_size)) {
                cur_entry.block_size += next_entry.block_size;
            } else {
                if (cur_valid) {
                    // ASSERT(IsHeapPhysicalAddress(cur_entry.phys_addr));
                    R_TRY(pg.AddBlock(cur_entry.phys_addr, cur_entry.block_size / PageSize));
                }

                // Update tracking variables.
                tot_size += cur_entry.block_size;
                cur_entry = next_entry;
                cur_valid = next_valid;
            }

            if (cur_entry.block_size + tot_size >= size) {
                break;
            }

            next_valid = impl.ContinueTraversal(next_entry, context);
        }

        // Add the last block.
        if (cur_valid) {
            // ASSERT(IsHeapPhysicalAddress(cur_entry.phys_addr));
            R_TRY(pg.AddBlock(cur_entry.phys_addr, (size - tot_size) / PageSize));
        }
    }
    ASSERT(pg.GetNumPages() == mapped_size / PageSize);

    // Reset the current tracking address, and make sure we clean up on failure.
    cur_address = address;
    auto remap_guard = detail::ScopeExit([&] {
        if (cur_address > address) {
            const VAddr last_map_address = cur_address - 1;
            cur_address = address;

            // Iterate over the memory we unmapped.
            auto it = block_manager->FindIterator(cur_address);
            auto pg_it = pg.Nodes().begin();
            PAddr pg_phys_addr = pg_it->GetAddress();
            size_t pg_pages = pg_it->GetNumPages();

            while (true) {
                // Get the memory info for the pages we unmapped, convert to property.
                const KMemoryInfo info = it->GetMemoryInfo();

                // If the memory is normal, we unmapped it and need to re-map it.
                if (info.GetState() == KMemoryState::Normal) {
                    // Determine the range to map.
                    size_t map_pages = std::min(VAddr(info.GetEndAddress()) - cur_address,
                                                last_map_address + 1 - cur_address) /
                                       PageSize;

                    // While we have pages to map, map them.
                    while (map_pages > 0) {
                        // Check if we're at the end of the physical block.
                        if (pg_pages == 0) {
                            // Ensure there are more pages to map.
                            ASSERT(pg_it != pg.Nodes().end());

                            // Advance our physical block.
                            ++pg_it;
                            pg_phys_addr = pg_it->GetAddress();
                            pg_pages = pg_it->GetNumPages();
                        }

                        // Map whatever we can.
                        const size_t cur_pages = std::min(pg_pages, map_pages);
                        ASSERT(this->Operate(cur_address, cur_pages, info.GetPermission(),
                                             OperationType::Map, pg_phys_addr) == ResultSuccess);

                        // Advance.
                        cur_address += cur_pages * PageSize;
                        map_pages -= cur_pages;

                        pg_phys_addr += cur_pages * PageSize;
                        pg_pages -= cur_pages;
                    }
                }

                // Check if we're done.
                if (last_map_address <= info.GetLastAddress()) {
                    break;
                }

                // Advance.
                ++it;
            }
        }
    });

    // Iterate over the memory, unmapping as we go.
    auto it = block_manager->FindIterator(cur_address);
    while (true) {
        // Check that the iterator is valid.
        ASSERT(it != block_manager->end());

        // Get the memory info.
        const KMemoryInfo info = it->GetMemoryInfo();

        // If the memory state is normal, we need to unmap it.
        if (info.GetState() == KMemoryState::Normal) {
            // Determine the range to unmap.
            const size_t cur_pages = std::min(VAddr(info.GetEndAddress()) - cur_address,
                                              last_address + 1 - cur_address) /
                                     PageSize;

            // Unmap.
            R_TRY(Operate(cur_address, cur_pages, KMemoryPermission::None, OperationType::Unmap));
        }

        // Check if we're done.
        if (last_address <= info.GetLastAddress()) {
            break;
        }

        // Advance.
        cur_address = info.GetEndAddress();
        ++it;
    }

    // Release the memory resource.
    mapped_physical_memory_size -= mapped_size;
    auto process{system.Kernel().CurrentProcess()};
    process->GetResourceLimit()->Release(LimitableResource::PhysicalMemory, mapped_size);

    // Update memory blocks.
    block_manager->Update(address, size / PageSize, KMemoryState::Free, KMemoryPermission::None,
                          KMemoryAttribute::None);

    // TODO(bunnei): This is a workaround until the next set of changes, where we add reference
    // counting for mapped pages. Until then, we must manually close the reference to the page
    // group.
    system.Kernel().MemoryManager().Close(pg);

    // We succeeded.
    remap_guard.Cancel();

    return ResultSuccess;
}

ResultCode KPageTable::MapMemory(VAddr dst_addr, VAddr src_addr, std::size_t size) {
    KScopedLightLock lk(general_lock);

    KMemoryState src_state{};
    CASCADE_CODE(CheckMemoryState(
        &src_state, nullptr, nullptr, nullptr, src_addr, size, KMemoryState::FlagCanAlias,
        KMemoryState::FlagCanAlias, KMemoryPermission::All, KMemoryPermission::UserReadWrite,
        KMemoryAttribute::Mask, KMemoryAttribute::None, KMemoryAttribute::IpcAndDeviceMapped));

    if (IsRegionMapped(dst_addr, size)) {
        return ResultInvalidCurrentMemory;
    }

    KPageLinkedList page_linked_list;
    const std::size_t num_pages{size / PageSize};

    AddRegionToPages(src_addr, num_pages, page_linked_list);

    {
        auto block_guard = detail::ScopeExit([&] {
            Operate(src_addr, num_pages, KMemoryPermission::UserReadWrite,
                    OperationType::ChangePermissions);
        });

        CASCADE_CODE(Operate(src_addr, num_pages, KMemoryPermission::None,
                             OperationType::ChangePermissions));
        CASCADE_CODE(MapPages(dst_addr, page_linked_list, KMemoryPermission::UserReadWrite));

        block_guard.Cancel();
    }

    block_manager->Update(src_addr, num_pages, src_state, KMemoryPermission::None,
                          KMemoryAttribute::Locked);
    block_manager->Update(dst_addr, num_pages, KMemoryState::Stack,
                          KMemoryPermission::UserReadWrite);

    return ResultSuccess;
}

ResultCode KPageTable::UnmapMemory(VAddr dst_addr, VAddr src_addr, std::size_t size) {
    KScopedLightLock lk(general_lock);

    KMemoryState src_state{};
    CASCADE_CODE(CheckMemoryState(
        &src_state, nullptr, nullptr, nullptr, src_addr, size, KMemoryState::FlagCanAlias,
        KMemoryState::FlagCanAlias, KMemoryPermission::All, KMemoryPermission::None,
        KMemoryAttribute::Mask, KMemoryAttribute::Locked, KMemoryAttribute::IpcAndDeviceMapped));

    KMemoryPermission dst_perm{};
    CASCADE_CODE(CheckMemoryState(nullptr, &dst_perm, nullptr, nullptr, dst_addr, size,
                                  KMemoryState::All, KMemoryState::Stack, KMemoryPermission::None,
                                  KMemoryPermission::None, KMemoryAttribute::Mask,
                                  KMemoryAttribute::None, KMemoryAttribute::IpcAndDeviceMapped));

    KPageLinkedList src_pages;
    KPageLinkedList dst_pages;
    const std::size_t num_pages{size / PageSize};

    AddRegionToPages(src_addr, num_pages, src_pages);
    AddRegionToPages(dst_addr, num_pages, dst_pages);

    if (!dst_pages.IsEqual(src_pages)) {
        return ResultInvalidMemoryRegion;
    }

    {
        auto block_guard = detail::ScopeExit([&] { MapPages(dst_addr, dst_pages, dst_perm); });

        CASCADE_CODE(Operate(dst_addr, num_pages, KMemoryPermission::None, OperationType::Unmap));
        CASCADE_CODE(Operate(src_addr, num_pages, KMemoryPermission::UserReadWrite,
                             OperationType::ChangePermissions));

        block_guard.Cancel();
    }

    block_manager->Update(src_addr, num_pages, src_state, KMemoryPermission::UserReadWrite);
    block_manager->Update(dst_addr, num_pages, KMemoryState::Free);

    return ResultSuccess;
}

ResultCode KPageTable::MapPages(VAddr addr, const KPageLinkedList& page_linked_list,
                                KMemoryPermission perm) {
    ASSERT(this->IsLockedByCurrentThread());

    VAddr cur_addr{addr};

    for (const auto& node : page_linked_list.Nodes()) {
        if (const auto result{
                Operate(cur_addr, node.GetNumPages(), perm, OperationType::Map, node.GetAddress())};
            result.IsError()) {
            const std::size_t num_pages{(addr - cur_addr) / PageSize};

            ASSERT(Operate(addr, num_pages, KMemoryPermission::None, OperationType::Unmap)
                       .IsSuccess());

            return result;
        }

        cur_addr += node.GetNumPages() * PageSize;
    }

    return ResultSuccess;
}

ResultCode KPageTable::MapPages(VAddr address, KPageLinkedList& page_linked_list,
                                KMemoryState state, KMemoryPermission perm) {
    // Check that the map is in range.
    const std::size_t num_pages{page_linked_list.GetNumPages()};
    const std::size_t size{num_pages * PageSize};
    R_UNLESS(this->CanContain(address, size, state), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Check the memory state.
    R_TRY(this->CheckMemoryState(address, size, KMemoryState::All, KMemoryState::Free,
                                 KMemoryPermission::None, KMemoryPermission::None,
                                 KMemoryAttribute::None, KMemoryAttribute::None));

    // Map the pages.
    R_TRY(MapPages(address, page_linked_list, perm));

    // Update the blocks.
    block_manager->Update(address, num_pages, state, perm);

    return ResultSuccess;
}

ResultCode KPageTable::MapPages(VAddr* out_addr, std::size_t num_pages, std::size_t alignment,
                                PAddr phys_addr, bool is_pa_valid, VAddr region_start,
                                std::size_t region_num_pages, KMemoryState state,
                                KMemoryPermission perm) {
    ASSERT(Common::IsAligned(alignment, PageSize) && alignment >= PageSize);

    // Ensure this is a valid map request.
    R_UNLESS(this->CanContain(region_start, region_num_pages * PageSize, state),
             ResultInvalidCurrentMemory);
    R_UNLESS(num_pages < region_num_pages, ResultOutOfMemory);

    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Find a random address to map at.
    VAddr addr = this->FindFreeArea(region_start, region_num_pages, num_pages, alignment, 0,
                                    this->GetNumGuardPages());
    R_UNLESS(addr != 0, ResultOutOfMemory);
    ASSERT(Common::IsAligned(addr, alignment));
    ASSERT(this->CanContain(addr, num_pages * PageSize, state));
    ASSERT(this->CheckMemoryState(addr, num_pages * PageSize, KMemoryState::All, KMemoryState::Free,
                                  KMemoryPermission::None, KMemoryPermission::None,
                                  KMemoryAttribute::None, KMemoryAttribute::None)
               .IsSuccess());

    // Perform mapping operation.
    if (is_pa_valid) {
        R_TRY(this->Operate(addr, num_pages, perm, OperationType::Map, phys_addr));
    } else {
        UNIMPLEMENTED();
    }

    // Update the blocks.
    block_manager->Update(addr, num_pages, state, perm);

    // We successfully mapped the pages.
    *out_addr = addr;
    return ResultSuccess;
}

ResultCode KPageTable::UnmapPages(VAddr addr, const KPageLinkedList& page_linked_list) {
    ASSERT(this->IsLockedByCurrentThread());

    VAddr cur_addr{addr};

    for (const auto& node : page_linked_list.Nodes()) {
        if (const auto result{Operate(cur_addr, node.GetNumPages(), KMemoryPermission::None,
                                      OperationType::Unmap)};
            result.IsError()) {
            return result;
        }

        cur_addr += node.GetNumPages() * PageSize;
    }

    return ResultSuccess;
}

ResultCode KPageTable::UnmapPages(VAddr addr, KPageLinkedList& page_linked_list,
                                  KMemoryState state) {
    // Check that the unmap is in range.
    const std::size_t num_pages{page_linked_list.GetNumPages()};
    const std::size_t size{num_pages * PageSize};
    R_UNLESS(this->Contains(addr, size), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Check the memory state.
    R_TRY(this->CheckMemoryState(addr, size, KMemoryState::All, state, KMemoryPermission::None,
                                 KMemoryPermission::None, KMemoryAttribute::All,
                                 KMemoryAttribute::None));

    // Perform the unmap.
    R_TRY(UnmapPages(addr, page_linked_list));

    // Update the blocks.
    block_manager->Update(addr, num_pages, state, KMemoryPermission::None);

    return ResultSuccess;
}

ResultCode KPageTable::UnmapPages(VAddr address, std::size_t num_pages, KMemoryState state) {
    // Check that the unmap is in range.
    const std::size_t size = num_pages * PageSize;
    R_UNLESS(this->Contains(address, size), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Check the memory state.
    std::size_t num_allocator_blocks{};
    R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size,
                                 KMemoryState::All, state, KMemoryPermission::None,
                                 KMemoryPermission::None, KMemoryAttribute::All,
                                 KMemoryAttribute::None));

    // Perform the unmap.
    R_TRY(Operate(address, num_pages, KMemoryPermission::None, OperationType::Unmap));

    // Update the blocks.
    block_manager->Update(address, num_pages, KMemoryState::Free, KMemoryPermission::None);

    return ResultSuccess;
}

ResultCode KPageTable::MakeAndOpenPageGroup(KPageLinkedList* out, VAddr address, size_t num_pages,
                                            KMemoryState state_mask, KMemoryState state,
                                            KMemoryPermission perm_mask, KMemoryPermission perm,
                                            KMemoryAttribute attr_mask, KMemoryAttribute attr) {
    // Ensure that the page group isn't null.
    ASSERT(out != nullptr);

    // Make sure that the region we're mapping is valid for the table.
    const size_t size = num_pages * PageSize;
    R_UNLESS(this->Contains(address, size), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Check if state allows us to create the group.
    R_TRY(this->CheckMemoryState(address, size, state_mask | KMemoryState::FlagReferenceCounted,
                                 state | KMemoryState::FlagReferenceCounted, perm_mask, perm,
                                 attr_mask, attr));

    // Create a new page group for the region.
    R_TRY(this->MakePageGroup(*out, address, num_pages));

    return ResultSuccess;
}

ResultCode KPageTable::SetProcessMemoryPermission(VAddr addr, std::size_t size,
                                                  Svc::MemoryPermission svc_perm) {
    const size_t num_pages = size / PageSize;

    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Verify we can change the memory permission.
    KMemoryState old_state;
    KMemoryPermission old_perm;
    size_t num_allocator_blocks;
    R_TRY(this->CheckMemoryState(std::addressof(old_state), std::addressof(old_perm), nullptr,
                                 std::addressof(num_allocator_blocks), addr, size,
                                 KMemoryState::FlagCode, KMemoryState::FlagCode,
                                 KMemoryPermission::None, KMemoryPermission::None,
                                 KMemoryAttribute::All, KMemoryAttribute::None));

    // Determine new perm/state.
    const KMemoryPermission new_perm = ConvertToKMemoryPermission(svc_perm);
    KMemoryState new_state = old_state;
    const bool is_w = (new_perm & KMemoryPermission::UserWrite) == KMemoryPermission::UserWrite;
    const bool is_x = (new_perm & KMemoryPermission::UserExecute) == KMemoryPermission::UserExecute;
    const bool was_x =
        (old_perm & KMemoryPermission::UserExecute) == KMemoryPermission::UserExecute;
    ASSERT(!(is_w && is_x));

    if (is_w) {
        switch (old_state) {
        case KMemoryState::Code:
            new_state = KMemoryState::CodeData;
            break;
        case KMemoryState::AliasCode:
            new_state = KMemoryState::AliasCodeData;
            break;
        default:
            ASSERT(false);
        }
    }

    // Succeed if there's nothing to do.
    R_SUCCEED_IF(old_perm == new_perm && old_state == new_state);

    // Perform mapping operation.
    const auto operation =
        was_x ? OperationType::ChangePermissionsAndRefresh : OperationType::ChangePermissions;
    R_TRY(Operate(addr, num_pages, new_perm, operation));

    // Update the blocks.
    block_manager->Update(addr, num_pages, new_state, new_perm, KMemoryAttribute::None);

    // Ensure cache coherency, if we're setting pages as executable.
    if (is_x) {
        system.InvalidateCpuInstructionCacheRange(addr, size);
    }

    return ResultSuccess;
}

KMemoryInfo KPageTable::QueryInfoImpl(VAddr addr) {
    KScopedLightLock lk(general_lock);

    return block_manager->FindBlock(addr).GetMemoryInfo();
}

KMemoryInfo KPageTable::QueryInfo(VAddr addr) {
    if (!Contains(addr, 1)) {
        return {address_space_end,       0 - address_space_end,  KMemoryState::Inaccessible,
                KMemoryPermission::None, KMemoryAttribute::None, KMemoryPermission::None};
    }

    return QueryInfoImpl(addr);
}

ResultCode KPageTable::ReserveTransferMemory(VAddr addr, std::size_t size, KMemoryPermission perm) {
    KScopedLightLock lk(general_lock);

    KMemoryState state{};
    KMemoryAttribute attribute{};

    R_TRY(CheckMemoryState(&state, nullptr, &attribute, nullptr, addr, size,
                           KMemoryState::FlagCanTransfer | KMemoryState::FlagReferenceCounted,
                           KMemoryState::FlagCanTransfer | KMemoryState::FlagReferenceCounted,
                           KMemoryPermission::All, KMemoryPermission::UserReadWrite,
                           KMemoryAttribute::Mask, KMemoryAttribute::None,
                           KMemoryAttribute::IpcAndDeviceMapped));

    block_manager->Update(addr, size / PageSize, state, perm, attribute | KMemoryAttribute::Locked);

    return ResultSuccess;
}

ResultCode KPageTable::ResetTransferMemory(VAddr addr, std::size_t size) {
    KScopedLightLock lk(general_lock);

    KMemoryState state{};

    R_TRY(CheckMemoryState(&state, nullptr, nullptr, nullptr, addr, size,
                           KMemoryState::FlagCanTransfer | KMemoryState::FlagReferenceCounted,
                           KMemoryState::FlagCanTransfer | KMemoryState::FlagReferenceCounted,
                           KMemoryPermission::None, KMemoryPermission::None, KMemoryAttribute::Mask,
                           KMemoryAttribute::Locked, KMemoryAttribute::IpcAndDeviceMapped));

    block_manager->Update(addr, size / PageSize, state, KMemoryPermission::UserReadWrite);
    return ResultSuccess;
}

ResultCode KPageTable::SetMemoryPermission(VAddr addr, std::size_t size,
                                           Svc::MemoryPermission svc_perm) {
    const size_t num_pages = size / PageSize;

    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Verify we can change the memory permission.
    KMemoryState old_state;
    KMemoryPermission old_perm;
    R_TRY(this->CheckMemoryState(
        std::addressof(old_state), std::addressof(old_perm), nullptr, nullptr, addr, size,
        KMemoryState::FlagCanReprotect, KMemoryState::FlagCanReprotect, KMemoryPermission::None,
        KMemoryPermission::None, KMemoryAttribute::All, KMemoryAttribute::None));

    // Determine new perm.
    const KMemoryPermission new_perm = ConvertToKMemoryPermission(svc_perm);
    R_SUCCEED_IF(old_perm == new_perm);

    // Perform mapping operation.
    R_TRY(Operate(addr, num_pages, new_perm, OperationType::ChangePermissions));

    // Update the blocks.
    block_manager->Update(addr, num_pages, old_state, new_perm, KMemoryAttribute::None);

    return ResultSuccess;
}

ResultCode KPageTable::SetMemoryAttribute(VAddr addr, std::size_t size, u32 mask, u32 attr) {
    const size_t num_pages = size / PageSize;
    ASSERT((static_cast<KMemoryAttribute>(mask) | KMemoryAttribute::SetMask) ==
           KMemoryAttribute::SetMask);

    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Verify we can change the memory attribute.
    KMemoryState old_state;
    KMemoryPermission old_perm;
    KMemoryAttribute old_attr;
    size_t num_allocator_blocks;
    constexpr auto AttributeTestMask =
        ~(KMemoryAttribute::SetMask | KMemoryAttribute::DeviceShared);
    R_TRY(this->CheckMemoryState(
        std::addressof(old_state), std::addressof(old_perm), std::addressof(old_attr),
        std::addressof(num_allocator_blocks), addr, size, KMemoryState::FlagCanChangeAttribute,
        KMemoryState::FlagCanChangeAttribute, KMemoryPermission::None, KMemoryPermission::None,
        AttributeTestMask, KMemoryAttribute::None, ~AttributeTestMask));

    // Determine the new attribute.
    const KMemoryAttribute new_attr =
        static_cast<KMemoryAttribute>(((old_attr & static_cast<KMemoryAttribute>(~mask)) |
                                       static_cast<KMemoryAttribute>(attr & mask)));

    // Perform operation.
    this->Operate(addr, num_pages, old_perm, OperationType::ChangePermissionsAndRefresh);

    // Update the blocks.
    block_manager->Update(addr, num_pages, old_state, old_perm, new_attr);

    return ResultSuccess;
}

ResultCode KPageTable::SetMaxHeapSize(std::size_t size) {
    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Only process page tables are allowed to set heap size.
    ASSERT(!this->IsKernel());

    max_heap_size = size;

    return ResultSuccess;
}

ResultCode KPageTable::SetHeapSize(VAddr* out, std::size_t size) {
    // Lock the physical memory mutex.
    KScopedLightLock map_phys_mem_lk(map_physical_memory_lock);

    // Try to perform a reduction in heap, instead of an extension.
    VAddr cur_address{};
    std::size_t allocation_size{};
    {
        // Lock the table.
        KScopedLightLock lk(general_lock);

        // Validate that setting heap size is possible at all.
        R_UNLESS(!is_kernel, ResultOutOfMemory);
        R_UNLESS(size <= static_cast<std::size_t>(heap_region_end - heap_region_start),
                 ResultOutOfMemory);
        R_UNLESS(size <= max_heap_size, ResultOutOfMemory);

        if (size < GetHeapSize()) {
            // The size being requested is less than the current size, so we need to free the end of
            // the heap.

            // Validate memory state.
            std::size_t num_allocator_blocks;
            R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks),
                                         heap_region_start + size, GetHeapSize() - size,
                                         KMemoryState::All, KMemoryState::Normal,
                                         KMemoryPermission::All, KMemoryPermission::UserReadWrite,
                                         KMemoryAttribute::All, KMemoryAttribute::None));

            // Unmap the end of the heap.
            const auto num_pages = (GetHeapSize() - size) / PageSize;
            R_TRY(Operate(heap_region_start + size, num_pages, KMemoryPermission::None,
                          OperationType::Unmap));

            // Release the memory from the resource limit.
            system.Kernel().CurrentProcess()->GetResourceLimit()->Release(
                LimitableResource::PhysicalMemory, num_pages * PageSize);

            // Apply the memory block update.
            block_manager->Update(heap_region_start + size, num_pages, KMemoryState::Free,
                                  KMemoryPermission::None, KMemoryAttribute::None);

            // Update the current heap end.
            current_heap_end = heap_region_start + size;

            // Set the output.
            *out = heap_region_start;
            return ResultSuccess;
        } else if (size == GetHeapSize()) {
            // The size requested is exactly the current size.
            *out = heap_region_start;
            return ResultSuccess;
        } else {
            // We have to allocate memory. Determine how much to allocate and where while the table
            // is locked.
            cur_address = current_heap_end;
            allocation_size = size - GetHeapSize();
        }
    }

    // Reserve memory for the heap extension.
    KScopedResourceReservation memory_reservation(
        system.Kernel().CurrentProcess()->GetResourceLimit(), LimitableResource::PhysicalMemory,
        allocation_size);
    R_UNLESS(memory_reservation.Succeeded(), ResultLimitReached);

    // Allocate pages for the heap extension.
    KPageLinkedList pg;
    R_TRY(system.Kernel().MemoryManager().AllocateAndOpen(
        &pg, allocation_size / PageSize,
        KMemoryManager::EncodeOption(memory_pool, allocation_option)));

    // Clear all the newly allocated pages.
    for (const auto& it : pg.Nodes()) {
        std::memset(system.DeviceMemory().GetPointer(it.GetAddress()), heap_fill_value,
                    it.GetSize());
    }

    // Map the pages.
    {
        // Lock the table.
        KScopedLightLock lk(general_lock);

        // Ensure that the heap hasn't changed since we began executing.
        ASSERT(cur_address == current_heap_end);

        // Check the memory state.
        std::size_t num_allocator_blocks{};
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), current_heap_end,
                                     allocation_size, KMemoryState::All, KMemoryState::Free,
                                     KMemoryPermission::None, KMemoryPermission::None,
                                     KMemoryAttribute::None, KMemoryAttribute::None));

        // Map the pages.
        const auto num_pages = allocation_size / PageSize;
        R_TRY(Operate(current_heap_end, num_pages, pg, OperationType::MapGroup));

        // Clear all the newly allocated pages.
        for (std::size_t cur_page = 0; cur_page < num_pages; ++cur_page) {
            std::memset(system.Memory().GetPointer(current_heap_end + (cur_page * PageSize)), 0,
                        PageSize);
        }

        // We succeeded, so commit our memory reservation.
        memory_reservation.Commit();

        // Apply the memory block update.
        block_manager->Update(current_heap_end, num_pages, KMemoryState::Normal,
                              KMemoryPermission::UserReadWrite, KMemoryAttribute::None);

        // Update the current heap end.
        current_heap_end = heap_region_start + size;

        // Set the output.
        *out = heap_region_start;
        return ResultSuccess;
    }
}

ResultVal<VAddr> KPageTable::AllocateAndMapMemory(std::size_t needed_num_pages, std::size_t align,
                                                  bool is_map_only, VAddr region_start,
                                                  std::size_t region_num_pages, KMemoryState state,
                                                  KMemoryPermission perm, PAddr map_addr) {
    KScopedLightLock lk(general_lock);

    if (!CanContain(region_start, region_num_pages * PageSize, state)) {
        return ResultInvalidCurrentMemory;
    }

    if (region_num_pages <= needed_num_pages) {
        return ResultOutOfMemory;
    }

    const VAddr addr{
        AllocateVirtualMemory(region_start, region_num_pages, needed_num_pages, align)};
    if (!addr) {
        return ResultOutOfMemory;
    }

    if (is_map_only) {
        R_TRY(Operate(addr, needed_num_pages, perm, OperationType::Map, map_addr));
    } else {
        KPageLinkedList page_group;
        R_TRY(system.Kernel().MemoryManager().AllocateAndOpenForProcess(
            &page_group, needed_num_pages,
            KMemoryManager::EncodeOption(memory_pool, allocation_option), 0, 0));
        R_TRY(Operate(addr, needed_num_pages, page_group, OperationType::MapGroup));
    }

    block_manager->Update(addr, needed_num_pages, state, perm);

    return addr;
}

ResultCode KPageTable::LockForDeviceAddressSpace(VAddr addr, std::size_t size) {
    KScopedLightLock lk(general_lock);

    KMemoryPermission perm{};
    if (const ResultCode result{CheckMemoryState(
            nullptr, &perm, nullptr, nullptr, addr, size, KMemoryState::FlagCanChangeAttribute,
            KMemoryState::FlagCanChangeAttribute, KMemoryPermission::None, KMemoryPermission::None,
            KMemoryAttribute::LockedAndIpcLocked, KMemoryAttribute::None,
            KMemoryAttribute::DeviceSharedAndUncached)};
        result.IsError()) {
        return result;
    }

    block_manager->UpdateLock(
        addr, size / PageSize,
        [](KMemoryBlockManager::iterator block, KMemoryPermission permission) {
            block->ShareToDevice(permission);
        },
        perm);

    return ResultSuccess;
}

ResultCode KPageTable::UnlockForDeviceAddressSpace(VAddr addr, std::size_t size) {
    KScopedLightLock lk(general_lock);

    KMemoryPermission perm{};
    if (const ResultCode result{CheckMemoryState(
            nullptr, &perm, nullptr, nullptr, addr, size, KMemoryState::FlagCanChangeAttribute,
            KMemoryState::FlagCanChangeAttribute, KMemoryPermission::None, KMemoryPermission::None,
            KMemoryAttribute::LockedAndIpcLocked, KMemoryAttribute::None,
            KMemoryAttribute::DeviceSharedAndUncached)};
        result.IsError()) {
        return result;
    }

    block_manager->UpdateLock(
        addr, size / PageSize,
        [](KMemoryBlockManager::iterator block, KMemoryPermission permission) {
            block->UnshareToDevice(permission);
        },
        perm);

    return ResultSuccess;
}

ResultCode KPageTable::LockForCodeMemory(KPageLinkedList* out, VAddr addr, std::size_t size) {
    return this->LockMemoryAndOpen(
        out, nullptr, addr, size, KMemoryState::FlagCanCodeMemory, KMemoryState::FlagCanCodeMemory,
        KMemoryPermission::All, KMemoryPermission::UserReadWrite, KMemoryAttribute::All,
        KMemoryAttribute::None,
        static_cast<KMemoryPermission>(KMemoryPermission::NotMapped |
                                       KMemoryPermission::KernelReadWrite),
        KMemoryAttribute::Locked);
}

ResultCode KPageTable::UnlockForCodeMemory(VAddr addr, std::size_t size,
                                           const KPageLinkedList& pg) {
    return this->UnlockMemory(
        addr, size, KMemoryState::FlagCanCodeMemory, KMemoryState::FlagCanCodeMemory,
        KMemoryPermission::None, KMemoryPermission::None, KMemoryAttribute::All,
        KMemoryAttribute::Locked, KMemoryPermission::UserReadWrite, KMemoryAttribute::Locked, &pg);
}

ResultCode KPageTable::InitializeMemoryLayout(VAddr start, VAddr end) {
    block_manager = std::make_unique<KMemoryBlockManager>(start, end);

    return ResultSuccess;
}

bool KPageTable::IsRegionMapped(VAddr address, u64 size) {
    return CheckMemoryState(address, size, KMemoryState::All, KMemoryState::Free,
                            KMemoryPermission::All, KMemoryPermission::None, KMemoryAttribute::Mask,
                            KMemoryAttribute::None, KMemoryAttribute::IpcAndDeviceMapped)
        .IsError();
}

bool KPageTable::IsRegionContiguous(VAddr addr, u64 size) const {
    auto start_ptr = system.Memory().GetPointer(addr);
    for (u64 offset{}; offset < size; offset += PageSize) {
        if (start_ptr != system.Memory().GetPointer(addr + offset)) {
            return false;
        }
        start_ptr += PageSize;
    }
    return true;
}

void KPageTable::AddRegionToPages(VAddr start, std::size_t num_pages,
                                  KPageLinkedList& page_linked_list) {
    VAddr addr{start};
    while (addr < start + (num_pages * PageSize)) {
        const PAddr paddr{GetPhysicalAddr(addr)};
        ASSERT(paddr != 0);
        page_linked_list.AddBlock(paddr, 1);
        addr += PageSize;
    }
}

VAddr KPageTable::AllocateVirtualMemory(VAddr start, std::size_t region_num_pages,
                                        u64 needed_num_pages, std::size_t align) {
    if (is_aslr_enabled) {
        UNIMPLEMENTED();
    }
    return block_manager->FindFreeArea(start, region_num_pages, needed_num_pages, align, 0,
                                       IsKernel() ? 1 : 4);
}

ResultCode KPageTable::Operate(VAddr addr, std::size_t num_pages, const KPageLinkedList& page_group,
                               OperationType operation) {
    ASSERT(this->IsLockedByCurrentThread());

    ASSERT(Common::IsAligned(addr, PageSize));
    ASSERT(num_pages > 0);
    ASSERT(num_pages == page_group.GetNumPages());

    for (const auto& node : page_group.Nodes()) {
        const std::size_t size{node.GetNumPages() * PageSize};

        switch (operation) {
        case OperationType::MapGroup:
            system.Memory().MapMemoryRegion(page_table_impl, addr, size, node.GetAddress());
            break;
        default:
            ASSERT(false);
        }

        addr += size;
    }

    return ResultSuccess;
}

ResultCode KPageTable::Operate(VAddr addr, std::size_t num_pages, KMemoryPermission perm,
                               OperationType operation, PAddr map_addr) {
    ASSERT(this->IsLockedByCurrentThread());

    ASSERT(num_pages > 0);
    ASSERT(Common::IsAligned(addr, PageSize));
    ASSERT(ContainsPages(addr, num_pages));

    switch (operation) {
    case OperationType::Unmap:
        system.Memory().UnmapRegion(page_table_impl, addr, num_pages * PageSize);
        break;
    case OperationType::Map: {
        ASSERT(map_addr);
        ASSERT(Common::IsAligned(map_addr, PageSize));
        system.Memory().MapMemoryRegion(page_table_impl, addr, num_pages * PageSize, map_addr);
        break;
    }
    case OperationType::ChangePermissions:
    case OperationType::ChangePermissionsAndRefresh:
        break;
    default:
        ASSERT(false);
    }
    return ResultSuccess;
}

VAddr KPageTable::GetRegionAddress(KMemoryState state) const {
    switch (state) {
    case KMemoryState::Free:
    case KMemoryState::Kernel:
        return address_space_start;
    case KMemoryState::Normal:
        return heap_region_start;
    case KMemoryState::Ipc:
    case KMemoryState::NonSecureIpc:
    case KMemoryState::NonDeviceIpc:
        return alias_region_start;
    case KMemoryState::Stack:
        return stack_region_start;
    case KMemoryState::Static:
    case KMemoryState::ThreadLocal:
        return kernel_map_region_start;
    case KMemoryState::Io:
    case KMemoryState::Shared:
    case KMemoryState::AliasCode:
    case KMemoryState::AliasCodeData:
    case KMemoryState::Transfered:
    case KMemoryState::SharedTransfered:
    case KMemoryState::SharedCode:
    case KMemoryState::GeneratedCode:
    case KMemoryState::CodeOut:
    case KMemoryState::Coverage:
        return alias_code_region_start;
    case KMemoryState::Code:
    case KMemoryState::CodeData:
        return code_region_start;
    default:
        UNREACHABLE();
    }
}

std::size_t KPageTable::GetRegionSize(KMemoryState state) const {
    switch (state) {
    case KMemoryState::Free:
    case KMemoryState::Kernel:
        return address_space_end - address_space_start;
    case KMemoryState::Normal:
        return heap_region_end - heap_region_start;
    case KMemoryState::Ipc:
    case KMemoryState::NonSecureIpc:
    case KMemoryState::NonDeviceIpc:
        return alias_region_end - alias_region_start;
    case KMemoryState::Stack:
        return stack_region_end - stack_region_start;
    case KMemoryState::Static:
    case KMemoryState::ThreadLocal:
        return kernel_map_region_end - kernel_map_region_start;
    case KMemoryState::Io:
    case KMemoryState::Shared:
    case KMemoryState::AliasCode:
    case KMemoryState::AliasCodeData:
    case KMemoryState::Transfered:
    case KMemoryState::SharedTransfered:
    case KMemoryState::SharedCode:
    case KMemoryState::GeneratedCode:
    case KMemoryState::CodeOut:
    case KMemoryState::Coverage:
        return alias_code_region_end - alias_code_region_start;
    case KMemoryState::Code:
    case KMemoryState::CodeData:
        return code_region_end - code_region_start;
    default:
        UNREACHABLE();
    }
}

bool KPageTable::CanContain(VAddr addr, std::size_t size, KMemoryState state) const {
    const VAddr end = addr + size;
    const VAddr last = end - 1;

    const VAddr region_start = this->GetRegionAddress(state);
    const size_t region_size = this->GetRegionSize(state);

    const bool is_in_region =
        region_start <= addr && addr < end && last <= region_start + region_size - 1;
    const bool is_in_heap = !(end <= heap_region_start || heap_region_end <= addr ||
                              heap_region_start == heap_region_end);
    const bool is_in_alias = !(end <= alias_region_start || alias_region_end <= addr ||
                               alias_region_start == alias_region_end);
    switch (state) {
    case KMemoryState::Free:
    case KMemoryState::Kernel:
        return is_in_region;
    case KMemoryState::Io:
    case KMemoryState::Static:
    case KMemoryState::Code:
    case KMemoryState::CodeData:
    case KMemoryState::Shared:
    case KMemoryState::AliasCode:
    case KMemoryState::AliasCodeData:
    case KMemoryState::Stack:
    case KMemoryState::ThreadLocal:
    case KMemoryState::Transfered:
    case KMemoryState::SharedTransfered:
    case KMemoryState::SharedCode:
    case KMemoryState::GeneratedCode:
    case KMemoryState::CodeOut:
    case KMemoryState::Coverage:
        return is_in_region && !is_in_heap && !is_in_alias;
    case KMemoryState::Normal:
        ASSERT(is_in_heap);
        return is_in_region && !is_in_alias;
    case KMemoryState::Ipc:
    case KMemoryState::NonSecureIpc:
    case KMemoryState::NonDeviceIpc:
        ASSERT(is_in_alias);
        return is_in_region && !is_in_heap;
    default:
        return false;
    }
}

ResultCode KPageTable::CheckMemoryState(const KMemoryInfo& info, KMemoryState state_mask,
                                        KMemoryState state, KMemoryPermission perm_mask,
                                        KMemoryPermission perm, KMemoryAttribute attr_mask,
                                        KMemoryAttribute attr) const {
    // Validate the states match expectation.
    R_UNLESS((info.state & state_mask) == state, ResultInvalidCurrentMemory);
    R_UNLESS((info.perm & perm_mask) == perm, ResultInvalidCurrentMemory);
    R_UNLESS((info.attribute & attr_mask) == attr, ResultInvalidCurrentMemory);

    return ResultSuccess;
}

ResultCode KPageTable::CheckMemoryStateContiguous(std::size_t* out_blocks_needed, VAddr addr,
                                                  std::size_t size, KMemoryState state_mask,
                                                  KMemoryState state, KMemoryPermission perm_mask,
                                                  KMemoryPermission perm,
                                                  KMemoryAttribute attr_mask,
                                                  KMemoryAttribute attr) const {
    ASSERT(this->IsLockedByCurrentThread());

    // Get information about the first block.
    const VAddr last_addr = addr + size - 1;
    KMemoryBlockManager::const_iterator it = block_manager->FindIterator(addr);
    KMemoryInfo info = it->GetMemoryInfo();

    // If the start address isn't aligned, we need a block.
    const size_t blocks_for_start_align =
        (Common::AlignDown(addr, PageSize) != info.GetAddress()) ? 1 : 0;

    while (true) {
        // Validate against the provided masks.
        R_TRY(this->CheckMemoryState(info, state_mask, state, perm_mask, perm, attr_mask, attr));

        // Break once we're done.
        if (last_addr <= info.GetLastAddress()) {
            break;
        }

        // Advance our iterator.
        it++;
        ASSERT(it != block_manager->cend());
        info = it->GetMemoryInfo();
    }

    // If the end address isn't aligned, we need a block.
    const size_t blocks_for_end_align =
        (Common::AlignUp(addr + size, PageSize) != info.GetEndAddress()) ? 1 : 0;

    if (out_blocks_needed != nullptr) {
        *out_blocks_needed = blocks_for_start_align + blocks_for_end_align;
    }

    return ResultSuccess;
}

ResultCode KPageTable::CheckMemoryState(KMemoryState* out_state, KMemoryPermission* out_perm,
                                        KMemoryAttribute* out_attr, std::size_t* out_blocks_needed,
                                        VAddr addr, std::size_t size, KMemoryState state_mask,
                                        KMemoryState state, KMemoryPermission perm_mask,
                                        KMemoryPermission perm, KMemoryAttribute attr_mask,
                                        KMemoryAttribute attr, KMemoryAttribute ignore_attr) const {
    ASSERT(this->IsLockedByCurrentThread());

    // Get information about the first block.
    const VAddr last_addr = addr + size - 1;
    KMemoryBlockManager::const_iterator it = block_manager->FindIterator(addr);
    KMemoryInfo info = it->GetMemoryInfo();

    // If the start address isn't aligned, we need a block.
    const size_t blocks_for_start_align =
        (Common::AlignDown(addr, PageSize) != info.GetAddress()) ? 1 : 0;

    // Validate all blocks in the range have correct state.
    const KMemoryState first_state = info.state;
    const KMemoryPermission first_perm = info.perm;
    const KMemoryAttribute first_attr = info.attribute;
    while (true) {
        // Validate the current block.
        R_UNLESS(info.state == first_state, ResultInvalidCurrentMemory);
        R_UNLESS(info.perm == first_perm, ResultInvalidCurrentMemory);
        R_UNLESS((info.attribute | ignore_attr) == (first_attr | ignore_attr),
                 ResultInvalidCurrentMemory);

        // Validate against the provided masks.
        R_TRY(this->CheckMemoryState(info, state_mask, state, perm_mask, perm, attr_mask, attr));

        // Break once we're done.
        if (last_addr <= info.GetLastAddress()) {
            break;
        }

        // Advance our iterator.
        it++;
        ASSERT(it != block_manager->cend());
        info = it->GetMemoryInfo();
    }

    // If the end address isn't aligned, we need a block.
    const size_t blocks_for_end_align =
        (Common::AlignUp(addr + size, PageSize) != info.GetEndAddress()) ? 1 : 0;

    // Write output state.
    if (out_state != nullptr) {
        *out_state = first_state;
    }
    if (out_perm != nullptr) {
        *out_perm = first_perm;
    }
    if (out_attr != nullptr) {
        *out_attr = static_cast<KMemoryAttribute>(first_attr & ~ignore_attr);
    }
    if (out_blocks_needed != nullptr) {
        *out_blocks_needed = blocks_for_start_align + blocks_for_end_align;
    }
    return ResultSuccess;
}

ResultCode KPageTable::LockMemoryAndOpen(KPageLinkedList* out_pg, PAddr* out_paddr, VAddr addr,
                                         size_t size, KMemoryState state_mask, KMemoryState state,
                                         KMemoryPermission perm_mask, KMemoryPermission perm,
                                         KMemoryAttribute attr_mask, KMemoryAttribute attr,
                                         KMemoryPermission new_perm, KMemoryAttribute lock_attr) {
    // Validate basic preconditions.
    ASSERT((lock_attr & attr) == KMemoryAttribute::None);
    ASSERT((lock_attr & (KMemoryAttribute::IpcLocked | KMemoryAttribute::DeviceShared)) ==
           KMemoryAttribute::None);

    // Validate the lock request.
    const size_t num_pages = size / PageSize;
    R_UNLESS(this->Contains(addr, size), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Check that the output page group is empty, if it exists.
    if (out_pg) {
        ASSERT(out_pg->GetNumPages() == 0);
    }

    // Check the state.
    KMemoryState old_state{};
    KMemoryPermission old_perm{};
    KMemoryAttribute old_attr{};
    size_t num_allocator_blocks{};
    R_TRY(this->CheckMemoryState(std::addressof(old_state), std::addressof(old_perm),
                                 std::addressof(old_attr), std::addressof(num_allocator_blocks),
                                 addr, size, state_mask | KMemoryState::FlagReferenceCounted,
                                 state | KMemoryState::FlagReferenceCounted, perm_mask, perm,
                                 attr_mask, attr));

    // Get the physical address, if we're supposed to.
    if (out_paddr != nullptr) {
        ASSERT(this->GetPhysicalAddressLocked(out_paddr, addr));
    }

    // Make the page group, if we're supposed to.
    if (out_pg != nullptr) {
        R_TRY(this->MakePageGroup(*out_pg, addr, num_pages));
    }

    // Decide on new perm and attr.
    new_perm = (new_perm != KMemoryPermission::None) ? new_perm : old_perm;
    KMemoryAttribute new_attr = static_cast<KMemoryAttribute>(old_attr | lock_attr);

    // Update permission, if we need to.
    if (new_perm != old_perm) {
        R_TRY(Operate(addr, num_pages, new_perm, OperationType::ChangePermissions));
    }

    // Apply the memory block updates.
    block_manager->Update(addr, num_pages, old_state, new_perm, new_attr);

    return ResultSuccess;
}

ResultCode KPageTable::UnlockMemory(VAddr addr, size_t size, KMemoryState state_mask,
                                    KMemoryState state, KMemoryPermission perm_mask,
                                    KMemoryPermission perm, KMemoryAttribute attr_mask,
                                    KMemoryAttribute attr, KMemoryPermission new_perm,
                                    KMemoryAttribute lock_attr, const KPageLinkedList* pg) {
    // Validate basic preconditions.
    ASSERT((attr_mask & lock_attr) == lock_attr);
    ASSERT((attr & lock_attr) == lock_attr);

    // Validate the unlock request.
    const size_t num_pages = size / PageSize;
    R_UNLESS(this->Contains(addr, size), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(general_lock);

    // Check the state.
    KMemoryState old_state{};
    KMemoryPermission old_perm{};
    KMemoryAttribute old_attr{};
    size_t num_allocator_blocks{};
    R_TRY(this->CheckMemoryState(std::addressof(old_state), std::addressof(old_perm),
                                 std::addressof(old_attr), std::addressof(num_allocator_blocks),
                                 addr, size, state_mask | KMemoryState::FlagReferenceCounted,
                                 state | KMemoryState::FlagReferenceCounted, perm_mask, perm,
                                 attr_mask, attr));

    // Check the page group.
    if (pg != nullptr) {
        R_UNLESS(this->IsValidPageGroup(*pg, addr, num_pages), ResultInvalidMemoryRegion);
    }

    // Decide on new perm and attr.
    new_perm = (new_perm != KMemoryPermission::None) ? new_perm : old_perm;
    KMemoryAttribute new_attr = static_cast<KMemoryAttribute>(old_attr & ~lock_attr);

    // Update permission, if we need to.
    if (new_perm != old_perm) {
        R_TRY(Operate(addr, num_pages, new_perm, OperationType::ChangePermissions));
    }

    // Apply the memory block updates.
    block_manager->Update(addr, num_pages, old_state, new_perm, new_attr);

    return ResultSuccess;
}

} // namespace Kernel

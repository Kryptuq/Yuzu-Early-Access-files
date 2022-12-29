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
#include "core/hle/kernel/k_page_group.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/k_scoped_resource_reservation.h"
#include "core/hle/kernel/k_system_control.h"
#include "core/hle/kernel/k_system_resource.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_results.h"
#include "core/memory.h"

namespace Kernel {

namespace {

class KScopedLightLockPair {
    YUZU_NON_COPYABLE(KScopedLightLockPair);
    YUZU_NON_MOVEABLE(KScopedLightLockPair);

private:
    KLightLock* m_lower;
    KLightLock* m_upper;

public:
    KScopedLightLockPair(KLightLock& lhs, KLightLock& rhs) {
        // Ensure our locks are in a consistent order.
        if (std::addressof(lhs) <= std::addressof(rhs)) {
            m_lower = std::addressof(lhs);
            m_upper = std::addressof(rhs);
        } else {
            m_lower = std::addressof(rhs);
            m_upper = std::addressof(lhs);
        }

        // Acquire both locks.
        m_lower->Lock();
        if (m_lower != m_upper) {
            m_upper->Lock();
        }
    }

    ~KScopedLightLockPair() {
        // Unlock the upper lock.
        if (m_upper != nullptr && m_upper != m_lower) {
            m_upper->Unlock();
        }

        // Unlock the lower lock.
        if (m_lower != nullptr) {
            m_lower->Unlock();
        }
    }

public:
    // Utility.
    void TryUnlockHalf(KLightLock& lock) {
        // Only allow unlocking if the lock is half the pair.
        if (m_lower != m_upper) {
            // We want to be sure the lock is one we own.
            if (m_lower == std::addressof(lock)) {
                lock.Unlock();
                m_lower = nullptr;
            } else if (m_upper == std::addressof(lock)) {
                lock.Unlock();
                m_upper = nullptr;
            }
        }
    }
};

using namespace Common::Literals;

constexpr size_t GetAddressSpaceWidthFromType(FileSys::ProgramAddressSpaceType as_type) {
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
    : m_general_lock{system_.Kernel()},
      m_map_physical_memory_lock{system_.Kernel()}, m_system{system_}, m_kernel{system_.Kernel()} {}

KPageTable::~KPageTable() = default;

Result KPageTable::InitializeForProcess(FileSys::ProgramAddressSpaceType as_type, bool enable_aslr,
                                        bool enable_das_merge, bool from_back,
                                        KMemoryManager::Pool pool, VAddr code_addr,
                                        size_t code_size, KSystemResource* system_resource,
                                        KResourceLimit* resource_limit) {

    const auto GetSpaceStart = [this](KAddressSpaceInfo::Type type) {
        return KAddressSpaceInfo::GetAddressSpaceStart(m_address_space_width, type);
    };
    const auto GetSpaceSize = [this](KAddressSpaceInfo::Type type) {
        return KAddressSpaceInfo::GetAddressSpaceSize(m_address_space_width, type);
    };

    //  Set our width and heap/alias sizes
    m_address_space_width = GetAddressSpaceWidthFromType(as_type);
    const VAddr start = 0;
    const VAddr end{1ULL << m_address_space_width};
    size_t alias_region_size{GetSpaceSize(KAddressSpaceInfo::Type::Alias)};
    size_t heap_region_size{GetSpaceSize(KAddressSpaceInfo::Type::Heap)};

    ASSERT(code_addr < code_addr + code_size);
    ASSERT(code_addr + code_size - 1 <= end - 1);

    // Adjust heap/alias size if we don't have an alias region
    if (as_type == FileSys::ProgramAddressSpaceType::Is32BitNoMap) {
        heap_region_size += alias_region_size;
        alias_region_size = 0;
    }

    // Set code regions and determine remaining
    constexpr size_t RegionAlignment{2_MiB};
    VAddr process_code_start{};
    VAddr process_code_end{};
    size_t stack_region_size{};
    size_t kernel_map_region_size{};

    if (m_address_space_width == 39) {
        alias_region_size = GetSpaceSize(KAddressSpaceInfo::Type::Alias);
        heap_region_size = GetSpaceSize(KAddressSpaceInfo::Type::Heap);
        stack_region_size = GetSpaceSize(KAddressSpaceInfo::Type::Stack);
        kernel_map_region_size = GetSpaceSize(KAddressSpaceInfo::Type::MapSmall);
        m_code_region_start = GetSpaceStart(KAddressSpaceInfo::Type::Map39Bit);
        m_code_region_end = m_code_region_start + GetSpaceSize(KAddressSpaceInfo::Type::Map39Bit);
        m_alias_code_region_start = m_code_region_start;
        m_alias_code_region_end = m_code_region_end;
        process_code_start = Common::AlignDown(code_addr, RegionAlignment);
        process_code_end = Common::AlignUp(code_addr + code_size, RegionAlignment);
    } else {
        stack_region_size = 0;
        kernel_map_region_size = 0;
        m_code_region_start = GetSpaceStart(KAddressSpaceInfo::Type::MapSmall);
        m_code_region_end = m_code_region_start + GetSpaceSize(KAddressSpaceInfo::Type::MapSmall);
        m_stack_region_start = m_code_region_start;
        m_alias_code_region_start = m_code_region_start;
        m_alias_code_region_end = GetSpaceStart(KAddressSpaceInfo::Type::MapLarge) +
                                  GetSpaceSize(KAddressSpaceInfo::Type::MapLarge);
        m_stack_region_end = m_code_region_end;
        m_kernel_map_region_start = m_code_region_start;
        m_kernel_map_region_end = m_code_region_end;
        process_code_start = m_code_region_start;
        process_code_end = m_code_region_end;
    }

    // Set other basic fields
    m_enable_aslr = enable_aslr;
    m_enable_device_address_space_merge = enable_das_merge;
    m_address_space_start = start;
    m_address_space_end = end;
    m_is_kernel = false;
    m_memory_block_slab_manager = system_resource->GetMemoryBlockSlabManagerPointer();
    m_block_info_manager = system_resource->GetBlockInfoManagerPointer();
    m_resource_limit = resource_limit;

    // Determine the region we can place our undetermineds in
    VAddr alloc_start{};
    size_t alloc_size{};
    if ((process_code_start - m_code_region_start) >= (end - process_code_end)) {
        alloc_start = m_code_region_start;
        alloc_size = process_code_start - m_code_region_start;
    } else {
        alloc_start = process_code_end;
        alloc_size = end - process_code_end;
    }
    const size_t needed_size =
        (alias_region_size + heap_region_size + stack_region_size + kernel_map_region_size);
    R_UNLESS(alloc_size >= needed_size, ResultOutOfMemory);

    const size_t remaining_size{alloc_size - needed_size};

    // Determine random placements for each region
    size_t alias_rnd{}, heap_rnd{}, stack_rnd{}, kmap_rnd{};
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
    m_alias_region_start = alloc_start + alias_rnd;
    m_alias_region_end = m_alias_region_start + alias_region_size;
    m_heap_region_start = alloc_start + heap_rnd;
    m_heap_region_end = m_heap_region_start + heap_region_size;

    if (alias_rnd <= heap_rnd) {
        m_heap_region_start += alias_region_size;
        m_heap_region_end += alias_region_size;
    } else {
        m_alias_region_start += heap_region_size;
        m_alias_region_end += heap_region_size;
    }

    // Setup stack region
    if (stack_region_size) {
        m_stack_region_start = alloc_start + stack_rnd;
        m_stack_region_end = m_stack_region_start + stack_region_size;

        if (alias_rnd < stack_rnd) {
            m_stack_region_start += alias_region_size;
            m_stack_region_end += alias_region_size;
        } else {
            m_alias_region_start += stack_region_size;
            m_alias_region_end += stack_region_size;
        }

        if (heap_rnd < stack_rnd) {
            m_stack_region_start += heap_region_size;
            m_stack_region_end += heap_region_size;
        } else {
            m_heap_region_start += stack_region_size;
            m_heap_region_end += stack_region_size;
        }
    }

    // Setup kernel map region
    if (kernel_map_region_size) {
        m_kernel_map_region_start = alloc_start + kmap_rnd;
        m_kernel_map_region_end = m_kernel_map_region_start + kernel_map_region_size;

        if (alias_rnd < kmap_rnd) {
            m_kernel_map_region_start += alias_region_size;
            m_kernel_map_region_end += alias_region_size;
        } else {
            m_alias_region_start += kernel_map_region_size;
            m_alias_region_end += kernel_map_region_size;
        }

        if (heap_rnd < kmap_rnd) {
            m_kernel_map_region_start += heap_region_size;
            m_kernel_map_region_end += heap_region_size;
        } else {
            m_heap_region_start += kernel_map_region_size;
            m_heap_region_end += kernel_map_region_size;
        }

        if (stack_region_size) {
            if (stack_rnd < kmap_rnd) {
                m_kernel_map_region_start += stack_region_size;
                m_kernel_map_region_end += stack_region_size;
            } else {
                m_stack_region_start += kernel_map_region_size;
                m_stack_region_end += kernel_map_region_size;
            }
        }
    }

    // Set heap and fill members.
    m_current_heap_end = m_heap_region_start;
    m_max_heap_size = 0;
    m_mapped_physical_memory_size = 0;
    m_mapped_unsafe_physical_memory = 0;
    m_mapped_insecure_memory = 0;
    m_mapped_ipc_server_memory = 0;

    m_heap_fill_value = 0;
    m_ipc_fill_value = 0;
    m_stack_fill_value = 0;

    // Set allocation option.
    m_allocate_option =
        KMemoryManager::EncodeOption(pool, from_back ? KMemoryManager::Direction::FromBack
                                                     : KMemoryManager::Direction::FromFront);

    // Ensure that we regions inside our address space
    auto IsInAddressSpace = [&](VAddr addr) {
        return m_address_space_start <= addr && addr <= m_address_space_end;
    };
    ASSERT(IsInAddressSpace(m_alias_region_start));
    ASSERT(IsInAddressSpace(m_alias_region_end));
    ASSERT(IsInAddressSpace(m_heap_region_start));
    ASSERT(IsInAddressSpace(m_heap_region_end));
    ASSERT(IsInAddressSpace(m_stack_region_start));
    ASSERT(IsInAddressSpace(m_stack_region_end));
    ASSERT(IsInAddressSpace(m_kernel_map_region_start));
    ASSERT(IsInAddressSpace(m_kernel_map_region_end));

    // Ensure that we selected regions that don't overlap
    const VAddr alias_start{m_alias_region_start};
    const VAddr alias_last{m_alias_region_end - 1};
    const VAddr heap_start{m_heap_region_start};
    const VAddr heap_last{m_heap_region_end - 1};
    const VAddr stack_start{m_stack_region_start};
    const VAddr stack_last{m_stack_region_end - 1};
    const VAddr kmap_start{m_kernel_map_region_start};
    const VAddr kmap_last{m_kernel_map_region_end - 1};
    ASSERT(alias_last < heap_start || heap_last < alias_start);
    ASSERT(alias_last < stack_start || stack_last < alias_start);
    ASSERT(alias_last < kmap_start || kmap_last < alias_start);
    ASSERT(heap_last < stack_start || stack_last < heap_start);
    ASSERT(heap_last < kmap_start || kmap_last < heap_start);

    m_current_heap_end = m_heap_region_start;
    m_max_heap_size = 0;
    m_mapped_physical_memory_size = 0;
    m_memory_pool = pool;

    m_page_table_impl = std::make_unique<Common::PageTable>();
    m_page_table_impl->Resize(m_address_space_width, PageBits);

    // Initialize our memory block manager.
    R_RETURN(m_memory_block_manager.Initialize(m_address_space_start, m_address_space_end,
                                               m_memory_block_slab_manager));
}

void KPageTable::Finalize() {
    // Finalize memory blocks.
    m_memory_block_manager.Finalize(m_memory_block_slab_manager, [&](VAddr addr, u64 size) {
        m_system.Memory().UnmapRegion(*m_page_table_impl, addr, size);
    });

    // Release any insecure mapped memory.
    if (m_mapped_insecure_memory) {
        UNIMPLEMENTED();
    }

    // Release any ipc server memory.
    if (m_mapped_ipc_server_memory) {
        UNIMPLEMENTED();
    }

    // Close the backing page table, as the destructor is not called for guest objects.
    m_page_table_impl.reset();
}

Result KPageTable::MapProcessCode(VAddr addr, size_t num_pages, KMemoryState state,
                                  KMemoryPermission perm) {
    const u64 size{num_pages * PageSize};

    // Validate the mapping request.
    R_UNLESS(this->CanContain(addr, size, state), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Verify that the destination memory is unmapped.
    R_TRY(this->CheckMemoryState(addr, size, KMemoryState::All, KMemoryState::Free,
                                 KMemoryPermission::None, KMemoryPermission::None,
                                 KMemoryAttribute::None, KMemoryAttribute::None));

    // Create an update allocator.
    Result allocator_result{ResultSuccess};
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager);

    // Allocate and open.
    KPageGroup pg{m_kernel, m_block_info_manager};
    R_TRY(m_system.Kernel().MemoryManager().AllocateAndOpen(
        &pg, num_pages,
        KMemoryManager::EncodeOption(KMemoryManager::Pool::Application, m_allocation_option)));

    R_TRY(Operate(addr, num_pages, pg, OperationType::MapGroup));

    // Update the blocks.
    m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, state, perm,
                                  KMemoryAttribute::None, KMemoryBlockDisableMergeAttribute::Normal,
                                  KMemoryBlockDisableMergeAttribute::None);

    R_SUCCEED();
}

Result KPageTable::MapCodeMemory(VAddr dst_address, VAddr src_address, size_t size) {
    // Validate the mapping request.
    R_UNLESS(this->CanContain(dst_address, size, KMemoryState::AliasCode),
             ResultInvalidMemoryRegion);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Verify that the source memory is normal heap.
    KMemoryState src_state{};
    KMemoryPermission src_perm{};
    size_t num_src_allocator_blocks{};
    R_TRY(this->CheckMemoryState(&src_state, &src_perm, nullptr, &num_src_allocator_blocks,
                                 src_address, size, KMemoryState::All, KMemoryState::Normal,
                                 KMemoryPermission::All, KMemoryPermission::UserReadWrite,
                                 KMemoryAttribute::All, KMemoryAttribute::None));

    // Verify that the destination memory is unmapped.
    size_t num_dst_allocator_blocks{};
    R_TRY(this->CheckMemoryState(&num_dst_allocator_blocks, dst_address, size, KMemoryState::All,
                                 KMemoryState::Free, KMemoryPermission::None,
                                 KMemoryPermission::None, KMemoryAttribute::None,
                                 KMemoryAttribute::None));

    // Create an update allocator for the source.
    Result src_allocator_result{ResultSuccess};
    KMemoryBlockManagerUpdateAllocator src_allocator(std::addressof(src_allocator_result),
                                                     m_memory_block_slab_manager,
                                                     num_src_allocator_blocks);
    R_TRY(src_allocator_result);

    // Create an update allocator for the destination.
    Result dst_allocator_result{ResultSuccess};
    KMemoryBlockManagerUpdateAllocator dst_allocator(std::addressof(dst_allocator_result),
                                                     m_memory_block_slab_manager,
                                                     num_dst_allocator_blocks);
    R_TRY(dst_allocator_result);

    // Map the code memory.
    {
        // Determine the number of pages being operated on.
        const size_t num_pages = size / PageSize;

        // Create page groups for the memory being mapped.
        KPageGroup pg{m_kernel, m_block_info_manager};
        AddRegionToPages(src_address, num_pages, pg);

        // We're going to perform an update, so create a helper.
        KScopedPageTableUpdater updater(this);

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
        const KPageProperties dst_properties = {new_perm, false, false,
                                                DisableMergeAttribute::DisableHead};
        R_TRY(
            this->MapPageGroupImpl(updater.GetPageList(), dst_address, pg, dst_properties, false));

        // We successfully mapped the alias pages, so we don't need to unprotect the src pages on
        // failure.
        unprot_guard.Cancel();

        // Apply the memory block updates.
        m_memory_block_manager.Update(std::addressof(src_allocator), src_address, num_pages,
                                      src_state, new_perm, KMemoryAttribute::Locked,
                                      KMemoryBlockDisableMergeAttribute::Locked,
                                      KMemoryBlockDisableMergeAttribute::None);
        m_memory_block_manager.Update(std::addressof(dst_allocator), dst_address, num_pages,
                                      KMemoryState::AliasCode, new_perm, KMemoryAttribute::None,
                                      KMemoryBlockDisableMergeAttribute::Normal,
                                      KMemoryBlockDisableMergeAttribute::None);
    }

    R_SUCCEED();
}

Result KPageTable::UnmapCodeMemory(VAddr dst_address, VAddr src_address, size_t size,
                                   ICacheInvalidationStrategy icache_invalidation_strategy) {
    // Validate the mapping request.
    R_UNLESS(this->CanContain(dst_address, size, KMemoryState::AliasCode),
             ResultInvalidMemoryRegion);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Verify that the source memory is locked normal heap.
    size_t num_src_allocator_blocks{};
    R_TRY(this->CheckMemoryState(std::addressof(num_src_allocator_blocks), src_address, size,
                                 KMemoryState::All, KMemoryState::Normal, KMemoryPermission::None,
                                 KMemoryPermission::None, KMemoryAttribute::All,
                                 KMemoryAttribute::Locked));

    // Verify that the destination memory is aliasable code.
    size_t num_dst_allocator_blocks{};
    R_TRY(this->CheckMemoryStateContiguous(
        std::addressof(num_dst_allocator_blocks), dst_address, size, KMemoryState::FlagCanCodeAlias,
        KMemoryState::FlagCanCodeAlias, KMemoryPermission::None, KMemoryPermission::None,
        KMemoryAttribute::All, KMemoryAttribute::None));

    // Determine whether any pages being unmapped are code.
    bool any_code_pages = false;
    {
        KMemoryBlockManager::const_iterator it = m_memory_block_manager.FindIterator(dst_address);
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
                m_system.InvalidateCpuInstructionCacheRange(dst_address, size);
            } else {
                m_system.InvalidateCpuInstructionCaches();
            }
        }
    });

    // Unmap.
    {
        // Determine the number of pages being operated on.
        const size_t num_pages = size / PageSize;

        // Create an update allocator for the source.
        Result src_allocator_result{ResultSuccess};
        KMemoryBlockManagerUpdateAllocator src_allocator(std::addressof(src_allocator_result),
                                                         m_memory_block_slab_manager,
                                                         num_src_allocator_blocks);
        R_TRY(src_allocator_result);

        // Create an update allocator for the destination.
        Result dst_allocator_result{ResultSuccess};
        KMemoryBlockManagerUpdateAllocator dst_allocator(std::addressof(dst_allocator_result),
                                                         m_memory_block_slab_manager,
                                                         num_dst_allocator_blocks);
        R_TRY(dst_allocator_result);

        // Unmap the aliased copy of the pages.
        R_TRY(Operate(dst_address, num_pages, KMemoryPermission::None, OperationType::Unmap));

        // Try to set the permissions for the source pages back to what they should be.
        R_TRY(Operate(src_address, num_pages, KMemoryPermission::UserReadWrite,
                      OperationType::ChangePermissions));

        // Apply the memory block updates.
        m_memory_block_manager.Update(
            std::addressof(dst_allocator), dst_address, num_pages, KMemoryState::None,
            KMemoryPermission::None, KMemoryAttribute::None,
            KMemoryBlockDisableMergeAttribute::None, KMemoryBlockDisableMergeAttribute::Normal);
        m_memory_block_manager.Update(
            std::addressof(src_allocator), src_address, num_pages, KMemoryState::Normal,
            KMemoryPermission::UserReadWrite, KMemoryAttribute::None,
            KMemoryBlockDisableMergeAttribute::None, KMemoryBlockDisableMergeAttribute::Locked);

        // Note that we reprotected pages.
        reprotected_pages = true;
    }

    R_SUCCEED();
}

VAddr KPageTable::FindFreeArea(VAddr region_start, size_t region_num_pages, size_t num_pages,
                               size_t alignment, size_t offset, size_t guard_pages) {
    VAddr address = 0;

    if (num_pages <= region_num_pages) {
        if (this->IsAslrEnabled()) {
            UNIMPLEMENTED();
        }
        // Find the first free area.
        if (address == 0) {
            address = m_memory_block_manager.FindFreeArea(region_start, region_num_pages, num_pages,
                                                          alignment, offset, guard_pages);
        }
    }

    return address;
}

Result KPageTable::MakePageGroup(KPageGroup& pg, VAddr addr, size_t num_pages) {
    ASSERT(this->IsLockedByCurrentThread());

    const size_t size = num_pages * PageSize;

    // We're making a new group, not adding to an existing one.
    R_UNLESS(pg.empty(), ResultInvalidCurrentMemory);

    // Begin traversal.
    Common::PageTable::TraversalContext context;
    Common::PageTable::TraversalEntry next_entry;
    R_UNLESS(m_page_table_impl->BeginTraversal(next_entry, context, addr),
             ResultInvalidCurrentMemory);

    // Prepare tracking variables.
    PAddr cur_addr = next_entry.phys_addr;
    size_t cur_size = next_entry.block_size - (cur_addr & (next_entry.block_size - 1));
    size_t tot_size = cur_size;

    // Iterate, adding to group as we go.
    const auto& memory_layout = m_system.Kernel().MemoryLayout();
    while (tot_size < size) {
        R_UNLESS(m_page_table_impl->ContinueTraversal(next_entry, context),
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

    R_SUCCEED();
}

bool KPageTable::IsValidPageGroup(const KPageGroup& pg, VAddr addr, size_t num_pages) {
    ASSERT(this->IsLockedByCurrentThread());

    const size_t size = num_pages * PageSize;
    const auto& memory_layout = m_system.Kernel().MemoryLayout();

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
    if (!m_page_table_impl->BeginTraversal(next_entry, context, addr)) {
        return false;
    }

    // Prepare tracking variables.
    PAddr cur_addr = next_entry.phys_addr;
    size_t cur_size = next_entry.block_size - (cur_addr & (next_entry.block_size - 1));
    size_t tot_size = cur_size;

    // Iterate, comparing expected to actual.
    while (tot_size < size) {
        if (!m_page_table_impl->ContinueTraversal(next_entry, context)) {
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

Result KPageTable::UnmapProcessMemory(VAddr dst_addr, size_t size, KPageTable& src_page_table,
                                      VAddr src_addr) {
    // Acquire the table locks.
    KScopedLightLockPair lk(src_page_table.m_general_lock, m_general_lock);

    const size_t num_pages{size / PageSize};

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

    // Create an update allocator.
    Result allocator_result{ResultSuccess};
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    CASCADE_CODE(Operate(dst_addr, num_pages, KMemoryPermission::None, OperationType::Unmap));

    // Apply the memory block update.
    m_memory_block_manager.Update(std::addressof(allocator), dst_addr, num_pages,
                                  KMemoryState::Free, KMemoryPermission::None,
                                  KMemoryAttribute::None, KMemoryBlockDisableMergeAttribute::None,
                                  KMemoryBlockDisableMergeAttribute::Normal);

    m_system.InvalidateCpuInstructionCaches();

    R_SUCCEED();
}

Result KPageTable::SetupForIpcClient(PageLinkedList* page_list, size_t* out_blocks_needed,
                                     VAddr address, size_t size, KMemoryPermission test_perm,
                                     KMemoryState dst_state) {
    // Validate pre-conditions.
    ASSERT(this->IsLockedByCurrentThread());
    ASSERT(test_perm == KMemoryPermission::UserReadWrite ||
           test_perm == KMemoryPermission::UserRead);

    // Check that the address is in range.
    R_UNLESS(this->Contains(address, size), ResultInvalidCurrentMemory);

    // Get the source permission.
    const auto src_perm = (test_perm == KMemoryPermission::UserReadWrite)
                              ? KMemoryPermission::KernelReadWrite | KMemoryPermission::NotMapped
                              : KMemoryPermission::UserRead;

    // Get aligned extents.
    const VAddr aligned_src_start = Common::AlignDown((address), PageSize);
    const VAddr aligned_src_end = Common::AlignUp((address) + size, PageSize);
    const VAddr mapping_src_start = Common::AlignUp((address), PageSize);
    const VAddr mapping_src_end = Common::AlignDown((address) + size, PageSize);

    const auto aligned_src_last = (aligned_src_end)-1;
    const auto mapping_src_last = (mapping_src_end)-1;

    // Get the test state and attribute mask.
    KMemoryState test_state;
    KMemoryAttribute test_attr_mask;
    switch (dst_state) {
    case KMemoryState::Ipc:
        test_state = KMemoryState::FlagCanUseIpc;
        test_attr_mask =
            KMemoryAttribute::Uncached | KMemoryAttribute::DeviceShared | KMemoryAttribute::Locked;
        break;
    case KMemoryState::NonSecureIpc:
        test_state = KMemoryState::FlagCanUseNonSecureIpc;
        test_attr_mask = KMemoryAttribute::Uncached | KMemoryAttribute::Locked;
        break;
    case KMemoryState::NonDeviceIpc:
        test_state = KMemoryState::FlagCanUseNonDeviceIpc;
        test_attr_mask = KMemoryAttribute::Uncached | KMemoryAttribute::Locked;
        break;
    default:
        R_THROW(ResultInvalidCombination);
    }

    // Ensure that on failure, we roll back appropriately.
    size_t mapped_size = 0;
    ON_RESULT_FAILURE {
        if (mapped_size > 0) {
            this->CleanupForIpcClientOnServerSetupFailure(page_list, mapping_src_start, mapped_size,
                                                          src_perm);
        }
    };

    size_t blocks_needed = 0;

    // Iterate, mapping as needed.
    KMemoryBlockManager::const_iterator it = m_memory_block_manager.FindIterator(aligned_src_start);
    while (true) {
        const KMemoryInfo info = it->GetMemoryInfo();

        // Validate the current block.
        R_TRY(this->CheckMemoryState(info, test_state, test_state, test_perm, test_perm,
                                     test_attr_mask, KMemoryAttribute::None));

        if (mapping_src_start < mapping_src_end && (mapping_src_start) < info.GetEndAddress() &&
            info.GetAddress() < (mapping_src_end)) {
            const auto cur_start =
                info.GetAddress() >= (mapping_src_start) ? info.GetAddress() : (mapping_src_start);
            const auto cur_end = mapping_src_last >= info.GetLastAddress() ? info.GetEndAddress()
                                                                           : (mapping_src_end);
            const size_t cur_size = cur_end - cur_start;

            if (info.GetAddress() < (mapping_src_start)) {
                ++blocks_needed;
            }
            if (mapping_src_last < info.GetLastAddress()) {
                ++blocks_needed;
            }

            // Set the permissions on the block, if we need to.
            if ((info.GetPermission() & KMemoryPermission::IpcLockChangeMask) != src_perm) {
                R_TRY(Operate(cur_start, cur_size / PageSize, src_perm,
                              OperationType::ChangePermissions));
            }

            // Note that we mapped this part.
            mapped_size += cur_size;
        }

        // If the block is at the end, we're done.
        if (aligned_src_last <= info.GetLastAddress()) {
            break;
        }

        // Advance.
        ++it;
        ASSERT(it != m_memory_block_manager.end());
    }

    if (out_blocks_needed != nullptr) {
        ASSERT(blocks_needed <= KMemoryBlockManagerUpdateAllocator::MaxBlocks);
        *out_blocks_needed = blocks_needed;
    }

    R_SUCCEED();
}

Result KPageTable::SetupForIpcServer(VAddr* out_addr, size_t size, VAddr src_addr,
                                     KMemoryPermission test_perm, KMemoryState dst_state,
                                     KPageTable& src_page_table, bool send) {
    ASSERT(this->IsLockedByCurrentThread());
    ASSERT(src_page_table.IsLockedByCurrentThread());

    // Check that we can theoretically map.
    const VAddr region_start = m_alias_region_start;
    const size_t region_size = m_alias_region_end - m_alias_region_start;
    R_UNLESS(size < region_size, ResultOutOfAddressSpace);

    // Get aligned source extents.
    const VAddr src_start = src_addr;
    const VAddr src_end = src_addr + size;
    const VAddr aligned_src_start = Common::AlignDown((src_start), PageSize);
    const VAddr aligned_src_end = Common::AlignUp((src_start) + size, PageSize);
    const VAddr mapping_src_start = Common::AlignUp((src_start), PageSize);
    const VAddr mapping_src_end = Common::AlignDown((src_start) + size, PageSize);
    const size_t aligned_src_size = aligned_src_end - aligned_src_start;
    const size_t mapping_src_size =
        (mapping_src_start < mapping_src_end) ? (mapping_src_end - mapping_src_start) : 0;

    // Select a random address to map at.
    VAddr dst_addr =
        this->FindFreeArea(region_start, region_size / PageSize, aligned_src_size / PageSize,
                           PageSize, 0, this->GetNumGuardPages());

    R_UNLESS(dst_addr != 0, ResultOutOfAddressSpace);

    // Check that we can perform the operation we're about to perform.
    ASSERT(this->CanContain(dst_addr, aligned_src_size, dst_state));

    // Create an update allocator.
    Result allocator_result;
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager);
    R_TRY(allocator_result);

    // We're going to perform an update, so create a helper.
    KScopedPageTableUpdater updater(this);

    // Reserve space for any partial pages we allocate.
    const size_t unmapped_size = aligned_src_size - mapping_src_size;
    KScopedResourceReservation memory_reservation(
        m_resource_limit, LimitableResource::PhysicalMemoryMax, unmapped_size);
    R_UNLESS(memory_reservation.Succeeded(), ResultLimitReached);

    // Ensure that we manage page references correctly.
    PAddr start_partial_page = 0;
    PAddr end_partial_page = 0;
    VAddr cur_mapped_addr = dst_addr;

    // If the partial pages are mapped, an extra reference will have been opened. Otherwise, they'll
    // free on scope exit.
    SCOPE_EXIT({
        if (start_partial_page != 0) {
            m_system.Kernel().MemoryManager().Close(start_partial_page, 1);
        }
        if (end_partial_page != 0) {
            m_system.Kernel().MemoryManager().Close(end_partial_page, 1);
        }
    });

    ON_RESULT_FAILURE {
        if (cur_mapped_addr != dst_addr) {
            ASSERT(Operate(dst_addr, (cur_mapped_addr - dst_addr) / PageSize,
                           KMemoryPermission::None, OperationType::Unmap)
                       .IsSuccess());
        }
    };

    // Allocate the start page as needed.
    if (aligned_src_start < mapping_src_start) {
        start_partial_page =
            m_system.Kernel().MemoryManager().AllocateAndOpenContinuous(1, 1, m_allocate_option);
        R_UNLESS(start_partial_page != 0, ResultOutOfMemory);
    }

    // Allocate the end page as needed.
    if (mapping_src_end < aligned_src_end &&
        (aligned_src_start < mapping_src_end || aligned_src_start == mapping_src_start)) {
        end_partial_page =
            m_system.Kernel().MemoryManager().AllocateAndOpenContinuous(1, 1, m_allocate_option);
        R_UNLESS(end_partial_page != 0, ResultOutOfMemory);
    }

    // Get the implementation.
    auto& src_impl = src_page_table.PageTableImpl();

    // Get the fill value for partial pages.
    const auto fill_val = m_ipc_fill_value;

    // Begin traversal.
    Common::PageTable::TraversalContext context;
    Common::PageTable::TraversalEntry next_entry;
    bool traverse_valid = src_impl.BeginTraversal(next_entry, context, aligned_src_start);
    ASSERT(traverse_valid);

    // Prepare tracking variables.
    PAddr cur_block_addr = next_entry.phys_addr;
    size_t cur_block_size =
        next_entry.block_size - ((cur_block_addr) & (next_entry.block_size - 1));
    size_t tot_block_size = cur_block_size;

    // Map the start page, if we have one.
    if (start_partial_page != 0) {
        // Ensure the page holds correct data.
        const VAddr start_partial_virt =
            GetHeapVirtualAddress(m_system.Kernel().MemoryLayout(), start_partial_page);
        if (send) {
            const size_t partial_offset = src_start - aligned_src_start;
            size_t copy_size, clear_size;
            if (src_end < mapping_src_start) {
                copy_size = size;
                clear_size = mapping_src_start - src_end;
            } else {
                copy_size = mapping_src_start - src_start;
                clear_size = 0;
            }

            std::memset(m_system.Memory().GetPointer<void>(start_partial_virt), fill_val,
                        partial_offset);
            std::memcpy(
                m_system.Memory().GetPointer<void>(start_partial_virt + partial_offset),
                m_system.Memory().GetPointer<void>(
                    GetHeapVirtualAddress(m_system.Kernel().MemoryLayout(), cur_block_addr) +
                    partial_offset),
                copy_size);
            if (clear_size > 0) {
                std::memset(m_system.Memory().GetPointer<void>(start_partial_virt + partial_offset +
                                                               copy_size),
                            fill_val, clear_size);
            }
        } else {
            std::memset(m_system.Memory().GetPointer<void>(start_partial_virt), fill_val, PageSize);
        }

        // Map the page.
        R_TRY(Operate(cur_mapped_addr, 1, test_perm, OperationType::Map, start_partial_page));

        // Update tracking extents.
        cur_mapped_addr += PageSize;
        cur_block_addr += PageSize;
        cur_block_size -= PageSize;

        // If the block's size was one page, we may need to continue traversal.
        if (cur_block_size == 0 && aligned_src_size > PageSize) {
            traverse_valid = src_impl.ContinueTraversal(next_entry, context);
            ASSERT(traverse_valid);

            cur_block_addr = next_entry.phys_addr;
            cur_block_size = next_entry.block_size;
            tot_block_size += next_entry.block_size;
        }
    }

    // Map the remaining pages.
    while (aligned_src_start + tot_block_size < mapping_src_end) {
        // Continue the traversal.
        traverse_valid = src_impl.ContinueTraversal(next_entry, context);
        ASSERT(traverse_valid);

        // Process the block.
        if (next_entry.phys_addr != cur_block_addr + cur_block_size) {
            // Map the block we've been processing so far.
            R_TRY(Operate(cur_mapped_addr, cur_block_size / PageSize, test_perm, OperationType::Map,
                          cur_block_addr));

            // Update tracking extents.
            cur_mapped_addr += cur_block_size;
            cur_block_addr = next_entry.phys_addr;
            cur_block_size = next_entry.block_size;
        } else {
            cur_block_size += next_entry.block_size;
        }
        tot_block_size += next_entry.block_size;
    }

    // Handle the last direct-mapped page.
    if (const VAddr mapped_block_end = aligned_src_start + tot_block_size - cur_block_size;
        mapped_block_end < mapping_src_end) {
        const size_t last_block_size = mapping_src_end - mapped_block_end;

        // Map the last block.
        R_TRY(Operate(cur_mapped_addr, last_block_size / PageSize, test_perm, OperationType::Map,
                      cur_block_addr));

        // Update tracking extents.
        cur_mapped_addr += last_block_size;
        cur_block_addr += last_block_size;
        if (mapped_block_end + cur_block_size < aligned_src_end &&
            cur_block_size == last_block_size) {
            traverse_valid = src_impl.ContinueTraversal(next_entry, context);
            ASSERT(traverse_valid);

            cur_block_addr = next_entry.phys_addr;
        }
    }

    // Map the end page, if we have one.
    if (end_partial_page != 0) {
        // Ensure the page holds correct data.
        const VAddr end_partial_virt =
            GetHeapVirtualAddress(m_system.Kernel().MemoryLayout(), end_partial_page);
        if (send) {
            const size_t copy_size = src_end - mapping_src_end;
            std::memcpy(m_system.Memory().GetPointer<void>(end_partial_virt),
                        m_system.Memory().GetPointer<void>(GetHeapVirtualAddress(
                            m_system.Kernel().MemoryLayout(), cur_block_addr)),
                        copy_size);
            std::memset(m_system.Memory().GetPointer<void>(end_partial_virt + copy_size), fill_val,
                        PageSize - copy_size);
        } else {
            std::memset(m_system.Memory().GetPointer<void>(end_partial_virt), fill_val, PageSize);
        }

        // Map the page.
        R_TRY(Operate(cur_mapped_addr, 1, test_perm, OperationType::Map, end_partial_page));
    }

    // Update memory blocks to reflect our changes
    m_memory_block_manager.Update(std::addressof(allocator), dst_addr, aligned_src_size / PageSize,
                                  dst_state, test_perm, KMemoryAttribute::None,
                                  KMemoryBlockDisableMergeAttribute::Normal,
                                  KMemoryBlockDisableMergeAttribute::None);

    // Set the output address.
    *out_addr = dst_addr + (src_start - aligned_src_start);

    // We succeeded.
    memory_reservation.Commit();
    R_SUCCEED();
}

Result KPageTable::SetupForIpc(VAddr* out_dst_addr, size_t size, VAddr src_addr,
                               KPageTable& src_page_table, KMemoryPermission test_perm,
                               KMemoryState dst_state, bool send) {
    // For convenience, alias this.
    KPageTable& dst_page_table = *this;

    // Acquire the table locks.
    KScopedLightLockPair lk(src_page_table.m_general_lock, dst_page_table.m_general_lock);

    // We're going to perform an update, so create a helper.
    KScopedPageTableUpdater updater(std::addressof(src_page_table));

    // Perform client setup.
    size_t num_allocator_blocks;
    R_TRY(src_page_table.SetupForIpcClient(updater.GetPageList(),
                                           std::addressof(num_allocator_blocks), src_addr, size,
                                           test_perm, dst_state));

    // Create an update allocator.
    Result allocator_result;
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 src_page_table.m_memory_block_slab_manager,
                                                 num_allocator_blocks);
    R_TRY(allocator_result);

    // Get the mapped extents.
    const VAddr src_map_start = Common::AlignUp((src_addr), PageSize);
    const VAddr src_map_end = Common::AlignDown((src_addr) + size, PageSize);
    const size_t src_map_size = src_map_end - src_map_start;

    // Ensure that we clean up appropriately if we fail after this.
    const auto src_perm = (test_perm == KMemoryPermission::UserReadWrite)
                              ? KMemoryPermission::KernelReadWrite | KMemoryPermission::NotMapped
                              : KMemoryPermission::UserRead;
    ON_RESULT_FAILURE {
        if (src_map_end > src_map_start) {
            src_page_table.CleanupForIpcClientOnServerSetupFailure(
                updater.GetPageList(), src_map_start, src_map_size, src_perm);
        }
    };

    // Perform server setup.
    R_TRY(dst_page_table.SetupForIpcServer(out_dst_addr, size, src_addr, test_perm, dst_state,
                                           src_page_table, send));

    // If anything was mapped, ipc-lock the pages.
    if (src_map_start < src_map_end) {
        // Get the source permission.
        src_page_table.m_memory_block_manager.UpdateLock(std::addressof(allocator), src_map_start,
                                                         (src_map_end - src_map_start) / PageSize,
                                                         &KMemoryBlock::LockForIpc, src_perm);
    }

    R_SUCCEED();
}

Result KPageTable::CleanupForIpcServer(VAddr address, size_t size, KMemoryState dst_state) {
    // Validate the address.
    R_UNLESS(this->Contains(address, size), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Validate the memory state.
    size_t num_allocator_blocks;
    R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size,
                                 KMemoryState::All, dst_state, KMemoryPermission::UserRead,
                                 KMemoryPermission::UserRead, KMemoryAttribute::All,
                                 KMemoryAttribute::None));

    // Create an update allocator.
    Result allocator_result;
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    // We're going to perform an update, so create a helper.
    KScopedPageTableUpdater updater(this);

    // Get aligned extents.
    const VAddr aligned_start = Common::AlignDown((address), PageSize);
    const VAddr aligned_end = Common::AlignUp((address) + size, PageSize);
    const size_t aligned_size = aligned_end - aligned_start;
    const size_t aligned_num_pages = aligned_size / PageSize;

    // Unmap the pages.
    R_TRY(Operate(aligned_start, aligned_num_pages, KMemoryPermission::None, OperationType::Unmap));

    // Update memory blocks.
    m_memory_block_manager.Update(std::addressof(allocator), aligned_start, aligned_num_pages,
                                  KMemoryState::None, KMemoryPermission::None,
                                  KMemoryAttribute::None, KMemoryBlockDisableMergeAttribute::None,
                                  KMemoryBlockDisableMergeAttribute::Normal);

    // Release from the resource limit as relevant.
    const VAddr mapping_start = Common::AlignUp((address), PageSize);
    const VAddr mapping_end = Common::AlignDown((address) + size, PageSize);
    const size_t mapping_size = (mapping_start < mapping_end) ? mapping_end - mapping_start : 0;
    m_resource_limit->Release(LimitableResource::PhysicalMemoryMax, aligned_size - mapping_size);

    R_SUCCEED();
}

Result KPageTable::CleanupForIpcClient(VAddr address, size_t size, KMemoryState dst_state) {
    // Validate the address.
    R_UNLESS(this->Contains(address, size), ResultInvalidCurrentMemory);

    // Get aligned source extents.
    const VAddr mapping_start = Common::AlignUp((address), PageSize);
    const VAddr mapping_end = Common::AlignDown((address) + size, PageSize);
    const VAddr mapping_last = mapping_end - 1;
    const size_t mapping_size = (mapping_start < mapping_end) ? (mapping_end - mapping_start) : 0;

    // If nothing was mapped, we're actually done immediately.
    R_SUCCEED_IF(mapping_size == 0);

    // Get the test state and attribute mask.
    KMemoryState test_state;
    KMemoryAttribute test_attr_mask;
    switch (dst_state) {
    case KMemoryState::Ipc:
        test_state = KMemoryState::FlagCanUseIpc;
        test_attr_mask =
            KMemoryAttribute::Uncached | KMemoryAttribute::DeviceShared | KMemoryAttribute::Locked;
        break;
    case KMemoryState::NonSecureIpc:
        test_state = KMemoryState::FlagCanUseNonSecureIpc;
        test_attr_mask = KMemoryAttribute::Uncached | KMemoryAttribute::Locked;
        break;
    case KMemoryState::NonDeviceIpc:
        test_state = KMemoryState::FlagCanUseNonDeviceIpc;
        test_attr_mask = KMemoryAttribute::Uncached | KMemoryAttribute::Locked;
        break;
    default:
        R_THROW(ResultInvalidCombination);
    }

    // Lock the table.
    // NOTE: Nintendo does this *after* creating the updater below, but this does not follow
    // convention elsewhere in KPageTable.
    KScopedLightLock lk(m_general_lock);

    // We're going to perform an update, so create a helper.
    KScopedPageTableUpdater updater(this);

    // Ensure that on failure, we roll back appropriately.
    size_t mapped_size = 0;
    ON_RESULT_FAILURE {
        if (mapped_size > 0) {
            // Determine where the mapping ends.
            const auto mapped_end = (mapping_start) + mapped_size;
            const auto mapped_last = mapped_end - 1;

            // Get current and next iterators.
            KMemoryBlockManager::const_iterator start_it =
                m_memory_block_manager.FindIterator(mapping_start);
            KMemoryBlockManager::const_iterator next_it = start_it;
            ++next_it;

            // Get the current block info.
            KMemoryInfo cur_info = start_it->GetMemoryInfo();

            // Create tracking variables.
            VAddr cur_address = cur_info.GetAddress();
            size_t cur_size = cur_info.GetSize();
            bool cur_perm_eq = cur_info.GetPermission() == cur_info.GetOriginalPermission();
            bool cur_needs_set_perm = !cur_perm_eq && cur_info.GetIpcLockCount() == 1;
            bool first =
                cur_info.GetIpcDisableMergeCount() == 1 &&
                (cur_info.GetDisableMergeAttribute() & KMemoryBlockDisableMergeAttribute::Locked) ==
                    KMemoryBlockDisableMergeAttribute::None;

            while (((cur_address) + cur_size - 1) < mapped_last) {
                // Check that we have a next block.
                ASSERT(next_it != m_memory_block_manager.end());

                // Get the next info.
                const KMemoryInfo next_info = next_it->GetMemoryInfo();

                // Check if we can consolidate the next block's permission set with the current one.

                const bool next_perm_eq =
                    next_info.GetPermission() == next_info.GetOriginalPermission();
                const bool next_needs_set_perm = !next_perm_eq && next_info.GetIpcLockCount() == 1;
                if (cur_perm_eq == next_perm_eq && cur_needs_set_perm == next_needs_set_perm &&
                    cur_info.GetOriginalPermission() == next_info.GetOriginalPermission()) {
                    // We can consolidate the reprotection for the current and next block into a
                    // single call.
                    cur_size += next_info.GetSize();
                } else {
                    // We have to operate on the current block.
                    if ((cur_needs_set_perm || first) && !cur_perm_eq) {
                        ASSERT(Operate(cur_address, cur_size / PageSize, cur_info.GetPermission(),
                                       OperationType::ChangePermissions)
                                   .IsSuccess());
                    }

                    // Advance.
                    cur_address = next_info.GetAddress();
                    cur_size = next_info.GetSize();
                    first = false;
                }

                // Advance.
                cur_info = next_info;
                cur_perm_eq = next_perm_eq;
                cur_needs_set_perm = next_needs_set_perm;
                ++next_it;
            }

            // Process the last block.
            if ((first || cur_needs_set_perm) && !cur_perm_eq) {
                ASSERT(Operate(cur_address, cur_size / PageSize, cur_info.GetPermission(),
                               OperationType::ChangePermissions)
                           .IsSuccess());
            }
        }
    };

    // Iterate, reprotecting as needed.
    {
        // Get current and next iterators.
        KMemoryBlockManager::const_iterator start_it =
            m_memory_block_manager.FindIterator(mapping_start);
        KMemoryBlockManager::const_iterator next_it = start_it;
        ++next_it;

        // Validate the current block.
        KMemoryInfo cur_info = start_it->GetMemoryInfo();
        ASSERT(this->CheckMemoryState(cur_info, test_state, test_state, KMemoryPermission::None,
                                      KMemoryPermission::None,
                                      test_attr_mask | KMemoryAttribute::IpcLocked,
                                      KMemoryAttribute::IpcLocked)
                   .IsSuccess());

        // Create tracking variables.
        VAddr cur_address = cur_info.GetAddress();
        size_t cur_size = cur_info.GetSize();
        bool cur_perm_eq = cur_info.GetPermission() == cur_info.GetOriginalPermission();
        bool cur_needs_set_perm = !cur_perm_eq && cur_info.GetIpcLockCount() == 1;
        bool first =
            cur_info.GetIpcDisableMergeCount() == 1 &&
            (cur_info.GetDisableMergeAttribute() & KMemoryBlockDisableMergeAttribute::Locked) ==
                KMemoryBlockDisableMergeAttribute::None;

        while ((cur_address + cur_size - 1) < mapping_last) {
            // Check that we have a next block.
            ASSERT(next_it != m_memory_block_manager.end());

            // Get the next info.
            const KMemoryInfo next_info = next_it->GetMemoryInfo();

            // Validate the next block.
            ASSERT(this->CheckMemoryState(next_info, test_state, test_state,
                                          KMemoryPermission::None, KMemoryPermission::None,
                                          test_attr_mask | KMemoryAttribute::IpcLocked,
                                          KMemoryAttribute::IpcLocked)
                       .IsSuccess());

            // Check if we can consolidate the next block's permission set with the current one.
            const bool next_perm_eq =
                next_info.GetPermission() == next_info.GetOriginalPermission();
            const bool next_needs_set_perm = !next_perm_eq && next_info.GetIpcLockCount() == 1;
            if (cur_perm_eq == next_perm_eq && cur_needs_set_perm == next_needs_set_perm &&
                cur_info.GetOriginalPermission() == next_info.GetOriginalPermission()) {
                // We can consolidate the reprotection for the current and next block into a single
                // call.
                cur_size += next_info.GetSize();
            } else {
                // We have to operate on the current block.
                if ((cur_needs_set_perm || first) && !cur_perm_eq) {
                    R_TRY(Operate(cur_address, cur_size / PageSize,
                                  cur_needs_set_perm ? cur_info.GetOriginalPermission()
                                                     : cur_info.GetPermission(),
                                  OperationType::ChangePermissions));
                }

                // Mark that we mapped the block.
                mapped_size += cur_size;

                // Advance.
                cur_address = next_info.GetAddress();
                cur_size = next_info.GetSize();
                first = false;
            }

            // Advance.
            cur_info = next_info;
            cur_perm_eq = next_perm_eq;
            cur_needs_set_perm = next_needs_set_perm;
            ++next_it;
        }

        // Process the last block.
        const auto lock_count =
            cur_info.GetIpcLockCount() +
            (next_it != m_memory_block_manager.end()
                 ? (next_it->GetIpcDisableMergeCount() - next_it->GetIpcLockCount())
                 : 0);
        if ((first || cur_needs_set_perm || (lock_count == 1)) && !cur_perm_eq) {
            R_TRY(Operate(cur_address, cur_size / PageSize,
                          cur_needs_set_perm ? cur_info.GetOriginalPermission()
                                             : cur_info.GetPermission(),
                          OperationType::ChangePermissions));
        }
    }

    // Create an update allocator.
    // NOTE: Guaranteed zero blocks needed here.
    Result allocator_result;
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, 0);
    R_TRY(allocator_result);

    // Unlock the pages.
    m_memory_block_manager.UpdateLock(std::addressof(allocator), mapping_start,
                                      mapping_size / PageSize, &KMemoryBlock::UnlockForIpc,
                                      KMemoryPermission::None);

    R_SUCCEED();
}

void KPageTable::CleanupForIpcClientOnServerSetupFailure([[maybe_unused]] PageLinkedList* page_list,
                                                         VAddr address, size_t size,
                                                         KMemoryPermission prot_perm) {
    ASSERT(this->IsLockedByCurrentThread());
    ASSERT(Common::IsAligned(address, PageSize));
    ASSERT(Common::IsAligned(size, PageSize));

    // Get the mapped extents.
    const VAddr src_map_start = address;
    const VAddr src_map_end = address + size;
    const VAddr src_map_last = src_map_end - 1;

    // This function is only invoked when there's something to do.
    ASSERT(src_map_end > src_map_start);

    // Iterate over blocks, fixing permissions.
    KMemoryBlockManager::const_iterator it = m_memory_block_manager.FindIterator(address);
    while (true) {
        const KMemoryInfo info = it->GetMemoryInfo();

        const auto cur_start =
            info.GetAddress() >= src_map_start ? info.GetAddress() : src_map_start;
        const auto cur_end =
            src_map_last <= info.GetLastAddress() ? src_map_end : info.GetEndAddress();

        // If we can, fix the protections on the block.
        if ((info.GetIpcLockCount() == 0 &&
             (info.GetPermission() & KMemoryPermission::IpcLockChangeMask) != prot_perm) ||
            (info.GetIpcLockCount() != 0 &&
             (info.GetOriginalPermission() & KMemoryPermission::IpcLockChangeMask) != prot_perm)) {
            // Check if we actually need to fix the protections on the block.
            if (cur_end == src_map_end || info.GetAddress() <= src_map_start ||
                (info.GetPermission() & KMemoryPermission::IpcLockChangeMask) != prot_perm) {
                ASSERT(Operate(cur_start, (cur_end - cur_start) / PageSize, info.GetPermission(),
                               OperationType::ChangePermissions)
                           .IsSuccess());
            }
        }

        // If we're past the end of the region, we're done.
        if (src_map_last <= info.GetLastAddress()) {
            break;
        }

        // Advance.
        ++it;
        ASSERT(it != m_memory_block_manager.end());
    }
}

Result KPageTable::MapPhysicalMemory(VAddr address, size_t size) {
    // Lock the physical memory lock.
    KScopedLightLock phys_lk(m_map_physical_memory_lock);

    // Calculate the last address for convenience.
    const VAddr last_address = address + size - 1;

    // Define iteration variables.
    VAddr cur_address;
    size_t mapped_size;

    // The entire mapping process can be retried.
    while (true) {
        // Check if the memory is already mapped.
        {
            // Lock the table.
            KScopedLightLock lk(m_general_lock);

            // Iterate over the memory.
            cur_address = address;
            mapped_size = 0;

            auto it = m_memory_block_manager.FindIterator(cur_address);
            while (true) {
                // Check that the iterator is valid.
                ASSERT(it != m_memory_block_manager.end());

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
                m_resource_limit, LimitableResource::PhysicalMemoryMax, size - mapped_size);
            R_UNLESS(memory_reservation.Succeeded(), ResultLimitReached);

            // Allocate pages for the new memory.
            KPageGroup pg{m_kernel, m_block_info_manager};
            R_TRY(m_system.Kernel().MemoryManager().AllocateForProcess(
                &pg, (size - mapped_size) / PageSize, m_allocate_option, 0, 0));

            // If we fail in the next bit (or retry), we need to cleanup the pages.
            // auto pg_guard = SCOPE_GUARD {
            //    pg.OpenFirst();
            //    pg.Close();
            //};

            // Map the memory.
            {
                // Lock the table.
                KScopedLightLock lk(m_general_lock);

                size_t num_allocator_blocks = 0;

                // Verify that nobody has mapped memory since we first checked.
                {
                    // Iterate over the memory.
                    size_t checked_mapped_size = 0;
                    cur_address = address;

                    auto it = m_memory_block_manager.FindIterator(cur_address);
                    while (true) {
                        // Check that the iterator is valid.
                        ASSERT(it != m_memory_block_manager.end());

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

                // Create an update allocator.
                ASSERT(num_allocator_blocks <= KMemoryBlockManagerUpdateAllocator::MaxBlocks);
                Result allocator_result;
                KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                             m_memory_block_slab_manager,
                                                             num_allocator_blocks);
                R_TRY(allocator_result);

                // We're going to perform an update, so create a helper.
                KScopedPageTableUpdater updater(this);

                // Prepare to iterate over the memory.
                auto pg_it = pg.begin();
                PAddr pg_phys_addr = pg_it->GetAddress();
                size_t pg_pages = pg_it->GetNumPages();

                // Reset the current tracking address, and make sure we clean up on failure.
                // pg_guard.Cancel();
                cur_address = address;
                ON_RESULT_FAILURE {
                    if (cur_address > address) {
                        const VAddr last_unmap_address = cur_address - 1;

                        // Iterate, unmapping the pages.
                        cur_address = address;

                        auto it = m_memory_block_manager.FindIterator(cur_address);
                        while (true) {
                            // Check that the iterator is valid.
                            ASSERT(it != m_memory_block_manager.end());

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

                    // Release any remaining unmapped memory.
                    m_system.Kernel().MemoryManager().OpenFirst(pg_phys_addr, pg_pages);
                    m_system.Kernel().MemoryManager().Close(pg_phys_addr, pg_pages);
                    for (++pg_it; pg_it != pg.end(); ++pg_it) {
                        m_system.Kernel().MemoryManager().OpenFirst(pg_it->GetAddress(),
                                                                    pg_it->GetNumPages());
                        m_system.Kernel().MemoryManager().Close(pg_it->GetAddress(),
                                                                pg_it->GetNumPages());
                    }
                };

                auto it = m_memory_block_manager.FindIterator(cur_address);
                while (true) {
                    // Check that the iterator is valid.
                    ASSERT(it != m_memory_block_manager.end());

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
                                ASSERT(pg_it != pg.end());

                                // Advance our physical block.
                                ++pg_it;
                                pg_phys_addr = pg_it->GetAddress();
                                pg_pages = pg_it->GetNumPages();
                            }

                            // Map whatever we can.
                            const size_t cur_pages = std::min(pg_pages, map_pages);
                            R_TRY(Operate(cur_address, cur_pages, KMemoryPermission::UserReadWrite,
                                          OperationType::MapFirst, pg_phys_addr));

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
                m_mapped_physical_memory_size += (size - mapped_size);

                // Update the relevant memory blocks.
                m_memory_block_manager.UpdateIfMatch(
                    std::addressof(allocator), address, size / PageSize, KMemoryState::Free,
                    KMemoryPermission::None, KMemoryAttribute::None, KMemoryState::Normal,
                    KMemoryPermission::UserReadWrite, KMemoryAttribute::None);

                R_SUCCEED();
            }
        }
    }
}

Result KPageTable::UnmapPhysicalMemory(VAddr address, size_t size) {
    // Lock the physical memory lock.
    KScopedLightLock phys_lk(m_map_physical_memory_lock);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Calculate the last address for convenience.
    const VAddr last_address = address + size - 1;

    // Define iteration variables.
    VAddr map_start_address = 0;
    VAddr map_last_address = 0;

    VAddr cur_address;
    size_t mapped_size;
    size_t num_allocator_blocks = 0;

    // Check if the memory is mapped.
    {
        // Iterate over the memory.
        cur_address = address;
        mapped_size = 0;

        auto it = m_memory_block_manager.FindIterator(cur_address);
        while (true) {
            // Check that the iterator is valid.
            ASSERT(it != m_memory_block_manager.end());

            // Get the memory info.
            const KMemoryInfo info = it->GetMemoryInfo();

            // Verify the memory's state.
            const bool is_normal = info.GetState() == KMemoryState::Normal &&
                                   info.GetAttribute() == KMemoryAttribute::None;
            const bool is_free = info.GetState() == KMemoryState::Free;
            R_UNLESS(is_normal || is_free, ResultInvalidCurrentMemory);

            if (is_normal) {
                R_UNLESS(info.GetAttribute() == KMemoryAttribute::None, ResultInvalidCurrentMemory);

                if (map_start_address == 0) {
                    map_start_address = cur_address;
                }
                map_last_address =
                    (last_address >= info.GetLastAddress()) ? info.GetLastAddress() : last_address;

                if (info.GetAddress() < address) {
                    ++num_allocator_blocks;
                }
                if (last_address < info.GetLastAddress()) {
                    ++num_allocator_blocks;
                }

                mapped_size += (map_last_address + 1 - cur_address);
            }

            // Check if we're done.
            if (last_address <= info.GetLastAddress()) {
                break;
            }

            // Advance.
            cur_address = info.GetEndAddress();
            ++it;
        }

        // If there's nothing mapped, we've nothing to do.
        R_SUCCEED_IF(mapped_size == 0);
    }

    // Create an update allocator.
    ASSERT(num_allocator_blocks <= KMemoryBlockManagerUpdateAllocator::MaxBlocks);
    Result allocator_result;
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    // We're going to perform an update, so create a helper.
    KScopedPageTableUpdater updater(this);

    // Separate the mapping.
    R_TRY(Operate(map_start_address, (map_last_address + 1 - map_start_address) / PageSize,
                  KMemoryPermission::None, OperationType::Separate));

    // Reset the current tracking address, and make sure we clean up on failure.
    cur_address = address;

    // Iterate over the memory, unmapping as we go.
    auto it = m_memory_block_manager.FindIterator(cur_address);
    while (true) {
        // Check that the iterator is valid.
        ASSERT(it != m_memory_block_manager.end());

        // Get the memory info.
        const KMemoryInfo info = it->GetMemoryInfo();

        // If the memory state is normal, we need to unmap it.
        if (info.GetState() == KMemoryState::Normal) {
            // Determine the range to unmap.
            const size_t cur_pages = std::min(VAddr(info.GetEndAddress()) - cur_address,
                                              last_address + 1 - cur_address) /
                                     PageSize;

            // Unmap.
            ASSERT(Operate(cur_address, cur_pages, KMemoryPermission::None, OperationType::Unmap)
                       .IsSuccess());
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
    m_mapped_physical_memory_size -= mapped_size;
    m_resource_limit->Release(LimitableResource::PhysicalMemoryMax, mapped_size);

    // Update memory blocks.
    m_memory_block_manager.Update(std::addressof(allocator), address, size / PageSize,
                                  KMemoryState::Free, KMemoryPermission::None,
                                  KMemoryAttribute::None, KMemoryBlockDisableMergeAttribute::None,
                                  KMemoryBlockDisableMergeAttribute::None);

    // We succeeded.
    R_SUCCEED();
}

Result KPageTable::MapMemory(KProcessAddress dst_address, KProcessAddress src_address,
                             size_t size) {
    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Validate that the source address's state is valid.
    KMemoryState src_state;
    size_t num_src_allocator_blocks;
    R_TRY(this->CheckMemoryState(std::addressof(src_state), nullptr, nullptr,
                                 std::addressof(num_src_allocator_blocks), src_address, size,
                                 KMemoryState::FlagCanAlias, KMemoryState::FlagCanAlias,
                                 KMemoryPermission::All, KMemoryPermission::UserReadWrite,
                                 KMemoryAttribute::All, KMemoryAttribute::None));

    // Validate that the dst address's state is valid.
    size_t num_dst_allocator_blocks;
    R_TRY(this->CheckMemoryState(std::addressof(num_dst_allocator_blocks), dst_address, size,
                                 KMemoryState::All, KMemoryState::Free, KMemoryPermission::None,
                                 KMemoryPermission::None, KMemoryAttribute::None,
                                 KMemoryAttribute::None));

    // Create an update allocator for the source.
    Result src_allocator_result;
    KMemoryBlockManagerUpdateAllocator src_allocator(std::addressof(src_allocator_result),
                                                     m_memory_block_slab_manager,
                                                     num_src_allocator_blocks);
    R_TRY(src_allocator_result);

    // Create an update allocator for the destination.
    Result dst_allocator_result;
    KMemoryBlockManagerUpdateAllocator dst_allocator(std::addressof(dst_allocator_result),
                                                     m_memory_block_slab_manager,
                                                     num_dst_allocator_blocks);
    R_TRY(dst_allocator_result);

    // Map the memory.
    {
        // Determine the number of pages being operated on.
        const size_t num_pages = size / PageSize;

        // Create page groups for the memory being unmapped.
        KPageGroup pg{m_kernel, m_block_info_manager};

        // Create the page group representing the source.
        R_TRY(this->MakePageGroup(pg, src_address, num_pages));

        // We're going to perform an update, so create a helper.
        KScopedPageTableUpdater updater(this);

        // Reprotect the source as kernel-read/not mapped.
        const KMemoryPermission new_src_perm = static_cast<KMemoryPermission>(
            KMemoryPermission::KernelRead | KMemoryPermission::NotMapped);
        const KMemoryAttribute new_src_attr = KMemoryAttribute::Locked;
        const KPageProperties src_properties = {new_src_perm, false, false,
                                                DisableMergeAttribute::DisableHeadBodyTail};
        R_TRY(this->Operate(src_address, num_pages, src_properties.perm,
                            OperationType::ChangePermissions));

        // Ensure that we unprotect the source pages on failure.
        ON_RESULT_FAILURE {
            const KPageProperties unprotect_properties = {
                KMemoryPermission::UserReadWrite, false, false,
                DisableMergeAttribute::EnableHeadBodyTail};
            ASSERT(this->Operate(src_address, num_pages, unprotect_properties.perm,
                                 OperationType::ChangePermissions) == ResultSuccess);
        };

        // Map the alias pages.
        const KPageProperties dst_map_properties = {KMemoryPermission::UserReadWrite, false, false,
                                                    DisableMergeAttribute::DisableHead};
        R_TRY(this->MapPageGroupImpl(updater.GetPageList(), dst_address, pg, dst_map_properties,
                                     false));

        // Apply the memory block updates.
        m_memory_block_manager.Update(std::addressof(src_allocator), src_address, num_pages,
                                      src_state, new_src_perm, new_src_attr,
                                      KMemoryBlockDisableMergeAttribute::Locked,
                                      KMemoryBlockDisableMergeAttribute::None);
        m_memory_block_manager.Update(
            std::addressof(dst_allocator), dst_address, num_pages, KMemoryState::Stack,
            KMemoryPermission::UserReadWrite, KMemoryAttribute::None,
            KMemoryBlockDisableMergeAttribute::Normal, KMemoryBlockDisableMergeAttribute::None);
    }

    R_SUCCEED();
}

Result KPageTable::UnmapMemory(KProcessAddress dst_address, KProcessAddress src_address,
                               size_t size) {
    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Validate that the source address's state is valid.
    KMemoryState src_state;
    size_t num_src_allocator_blocks;
    R_TRY(this->CheckMemoryState(
        std::addressof(src_state), nullptr, nullptr, std::addressof(num_src_allocator_blocks),
        src_address, size, KMemoryState::FlagCanAlias, KMemoryState::FlagCanAlias,
        KMemoryPermission::All, KMemoryPermission::NotMapped | KMemoryPermission::KernelRead,
        KMemoryAttribute::All, KMemoryAttribute::Locked));

    // Validate that the dst address's state is valid.
    KMemoryPermission dst_perm;
    size_t num_dst_allocator_blocks;
    R_TRY(this->CheckMemoryState(
        nullptr, std::addressof(dst_perm), nullptr, std::addressof(num_dst_allocator_blocks),
        dst_address, size, KMemoryState::All, KMemoryState::Stack, KMemoryPermission::None,
        KMemoryPermission::None, KMemoryAttribute::All, KMemoryAttribute::None));

    // Create an update allocator for the source.
    Result src_allocator_result;
    KMemoryBlockManagerUpdateAllocator src_allocator(std::addressof(src_allocator_result),
                                                     m_memory_block_slab_manager,
                                                     num_src_allocator_blocks);
    R_TRY(src_allocator_result);

    // Create an update allocator for the destination.
    Result dst_allocator_result;
    KMemoryBlockManagerUpdateAllocator dst_allocator(std::addressof(dst_allocator_result),
                                                     m_memory_block_slab_manager,
                                                     num_dst_allocator_blocks);
    R_TRY(dst_allocator_result);

    // Unmap the memory.
    {
        // Determine the number of pages being operated on.
        const size_t num_pages = size / PageSize;

        // Create page groups for the memory being unmapped.
        KPageGroup pg{m_kernel, m_block_info_manager};

        // Create the page group representing the destination.
        R_TRY(this->MakePageGroup(pg, dst_address, num_pages));

        // Ensure the page group is the valid for the source.
        R_UNLESS(this->IsValidPageGroup(pg, src_address, num_pages), ResultInvalidMemoryRegion);

        // We're going to perform an update, so create a helper.
        KScopedPageTableUpdater updater(this);

        // Unmap the aliased copy of the pages.
        const KPageProperties dst_unmap_properties = {KMemoryPermission::None, false, false,
                                                      DisableMergeAttribute::None};
        R_TRY(
            this->Operate(dst_address, num_pages, dst_unmap_properties.perm, OperationType::Unmap));

        // Ensure that we re-map the aliased pages on failure.
        ON_RESULT_FAILURE {
            this->RemapPageGroup(updater.GetPageList(), dst_address, size, pg);
        };

        // Try to set the permissions for the source pages back to what they should be.
        const KPageProperties src_properties = {KMemoryPermission::UserReadWrite, false, false,
                                                DisableMergeAttribute::EnableAndMergeHeadBodyTail};
        R_TRY(this->Operate(src_address, num_pages, src_properties.perm,
                            OperationType::ChangePermissions));

        // Apply the memory block updates.
        m_memory_block_manager.Update(
            std::addressof(src_allocator), src_address, num_pages, src_state,
            KMemoryPermission::UserReadWrite, KMemoryAttribute::None,
            KMemoryBlockDisableMergeAttribute::None, KMemoryBlockDisableMergeAttribute::Locked);
        m_memory_block_manager.Update(
            std::addressof(dst_allocator), dst_address, num_pages, KMemoryState::None,
            KMemoryPermission::None, KMemoryAttribute::None,
            KMemoryBlockDisableMergeAttribute::None, KMemoryBlockDisableMergeAttribute::Normal);
    }

    R_SUCCEED();
}

Result KPageTable::AllocateAndMapPagesImpl(PageLinkedList* page_list, KProcessAddress address,
                                           size_t num_pages, KMemoryPermission perm) {
    ASSERT(this->IsLockedByCurrentThread());

    // Create a page group to hold the pages we allocate.
    KPageGroup pg{m_kernel, m_block_info_manager};

    // Allocate the pages.
    R_TRY(
        m_kernel.MemoryManager().AllocateAndOpen(std::addressof(pg), num_pages, m_allocate_option));

    // Ensure that the page group is closed when we're done working with it.
    SCOPE_EXIT({ pg.Close(); });

    // Clear all pages.
    for (const auto& it : pg) {
        std::memset(m_system.DeviceMemory().GetPointer<void>(it.GetAddress()), m_heap_fill_value,
                    it.GetSize());
    }

    // Map the pages.
    R_RETURN(this->Operate(address, num_pages, pg, OperationType::MapGroup));
}

Result KPageTable::MapPageGroupImpl(PageLinkedList* page_list, KProcessAddress address,
                                    const KPageGroup& pg, const KPageProperties properties,
                                    bool reuse_ll) {
    ASSERT(this->IsLockedByCurrentThread());

    // Note the current address, so that we can iterate.
    const KProcessAddress start_address = address;
    KProcessAddress cur_address = address;

    // Ensure that we clean up on failure.
    ON_RESULT_FAILURE {
        ASSERT(!reuse_ll);
        if (cur_address != start_address) {
            const KPageProperties unmap_properties = {KMemoryPermission::None, false, false,
                                                      DisableMergeAttribute::None};
            ASSERT(this->Operate(start_address, (cur_address - start_address) / PageSize,
                                 unmap_properties.perm, OperationType::Unmap) == ResultSuccess);
        }
    };

    // Iterate, mapping all pages in the group.
    for (const auto& block : pg) {
        // Map and advance.
        const KPageProperties cur_properties =
            (cur_address == start_address)
                ? properties
                : KPageProperties{properties.perm, properties.io, properties.uncached,
                                  DisableMergeAttribute::None};
        this->Operate(cur_address, block.GetNumPages(), cur_properties.perm, OperationType::Map,
                      block.GetAddress());
        cur_address += block.GetSize();
    }

    // We succeeded!
    R_SUCCEED();
}

void KPageTable::RemapPageGroup(PageLinkedList* page_list, KProcessAddress address, size_t size,
                                const KPageGroup& pg) {
    ASSERT(this->IsLockedByCurrentThread());

    // Note the current address, so that we can iterate.
    const KProcessAddress start_address = address;
    const KProcessAddress last_address = start_address + size - 1;
    const KProcessAddress end_address = last_address + 1;

    // Iterate over the memory.
    auto pg_it = pg.begin();
    ASSERT(pg_it != pg.end());

    KPhysicalAddress pg_phys_addr = pg_it->GetAddress();
    size_t pg_pages = pg_it->GetNumPages();

    auto it = m_memory_block_manager.FindIterator(start_address);
    while (true) {
        // Check that the iterator is valid.
        ASSERT(it != m_memory_block_manager.end());

        // Get the memory info.
        const KMemoryInfo info = it->GetMemoryInfo();

        // Determine the range to map.
        KProcessAddress map_address = std::max(info.GetAddress(), start_address);
        const KProcessAddress map_end_address = std::min(info.GetEndAddress(), end_address);
        ASSERT(map_end_address != map_address);

        // Determine if we should disable head merge.
        const bool disable_head_merge =
            info.GetAddress() >= start_address &&
            True(info.GetDisableMergeAttribute() & KMemoryBlockDisableMergeAttribute::Normal);
        const KPageProperties map_properties = {
            info.GetPermission(), false, false,
            disable_head_merge ? DisableMergeAttribute::DisableHead : DisableMergeAttribute::None};

        // While we have pages to map, map them.
        size_t map_pages = (map_end_address - map_address) / PageSize;
        while (map_pages > 0) {
            // Check if we're at the end of the physical block.
            if (pg_pages == 0) {
                // Ensure there are more pages to map.
                ASSERT(pg_it != pg.end());

                // Advance our physical block.
                ++pg_it;
                pg_phys_addr = pg_it->GetAddress();
                pg_pages = pg_it->GetNumPages();
            }

            // Map whatever we can.
            const size_t cur_pages = std::min(pg_pages, map_pages);
            ASSERT(this->Operate(map_address, map_pages, map_properties.perm, OperationType::Map,
                                 pg_phys_addr) == ResultSuccess);

            // Advance.
            map_address += cur_pages * PageSize;
            map_pages -= cur_pages;

            pg_phys_addr += cur_pages * PageSize;
            pg_pages -= cur_pages;
        }

        // Check if we're done.
        if (last_address <= info.GetLastAddress()) {
            break;
        }

        // Advance.
        ++it;
    }

    // Check that we re-mapped precisely the page group.
    ASSERT((++pg_it) == pg.end());
}

Result KPageTable::MapPages(KProcessAddress* out_addr, size_t num_pages, size_t alignment,
                            KPhysicalAddress phys_addr, bool is_pa_valid,
                            KProcessAddress region_start, size_t region_num_pages,
                            KMemoryState state, KMemoryPermission perm) {
    ASSERT(Common::IsAligned(alignment, PageSize) && alignment >= PageSize);

    // Ensure this is a valid map request.
    R_UNLESS(this->CanContain(region_start, region_num_pages * PageSize, state),
             ResultInvalidCurrentMemory);
    R_UNLESS(num_pages < region_num_pages, ResultOutOfMemory);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Find a random address to map at.
    KProcessAddress addr = this->FindFreeArea(region_start, region_num_pages, num_pages, alignment,
                                              0, this->GetNumGuardPages());
    R_UNLESS(addr != 0, ResultOutOfMemory);
    ASSERT(Common::IsAligned(addr, alignment));
    ASSERT(this->CanContain(addr, num_pages * PageSize, state));
    ASSERT(this->CheckMemoryState(addr, num_pages * PageSize, KMemoryState::All, KMemoryState::Free,
                                  KMemoryPermission::None, KMemoryPermission::None,
                                  KMemoryAttribute::None, KMemoryAttribute::None) == ResultSuccess);

    // Create an update allocator.
    Result allocator_result;
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager);
    R_TRY(allocator_result);

    // We're going to perform an update, so create a helper.
    KScopedPageTableUpdater updater(this);

    // Perform mapping operation.
    if (is_pa_valid) {
        const KPageProperties properties = {perm, false, false, DisableMergeAttribute::DisableHead};
        R_TRY(this->Operate(addr, num_pages, properties.perm, OperationType::Map, phys_addr));
    } else {
        R_TRY(this->AllocateAndMapPagesImpl(updater.GetPageList(), addr, num_pages, perm));
    }

    // Update the blocks.
    m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, state, perm,
                                  KMemoryAttribute::None, KMemoryBlockDisableMergeAttribute::Normal,
                                  KMemoryBlockDisableMergeAttribute::None);

    // We successfully mapped the pages.
    *out_addr = addr;
    R_SUCCEED();
}

Result KPageTable::MapPages(KProcessAddress address, size_t num_pages, KMemoryState state,
                            KMemoryPermission perm) {
    // Check that the map is in range.
    const size_t size = num_pages * PageSize;
    R_UNLESS(this->CanContain(address, size, state), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Check the memory state.
    size_t num_allocator_blocks;
    R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size,
                                 KMemoryState::All, KMemoryState::Free, KMemoryPermission::None,
                                 KMemoryPermission::None, KMemoryAttribute::None,
                                 KMemoryAttribute::None));

    // Create an update allocator.
    Result allocator_result;
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    // We're going to perform an update, so create a helper.
    KScopedPageTableUpdater updater(this);

    // Map the pages.
    R_TRY(this->AllocateAndMapPagesImpl(updater.GetPageList(), address, num_pages, perm));

    // Update the blocks.
    m_memory_block_manager.Update(std::addressof(allocator), address, num_pages, state, perm,
                                  KMemoryAttribute::None, KMemoryBlockDisableMergeAttribute::Normal,
                                  KMemoryBlockDisableMergeAttribute::None);

    R_SUCCEED();
}

Result KPageTable::UnmapPages(KProcessAddress address, size_t num_pages, KMemoryState state) {
    // Check that the unmap is in range.
    const size_t size = num_pages * PageSize;
    R_UNLESS(this->Contains(address, size), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Check the memory state.
    size_t num_allocator_blocks;
    R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size,
                                 KMemoryState::All, state, KMemoryPermission::None,
                                 KMemoryPermission::None, KMemoryAttribute::All,
                                 KMemoryAttribute::None));

    // Create an update allocator.
    Result allocator_result;
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    // We're going to perform an update, so create a helper.
    KScopedPageTableUpdater updater(this);

    // Perform the unmap.
    const KPageProperties unmap_properties = {KMemoryPermission::None, false, false,
                                              DisableMergeAttribute::None};
    R_TRY(this->Operate(address, num_pages, unmap_properties.perm, OperationType::Unmap));

    // Update the blocks.
    m_memory_block_manager.Update(std::addressof(allocator), address, num_pages, KMemoryState::Free,
                                  KMemoryPermission::None, KMemoryAttribute::None,
                                  KMemoryBlockDisableMergeAttribute::None,
                                  KMemoryBlockDisableMergeAttribute::Normal);

    R_SUCCEED();
}

Result KPageTable::MapPageGroup(KProcessAddress* out_addr, const KPageGroup& pg,
                                KProcessAddress region_start, size_t region_num_pages,
                                KMemoryState state, KMemoryPermission perm) {
    ASSERT(!this->IsLockedByCurrentThread());

    // Ensure this is a valid map request.
    const size_t num_pages = pg.GetNumPages();
    R_UNLESS(this->CanContain(region_start, region_num_pages * PageSize, state),
             ResultInvalidCurrentMemory);
    R_UNLESS(num_pages < region_num_pages, ResultOutOfMemory);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Find a random address to map at.
    KProcessAddress addr = this->FindFreeArea(region_start, region_num_pages, num_pages, PageSize,
                                              0, this->GetNumGuardPages());
    R_UNLESS(addr != 0, ResultOutOfMemory);
    ASSERT(this->CanContain(addr, num_pages * PageSize, state));
    ASSERT(this->CheckMemoryState(addr, num_pages * PageSize, KMemoryState::All, KMemoryState::Free,
                                  KMemoryPermission::None, KMemoryPermission::None,
                                  KMemoryAttribute::None, KMemoryAttribute::None) == ResultSuccess);

    // Create an update allocator.
    Result allocator_result;
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager);
    R_TRY(allocator_result);

    // We're going to perform an update, so create a helper.
    KScopedPageTableUpdater updater(this);

    // Perform mapping operation.
    const KPageProperties properties = {perm, state == KMemoryState::Io, false,
                                        DisableMergeAttribute::DisableHead};
    R_TRY(this->MapPageGroupImpl(updater.GetPageList(), addr, pg, properties, false));

    // Update the blocks.
    m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, state, perm,
                                  KMemoryAttribute::None, KMemoryBlockDisableMergeAttribute::Normal,
                                  KMemoryBlockDisableMergeAttribute::None);

    // We successfully mapped the pages.
    *out_addr = addr;
    R_SUCCEED();
}

Result KPageTable::MapPageGroup(KProcessAddress addr, const KPageGroup& pg, KMemoryState state,
                                KMemoryPermission perm) {
    ASSERT(!this->IsLockedByCurrentThread());

    // Ensure this is a valid map request.
    const size_t num_pages = pg.GetNumPages();
    const size_t size = num_pages * PageSize;
    R_UNLESS(this->CanContain(addr, size, state), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Check if state allows us to map.
    size_t num_allocator_blocks;
    R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), addr, size,
                                 KMemoryState::All, KMemoryState::Free, KMemoryPermission::None,
                                 KMemoryPermission::None, KMemoryAttribute::None,
                                 KMemoryAttribute::None));

    // Create an update allocator.
    Result allocator_result;
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    // We're going to perform an update, so create a helper.
    KScopedPageTableUpdater updater(this);

    // Perform mapping operation.
    const KPageProperties properties = {perm, state == KMemoryState::Io, false,
                                        DisableMergeAttribute::DisableHead};
    R_TRY(this->MapPageGroupImpl(updater.GetPageList(), addr, pg, properties, false));

    // Update the blocks.
    m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, state, perm,
                                  KMemoryAttribute::None, KMemoryBlockDisableMergeAttribute::Normal,
                                  KMemoryBlockDisableMergeAttribute::None);

    // We successfully mapped the pages.
    R_SUCCEED();
}

Result KPageTable::UnmapPageGroup(KProcessAddress address, const KPageGroup& pg,
                                  KMemoryState state) {
    ASSERT(!this->IsLockedByCurrentThread());

    // Ensure this is a valid unmap request.
    const size_t num_pages = pg.GetNumPages();
    const size_t size = num_pages * PageSize;
    R_UNLESS(this->CanContain(address, size, state), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Check if state allows us to unmap.
    size_t num_allocator_blocks;
    R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), address, size,
                                 KMemoryState::All, state, KMemoryPermission::None,
                                 KMemoryPermission::None, KMemoryAttribute::All,
                                 KMemoryAttribute::None));

    // Check that the page group is valid.
    R_UNLESS(this->IsValidPageGroup(pg, address, num_pages), ResultInvalidCurrentMemory);

    // Create an update allocator.
    Result allocator_result;
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    // We're going to perform an update, so create a helper.
    KScopedPageTableUpdater updater(this);

    // Perform unmapping operation.
    const KPageProperties properties = {KMemoryPermission::None, false, false,
                                        DisableMergeAttribute::None};
    R_TRY(this->Operate(address, num_pages, properties.perm, OperationType::Unmap));

    // Update the blocks.
    m_memory_block_manager.Update(std::addressof(allocator), address, num_pages, KMemoryState::Free,
                                  KMemoryPermission::None, KMemoryAttribute::None,
                                  KMemoryBlockDisableMergeAttribute::None,
                                  KMemoryBlockDisableMergeAttribute::Normal);

    R_SUCCEED();
}

Result KPageTable::MakeAndOpenPageGroup(KPageGroup* out, VAddr address, size_t num_pages,
                                        KMemoryState state_mask, KMemoryState state,
                                        KMemoryPermission perm_mask, KMemoryPermission perm,
                                        KMemoryAttribute attr_mask, KMemoryAttribute attr) {
    // Ensure that the page group isn't null.
    ASSERT(out != nullptr);

    // Make sure that the region we're mapping is valid for the table.
    const size_t size = num_pages * PageSize;
    R_UNLESS(this->Contains(address, size), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Check if state allows us to create the group.
    R_TRY(this->CheckMemoryState(address, size, state_mask | KMemoryState::FlagReferenceCounted,
                                 state | KMemoryState::FlagReferenceCounted, perm_mask, perm,
                                 attr_mask, attr));

    // Create a new page group for the region.
    R_TRY(this->MakePageGroup(*out, address, num_pages));

    R_SUCCEED();
}

Result KPageTable::SetProcessMemoryPermission(VAddr addr, size_t size,
                                              Svc::MemoryPermission svc_perm) {
    const size_t num_pages = size / PageSize;

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

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
            break;
        }
    }

    // Succeed if there's nothing to do.
    R_SUCCEED_IF(old_perm == new_perm && old_state == new_state);

    // Create an update allocator.
    Result allocator_result{ResultSuccess};
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    // Perform mapping operation.
    const auto operation =
        was_x ? OperationType::ChangePermissionsAndRefresh : OperationType::ChangePermissions;
    R_TRY(Operate(addr, num_pages, new_perm, operation));

    // Update the blocks.
    m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, new_state, new_perm,
                                  KMemoryAttribute::None, KMemoryBlockDisableMergeAttribute::None,
                                  KMemoryBlockDisableMergeAttribute::None);

    // Ensure cache coherency, if we're setting pages as executable.
    if (is_x) {
        m_system.InvalidateCpuInstructionCacheRange(addr, size);
    }

    R_SUCCEED();
}

KMemoryInfo KPageTable::QueryInfoImpl(VAddr addr) {
    KScopedLightLock lk(m_general_lock);

    return m_memory_block_manager.FindBlock(addr)->GetMemoryInfo();
}

KMemoryInfo KPageTable::QueryInfo(VAddr addr) {
    if (!Contains(addr, 1)) {
        return {
            .m_address = m_address_space_end,
            .m_size = 0 - m_address_space_end,
            .m_state = static_cast<KMemoryState>(Svc::MemoryState::Inaccessible),
            .m_device_disable_merge_left_count = 0,
            .m_device_disable_merge_right_count = 0,
            .m_ipc_lock_count = 0,
            .m_device_use_count = 0,
            .m_ipc_disable_merge_count = 0,
            .m_permission = KMemoryPermission::None,
            .m_attribute = KMemoryAttribute::None,
            .m_original_permission = KMemoryPermission::None,
            .m_disable_merge_attribute = KMemoryBlockDisableMergeAttribute::None,
        };
    }

    return QueryInfoImpl(addr);
}

Result KPageTable::SetMemoryPermission(VAddr addr, size_t size, Svc::MemoryPermission svc_perm) {
    const size_t num_pages = size / PageSize;

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Verify we can change the memory permission.
    KMemoryState old_state;
    KMemoryPermission old_perm;
    size_t num_allocator_blocks;
    R_TRY(this->CheckMemoryState(std::addressof(old_state), std::addressof(old_perm), nullptr,
                                 std::addressof(num_allocator_blocks), addr, size,
                                 KMemoryState::FlagCanReprotect, KMemoryState::FlagCanReprotect,
                                 KMemoryPermission::None, KMemoryPermission::None,
                                 KMemoryAttribute::All, KMemoryAttribute::None));

    // Determine new perm.
    const KMemoryPermission new_perm = ConvertToKMemoryPermission(svc_perm);
    R_SUCCEED_IF(old_perm == new_perm);

    // Create an update allocator.
    Result allocator_result{ResultSuccess};
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    // Perform mapping operation.
    R_TRY(Operate(addr, num_pages, new_perm, OperationType::ChangePermissions));

    // Update the blocks.
    m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, old_state, new_perm,
                                  KMemoryAttribute::None, KMemoryBlockDisableMergeAttribute::None,
                                  KMemoryBlockDisableMergeAttribute::None);

    R_SUCCEED();
}

Result KPageTable::SetMemoryAttribute(VAddr addr, size_t size, u32 mask, u32 attr) {
    const size_t num_pages = size / PageSize;
    ASSERT((static_cast<KMemoryAttribute>(mask) | KMemoryAttribute::SetMask) ==
           KMemoryAttribute::SetMask);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

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

    // Create an update allocator.
    Result allocator_result{ResultSuccess};
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    // Determine the new attribute.
    const KMemoryAttribute new_attr =
        static_cast<KMemoryAttribute>(((old_attr & static_cast<KMemoryAttribute>(~mask)) |
                                       static_cast<KMemoryAttribute>(attr & mask)));

    // Perform operation.
    this->Operate(addr, num_pages, old_perm, OperationType::ChangePermissionsAndRefresh);

    // Update the blocks.
    m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, old_state, old_perm,
                                  new_attr, KMemoryBlockDisableMergeAttribute::None,
                                  KMemoryBlockDisableMergeAttribute::None);

    R_SUCCEED();
}

Result KPageTable::SetMaxHeapSize(size_t size) {
    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Only process page tables are allowed to set heap size.
    ASSERT(!this->IsKernel());

    m_max_heap_size = size;

    R_SUCCEED();
}

Result KPageTable::SetHeapSize(VAddr* out, size_t size) {
    // Lock the physical memory mutex.
    KScopedLightLock map_phys_mem_lk(m_map_physical_memory_lock);

    // Try to perform a reduction in heap, instead of an extension.
    VAddr cur_address{};
    size_t allocation_size{};
    {
        // Lock the table.
        KScopedLightLock lk(m_general_lock);

        // Validate that setting heap size is possible at all.
        R_UNLESS(!m_is_kernel, ResultOutOfMemory);
        R_UNLESS(size <= static_cast<size_t>(m_heap_region_end - m_heap_region_start),
                 ResultOutOfMemory);
        R_UNLESS(size <= m_max_heap_size, ResultOutOfMemory);

        if (size < GetHeapSize()) {
            // The size being requested is less than the current size, so we need to free the end of
            // the heap.

            // Validate memory state.
            size_t num_allocator_blocks;
            R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks),
                                         m_heap_region_start + size, GetHeapSize() - size,
                                         KMemoryState::All, KMemoryState::Normal,
                                         KMemoryPermission::All, KMemoryPermission::UserReadWrite,
                                         KMemoryAttribute::All, KMemoryAttribute::None));

            // Create an update allocator.
            Result allocator_result{ResultSuccess};
            KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                         m_memory_block_slab_manager,
                                                         num_allocator_blocks);
            R_TRY(allocator_result);

            // Unmap the end of the heap.
            const auto num_pages = (GetHeapSize() - size) / PageSize;
            R_TRY(Operate(m_heap_region_start + size, num_pages, KMemoryPermission::None,
                          OperationType::Unmap));

            // Release the memory from the resource limit.
            m_resource_limit->Release(LimitableResource::PhysicalMemoryMax, num_pages * PageSize);

            // Apply the memory block update.
            m_memory_block_manager.Update(std::addressof(allocator), m_heap_region_start + size,
                                          num_pages, KMemoryState::Free, KMemoryPermission::None,
                                          KMemoryAttribute::None,
                                          KMemoryBlockDisableMergeAttribute::None,
                                          size == 0 ? KMemoryBlockDisableMergeAttribute::Normal
                                                    : KMemoryBlockDisableMergeAttribute::None);

            // Update the current heap end.
            m_current_heap_end = m_heap_region_start + size;

            // Set the output.
            *out = m_heap_region_start;
            R_SUCCEED();
        } else if (size == GetHeapSize()) {
            // The size requested is exactly the current size.
            *out = m_heap_region_start;
            R_SUCCEED();
        } else {
            // We have to allocate memory. Determine how much to allocate and where while the table
            // is locked.
            cur_address = m_current_heap_end;
            allocation_size = size - GetHeapSize();
        }
    }

    // Reserve memory for the heap extension.
    KScopedResourceReservation memory_reservation(
        m_resource_limit, LimitableResource::PhysicalMemoryMax, allocation_size);
    R_UNLESS(memory_reservation.Succeeded(), ResultLimitReached);

    // Allocate pages for the heap extension.
    KPageGroup pg{m_kernel, m_block_info_manager};
    R_TRY(m_system.Kernel().MemoryManager().AllocateAndOpen(
        &pg, allocation_size / PageSize,
        KMemoryManager::EncodeOption(m_memory_pool, m_allocation_option)));

    // Clear all the newly allocated pages.
    for (const auto& it : pg) {
        std::memset(m_system.DeviceMemory().GetPointer<void>(it.GetAddress()), m_heap_fill_value,
                    it.GetSize());
    }

    // Map the pages.
    {
        // Lock the table.
        KScopedLightLock lk(m_general_lock);

        // Ensure that the heap hasn't changed since we began executing.
        ASSERT(cur_address == m_current_heap_end);

        // Check the memory state.
        size_t num_allocator_blocks{};
        R_TRY(this->CheckMemoryState(std::addressof(num_allocator_blocks), m_current_heap_end,
                                     allocation_size, KMemoryState::All, KMemoryState::Free,
                                     KMemoryPermission::None, KMemoryPermission::None,
                                     KMemoryAttribute::None, KMemoryAttribute::None));

        // Create an update allocator.
        Result allocator_result{ResultSuccess};
        KMemoryBlockManagerUpdateAllocator allocator(
            std::addressof(allocator_result), m_memory_block_slab_manager, num_allocator_blocks);
        R_TRY(allocator_result);

        // Map the pages.
        const auto num_pages = allocation_size / PageSize;
        R_TRY(Operate(m_current_heap_end, num_pages, pg, OperationType::MapGroup));

        // Clear all the newly allocated pages.
        for (size_t cur_page = 0; cur_page < num_pages; ++cur_page) {
            std::memset(m_system.Memory().GetPointer(m_current_heap_end + (cur_page * PageSize)), 0,
                        PageSize);
        }

        // We succeeded, so commit our memory reservation.
        memory_reservation.Commit();

        // Apply the memory block update.
        m_memory_block_manager.Update(
            std::addressof(allocator), m_current_heap_end, num_pages, KMemoryState::Normal,
            KMemoryPermission::UserReadWrite, KMemoryAttribute::None,
            m_heap_region_start == m_current_heap_end ? KMemoryBlockDisableMergeAttribute::Normal
                                                      : KMemoryBlockDisableMergeAttribute::None,
            KMemoryBlockDisableMergeAttribute::None);

        // Update the current heap end.
        m_current_heap_end = m_heap_region_start + size;

        // Set the output.
        *out = m_heap_region_start;
        R_SUCCEED();
    }
}

Result KPageTable::LockForMapDeviceAddressSpace(bool* out_is_io, VAddr address, size_t size,
                                                KMemoryPermission perm, bool is_aligned,
                                                bool check_heap) {
    // Lightly validate the range before doing anything else.
    const size_t num_pages = size / PageSize;
    R_UNLESS(this->Contains(address, size), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Check the memory state.
    const auto test_state =
        (is_aligned ? KMemoryState::FlagCanAlignedDeviceMap : KMemoryState::FlagCanDeviceMap) |
        (check_heap ? KMemoryState::FlagReferenceCounted : KMemoryState::None);
    size_t num_allocator_blocks;
    KMemoryState old_state;
    R_TRY(this->CheckMemoryState(std::addressof(old_state), nullptr, nullptr,
                                 std::addressof(num_allocator_blocks), address, size, test_state,
                                 test_state, perm, perm,
                                 KMemoryAttribute::IpcLocked | KMemoryAttribute::Locked,
                                 KMemoryAttribute::None, KMemoryAttribute::DeviceShared));

    // Create an update allocator.
    Result allocator_result;
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    // Update the memory blocks.
    m_memory_block_manager.UpdateLock(std::addressof(allocator), address, num_pages,
                                      &KMemoryBlock::ShareToDevice, KMemoryPermission::None);

    // Set whether the locked memory was io.
    *out_is_io = old_state == KMemoryState::Io;

    R_SUCCEED();
}

Result KPageTable::LockForUnmapDeviceAddressSpace(VAddr address, size_t size, bool check_heap) {
    // Lightly validate the range before doing anything else.
    const size_t num_pages = size / PageSize;
    R_UNLESS(this->Contains(address, size), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Check the memory state.
    const auto test_state = KMemoryState::FlagCanDeviceMap |
                            (check_heap ? KMemoryState::FlagReferenceCounted : KMemoryState::None);
    size_t num_allocator_blocks;
    R_TRY(this->CheckMemoryStateContiguous(
        std::addressof(num_allocator_blocks), address, size, test_state, test_state,
        KMemoryPermission::None, KMemoryPermission::None,
        KMemoryAttribute::DeviceShared | KMemoryAttribute::Locked, KMemoryAttribute::DeviceShared));

    // Create an update allocator.
    Result allocator_result;
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    // Update the memory blocks.
    const KMemoryBlockManager::MemoryBlockLockFunction lock_func =
        m_enable_device_address_space_merge
            ? &KMemoryBlock::UpdateDeviceDisableMergeStateForShare
            : &KMemoryBlock::UpdateDeviceDisableMergeStateForShareRight;
    m_memory_block_manager.UpdateLock(std::addressof(allocator), address, num_pages, lock_func,
                                      KMemoryPermission::None);

    R_SUCCEED();
}

Result KPageTable::UnlockForDeviceAddressSpace(VAddr address, size_t size) {
    // Lightly validate the range before doing anything else.
    const size_t num_pages = size / PageSize;
    R_UNLESS(this->Contains(address, size), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

    // Check the memory state.
    size_t num_allocator_blocks;
    R_TRY(this->CheckMemoryStateContiguous(
        std::addressof(num_allocator_blocks), address, size, KMemoryState::FlagCanDeviceMap,
        KMemoryState::FlagCanDeviceMap, KMemoryPermission::None, KMemoryPermission::None,
        KMemoryAttribute::DeviceShared | KMemoryAttribute::Locked, KMemoryAttribute::DeviceShared));

    // Create an update allocator.
    Result allocator_result{ResultSuccess};
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    // Update the memory blocks.
    m_memory_block_manager.UpdateLock(std::addressof(allocator), address, num_pages,
                                      &KMemoryBlock::UnshareToDevice, KMemoryPermission::None);

    R_SUCCEED();
}

Result KPageTable::LockForIpcUserBuffer(PAddr* out, VAddr address, size_t size) {
    R_RETURN(this->LockMemoryAndOpen(
        nullptr, out, address, size, KMemoryState::FlagCanIpcUserBuffer,
        KMemoryState::FlagCanIpcUserBuffer, KMemoryPermission::All,
        KMemoryPermission::UserReadWrite, KMemoryAttribute::All, KMemoryAttribute::None,
        KMemoryPermission::NotMapped | KMemoryPermission::KernelReadWrite,
        KMemoryAttribute::Locked));
}

Result KPageTable::UnlockForIpcUserBuffer(VAddr address, size_t size) {
    R_RETURN(this->UnlockMemory(address, size, KMemoryState::FlagCanIpcUserBuffer,
                                KMemoryState::FlagCanIpcUserBuffer, KMemoryPermission::None,
                                KMemoryPermission::None, KMemoryAttribute::All,
                                KMemoryAttribute::Locked, KMemoryPermission::UserReadWrite,
                                KMemoryAttribute::Locked, nullptr));
}

Result KPageTable::LockForCodeMemory(KPageGroup* out, VAddr addr, size_t size) {
    R_RETURN(this->LockMemoryAndOpen(
        out, nullptr, addr, size, KMemoryState::FlagCanCodeMemory, KMemoryState::FlagCanCodeMemory,
        KMemoryPermission::All, KMemoryPermission::UserReadWrite, KMemoryAttribute::All,
        KMemoryAttribute::None, KMemoryPermission::NotMapped | KMemoryPermission::KernelReadWrite,
        KMemoryAttribute::Locked));
}

Result KPageTable::UnlockForCodeMemory(VAddr addr, size_t size, const KPageGroup& pg) {
    R_RETURN(this->UnlockMemory(
        addr, size, KMemoryState::FlagCanCodeMemory, KMemoryState::FlagCanCodeMemory,
        KMemoryPermission::None, KMemoryPermission::None, KMemoryAttribute::All,
        KMemoryAttribute::Locked, KMemoryPermission::UserReadWrite, KMemoryAttribute::Locked, &pg));
}

bool KPageTable::IsRegionContiguous(VAddr addr, u64 size) const {
    auto start_ptr = m_system.DeviceMemory().GetPointer<u8>(addr);
    for (u64 offset{}; offset < size; offset += PageSize) {
        if (start_ptr != m_system.DeviceMemory().GetPointer<u8>(addr + offset)) {
            return false;
        }
        start_ptr += PageSize;
    }
    return true;
}

void KPageTable::AddRegionToPages(VAddr start, size_t num_pages, KPageGroup& page_linked_list) {
    VAddr addr{start};
    while (addr < start + (num_pages * PageSize)) {
        const PAddr paddr{GetPhysicalAddr(addr)};
        ASSERT(paddr != 0);
        page_linked_list.AddBlock(paddr, 1);
        addr += PageSize;
    }
}

VAddr KPageTable::AllocateVirtualMemory(VAddr start, size_t region_num_pages, u64 needed_num_pages,
                                        size_t align) {
    if (m_enable_aslr) {
        UNIMPLEMENTED();
    }
    return m_memory_block_manager.FindFreeArea(start, region_num_pages, needed_num_pages, align, 0,
                                               IsKernel() ? 1 : 4);
}

Result KPageTable::Operate(VAddr addr, size_t num_pages, const KPageGroup& page_group,
                           OperationType operation) {
    ASSERT(this->IsLockedByCurrentThread());

    ASSERT(Common::IsAligned(addr, PageSize));
    ASSERT(num_pages > 0);
    ASSERT(num_pages == page_group.GetNumPages());

    switch (operation) {
    case OperationType::MapGroup: {
        // We want to maintain a new reference to every page in the group.
        KScopedPageGroup spg(page_group);

        for (const auto& node : page_group) {
            const size_t size{node.GetNumPages() * PageSize};

            // Map the pages.
            m_system.Memory().MapMemoryRegion(*m_page_table_impl, addr, size, node.GetAddress());

            addr += size;
        }

        // We succeeded! We want to persist the reference to the pages.
        spg.CancelClose();

        break;
    }
    default:
        ASSERT(false);
        break;
    }

    R_SUCCEED();
}

Result KPageTable::Operate(VAddr addr, size_t num_pages, KMemoryPermission perm,
                           OperationType operation, PAddr map_addr) {
    ASSERT(this->IsLockedByCurrentThread());

    ASSERT(num_pages > 0);
    ASSERT(Common::IsAligned(addr, PageSize));
    ASSERT(ContainsPages(addr, num_pages));

    switch (operation) {
    case OperationType::Unmap: {
        // Ensure that any pages we track close on exit.
        KPageGroup pages_to_close{m_kernel, this->GetBlockInfoManager()};
        SCOPE_EXIT({ pages_to_close.CloseAndReset(); });

        this->AddRegionToPages(addr, num_pages, pages_to_close);
        m_system.Memory().UnmapRegion(*m_page_table_impl, addr, num_pages * PageSize);
        break;
    }
    case OperationType::MapFirst:
    case OperationType::Map: {
        ASSERT(map_addr);
        ASSERT(Common::IsAligned(map_addr, PageSize));
        m_system.Memory().MapMemoryRegion(*m_page_table_impl, addr, num_pages * PageSize, map_addr);

        // Open references to pages, if we should.
        if (IsHeapPhysicalAddress(m_kernel.MemoryLayout(), map_addr)) {
            if (operation == OperationType::MapFirst) {
                m_kernel.MemoryManager().OpenFirst(map_addr, num_pages);
            } else {
                m_kernel.MemoryManager().Open(map_addr, num_pages);
            }
        }
        break;
    }
    case OperationType::Separate: {
        // HACK: Unimplemented.
        break;
    }
    case OperationType::ChangePermissions:
    case OperationType::ChangePermissionsAndRefresh:
        break;
    default:
        ASSERT(false);
        break;
    }
    R_SUCCEED();
}

void KPageTable::FinalizeUpdate(PageLinkedList* page_list) {
    while (page_list->Peek()) {
        [[maybe_unused]] auto page = page_list->Pop();

        // TODO(bunnei): Free pages once they are allocated in guest memory
        // ASSERT(this->GetPageTableManager().IsInPageTableHeap(page));
        // ASSERT(this->GetPageTableManager().GetRefCount(page) == 0);
        // this->GetPageTableManager().Free(page);
    }
}

VAddr KPageTable::GetRegionAddress(KMemoryState state) const {
    switch (state) {
    case KMemoryState::Free:
    case KMemoryState::Kernel:
        return m_address_space_start;
    case KMemoryState::Normal:
        return m_heap_region_start;
    case KMemoryState::Ipc:
    case KMemoryState::NonSecureIpc:
    case KMemoryState::NonDeviceIpc:
        return m_alias_region_start;
    case KMemoryState::Stack:
        return m_stack_region_start;
    case KMemoryState::Static:
    case KMemoryState::ThreadLocal:
        return m_kernel_map_region_start;
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
    case KMemoryState::Insecure:
        return m_alias_code_region_start;
    case KMemoryState::Code:
    case KMemoryState::CodeData:
        return m_code_region_start;
    default:
        UNREACHABLE();
    }
}

size_t KPageTable::GetRegionSize(KMemoryState state) const {
    switch (state) {
    case KMemoryState::Free:
    case KMemoryState::Kernel:
        return m_address_space_end - m_address_space_start;
    case KMemoryState::Normal:
        return m_heap_region_end - m_heap_region_start;
    case KMemoryState::Ipc:
    case KMemoryState::NonSecureIpc:
    case KMemoryState::NonDeviceIpc:
        return m_alias_region_end - m_alias_region_start;
    case KMemoryState::Stack:
        return m_stack_region_end - m_stack_region_start;
    case KMemoryState::Static:
    case KMemoryState::ThreadLocal:
        return m_kernel_map_region_end - m_kernel_map_region_start;
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
    case KMemoryState::Insecure:
        return m_alias_code_region_end - m_alias_code_region_start;
    case KMemoryState::Code:
    case KMemoryState::CodeData:
        return m_code_region_end - m_code_region_start;
    default:
        UNREACHABLE();
    }
}

bool KPageTable::CanContain(VAddr addr, size_t size, KMemoryState state) const {
    const VAddr end = addr + size;
    const VAddr last = end - 1;

    const VAddr region_start = this->GetRegionAddress(state);
    const size_t region_size = this->GetRegionSize(state);

    const bool is_in_region =
        region_start <= addr && addr < end && last <= region_start + region_size - 1;
    const bool is_in_heap = !(end <= m_heap_region_start || m_heap_region_end <= addr ||
                              m_heap_region_start == m_heap_region_end);
    const bool is_in_alias = !(end <= m_alias_region_start || m_alias_region_end <= addr ||
                               m_alias_region_start == m_alias_region_end);
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
    case KMemoryState::Insecure:
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

Result KPageTable::CheckMemoryState(const KMemoryInfo& info, KMemoryState state_mask,
                                    KMemoryState state, KMemoryPermission perm_mask,
                                    KMemoryPermission perm, KMemoryAttribute attr_mask,
                                    KMemoryAttribute attr) const {
    // Validate the states match expectation.
    R_UNLESS((info.m_state & state_mask) == state, ResultInvalidCurrentMemory);
    R_UNLESS((info.m_permission & perm_mask) == perm, ResultInvalidCurrentMemory);
    R_UNLESS((info.m_attribute & attr_mask) == attr, ResultInvalidCurrentMemory);

    R_SUCCEED();
}

Result KPageTable::CheckMemoryStateContiguous(size_t* out_blocks_needed, VAddr addr, size_t size,
                                              KMemoryState state_mask, KMemoryState state,
                                              KMemoryPermission perm_mask, KMemoryPermission perm,
                                              KMemoryAttribute attr_mask,
                                              KMemoryAttribute attr) const {
    ASSERT(this->IsLockedByCurrentThread());

    // Get information about the first block.
    const VAddr last_addr = addr + size - 1;
    KMemoryBlockManager::const_iterator it = m_memory_block_manager.FindIterator(addr);
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
        ASSERT(it != m_memory_block_manager.cend());
        info = it->GetMemoryInfo();
    }

    // If the end address isn't aligned, we need a block.
    const size_t blocks_for_end_align =
        (Common::AlignUp(addr + size, PageSize) != info.GetEndAddress()) ? 1 : 0;

    if (out_blocks_needed != nullptr) {
        *out_blocks_needed = blocks_for_start_align + blocks_for_end_align;
    }

    R_SUCCEED();
}

Result KPageTable::CheckMemoryState(KMemoryState* out_state, KMemoryPermission* out_perm,
                                    KMemoryAttribute* out_attr, size_t* out_blocks_needed,
                                    VAddr addr, size_t size, KMemoryState state_mask,
                                    KMemoryState state, KMemoryPermission perm_mask,
                                    KMemoryPermission perm, KMemoryAttribute attr_mask,
                                    KMemoryAttribute attr, KMemoryAttribute ignore_attr) const {
    ASSERT(this->IsLockedByCurrentThread());

    // Get information about the first block.
    const VAddr last_addr = addr + size - 1;
    KMemoryBlockManager::const_iterator it = m_memory_block_manager.FindIterator(addr);
    KMemoryInfo info = it->GetMemoryInfo();

    // If the start address isn't aligned, we need a block.
    const size_t blocks_for_start_align =
        (Common::AlignDown(addr, PageSize) != info.GetAddress()) ? 1 : 0;

    // Validate all blocks in the range have correct state.
    const KMemoryState first_state = info.m_state;
    const KMemoryPermission first_perm = info.m_permission;
    const KMemoryAttribute first_attr = info.m_attribute;
    while (true) {
        // Validate the current block.
        R_UNLESS(info.m_state == first_state, ResultInvalidCurrentMemory);
        R_UNLESS(info.m_permission == first_perm, ResultInvalidCurrentMemory);
        R_UNLESS((info.m_attribute | ignore_attr) == (first_attr | ignore_attr),
                 ResultInvalidCurrentMemory);

        // Validate against the provided masks.
        R_TRY(this->CheckMemoryState(info, state_mask, state, perm_mask, perm, attr_mask, attr));

        // Break once we're done.
        if (last_addr <= info.GetLastAddress()) {
            break;
        }

        // Advance our iterator.
        it++;
        ASSERT(it != m_memory_block_manager.cend());
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
    R_SUCCEED();
}

Result KPageTable::LockMemoryAndOpen(KPageGroup* out_pg, PAddr* out_paddr, VAddr addr, size_t size,
                                     KMemoryState state_mask, KMemoryState state,
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
    KScopedLightLock lk(m_general_lock);

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

    // Create an update allocator.
    Result allocator_result{ResultSuccess};
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    // Decide on new perm and attr.
    new_perm = (new_perm != KMemoryPermission::None) ? new_perm : old_perm;
    KMemoryAttribute new_attr = static_cast<KMemoryAttribute>(old_attr | lock_attr);

    // Update permission, if we need to.
    if (new_perm != old_perm) {
        R_TRY(Operate(addr, num_pages, new_perm, OperationType::ChangePermissions));
    }

    // Apply the memory block updates.
    m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, old_state, new_perm,
                                  new_attr, KMemoryBlockDisableMergeAttribute::Locked,
                                  KMemoryBlockDisableMergeAttribute::None);

    R_SUCCEED();
}

Result KPageTable::UnlockMemory(VAddr addr, size_t size, KMemoryState state_mask,
                                KMemoryState state, KMemoryPermission perm_mask,
                                KMemoryPermission perm, KMemoryAttribute attr_mask,
                                KMemoryAttribute attr, KMemoryPermission new_perm,
                                KMemoryAttribute lock_attr, const KPageGroup* pg) {
    // Validate basic preconditions.
    ASSERT((attr_mask & lock_attr) == lock_attr);
    ASSERT((attr & lock_attr) == lock_attr);

    // Validate the unlock request.
    const size_t num_pages = size / PageSize;
    R_UNLESS(this->Contains(addr, size), ResultInvalidCurrentMemory);

    // Lock the table.
    KScopedLightLock lk(m_general_lock);

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

    // Create an update allocator.
    Result allocator_result{ResultSuccess};
    KMemoryBlockManagerUpdateAllocator allocator(std::addressof(allocator_result),
                                                 m_memory_block_slab_manager, num_allocator_blocks);
    R_TRY(allocator_result);

    // Update permission, if we need to.
    if (new_perm != old_perm) {
        R_TRY(Operate(addr, num_pages, new_perm, OperationType::ChangePermissions));
    }

    // Apply the memory block updates.
    m_memory_block_manager.Update(std::addressof(allocator), addr, num_pages, old_state, new_perm,
                                  new_attr, KMemoryBlockDisableMergeAttribute::None,
                                  KMemoryBlockDisableMergeAttribute::Locked);

    R_SUCCEED();
}

} // namespace Kernel

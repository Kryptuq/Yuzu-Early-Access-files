// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
        UNREACHABLE();
        return {};
    }
}

constexpr u64 GetAddressInRange(const KMemoryInfo& info, VAddr addr) {
    if (info.GetAddress() < addr) {
        return addr;
    }
    return info.GetAddress();
}

constexpr std::size_t GetSizeInRange(const KMemoryInfo& info, VAddr start, VAddr end) {
    std::size_t size{info.GetSize()};
    if (info.GetAddress() < start) {
        size -= start - info.GetAddress();
    }
    if (info.GetEndAddress() > end) {
        size -= info.GetEndAddress() - end;
    }
    return size;
}

} // namespace

KPageTable::KPageTable(Core::System& system_) : system{system_} {}

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

    ASSERT(start <= code_addr);
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
        UNREACHABLE();
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
    std::lock_guard lock{page_table_lock};

    const u64 size{num_pages * PageSize};

    if (!CanContain(addr, size, state)) {
        return ResultInvalidCurrentMemory;
    }

    if (IsRegionMapped(addr, size)) {
        return ResultInvalidCurrentMemory;
    }

    KPageLinkedList page_linked_list;
    CASCADE_CODE(system.Kernel().MemoryManager().Allocate(page_linked_list, num_pages, memory_pool,
                                                          allocation_option));
    CASCADE_CODE(Operate(addr, num_pages, page_linked_list, OperationType::MapGroup));

    block_manager->Update(addr, num_pages, state, perm);

    return ResultSuccess;
}

ResultCode KPageTable::MapCodeMemory(VAddr dst_addr, VAddr src_addr, std::size_t size) {
    std::lock_guard lock{page_table_lock};

    const std::size_t num_pages{size / PageSize};

    KMemoryState state{};
    KMemoryPermission perm{};
    CASCADE_CODE(CheckMemoryState(&state, &perm, nullptr, nullptr, src_addr, size,
                                  KMemoryState::All, KMemoryState::Normal, KMemoryPermission::All,
                                  KMemoryPermission::UserReadWrite, KMemoryAttribute::Mask,
                                  KMemoryAttribute::None, KMemoryAttribute::IpcAndDeviceMapped));

    if (IsRegionMapped(dst_addr, size)) {
        return ResultInvalidCurrentMemory;
    }

    KPageLinkedList page_linked_list;
    AddRegionToPages(src_addr, num_pages, page_linked_list);

    {
        auto block_guard = detail::ScopeExit(
            [&] { Operate(src_addr, num_pages, perm, OperationType::ChangePermissions); });

        CASCADE_CODE(Operate(src_addr, num_pages, KMemoryPermission::None,
                             OperationType::ChangePermissions));
        CASCADE_CODE(MapPages(dst_addr, page_linked_list, KMemoryPermission::None));

        block_guard.Cancel();
    }

    block_manager->Update(src_addr, num_pages, state, KMemoryPermission::None,
                          KMemoryAttribute::Locked);
    block_manager->Update(dst_addr, num_pages, KMemoryState::AliasCode);

    return ResultSuccess;
}

ResultCode KPageTable::UnmapCodeMemory(VAddr dst_addr, VAddr src_addr, std::size_t size) {
    std::lock_guard lock{page_table_lock};

    if (!size) {
        return ResultSuccess;
    }

    const std::size_t num_pages{size / PageSize};

    CASCADE_CODE(CheckMemoryState(nullptr, nullptr, nullptr, nullptr, src_addr, size,
                                  KMemoryState::All, KMemoryState::Normal, KMemoryPermission::None,
                                  KMemoryPermission::None, KMemoryAttribute::Mask,
                                  KMemoryAttribute::Locked, KMemoryAttribute::IpcAndDeviceMapped));

    KMemoryState state{};
    CASCADE_CODE(CheckMemoryState(
        &state, nullptr, nullptr, nullptr, dst_addr, PageSize, KMemoryState::FlagCanCodeAlias,
        KMemoryState::FlagCanCodeAlias, KMemoryPermission::None, KMemoryPermission::None,
        KMemoryAttribute::Mask, KMemoryAttribute::None, KMemoryAttribute::IpcAndDeviceMapped));
    CASCADE_CODE(CheckMemoryState(dst_addr, size, KMemoryState::All, state, KMemoryPermission::None,
                                  KMemoryPermission::None, KMemoryAttribute::Mask,
                                  KMemoryAttribute::None));
    CASCADE_CODE(Operate(dst_addr, num_pages, KMemoryPermission::None, OperationType::Unmap));

    block_manager->Update(dst_addr, num_pages, KMemoryState::Free);
    block_manager->Update(src_addr, num_pages, KMemoryState::Normal,
                          KMemoryPermission::UserReadWrite);

    system.InvalidateCpuInstructionCacheRange(dst_addr, size);

    return ResultSuccess;
}

ResultCode KPageTable::UnmapProcessMemory(VAddr dst_addr, std::size_t size,
                                          KPageTable& src_page_table, VAddr src_addr) {
    std::lock_guard lock{page_table_lock};

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

    return ResultSuccess;
}
void KPageTable::MapPhysicalMemory(KPageLinkedList& page_linked_list, VAddr start, VAddr end) {
    auto node{page_linked_list.Nodes().begin()};
    PAddr map_addr{node->GetAddress()};
    std::size_t src_num_pages{node->GetNumPages()};

    block_manager->IterateForRange(start, end, [&](const KMemoryInfo& info) {
        if (info.state != KMemoryState::Free) {
            return;
        }

        std::size_t dst_num_pages{GetSizeInRange(info, start, end) / PageSize};
        VAddr dst_addr{GetAddressInRange(info, start)};

        while (dst_num_pages) {
            if (!src_num_pages) {
                node = std::next(node);
                map_addr = node->GetAddress();
                src_num_pages = node->GetNumPages();
            }

            const std::size_t num_pages{std::min(src_num_pages, dst_num_pages)};
            Operate(dst_addr, num_pages, KMemoryPermission::UserReadWrite, OperationType::Map,
                    map_addr);

            dst_addr += num_pages * PageSize;
            map_addr += num_pages * PageSize;
            src_num_pages -= num_pages;
            dst_num_pages -= num_pages;
        }
    });
}

ResultCode KPageTable::MapPhysicalMemory(VAddr addr, std::size_t size) {
    std::lock_guard lock{page_table_lock};

    std::size_t mapped_size{};
    const VAddr end_addr{addr + size};

    block_manager->IterateForRange(addr, end_addr, [&](const KMemoryInfo& info) {
        if (info.state != KMemoryState::Free) {
            mapped_size += GetSizeInRange(info, addr, end_addr);
        }
    });

    if (mapped_size == size) {
        return ResultSuccess;
    }

    const std::size_t remaining_size{size - mapped_size};
    const std::size_t remaining_pages{remaining_size / PageSize};

    // Reserve the memory from the process resource limit.
    KScopedResourceReservation memory_reservation(
        system.Kernel().CurrentProcess()->GetResourceLimit(), LimitableResource::PhysicalMemory,
        remaining_size);
    if (!memory_reservation.Succeeded()) {
        LOG_ERROR(Kernel, "Could not reserve remaining {:X} bytes", remaining_size);
        return ResultLimitReached;
    }

    KPageLinkedList page_linked_list;

    CASCADE_CODE(system.Kernel().MemoryManager().Allocate(page_linked_list, remaining_pages,
                                                          memory_pool, allocation_option));

    // We succeeded, so commit the memory reservation.
    memory_reservation.Commit();

    MapPhysicalMemory(page_linked_list, addr, end_addr);

    mapped_physical_memory_size += remaining_size;

    const std::size_t num_pages{size / PageSize};
    block_manager->Update(addr, num_pages, KMemoryState::Free, KMemoryPermission::None,
                          KMemoryAttribute::None, KMemoryState::Normal,
                          KMemoryPermission::UserReadWrite, KMemoryAttribute::None);

    return ResultSuccess;
}

ResultCode KPageTable::UnmapPhysicalMemory(VAddr addr, std::size_t size) {
    std::lock_guard lock{page_table_lock};

    const VAddr end_addr{addr + size};
    ResultCode result{ResultSuccess};
    std::size_t mapped_size{};

    // Verify that the region can be unmapped
    block_manager->IterateForRange(addr, end_addr, [&](const KMemoryInfo& info) {
        if (info.state == KMemoryState::Normal) {
            if (info.attribute != KMemoryAttribute::None) {
                result = ResultInvalidCurrentMemory;
                return;
            }
            mapped_size += GetSizeInRange(info, addr, end_addr);
        } else if (info.state != KMemoryState::Free) {
            result = ResultInvalidCurrentMemory;
        }
    });

    if (result.IsError()) {
        return result;
    }

    if (!mapped_size) {
        return ResultSuccess;
    }

    CASCADE_CODE(UnmapMemory(addr, size));

    auto process{system.Kernel().CurrentProcess()};
    process->GetResourceLimit()->Release(LimitableResource::PhysicalMemory, mapped_size);
    mapped_physical_memory_size -= mapped_size;

    return ResultSuccess;
}

ResultCode KPageTable::UnmapMemory(VAddr addr, std::size_t size) {
    std::lock_guard lock{page_table_lock};

    const VAddr end_addr{addr + size};
    ResultCode result{ResultSuccess};
    KPageLinkedList page_linked_list;

    // Unmap each region within the range
    block_manager->IterateForRange(addr, end_addr, [&](const KMemoryInfo& info) {
        if (info.state == KMemoryState::Normal) {
            const std::size_t block_size{GetSizeInRange(info, addr, end_addr)};
            const std::size_t block_num_pages{block_size / PageSize};
            const VAddr block_addr{GetAddressInRange(info, addr)};

            AddRegionToPages(block_addr, block_size / PageSize, page_linked_list);

            if (result = Operate(block_addr, block_num_pages, KMemoryPermission::None,
                                 OperationType::Unmap);
                result.IsError()) {
                return;
            }
        }
    });

    if (result.IsError()) {
        return result;
    }

    const std::size_t num_pages{size / PageSize};
    system.Kernel().MemoryManager().Free(page_linked_list, num_pages, memory_pool,
                                         allocation_option);

    block_manager->Update(addr, num_pages, KMemoryState::Free);

    return ResultSuccess;
}

ResultCode KPageTable::Map(VAddr dst_addr, VAddr src_addr, std::size_t size) {
    std::lock_guard lock{page_table_lock};

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

ResultCode KPageTable::Unmap(VAddr dst_addr, VAddr src_addr, std::size_t size) {
    std::lock_guard lock{page_table_lock};

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

ResultCode KPageTable::MapPages(VAddr addr, KPageLinkedList& page_linked_list, KMemoryState state,
                                KMemoryPermission perm) {
    std::lock_guard lock{page_table_lock};

    const std::size_t num_pages{page_linked_list.GetNumPages()};
    const std::size_t size{num_pages * PageSize};

    if (!CanContain(addr, size, state)) {
        return ResultInvalidCurrentMemory;
    }

    if (IsRegionMapped(addr, num_pages * PageSize)) {
        return ResultInvalidCurrentMemory;
    }

    CASCADE_CODE(MapPages(addr, page_linked_list, perm));

    block_manager->Update(addr, num_pages, state, perm);

    return ResultSuccess;
}

ResultCode KPageTable::UnmapPages(VAddr addr, const KPageLinkedList& page_linked_list) {
    VAddr cur_addr{addr};

    for (const auto& node : page_linked_list.Nodes()) {
        const std::size_t num_pages{(addr - cur_addr) / PageSize};
        if (const auto result{
                Operate(addr, num_pages, KMemoryPermission::None, OperationType::Unmap)};
            result.IsError()) {
            return result;
        }

        cur_addr += node.GetNumPages() * PageSize;
    }

    return ResultSuccess;
}

ResultCode KPageTable::UnmapPages(VAddr addr, KPageLinkedList& page_linked_list,
                                  KMemoryState state) {
    std::lock_guard lock{page_table_lock};

    const std::size_t num_pages{page_linked_list.GetNumPages()};
    const std::size_t size{num_pages * PageSize};

    if (!CanContain(addr, size, state)) {
        return ResultInvalidCurrentMemory;
    }

    if (IsRegionMapped(addr, num_pages * PageSize)) {
        return ResultInvalidCurrentMemory;
    }

    CASCADE_CODE(UnmapPages(addr, page_linked_list));

    block_manager->Update(addr, num_pages, state, KMemoryPermission::None);

    return ResultSuccess;
}

ResultCode KPageTable::SetProcessMemoryPermission(VAddr addr, std::size_t size,
                                                  Svc::MemoryPermission svc_perm) {
    const size_t num_pages = size / PageSize;

    // Lock the table.
    std::lock_guard lock{page_table_lock};

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
            UNREACHABLE();
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
        // Memory execution state is changing, invalidate CPU cache range
        system.InvalidateCpuInstructionCacheRange(addr, size);
    }

    return ResultSuccess;
}

KMemoryInfo KPageTable::QueryInfoImpl(VAddr addr) {
    std::lock_guard lock{page_table_lock};

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
    std::lock_guard lock{page_table_lock};

    KMemoryState state{};
    KMemoryAttribute attribute{};

    CASCADE_CODE(CheckMemoryState(
        &state, nullptr, &attribute, nullptr, addr, size,
        KMemoryState::FlagCanTransfer | KMemoryState::FlagReferenceCounted,
        KMemoryState::FlagCanTransfer | KMemoryState::FlagReferenceCounted, KMemoryPermission::All,
        KMemoryPermission::UserReadWrite, KMemoryAttribute::Mask, KMemoryAttribute::None,
        KMemoryAttribute::IpcAndDeviceMapped));

    block_manager->Update(addr, size / PageSize, state, perm, attribute | KMemoryAttribute::Locked);

    return ResultSuccess;
}

ResultCode KPageTable::ResetTransferMemory(VAddr addr, std::size_t size) {
    std::lock_guard lock{page_table_lock};

    KMemoryState state{};

    CASCADE_CODE(
        CheckMemoryState(&state, nullptr, nullptr, nullptr, addr, size,
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
    std::lock_guard lock{page_table_lock};

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
    std::lock_guard lock{page_table_lock};

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
    const auto new_attr = ((old_attr & static_cast<KMemoryAttribute>(~mask)) |
                           static_cast<KMemoryAttribute>(attr & mask));

    // Perform operation.
    this->Operate(addr, num_pages, old_perm, OperationType::ChangePermissionsAndRefresh);

    // Update the blocks.
    block_manager->Update(addr, num_pages, old_state, old_perm, new_attr);

    return ResultSuccess;
}

ResultCode KPageTable::SetMaxHeapSize(std::size_t size) {
    // Lock the table.
    std::lock_guard lock{page_table_lock};

    // Only process page tables are allowed to set heap size.
    ASSERT(!this->IsKernel());

    max_heap_size = size;

    return ResultSuccess;
}

ResultCode KPageTable::SetHeapSize(VAddr* out, std::size_t size) {
    // Try to perform a reduction in heap, instead of an extension.
    VAddr cur_address{};
    std::size_t allocation_size{};
    {
        // Lock the table.
        std::lock_guard lk(page_table_lock);

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
    KPageLinkedList page_linked_list;
    R_TRY(system.Kernel().MemoryManager().Allocate(page_linked_list, allocation_size / PageSize,
                                                   memory_pool, allocation_option));

    // Map the pages.
    {
        // Lock the table.
        std::lock_guard lk(page_table_lock);

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
        R_TRY(Operate(current_heap_end, num_pages, page_linked_list, OperationType::MapGroup));

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
    std::lock_guard lock{page_table_lock};

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
        CASCADE_CODE(Operate(addr, needed_num_pages, perm, OperationType::Map, map_addr));
    } else {
        KPageLinkedList page_group;
        CASCADE_CODE(system.Kernel().MemoryManager().Allocate(page_group, needed_num_pages,
                                                              memory_pool, allocation_option));
        CASCADE_CODE(Operate(addr, needed_num_pages, page_group, OperationType::MapGroup));
    }

    block_manager->Update(addr, needed_num_pages, state, perm);

    return addr;
}

ResultCode KPageTable::LockForDeviceAddressSpace(VAddr addr, std::size_t size) {
    std::lock_guard lock{page_table_lock};

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
    std::lock_guard lock{page_table_lock};

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

ResultCode KPageTable::LockForCodeMemory(VAddr addr, std::size_t size) {
    std::lock_guard lock{page_table_lock};

    KMemoryPermission new_perm = KMemoryPermission::NotMapped | KMemoryPermission::KernelReadWrite;

    KMemoryPermission old_perm{};

    if (const ResultCode result{CheckMemoryState(
            nullptr, &old_perm, nullptr, nullptr, addr, size, KMemoryState::FlagCanCodeMemory,
            KMemoryState::FlagCanCodeMemory, KMemoryPermission::All,
            KMemoryPermission::UserReadWrite, KMemoryAttribute::All, KMemoryAttribute::None)};
        result.IsError()) {
        return result;
    }

    new_perm = (new_perm != KMemoryPermission::None) ? new_perm : old_perm;

    block_manager->UpdateLock(
        addr, size / PageSize,
        [](KMemoryBlockManager::iterator block, KMemoryPermission permission) {
            block->ShareToDevice(permission);
        },
        new_perm);

    return ResultSuccess;
}

ResultCode KPageTable::UnlockForCodeMemory(VAddr addr, std::size_t size) {
    std::lock_guard lock{page_table_lock};

    KMemoryPermission new_perm = KMemoryPermission::UserReadWrite;

    KMemoryPermission old_perm{};

    if (const ResultCode result{CheckMemoryState(
            nullptr, &old_perm, nullptr, nullptr, addr, size, KMemoryState::FlagCanCodeMemory,
            KMemoryState::FlagCanCodeMemory, KMemoryPermission::None, KMemoryPermission::None,
            KMemoryAttribute::All, KMemoryAttribute::Locked)};
        result.IsError()) {
        return result;
    }

    new_perm = (new_perm != KMemoryPermission::None) ? new_perm : old_perm;

    block_manager->UpdateLock(
        addr, size / PageSize,
        [](KMemoryBlockManager::iterator block, KMemoryPermission permission) {
            block->UnshareToDevice(permission);
        },
        new_perm);

    return ResultSuccess;
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
        if (!paddr) {
            UNREACHABLE();
        }
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
    std::lock_guard lock{page_table_lock};

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
            UNREACHABLE();
        }

        addr += size;
    }

    return ResultSuccess;
}

ResultCode KPageTable::Operate(VAddr addr, std::size_t num_pages, KMemoryPermission perm,
                               OperationType operation, PAddr map_addr) {
    std::lock_guard lock{page_table_lock};

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
        UNREACHABLE();
    }
    return ResultSuccess;
}

constexpr VAddr KPageTable::GetRegionAddress(KMemoryState state) const {
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
        return {};
    }
}

constexpr std::size_t KPageTable::GetRegionSize(KMemoryState state) const {
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
        return {};
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

} // namespace Kernel

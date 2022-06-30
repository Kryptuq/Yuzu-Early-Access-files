// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/page_table.h"
#include "core/file_sys/program_metadata.h"
#include "core/hle/kernel/k_light_lock.h"
#include "core/hle/kernel/k_memory_block.h"
#include "core/hle/kernel/k_memory_layout.h"
#include "core/hle/kernel/k_memory_manager.h"
#include "core/hle/result.h"

namespace Core {
class System;
}

namespace Kernel {

class KMemoryBlockManager;

class KPageTable final {
public:
    enum class ICacheInvalidationStrategy : u32 { InvalidateRange, InvalidateAll };

    YUZU_NON_COPYABLE(KPageTable);
    YUZU_NON_MOVEABLE(KPageTable);

    explicit KPageTable(Core::System& system_);
    ~KPageTable();

    Result InitializeForProcess(FileSys::ProgramAddressSpaceType as_type, bool enable_aslr,
                                VAddr code_addr, std::size_t code_size, KMemoryManager::Pool pool);
    Result MapProcessCode(VAddr addr, std::size_t pages_count, KMemoryState state,
                          KMemoryPermission perm);
    Result MapCodeMemory(VAddr dst_address, VAddr src_address, std::size_t size);
    Result UnmapCodeMemory(VAddr dst_address, VAddr src_address, std::size_t size,
                           ICacheInvalidationStrategy icache_invalidation_strategy);
    Result UnmapProcessMemory(VAddr dst_addr, std::size_t size, KPageTable& src_page_table,
                              VAddr src_addr);
    Result MapPhysicalMemory(VAddr addr, std::size_t size);
    Result UnmapPhysicalMemory(VAddr addr, std::size_t size);
    Result MapMemory(VAddr dst_addr, VAddr src_addr, std::size_t size);
    Result UnmapMemory(VAddr dst_addr, VAddr src_addr, std::size_t size);
    Result MapPages(VAddr addr, KPageGroup& page_linked_list, KMemoryState state,
                    KMemoryPermission perm);
    Result MapPages(VAddr* out_addr, std::size_t num_pages, std::size_t alignment, PAddr phys_addr,
                    KMemoryState state, KMemoryPermission perm) {
        return this->MapPages(out_addr, num_pages, alignment, phys_addr, true,
                              this->GetRegionAddress(state), this->GetRegionSize(state) / PageSize,
                              state, perm);
    }
    Result UnmapPages(VAddr addr, KPageGroup& page_linked_list, KMemoryState state);
    Result UnmapPages(VAddr address, std::size_t num_pages, KMemoryState state);
    Result SetProcessMemoryPermission(VAddr addr, std::size_t size, Svc::MemoryPermission svc_perm);
    KMemoryInfo QueryInfo(VAddr addr);
    Result ReserveTransferMemory(VAddr addr, std::size_t size, KMemoryPermission perm);
    Result ResetTransferMemory(VAddr addr, std::size_t size);
    Result SetMemoryPermission(VAddr addr, std::size_t size, Svc::MemoryPermission perm);
    Result SetMemoryAttribute(VAddr addr, std::size_t size, u32 mask, u32 attr);
    Result SetMaxHeapSize(std::size_t size);
    Result SetHeapSize(VAddr* out, std::size_t size);
    ResultVal<VAddr> AllocateAndMapMemory(std::size_t needed_num_pages, std::size_t align,
                                          bool is_map_only, VAddr region_start,
                                          std::size_t region_num_pages, KMemoryState state,
                                          KMemoryPermission perm, PAddr map_addr = 0);
    Result LockForDeviceAddressSpace(VAddr addr, std::size_t size);
    Result UnlockForDeviceAddressSpace(VAddr addr, std::size_t size);
    Result LockForCodeMemory(KPageGroup* out, VAddr addr, std::size_t size);
    Result UnlockForCodeMemory(VAddr addr, std::size_t size, const KPageGroup& pg);
    Result MakeAndOpenPageGroup(KPageGroup* out, VAddr address, size_t num_pages,
                                KMemoryState state_mask, KMemoryState state,
                                KMemoryPermission perm_mask, KMemoryPermission perm,
                                KMemoryAttribute attr_mask, KMemoryAttribute attr);

    Common::PageTable& PageTableImpl() {
        return page_table_impl;
    }

    const Common::PageTable& PageTableImpl() const {
        return page_table_impl;
    }

    bool CanContain(VAddr addr, std::size_t size, KMemoryState state) const;

private:
    enum class OperationType : u32 {
        Map,
        MapGroup,
        Unmap,
        ChangePermissions,
        ChangePermissionsAndRefresh,
    };

    static constexpr KMemoryAttribute DefaultMemoryIgnoreAttr = KMemoryAttribute::DontCareMask |
                                                                KMemoryAttribute::IpcLocked |
                                                                KMemoryAttribute::DeviceShared;

    Result InitializeMemoryLayout(VAddr start, VAddr end);
    Result MapPages(VAddr addr, const KPageGroup& page_linked_list, KMemoryPermission perm);
    Result MapPages(VAddr* out_addr, std::size_t num_pages, std::size_t alignment, PAddr phys_addr,
                    bool is_pa_valid, VAddr region_start, std::size_t region_num_pages,
                    KMemoryState state, KMemoryPermission perm);
    Result UnmapPages(VAddr addr, const KPageGroup& page_linked_list);
    bool IsRegionMapped(VAddr address, u64 size);
    bool IsRegionContiguous(VAddr addr, u64 size) const;
    void AddRegionToPages(VAddr start, std::size_t num_pages, KPageGroup& page_linked_list);
    KMemoryInfo QueryInfoImpl(VAddr addr);
    VAddr AllocateVirtualMemory(VAddr start, std::size_t region_num_pages, u64 needed_num_pages,
                                std::size_t align);
    Result Operate(VAddr addr, std::size_t num_pages, const KPageGroup& page_group,
                   OperationType operation);
    Result Operate(VAddr addr, std::size_t num_pages, KMemoryPermission perm,
                   OperationType operation, PAddr map_addr = 0);
    VAddr GetRegionAddress(KMemoryState state) const;
    std::size_t GetRegionSize(KMemoryState state) const;

    VAddr FindFreeArea(VAddr region_start, std::size_t region_num_pages, std::size_t num_pages,
                       std::size_t alignment, std::size_t offset, std::size_t guard_pages);

    Result CheckMemoryStateContiguous(std::size_t* out_blocks_needed, VAddr addr, std::size_t size,
                                      KMemoryState state_mask, KMemoryState state,
                                      KMemoryPermission perm_mask, KMemoryPermission perm,
                                      KMemoryAttribute attr_mask, KMemoryAttribute attr) const;
    Result CheckMemoryStateContiguous(VAddr addr, std::size_t size, KMemoryState state_mask,
                                      KMemoryState state, KMemoryPermission perm_mask,
                                      KMemoryPermission perm, KMemoryAttribute attr_mask,
                                      KMemoryAttribute attr) const {
        return this->CheckMemoryStateContiguous(nullptr, addr, size, state_mask, state, perm_mask,
                                                perm, attr_mask, attr);
    }

    Result CheckMemoryState(const KMemoryInfo& info, KMemoryState state_mask, KMemoryState state,
                            KMemoryPermission perm_mask, KMemoryPermission perm,
                            KMemoryAttribute attr_mask, KMemoryAttribute attr) const;
    Result CheckMemoryState(KMemoryState* out_state, KMemoryPermission* out_perm,
                            KMemoryAttribute* out_attr, std::size_t* out_blocks_needed, VAddr addr,
                            std::size_t size, KMemoryState state_mask, KMemoryState state,
                            KMemoryPermission perm_mask, KMemoryPermission perm,
                            KMemoryAttribute attr_mask, KMemoryAttribute attr,
                            KMemoryAttribute ignore_attr = DefaultMemoryIgnoreAttr) const;
    Result CheckMemoryState(std::size_t* out_blocks_needed, VAddr addr, std::size_t size,
                            KMemoryState state_mask, KMemoryState state,
                            KMemoryPermission perm_mask, KMemoryPermission perm,
                            KMemoryAttribute attr_mask, KMemoryAttribute attr,
                            KMemoryAttribute ignore_attr = DefaultMemoryIgnoreAttr) const {
        return CheckMemoryState(nullptr, nullptr, nullptr, out_blocks_needed, addr, size,
                                state_mask, state, perm_mask, perm, attr_mask, attr, ignore_attr);
    }
    Result CheckMemoryState(VAddr addr, std::size_t size, KMemoryState state_mask,
                            KMemoryState state, KMemoryPermission perm_mask, KMemoryPermission perm,
                            KMemoryAttribute attr_mask, KMemoryAttribute attr,
                            KMemoryAttribute ignore_attr = DefaultMemoryIgnoreAttr) const {
        return this->CheckMemoryState(nullptr, addr, size, state_mask, state, perm_mask, perm,
                                      attr_mask, attr, ignore_attr);
    }

    Result LockMemoryAndOpen(KPageGroup* out_pg, PAddr* out_paddr, VAddr addr, size_t size,
                             KMemoryState state_mask, KMemoryState state,
                             KMemoryPermission perm_mask, KMemoryPermission perm,
                             KMemoryAttribute attr_mask, KMemoryAttribute attr,
                             KMemoryPermission new_perm, KMemoryAttribute lock_attr);
    Result UnlockMemory(VAddr addr, size_t size, KMemoryState state_mask, KMemoryState state,
                        KMemoryPermission perm_mask, KMemoryPermission perm,
                        KMemoryAttribute attr_mask, KMemoryAttribute attr,
                        KMemoryPermission new_perm, KMemoryAttribute lock_attr,
                        const KPageGroup* pg);

    Result MakePageGroup(KPageGroup& pg, VAddr addr, size_t num_pages);
    bool IsValidPageGroup(const KPageGroup& pg, VAddr addr, size_t num_pages);

    bool IsLockedByCurrentThread() const {
        return general_lock.IsLockedByCurrentThread();
    }

    bool IsHeapPhysicalAddress(const KMemoryLayout& layout, PAddr phys_addr) {
        ASSERT(this->IsLockedByCurrentThread());

        return layout.IsHeapPhysicalAddress(cached_physical_heap_region, phys_addr);
    }

    bool GetPhysicalAddressLocked(PAddr* out, VAddr virt_addr) const {
        ASSERT(this->IsLockedByCurrentThread());

        *out = GetPhysicalAddr(virt_addr);

        return *out != 0;
    }

    mutable KLightLock general_lock;
    mutable KLightLock map_physical_memory_lock;

    std::unique_ptr<KMemoryBlockManager> block_manager;

public:
    constexpr VAddr GetAddressSpaceStart() const {
        return address_space_start;
    }
    constexpr VAddr GetAddressSpaceEnd() const {
        return address_space_end;
    }
    constexpr std::size_t GetAddressSpaceSize() const {
        return address_space_end - address_space_start;
    }
    constexpr VAddr GetHeapRegionStart() const {
        return heap_region_start;
    }
    constexpr VAddr GetHeapRegionEnd() const {
        return heap_region_end;
    }
    constexpr std::size_t GetHeapRegionSize() const {
        return heap_region_end - heap_region_start;
    }
    constexpr VAddr GetAliasRegionStart() const {
        return alias_region_start;
    }
    constexpr VAddr GetAliasRegionEnd() const {
        return alias_region_end;
    }
    constexpr std::size_t GetAliasRegionSize() const {
        return alias_region_end - alias_region_start;
    }
    constexpr VAddr GetStackRegionStart() const {
        return stack_region_start;
    }
    constexpr VAddr GetStackRegionEnd() const {
        return stack_region_end;
    }
    constexpr std::size_t GetStackRegionSize() const {
        return stack_region_end - stack_region_start;
    }
    constexpr VAddr GetKernelMapRegionStart() const {
        return kernel_map_region_start;
    }
    constexpr VAddr GetKernelMapRegionEnd() const {
        return kernel_map_region_end;
    }
    constexpr VAddr GetCodeRegionStart() const {
        return code_region_start;
    }
    constexpr VAddr GetCodeRegionEnd() const {
        return code_region_end;
    }
    constexpr VAddr GetAliasCodeRegionStart() const {
        return alias_code_region_start;
    }
    constexpr VAddr GetAliasCodeRegionSize() const {
        return alias_code_region_end - alias_code_region_start;
    }
    std::size_t GetNormalMemorySize() {
        KScopedLightLock lk(general_lock);
        return GetHeapSize() + mapped_physical_memory_size;
    }
    constexpr std::size_t GetAddressSpaceWidth() const {
        return address_space_width;
    }
    constexpr std::size_t GetHeapSize() const {
        return current_heap_end - heap_region_start;
    }
    constexpr bool IsInsideAddressSpace(VAddr address, std::size_t size) const {
        return address_space_start <= address && address + size - 1 <= address_space_end - 1;
    }
    constexpr bool IsOutsideAliasRegion(VAddr address, std::size_t size) const {
        return alias_region_start > address || address + size - 1 > alias_region_end - 1;
    }
    constexpr bool IsOutsideStackRegion(VAddr address, std::size_t size) const {
        return stack_region_start > address || address + size - 1 > stack_region_end - 1;
    }
    constexpr bool IsInvalidRegion(VAddr address, std::size_t size) const {
        return address + size - 1 > GetAliasCodeRegionStart() + GetAliasCodeRegionSize() - 1;
    }
    constexpr bool IsInsideHeapRegion(VAddr address, std::size_t size) const {
        return address + size > heap_region_start && heap_region_end > address;
    }
    constexpr bool IsInsideAliasRegion(VAddr address, std::size_t size) const {
        return address + size > alias_region_start && alias_region_end > address;
    }
    constexpr bool IsOutsideASLRRegion(VAddr address, std::size_t size) const {
        if (IsInvalidRegion(address, size)) {
            return true;
        }
        if (IsInsideHeapRegion(address, size)) {
            return true;
        }
        if (IsInsideAliasRegion(address, size)) {
            return true;
        }
        return {};
    }
    constexpr bool IsInsideASLRRegion(VAddr address, std::size_t size) const {
        return !IsOutsideASLRRegion(address, size);
    }
    constexpr std::size_t GetNumGuardPages() const {
        return IsKernel() ? 1 : 4;
    }
    PAddr GetPhysicalAddr(VAddr addr) const {
        const auto backing_addr = page_table_impl.backing_addr[addr >> PageBits];
        ASSERT(backing_addr);
        return backing_addr + addr;
    }
    constexpr bool Contains(VAddr addr) const {
        return address_space_start <= addr && addr <= address_space_end - 1;
    }
    constexpr bool Contains(VAddr addr, std::size_t size) const {
        return address_space_start <= addr && addr < addr + size &&
               addr + size - 1 <= address_space_end - 1;
    }

private:
    constexpr bool IsKernel() const {
        return is_kernel;
    }
    constexpr bool IsAslrEnabled() const {
        return is_aslr_enabled;
    }

    constexpr bool ContainsPages(VAddr addr, std::size_t num_pages) const {
        return (address_space_start <= addr) &&
               (num_pages <= (address_space_end - address_space_start) / PageSize) &&
               (addr + num_pages * PageSize - 1 <= address_space_end - 1);
    }

private:
    VAddr address_space_start{};
    VAddr address_space_end{};
    VAddr heap_region_start{};
    VAddr heap_region_end{};
    VAddr current_heap_end{};
    VAddr alias_region_start{};
    VAddr alias_region_end{};
    VAddr stack_region_start{};
    VAddr stack_region_end{};
    VAddr kernel_map_region_start{};
    VAddr kernel_map_region_end{};
    VAddr code_region_start{};
    VAddr code_region_end{};
    VAddr alias_code_region_start{};
    VAddr alias_code_region_end{};

    std::size_t mapped_physical_memory_size{};
    std::size_t max_heap_size{};
    std::size_t max_physical_memory_size{};
    std::size_t address_space_width{};

    bool is_kernel{};
    bool is_aslr_enabled{};

    u32 heap_fill_value{};
    const KMemoryRegion* cached_physical_heap_region{};

    KMemoryManager::Pool memory_pool{KMemoryManager::Pool::Application};
    KMemoryManager::Direction allocation_option{KMemoryManager::Direction::FromFront};

    Common::PageTable page_table_impl;

    Core::System& system;
};

} // namespace Kernel

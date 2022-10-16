// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/page_table.h"
#include "core/file_sys/program_metadata.h"
#include "core/hle/kernel/k_dynamic_resource_manager.h"
#include "core/hle/kernel/k_light_lock.h"
#include "core/hle/kernel/k_memory_block.h"
#include "core/hle/kernel/k_memory_block_manager.h"
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
                                VAddr code_addr, size_t code_size,
                                KMemoryBlockSlabManager* mem_block_slab_manager,
                                KMemoryManager::Pool pool);

    void Finalize();

    Result MapProcessCode(VAddr addr, size_t pages_count, KMemoryState state,
                          KMemoryPermission perm);
    Result MapCodeMemory(VAddr dst_address, VAddr src_address, size_t size);
    Result UnmapCodeMemory(VAddr dst_address, VAddr src_address, size_t size,
                           ICacheInvalidationStrategy icache_invalidation_strategy);
    Result UnmapProcessMemory(VAddr dst_addr, size_t size, KPageTable& src_page_table,
                              VAddr src_addr);
    Result MapPhysicalMemory(VAddr addr, size_t size);
    Result UnmapPhysicalMemory(VAddr addr, size_t size);
    Result MapMemory(VAddr dst_addr, VAddr src_addr, size_t size);
    Result UnmapMemory(VAddr dst_addr, VAddr src_addr, size_t size);
    Result MapPages(VAddr addr, KPageGroup& page_linked_list, KMemoryState state,
                    KMemoryPermission perm);
    Result MapPages(VAddr* out_addr, size_t num_pages, size_t alignment, PAddr phys_addr,
                    KMemoryState state, KMemoryPermission perm) {
        R_RETURN(this->MapPages(out_addr, num_pages, alignment, phys_addr, true,
                                this->GetRegionAddress(state),
                                this->GetRegionSize(state) / PageSize, state, perm));
    }
    Result UnmapPages(VAddr addr, KPageGroup& page_linked_list, KMemoryState state);
    Result UnmapPages(VAddr address, size_t num_pages, KMemoryState state);
    Result SetProcessMemoryPermission(VAddr addr, size_t size, Svc::MemoryPermission svc_perm);
    KMemoryInfo QueryInfo(VAddr addr);
    Result SetMemoryPermission(VAddr addr, size_t size, Svc::MemoryPermission perm);
    Result SetMemoryAttribute(VAddr addr, size_t size, u32 mask, u32 attr);
    Result SetMaxHeapSize(size_t size);
    Result SetHeapSize(VAddr* out, size_t size);
    ResultVal<VAddr> AllocateAndMapMemory(size_t needed_num_pages, size_t align, bool is_map_only,
                                          VAddr region_start, size_t region_num_pages,
                                          KMemoryState state, KMemoryPermission perm,
                                          PAddr map_addr = 0);

    Result LockForMapDeviceAddressSpace(VAddr address, size_t size, KMemoryPermission perm,
                                        bool is_aligned);
    Result LockForUnmapDeviceAddressSpace(VAddr address, size_t size);

    Result UnlockForDeviceAddressSpace(VAddr addr, size_t size);

    Result LockForCodeMemory(KPageGroup* out, VAddr addr, size_t size);
    Result UnlockForCodeMemory(VAddr addr, size_t size, const KPageGroup& pg);
    Result MakeAndOpenPageGroup(KPageGroup* out, VAddr address, size_t num_pages,
                                KMemoryState state_mask, KMemoryState state,
                                KMemoryPermission perm_mask, KMemoryPermission perm,
                                KMemoryAttribute attr_mask, KMemoryAttribute attr);

    Common::PageTable& PageTableImpl() {
        return *m_page_table_impl;
    }

    const Common::PageTable& PageTableImpl() const {
        return *m_page_table_impl;
    }

    bool CanContain(VAddr addr, size_t size, KMemoryState state) const;

private:
    enum class OperationType : u32 {
        Map,
        MapGroup,
        Unmap,
        ChangePermissions,
        ChangePermissionsAndRefresh,
    };

    static constexpr KMemoryAttribute DefaultMemoryIgnoreAttr =
        KMemoryAttribute::IpcLocked | KMemoryAttribute::DeviceShared;

    Result MapPages(VAddr addr, const KPageGroup& page_linked_list, KMemoryPermission perm);
    Result MapPages(VAddr* out_addr, size_t num_pages, size_t alignment, PAddr phys_addr,
                    bool is_pa_valid, VAddr region_start, size_t region_num_pages,
                    KMemoryState state, KMemoryPermission perm);
    Result UnmapPages(VAddr addr, const KPageGroup& page_linked_list);
    bool IsRegionContiguous(VAddr addr, u64 size) const;
    void AddRegionToPages(VAddr start, size_t num_pages, KPageGroup& page_linked_list);
    KMemoryInfo QueryInfoImpl(VAddr addr);
    VAddr AllocateVirtualMemory(VAddr start, size_t region_num_pages, u64 needed_num_pages,
                                size_t align);
    Result Operate(VAddr addr, size_t num_pages, const KPageGroup& page_group,
                   OperationType operation);
    Result Operate(VAddr addr, size_t num_pages, KMemoryPermission perm, OperationType operation,
                   PAddr map_addr = 0);
    VAddr GetRegionAddress(KMemoryState state) const;
    size_t GetRegionSize(KMemoryState state) const;

    VAddr FindFreeArea(VAddr region_start, size_t region_num_pages, size_t num_pages,
                       size_t alignment, size_t offset, size_t guard_pages);

    Result CheckMemoryStateContiguous(size_t* out_blocks_needed, VAddr addr, size_t size,
                                      KMemoryState state_mask, KMemoryState state,
                                      KMemoryPermission perm_mask, KMemoryPermission perm,
                                      KMemoryAttribute attr_mask, KMemoryAttribute attr) const;
    Result CheckMemoryStateContiguous(VAddr addr, size_t size, KMemoryState state_mask,
                                      KMemoryState state, KMemoryPermission perm_mask,
                                      KMemoryPermission perm, KMemoryAttribute attr_mask,
                                      KMemoryAttribute attr) const {
        R_RETURN(this->CheckMemoryStateContiguous(nullptr, addr, size, state_mask, state, perm_mask,
                                                  perm, attr_mask, attr));
    }

    Result CheckMemoryState(const KMemoryInfo& info, KMemoryState state_mask, KMemoryState state,
                            KMemoryPermission perm_mask, KMemoryPermission perm,
                            KMemoryAttribute attr_mask, KMemoryAttribute attr) const;
    Result CheckMemoryState(KMemoryState* out_state, KMemoryPermission* out_perm,
                            KMemoryAttribute* out_attr, size_t* out_blocks_needed, VAddr addr,
                            size_t size, KMemoryState state_mask, KMemoryState state,
                            KMemoryPermission perm_mask, KMemoryPermission perm,
                            KMemoryAttribute attr_mask, KMemoryAttribute attr,
                            KMemoryAttribute ignore_attr = DefaultMemoryIgnoreAttr) const;
    Result CheckMemoryState(size_t* out_blocks_needed, VAddr addr, size_t size,
                            KMemoryState state_mask, KMemoryState state,
                            KMemoryPermission perm_mask, KMemoryPermission perm,
                            KMemoryAttribute attr_mask, KMemoryAttribute attr,
                            KMemoryAttribute ignore_attr = DefaultMemoryIgnoreAttr) const {
        R_RETURN(CheckMemoryState(nullptr, nullptr, nullptr, out_blocks_needed, addr, size,
                                  state_mask, state, perm_mask, perm, attr_mask, attr,
                                  ignore_attr));
    }
    Result CheckMemoryState(VAddr addr, size_t size, KMemoryState state_mask, KMemoryState state,
                            KMemoryPermission perm_mask, KMemoryPermission perm,
                            KMemoryAttribute attr_mask, KMemoryAttribute attr,
                            KMemoryAttribute ignore_attr = DefaultMemoryIgnoreAttr) const {
        R_RETURN(this->CheckMemoryState(nullptr, addr, size, state_mask, state, perm_mask, perm,
                                        attr_mask, attr, ignore_attr));
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
        return m_general_lock.IsLockedByCurrentThread();
    }

    bool IsHeapPhysicalAddress(const KMemoryLayout& layout, PAddr phys_addr) {
        ASSERT(this->IsLockedByCurrentThread());

        return layout.IsHeapPhysicalAddress(m_cached_physical_heap_region, phys_addr);
    }

    bool GetPhysicalAddressLocked(PAddr* out, VAddr virt_addr) const {
        ASSERT(this->IsLockedByCurrentThread());

        *out = GetPhysicalAddr(virt_addr);

        return *out != 0;
    }

    mutable KLightLock m_general_lock;
    mutable KLightLock m_map_physical_memory_lock;

public:
    constexpr VAddr GetAddressSpaceStart() const {
        return m_address_space_start;
    }
    constexpr VAddr GetAddressSpaceEnd() const {
        return m_address_space_end;
    }
    constexpr size_t GetAddressSpaceSize() const {
        return m_address_space_end - m_address_space_start;
    }
    constexpr VAddr GetHeapRegionStart() const {
        return m_heap_region_start;
    }
    constexpr VAddr GetHeapRegionEnd() const {
        return m_heap_region_end;
    }
    constexpr size_t GetHeapRegionSize() const {
        return m_heap_region_end - m_heap_region_start;
    }
    constexpr VAddr GetAliasRegionStart() const {
        return m_alias_region_start;
    }
    constexpr VAddr GetAliasRegionEnd() const {
        return m_alias_region_end;
    }
    constexpr size_t GetAliasRegionSize() const {
        return m_alias_region_end - m_alias_region_start;
    }
    constexpr VAddr GetStackRegionStart() const {
        return m_stack_region_start;
    }
    constexpr VAddr GetStackRegionEnd() const {
        return m_stack_region_end;
    }
    constexpr size_t GetStackRegionSize() const {
        return m_stack_region_end - m_stack_region_start;
    }
    constexpr VAddr GetKernelMapRegionStart() const {
        return m_kernel_map_region_start;
    }
    constexpr VAddr GetKernelMapRegionEnd() const {
        return m_kernel_map_region_end;
    }
    constexpr VAddr GetCodeRegionStart() const {
        return m_code_region_start;
    }
    constexpr VAddr GetCodeRegionEnd() const {
        return m_code_region_end;
    }
    constexpr VAddr GetAliasCodeRegionStart() const {
        return m_alias_code_region_start;
    }
    constexpr VAddr GetAliasCodeRegionSize() const {
        return m_alias_code_region_end - m_alias_code_region_start;
    }
    size_t GetNormalMemorySize() {
        KScopedLightLock lk(m_general_lock);
        return GetHeapSize() + m_mapped_physical_memory_size;
    }
    constexpr size_t GetAddressSpaceWidth() const {
        return m_address_space_width;
    }
    constexpr size_t GetHeapSize() const {
        return m_current_heap_end - m_heap_region_start;
    }
    constexpr bool IsInsideAddressSpace(VAddr address, size_t size) const {
        return m_address_space_start <= address && address + size - 1 <= m_address_space_end - 1;
    }
    constexpr bool IsOutsideAliasRegion(VAddr address, size_t size) const {
        return m_alias_region_start > address || address + size - 1 > m_alias_region_end - 1;
    }
    constexpr bool IsOutsideStackRegion(VAddr address, size_t size) const {
        return m_stack_region_start > address || address + size - 1 > m_stack_region_end - 1;
    }
    constexpr bool IsInvalidRegion(VAddr address, size_t size) const {
        return address + size - 1 > GetAliasCodeRegionStart() + GetAliasCodeRegionSize() - 1;
    }
    constexpr bool IsInsideHeapRegion(VAddr address, size_t size) const {
        return address + size > m_heap_region_start && m_heap_region_end > address;
    }
    constexpr bool IsInsideAliasRegion(VAddr address, size_t size) const {
        return address + size > m_alias_region_start && m_alias_region_end > address;
    }
    constexpr bool IsOutsideASLRRegion(VAddr address, size_t size) const {
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
    constexpr bool IsInsideASLRRegion(VAddr address, size_t size) const {
        return !IsOutsideASLRRegion(address, size);
    }
    constexpr size_t GetNumGuardPages() const {
        return IsKernel() ? 1 : 4;
    }
    PAddr GetPhysicalAddr(VAddr addr) const {
        const auto backing_addr = m_page_table_impl->backing_addr[addr >> PageBits];
        ASSERT(backing_addr);
        return backing_addr + addr;
    }
    constexpr bool Contains(VAddr addr) const {
        return m_address_space_start <= addr && addr <= m_address_space_end - 1;
    }
    constexpr bool Contains(VAddr addr, size_t size) const {
        return m_address_space_start <= addr && addr < addr + size &&
               addr + size - 1 <= m_address_space_end - 1;
    }

private:
    constexpr bool IsKernel() const {
        return m_is_kernel;
    }
    constexpr bool IsAslrEnabled() const {
        return m_enable_aslr;
    }

    constexpr bool ContainsPages(VAddr addr, size_t num_pages) const {
        return (m_address_space_start <= addr) &&
               (num_pages <= (m_address_space_end - m_address_space_start) / PageSize) &&
               (addr + num_pages * PageSize - 1 <= m_address_space_end - 1);
    }

private:
    VAddr m_address_space_start{};
    VAddr m_address_space_end{};
    VAddr m_heap_region_start{};
    VAddr m_heap_region_end{};
    VAddr m_current_heap_end{};
    VAddr m_alias_region_start{};
    VAddr m_alias_region_end{};
    VAddr m_stack_region_start{};
    VAddr m_stack_region_end{};
    VAddr m_kernel_map_region_start{};
    VAddr m_kernel_map_region_end{};
    VAddr m_code_region_start{};
    VAddr m_code_region_end{};
    VAddr m_alias_code_region_start{};
    VAddr m_alias_code_region_end{};

    size_t m_mapped_physical_memory_size{};
    size_t m_max_heap_size{};
    size_t m_max_physical_memory_size{};
    size_t m_address_space_width{};

    KMemoryBlockManager m_memory_block_manager;

    bool m_is_kernel{};
    bool m_enable_aslr{};
    bool m_enable_device_address_space_merge{};

    KMemoryBlockSlabManager* m_memory_block_slab_manager{};

    u32 m_heap_fill_value{};
    const KMemoryRegion* m_cached_physical_heap_region{};

    KMemoryManager::Pool m_memory_pool{KMemoryManager::Pool::Application};
    KMemoryManager::Direction m_allocation_option{KMemoryManager::Direction::FromFront};

    std::unique_ptr<Common::PageTable> m_page_table_impl;

    Core::System& m_system;
};

} // namespace Kernel

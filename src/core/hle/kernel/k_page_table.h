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
#include "core/memory.h"

namespace Core {
class System;
}

namespace Kernel {

enum class DisableMergeAttribute : u8 {
    None = (0U << 0),
    DisableHead = (1U << 0),
    DisableHeadAndBody = (1U << 1),
    EnableHeadAndBody = (1U << 2),
    DisableTail = (1U << 3),
    EnableTail = (1U << 4),
    EnableAndMergeHeadBodyTail = (1U << 5),
    EnableHeadBodyTail = EnableHeadAndBody | EnableTail,
    DisableHeadBodyTail = DisableHeadAndBody | DisableTail,
};

struct KPageProperties {
    KMemoryPermission perm;
    bool io;
    bool uncached;
    DisableMergeAttribute disable_merge_attributes;
};
static_assert(std::is_trivial_v<KPageProperties>);
static_assert(sizeof(KPageProperties) == sizeof(u32));

class KBlockInfoManager;
class KMemoryBlockManager;
class KResourceLimit;
class KSystemResource;

class KPageTable final {
protected:
    struct PageLinkedList;

public:
    enum class ICacheInvalidationStrategy : u32 { InvalidateRange, InvalidateAll };

    YUZU_NON_COPYABLE(KPageTable);
    YUZU_NON_MOVEABLE(KPageTable);

    explicit KPageTable(Core::System& system_);
    ~KPageTable();

    Result InitializeForProcess(FileSys::ProgramAddressSpaceType as_type, bool enable_aslr,
                                bool enable_das_merge, bool from_back, KMemoryManager::Pool pool,
                                VAddr code_addr, size_t code_size, KSystemResource* system_resource,
                                KResourceLimit* resource_limit);

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
    Result SetProcessMemoryPermission(VAddr addr, size_t size, Svc::MemoryPermission svc_perm);
    KMemoryInfo QueryInfo(VAddr addr);
    Result SetMemoryPermission(VAddr addr, size_t size, Svc::MemoryPermission perm);
    Result SetMemoryAttribute(VAddr addr, size_t size, u32 mask, u32 attr);
    Result SetMaxHeapSize(size_t size);
    Result SetHeapSize(VAddr* out, size_t size);
    Result LockForMapDeviceAddressSpace(bool* out_is_io, VAddr address, size_t size,
                                        KMemoryPermission perm, bool is_aligned, bool check_heap);
    Result LockForUnmapDeviceAddressSpace(VAddr address, size_t size, bool check_heap);

    Result UnlockForDeviceAddressSpace(VAddr addr, size_t size);

    Result LockForIpcUserBuffer(PAddr* out, VAddr address, size_t size);
    Result UnlockForIpcUserBuffer(VAddr address, size_t size);

    Result SetupForIpc(VAddr* out_dst_addr, size_t size, VAddr src_addr, KPageTable& src_page_table,
                       KMemoryPermission test_perm, KMemoryState dst_state, bool send);
    Result CleanupForIpcServer(VAddr address, size_t size, KMemoryState dst_state);
    Result CleanupForIpcClient(VAddr address, size_t size, KMemoryState dst_state);

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

    KBlockInfoManager* GetBlockInfoManager() {
        return m_block_info_manager;
    }

    bool CanContain(VAddr addr, size_t size, KMemoryState state) const;

    Result MapPages(KProcessAddress* out_addr, size_t num_pages, size_t alignment,
                    KPhysicalAddress phys_addr, KProcessAddress region_start,
                    size_t region_num_pages, KMemoryState state, KMemoryPermission perm) {
        R_RETURN(this->MapPages(out_addr, num_pages, alignment, phys_addr, true, region_start,
                                region_num_pages, state, perm));
    }

    Result MapPages(KProcessAddress* out_addr, size_t num_pages, size_t alignment,
                    KPhysicalAddress phys_addr, KMemoryState state, KMemoryPermission perm) {
        R_RETURN(this->MapPages(out_addr, num_pages, alignment, phys_addr, true,
                                this->GetRegionAddress(state),
                                this->GetRegionSize(state) / PageSize, state, perm));
    }

    Result MapPages(KProcessAddress* out_addr, size_t num_pages, KMemoryState state,
                    KMemoryPermission perm) {
        R_RETURN(this->MapPages(out_addr, num_pages, PageSize, 0, false,
                                this->GetRegionAddress(state),
                                this->GetRegionSize(state) / PageSize, state, perm));
    }

    Result MapPages(KProcessAddress address, size_t num_pages, KMemoryState state,
                    KMemoryPermission perm);
    Result UnmapPages(KProcessAddress address, size_t num_pages, KMemoryState state);

    Result MapPageGroup(KProcessAddress* out_addr, const KPageGroup& pg,
                        KProcessAddress region_start, size_t region_num_pages, KMemoryState state,
                        KMemoryPermission perm);
    Result MapPageGroup(KProcessAddress address, const KPageGroup& pg, KMemoryState state,
                        KMemoryPermission perm);
    Result UnmapPageGroup(KProcessAddress address, const KPageGroup& pg, KMemoryState state);
    void RemapPageGroup(PageLinkedList* page_list, KProcessAddress address, size_t size,
                        const KPageGroup& pg);

protected:
    struct PageLinkedList {
    private:
        struct Node {
            Node* m_next;
            std::array<u8, PageSize - sizeof(Node*)> m_buffer;
        };

    public:
        constexpr PageLinkedList() = default;

        void Push(Node* n) {
            ASSERT(Common::IsAligned(reinterpret_cast<uintptr_t>(n), PageSize));
            n->m_next = m_root;
            m_root = n;
        }

        void Push(Core::Memory::Memory& memory, VAddr addr) {
            this->Push(memory.GetPointer<Node>(addr));
        }

        Node* Peek() const {
            return m_root;
        }

        Node* Pop() {
            Node* const r = m_root;

            m_root = r->m_next;
            r->m_next = nullptr;

            return r;
        }

    private:
        Node* m_root{};
    };
    static_assert(std::is_trivially_destructible<PageLinkedList>::value);

private:
    enum class OperationType : u32 {
        Map = 0,
        MapFirst = 1,
        MapGroup = 2,
        Unmap = 3,
        ChangePermissions = 4,
        ChangePermissionsAndRefresh = 5,
        Separate = 6,
    };

    static constexpr KMemoryAttribute DefaultMemoryIgnoreAttr =
        KMemoryAttribute::IpcLocked | KMemoryAttribute::DeviceShared;

    Result MapPages(KProcessAddress* out_addr, size_t num_pages, size_t alignment,
                    KPhysicalAddress phys_addr, bool is_pa_valid, KProcessAddress region_start,
                    size_t region_num_pages, KMemoryState state, KMemoryPermission perm);
    bool IsRegionContiguous(VAddr addr, u64 size) const;
    void AddRegionToPages(VAddr start, size_t num_pages, KPageGroup& page_linked_list);
    KMemoryInfo QueryInfoImpl(VAddr addr);
    VAddr AllocateVirtualMemory(VAddr start, size_t region_num_pages, u64 needed_num_pages,
                                size_t align);
    Result Operate(VAddr addr, size_t num_pages, const KPageGroup& page_group,
                   OperationType operation);
    Result Operate(VAddr addr, size_t num_pages, KMemoryPermission perm, OperationType operation,
                   PAddr map_addr = 0);
    void FinalizeUpdate(PageLinkedList* page_list);
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

    Result SetupForIpcClient(PageLinkedList* page_list, size_t* out_blocks_needed, VAddr address,
                             size_t size, KMemoryPermission test_perm, KMemoryState dst_state);
    Result SetupForIpcServer(VAddr* out_addr, size_t size, VAddr src_addr,
                             KMemoryPermission test_perm, KMemoryState dst_state,
                             KPageTable& src_page_table, bool send);
    void CleanupForIpcClientOnServerSetupFailure(PageLinkedList* page_list, VAddr address,
                                                 size_t size, KMemoryPermission prot_perm);

    Result AllocateAndMapPagesImpl(PageLinkedList* page_list, KProcessAddress address,
                                   size_t num_pages, KMemoryPermission perm);
    Result MapPageGroupImpl(PageLinkedList* page_list, KProcessAddress address,
                            const KPageGroup& pg, const KPageProperties properties, bool reuse_ll);

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
    constexpr VAddr GetAliasCodeRegionEnd() const {
        return m_alias_code_region_end;
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

public:
    static VAddr GetLinearMappedVirtualAddress(const KMemoryLayout& layout, PAddr addr) {
        return layout.GetLinearVirtualAddress(addr);
    }

    static PAddr GetLinearMappedPhysicalAddress(const KMemoryLayout& layout, VAddr addr) {
        return layout.GetLinearPhysicalAddress(addr);
    }

    static VAddr GetHeapVirtualAddress(const KMemoryLayout& layout, PAddr addr) {
        return GetLinearMappedVirtualAddress(layout, addr);
    }

    static PAddr GetHeapPhysicalAddress(const KMemoryLayout& layout, VAddr addr) {
        return GetLinearMappedPhysicalAddress(layout, addr);
    }

    static VAddr GetPageTableVirtualAddress(const KMemoryLayout& layout, PAddr addr) {
        return GetLinearMappedVirtualAddress(layout, addr);
    }

    static PAddr GetPageTablePhysicalAddress(const KMemoryLayout& layout, VAddr addr) {
        return GetLinearMappedPhysicalAddress(layout, addr);
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
    class KScopedPageTableUpdater {
    private:
        KPageTable* m_pt{};
        PageLinkedList m_ll;

    public:
        explicit KScopedPageTableUpdater(KPageTable* pt) : m_pt(pt) {}
        explicit KScopedPageTableUpdater(KPageTable& pt) : KScopedPageTableUpdater(&pt) {}
        ~KScopedPageTableUpdater() {
            m_pt->FinalizeUpdate(this->GetPageList());
        }

        PageLinkedList* GetPageList() {
            return &m_ll;
        }
    };

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

    size_t m_max_heap_size{};
    size_t m_mapped_physical_memory_size{};
    size_t m_mapped_unsafe_physical_memory{};
    size_t m_mapped_insecure_memory{};
    size_t m_mapped_ipc_server_memory{};
    size_t m_address_space_width{};

    KMemoryBlockManager m_memory_block_manager;
    u32 m_allocate_option{};

    bool m_is_kernel{};
    bool m_enable_aslr{};
    bool m_enable_device_address_space_merge{};

    KMemoryBlockSlabManager* m_memory_block_slab_manager{};
    KBlockInfoManager* m_block_info_manager{};
    KResourceLimit* m_resource_limit{};

    u32 m_heap_fill_value{};
    u32 m_ipc_fill_value{};
    u32 m_stack_fill_value{};
    const KMemoryRegion* m_cached_physical_heap_region{};

    KMemoryManager::Pool m_memory_pool{KMemoryManager::Pool::Application};
    KMemoryManager::Direction m_allocation_option{KMemoryManager::Direction::FromFront};

    std::unique_ptr<Common::PageTable> m_page_table_impl;

    Core::System& m_system;
    KernelCore& m_kernel;
};

} // namespace Kernel

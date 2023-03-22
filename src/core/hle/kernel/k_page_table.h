// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include "common/common_funcs.h"
#include "common/page_table.h"
#include "core/file_sys/program_metadata.h"
#include "core/hle/kernel/k_dynamic_resource_manager.h"
#include "core/hle/kernel/k_light_lock.h"
#include "core/hle/kernel/k_memory_block.h"
#include "core/hle/kernel/k_memory_block_manager.h"
#include "core/hle/kernel/k_memory_layout.h"
#include "core/hle/kernel/k_memory_manager.h"
#include "core/hle/kernel/k_typed_address.h"
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
                                KProcessAddress code_addr, size_t code_size,
                                KSystemResource* system_resource, KResourceLimit* resource_limit);

    void Finalize();

    Result MapProcessCode(KProcessAddress addr, size_t pages_count, KMemoryState state,
                          KMemoryPermission perm);
    Result MapCodeMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size);
    Result UnmapCodeMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size,
                           ICacheInvalidationStrategy icache_invalidation_strategy);
    Result UnmapProcessMemory(KProcessAddress dst_addr, size_t size, KPageTable& src_page_table,
                              KProcessAddress src_addr);
    Result MapPhysicalMemory(KProcessAddress addr, size_t size);
    Result UnmapPhysicalMemory(KProcessAddress addr, size_t size);
    Result MapMemory(KProcessAddress dst_addr, KProcessAddress src_addr, size_t size);
    Result UnmapMemory(KProcessAddress dst_addr, KProcessAddress src_addr, size_t size);
    Result SetProcessMemoryPermission(KProcessAddress addr, size_t size,
                                      Svc::MemoryPermission svc_perm);
    KMemoryInfo QueryInfo(KProcessAddress addr);
    Result SetMemoryPermission(KProcessAddress addr, size_t size, Svc::MemoryPermission perm);
    Result SetMemoryAttribute(KProcessAddress addr, size_t size, u32 mask, u32 attr);
    Result SetMaxHeapSize(size_t size);
    Result SetHeapSize(u64* out, size_t size);
    Result LockForMapDeviceAddressSpace(bool* out_is_io, KProcessAddress address, size_t size,
                                        KMemoryPermission perm, bool is_aligned, bool check_heap);
    Result LockForUnmapDeviceAddressSpace(KProcessAddress address, size_t size, bool check_heap);

    Result UnlockForDeviceAddressSpace(KProcessAddress addr, size_t size);

    Result LockForIpcUserBuffer(KPhysicalAddress* out, KProcessAddress address, size_t size);
    Result UnlockForIpcUserBuffer(KProcessAddress address, size_t size);

    Result SetupForIpc(KProcessAddress* out_dst_addr, size_t size, KProcessAddress src_addr,
                       KPageTable& src_page_table, KMemoryPermission test_perm,
                       KMemoryState dst_state, bool send);
    Result CleanupForIpcServer(KProcessAddress address, size_t size, KMemoryState dst_state);
    Result CleanupForIpcClient(KProcessAddress address, size_t size, KMemoryState dst_state);

    Result LockForCodeMemory(KPageGroup* out, KProcessAddress addr, size_t size);
    Result UnlockForCodeMemory(KProcessAddress addr, size_t size, const KPageGroup& pg);
    Result MakeAndOpenPageGroup(KPageGroup* out, KProcessAddress address, size_t num_pages,
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

    bool CanContain(KProcessAddress addr, size_t size, KMemoryState state) const;

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

        void Push(Core::Memory::Memory& memory, KVirtualAddress addr) {
            this->Push(memory.GetPointer<Node>(GetInteger(addr)));
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
    bool IsRegionContiguous(KProcessAddress addr, u64 size) const;
    void AddRegionToPages(KProcessAddress start, size_t num_pages, KPageGroup& page_linked_list);
    KMemoryInfo QueryInfoImpl(KProcessAddress addr);
    KProcessAddress AllocateVirtualMemory(KProcessAddress start, size_t region_num_pages,
                                          u64 needed_num_pages, size_t align);
    Result Operate(KProcessAddress addr, size_t num_pages, const KPageGroup& page_group,
                   OperationType operation);
    Result Operate(KProcessAddress addr, size_t num_pages, KMemoryPermission perm,
                   OperationType operation, KPhysicalAddress map_addr = 0);
    void FinalizeUpdate(PageLinkedList* page_list);
    KProcessAddress GetRegionAddress(KMemoryState state) const;
    size_t GetRegionSize(KMemoryState state) const;

    KProcessAddress FindFreeArea(KProcessAddress region_start, size_t region_num_pages,
                                 size_t num_pages, size_t alignment, size_t offset,
                                 size_t guard_pages);

    Result CheckMemoryStateContiguous(size_t* out_blocks_needed, KProcessAddress addr, size_t size,
                                      KMemoryState state_mask, KMemoryState state,
                                      KMemoryPermission perm_mask, KMemoryPermission perm,
                                      KMemoryAttribute attr_mask, KMemoryAttribute attr) const;
    Result CheckMemoryStateContiguous(KProcessAddress addr, size_t size, KMemoryState state_mask,
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
                            KMemoryAttribute* out_attr, size_t* out_blocks_needed,
                            KProcessAddress addr, size_t size, KMemoryState state_mask,
                            KMemoryState state, KMemoryPermission perm_mask, KMemoryPermission perm,
                            KMemoryAttribute attr_mask, KMemoryAttribute attr,
                            KMemoryAttribute ignore_attr = DefaultMemoryIgnoreAttr) const;
    Result CheckMemoryState(size_t* out_blocks_needed, KProcessAddress addr, size_t size,
                            KMemoryState state_mask, KMemoryState state,
                            KMemoryPermission perm_mask, KMemoryPermission perm,
                            KMemoryAttribute attr_mask, KMemoryAttribute attr,
                            KMemoryAttribute ignore_attr = DefaultMemoryIgnoreAttr) const {
        R_RETURN(CheckMemoryState(nullptr, nullptr, nullptr, out_blocks_needed, addr, size,
                                  state_mask, state, perm_mask, perm, attr_mask, attr,
                                  ignore_attr));
    }
    Result CheckMemoryState(KProcessAddress addr, size_t size, KMemoryState state_mask,
                            KMemoryState state, KMemoryPermission perm_mask, KMemoryPermission perm,
                            KMemoryAttribute attr_mask, KMemoryAttribute attr,
                            KMemoryAttribute ignore_attr = DefaultMemoryIgnoreAttr) const {
        R_RETURN(this->CheckMemoryState(nullptr, addr, size, state_mask, state, perm_mask, perm,
                                        attr_mask, attr, ignore_attr));
    }

    Result LockMemoryAndOpen(KPageGroup* out_pg, KPhysicalAddress* out_KPhysicalAddress,
                             KProcessAddress addr, size_t size, KMemoryState state_mask,
                             KMemoryState state, KMemoryPermission perm_mask,
                             KMemoryPermission perm, KMemoryAttribute attr_mask,
                             KMemoryAttribute attr, KMemoryPermission new_perm,
                             KMemoryAttribute lock_attr);
    Result UnlockMemory(KProcessAddress addr, size_t size, KMemoryState state_mask,
                        KMemoryState state, KMemoryPermission perm_mask, KMemoryPermission perm,
                        KMemoryAttribute attr_mask, KMemoryAttribute attr,
                        KMemoryPermission new_perm, KMemoryAttribute lock_attr,
                        const KPageGroup* pg);

    Result MakePageGroup(KPageGroup& pg, KProcessAddress addr, size_t num_pages);
    bool IsValidPageGroup(const KPageGroup& pg, KProcessAddress addr, size_t num_pages);

    bool IsLockedByCurrentThread() const {
        return m_general_lock.IsLockedByCurrentThread();
    }

    bool IsHeapPhysicalAddress(const KMemoryLayout& layout, KPhysicalAddress phys_addr) {
        ASSERT(this->IsLockedByCurrentThread());

        return layout.IsHeapPhysicalAddress(m_cached_physical_heap_region, phys_addr);
    }

    bool GetPhysicalAddressLocked(KPhysicalAddress* out, KProcessAddress virt_addr) const {
        ASSERT(this->IsLockedByCurrentThread());

        *out = GetPhysicalAddr(virt_addr);

        return *out != 0;
    }

    Result SetupForIpcClient(PageLinkedList* page_list, size_t* out_blocks_needed,
                             KProcessAddress address, size_t size, KMemoryPermission test_perm,
                             KMemoryState dst_state);
    Result SetupForIpcServer(KProcessAddress* out_addr, size_t size, KProcessAddress src_addr,
                             KMemoryPermission test_perm, KMemoryState dst_state,
                             KPageTable& src_page_table, bool send);
    void CleanupForIpcClientOnServerSetupFailure(PageLinkedList* page_list, KProcessAddress address,
                                                 size_t size, KMemoryPermission prot_perm);

    Result AllocateAndMapPagesImpl(PageLinkedList* page_list, KProcessAddress address,
                                   size_t num_pages, KMemoryPermission perm);
    Result MapPageGroupImpl(PageLinkedList* page_list, KProcessAddress address,
                            const KPageGroup& pg, const KPageProperties properties, bool reuse_ll);

    mutable KLightLock m_general_lock;
    mutable KLightLock m_map_physical_memory_lock;

public:
    constexpr KProcessAddress GetAddressSpaceStart() const {
        return m_address_space_start;
    }
    constexpr KProcessAddress GetAddressSpaceEnd() const {
        return m_address_space_end;
    }
    constexpr size_t GetAddressSpaceSize() const {
        return m_address_space_end - m_address_space_start;
    }
    constexpr KProcessAddress GetHeapRegionStart() const {
        return m_heap_region_start;
    }
    constexpr KProcessAddress GetHeapRegionEnd() const {
        return m_heap_region_end;
    }
    constexpr size_t GetHeapRegionSize() const {
        return m_heap_region_end - m_heap_region_start;
    }
    constexpr KProcessAddress GetAliasRegionStart() const {
        return m_alias_region_start;
    }
    constexpr KProcessAddress GetAliasRegionEnd() const {
        return m_alias_region_end;
    }
    constexpr size_t GetAliasRegionSize() const {
        return m_alias_region_end - m_alias_region_start;
    }
    constexpr KProcessAddress GetStackRegionStart() const {
        return m_stack_region_start;
    }
    constexpr KProcessAddress GetStackRegionEnd() const {
        return m_stack_region_end;
    }
    constexpr size_t GetStackRegionSize() const {
        return m_stack_region_end - m_stack_region_start;
    }
    constexpr KProcessAddress GetKernelMapRegionStart() const {
        return m_kernel_map_region_start;
    }
    constexpr KProcessAddress GetKernelMapRegionEnd() const {
        return m_kernel_map_region_end;
    }
    constexpr KProcessAddress GetCodeRegionStart() const {
        return m_code_region_start;
    }
    constexpr KProcessAddress GetCodeRegionEnd() const {
        return m_code_region_end;
    }
    constexpr KProcessAddress GetAliasCodeRegionStart() const {
        return m_alias_code_region_start;
    }
    constexpr KProcessAddress GetAliasCodeRegionEnd() const {
        return m_alias_code_region_end;
    }
    constexpr size_t GetAliasCodeRegionSize() const {
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
    constexpr bool IsInsideAddressSpace(KProcessAddress address, size_t size) const {
        return m_address_space_start <= address && address + size - 1 <= m_address_space_end - 1;
    }
    constexpr bool IsOutsideAliasRegion(KProcessAddress address, size_t size) const {
        return m_alias_region_start > address || address + size - 1 > m_alias_region_end - 1;
    }
    constexpr bool IsOutsideStackRegion(KProcessAddress address, size_t size) const {
        return m_stack_region_start > address || address + size - 1 > m_stack_region_end - 1;
    }
    constexpr bool IsInvalidRegion(KProcessAddress address, size_t size) const {
        return address + size - 1 > GetAliasCodeRegionStart() + GetAliasCodeRegionSize() - 1;
    }
    constexpr bool IsInsideHeapRegion(KProcessAddress address, size_t size) const {
        return address + size > m_heap_region_start && m_heap_region_end > address;
    }
    constexpr bool IsInsideAliasRegion(KProcessAddress address, size_t size) const {
        return address + size > m_alias_region_start && m_alias_region_end > address;
    }
    constexpr bool IsOutsideASLRRegion(KProcessAddress address, size_t size) const {
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
    constexpr bool IsInsideASLRRegion(KProcessAddress address, size_t size) const {
        return !IsOutsideASLRRegion(address, size);
    }
    constexpr size_t GetNumGuardPages() const {
        return IsKernel() ? 1 : 4;
    }
    KPhysicalAddress GetPhysicalAddr(KProcessAddress addr) const {
        const auto backing_addr = m_page_table_impl->backing_addr[addr >> PageBits];
        ASSERT(backing_addr);
        return backing_addr + GetInteger(addr);
    }
    constexpr bool Contains(KProcessAddress addr) const {
        return m_address_space_start <= addr && addr <= m_address_space_end - 1;
    }
    constexpr bool Contains(KProcessAddress addr, size_t size) const {
        return m_address_space_start <= addr && addr < addr + size &&
               addr + size - 1 <= m_address_space_end - 1;
    }

public:
    static KVirtualAddress GetLinearMappedVirtualAddress(const KMemoryLayout& layout,
                                                         KPhysicalAddress addr) {
        return layout.GetLinearVirtualAddress(addr);
    }

    static KPhysicalAddress GetLinearMappedPhysicalAddress(const KMemoryLayout& layout,
                                                           KVirtualAddress addr) {
        return layout.GetLinearPhysicalAddress(addr);
    }

    static KVirtualAddress GetHeapVirtualAddress(const KMemoryLayout& layout,
                                                 KPhysicalAddress addr) {
        return GetLinearMappedVirtualAddress(layout, addr);
    }

    static KPhysicalAddress GetHeapPhysicalAddress(const KMemoryLayout& layout,
                                                   KVirtualAddress addr) {
        return GetLinearMappedPhysicalAddress(layout, addr);
    }

    static KVirtualAddress GetPageTableVirtualAddress(const KMemoryLayout& layout,
                                                      KPhysicalAddress addr) {
        return GetLinearMappedVirtualAddress(layout, addr);
    }

    static KPhysicalAddress GetPageTablePhysicalAddress(const KMemoryLayout& layout,
                                                        KVirtualAddress addr) {
        return GetLinearMappedPhysicalAddress(layout, addr);
    }

private:
    constexpr bool IsKernel() const {
        return m_is_kernel;
    }
    constexpr bool IsAslrEnabled() const {
        return m_enable_aslr;
    }

    constexpr bool ContainsPages(KProcessAddress addr, size_t num_pages) const {
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
            return std::addressof(m_ll);
        }
    };

private:
    KProcessAddress m_address_space_start{};
    KProcessAddress m_address_space_end{};
    KProcessAddress m_heap_region_start{};
    KProcessAddress m_heap_region_end{};
    KProcessAddress m_current_heap_end{};
    KProcessAddress m_alias_region_start{};
    KProcessAddress m_alias_region_end{};
    KProcessAddress m_stack_region_start{};
    KProcessAddress m_stack_region_end{};
    KProcessAddress m_kernel_map_region_start{};
    KProcessAddress m_kernel_map_region_end{};
    KProcessAddress m_code_region_start{};
    KProcessAddress m_code_region_end{};
    KProcessAddress m_alias_code_region_start{};
    KProcessAddress m_alias_code_region_end{};

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

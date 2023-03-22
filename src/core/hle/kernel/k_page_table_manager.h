// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>

#include "core/hle/kernel/k_dynamic_resource_manager.h"
#include "core/hle/kernel/k_page_table_slab_heap.h"
#include "core/hle/kernel/k_typed_address.h"

namespace Kernel {

class KPageTableManager : public KDynamicResourceManager<impl::PageTablePage, true> {
public:
    using RefCount = KPageTableSlabHeap::RefCount;
    static constexpr size_t PageTableSize = KPageTableSlabHeap::PageTableSize;

public:
    KPageTableManager() = default;

    void Initialize(KDynamicPageManager* page_allocator, KPageTableSlabHeap* pt_heap) {
        m_pt_heap = pt_heap;

        static_assert(std::derived_from<KPageTableSlabHeap, DynamicSlabType>);
        BaseHeap::Initialize(page_allocator, pt_heap);
    }

    KVirtualAddress Allocate() {
        return KVirtualAddress(BaseHeap::Allocate());
    }

    RefCount GetRefCount(KVirtualAddress addr) const {
        return m_pt_heap->GetRefCount(addr);
    }

    void Open(KVirtualAddress addr, int count) {
        return m_pt_heap->Open(addr, count);
    }

    bool Close(KVirtualAddress addr, int count) {
        return m_pt_heap->Close(addr, count);
    }

    bool IsInPageTableHeap(KVirtualAddress addr) const {
        return m_pt_heap->IsInRange(addr);
    }

private:
    using BaseHeap = KDynamicResourceManager<impl::PageTablePage, true>;

    KPageTableSlabHeap* m_pt_heap{};
};

} // namespace Kernel

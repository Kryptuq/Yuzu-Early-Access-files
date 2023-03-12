// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <vector>

#include "common/common_types.h"
#include "core/hle/kernel/k_dynamic_slab_heap.h"
#include "core/hle/kernel/slab_helpers.h"

namespace Kernel {

namespace impl {

class PageTablePage {
public:
    // Do not initialize anything.
    PageTablePage() = default;

private:
    // Initializer intentionally skipped
    std::array<u8, PageSize> m_buffer;
};
static_assert(sizeof(PageTablePage) == PageSize);

} // namespace impl

class KPageTableSlabHeap : public KDynamicSlabHeap<impl::PageTablePage, true> {
public:
    using RefCount = u16;
    static constexpr size_t PageTableSize = sizeof(impl::PageTablePage);
    static_assert(PageTableSize == PageSize);

public:
    KPageTableSlabHeap() = default;

    static constexpr size_t CalculateReferenceCountSize(size_t size) {
        return (size / PageSize) * sizeof(RefCount);
    }

    void Initialize(KDynamicPageManager* page_allocator, size_t object_count, RefCount* rc) {
        BaseHeap::Initialize(page_allocator, object_count);
        this->Initialize(rc);
    }

    RefCount GetRefCount(VAddr addr) {
        ASSERT(this->IsInRange(addr));
        return *this->GetRefCountPointer(addr);
    }

    void Open(VAddr addr, int count) {
        ASSERT(this->IsInRange(addr));

        *this->GetRefCountPointer(addr) += static_cast<RefCount>(count);

        ASSERT(this->GetRefCount(addr) > 0);
    }

    bool Close(VAddr addr, int count) {
        ASSERT(this->IsInRange(addr));
        ASSERT(this->GetRefCount(addr) >= count);

        *this->GetRefCountPointer(addr) -= static_cast<RefCount>(count);
        return this->GetRefCount(addr) == 0;
    }

    bool IsInPageTableHeap(VAddr addr) const {
        return this->IsInRange(addr);
    }

private:
    void Initialize([[maybe_unused]] RefCount* rc) {
        // TODO(bunnei): Use rc once we support kernel virtual memory allocations.
        const auto count = this->GetSize() / PageSize;
        m_ref_counts.resize(count);

        for (size_t i = 0; i < count; i++) {
            m_ref_counts[i] = 0;
        }
    }

    RefCount* GetRefCountPointer(VAddr addr) {
        return m_ref_counts.data() + ((addr - this->GetAddress()) / PageSize);
    }

private:
    using BaseHeap = KDynamicSlabHeap<impl::PageTablePage, true>;

    std::vector<RefCount> m_ref_counts;
};

} // namespace Kernel

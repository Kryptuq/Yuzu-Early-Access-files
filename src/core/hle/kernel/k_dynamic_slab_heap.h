// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>

#include "common/common_funcs.h"
#include "core/hle/kernel/k_dynamic_page_manager.h"
#include "core/hle/kernel/k_slab_heap.h"

namespace Kernel {

template <typename T, bool ClearNode = false>
class KDynamicSlabHeap : protected impl::KSlabHeapImpl {
    YUZU_NON_COPYABLE(KDynamicSlabHeap);
    YUZU_NON_MOVEABLE(KDynamicSlabHeap);

private:
    using PageBuffer = KDynamicPageManager::PageBuffer;

private:
    std::atomic<size_t> m_used{};
    std::atomic<size_t> m_peak{};
    std::atomic<size_t> m_count{};
    VAddr m_address{};
    size_t m_size{};

public:
    constexpr KDynamicSlabHeap() = default;

    constexpr VAddr GetAddress() const {
        return m_address;
    }
    constexpr size_t GetSize() const {
        return m_size;
    }
    constexpr size_t GetUsed() const {
        return m_used.load();
    }
    constexpr size_t GetPeak() const {
        return m_peak.load();
    }
    constexpr size_t GetCount() const {
        return m_count.load();
    }

    constexpr bool IsInRange(VAddr addr) const {
        return this->GetAddress() <= addr && addr <= this->GetAddress() + this->GetSize() - 1;
    }

    void Initialize(KDynamicPageManager* page_allocator, size_t num_objects) {
        ASSERT(page_allocator != nullptr);

        // Initialize members.
        m_address = page_allocator->GetAddress();
        m_size = page_allocator->GetSize();

        // Initialize the base allocator.
        KSlabHeapImpl::Initialize();

        // Allocate until we have the correct number of objects.
        while (m_count.load() < num_objects) {
            auto* allocated = reinterpret_cast<T*>(page_allocator->Allocate());
            ASSERT(allocated != nullptr);

            for (size_t i = 0; i < sizeof(PageBuffer) / sizeof(T); i++) {
                KSlabHeapImpl::Free(allocated + i);
            }

            m_count += sizeof(PageBuffer) / sizeof(T);
        }
    }

    T* Allocate(KDynamicPageManager* page_allocator) {
        T* allocated = static_cast<T*>(KSlabHeapImpl::Allocate());

        // If we successfully allocated and we should clear the node, do so.
        if constexpr (ClearNode) {
            if (allocated != nullptr) [[likely]] {
                reinterpret_cast<KSlabHeapImpl::Node*>(allocated)->next = nullptr;
            }
        }

        // If we fail to allocate, try to get a new page from our next allocator.
        if (allocated == nullptr) [[unlikely]] {
            if (page_allocator != nullptr) {
                allocated = reinterpret_cast<T*>(page_allocator->Allocate());
                if (allocated != nullptr) {
                    // If we succeeded in getting a page, free the rest to our slab.
                    for (size_t i = 1; i < sizeof(PageBuffer) / sizeof(T); i++) {
                        KSlabHeapImpl::Free(allocated + i);
                    }
                    m_count += sizeof(PageBuffer) / sizeof(T);
                }
            }
        }

        if (allocated != nullptr) [[likely]] {
            // Construct the object.
            std::construct_at(allocated);

            // Update our tracking.
            const size_t used = ++m_used;
            size_t peak = m_peak.load();
            while (peak < used) {
                if (m_peak.compare_exchange_weak(peak, used, std::memory_order_relaxed)) {
                    break;
                }
            }
        }

        return allocated;
    }

    void Free(T* t) {
        KSlabHeapImpl::Free(t);
        --m_used;
    }
};

} // namespace Kernel

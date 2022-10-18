// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>

#include "common/assert.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/spin_lock.h"

namespace Kernel {

class KernelCore;

namespace impl {

class KSlabHeapImpl {
    YUZU_NON_COPYABLE(KSlabHeapImpl);
    YUZU_NON_MOVEABLE(KSlabHeapImpl);

public:
    struct Node {
        Node* next{};
    };

public:
    constexpr KSlabHeapImpl() = default;

    void Initialize() {
        ASSERT(m_head == nullptr);
    }

    Node* GetHead() const {
        return m_head;
    }

    void* Allocate() {
        // KScopedInterruptDisable di;

        m_lock.lock();

        Node* ret = m_head;
        if (ret != nullptr) [[likely]] {
            m_head = ret->next;
        }

        m_lock.unlock();
        return ret;
    }

    void Free(void* obj) {
        // KScopedInterruptDisable di;

        m_lock.lock();

        Node* node = static_cast<Node*>(obj);
        node->next = m_head;
        m_head = node;

        m_lock.unlock();
    }

private:
    std::atomic<Node*> m_head{};
    Common::SpinLock m_lock;
};

} // namespace impl

template <bool SupportDynamicExpansion>
class KSlabHeapBase : protected impl::KSlabHeapImpl {
    YUZU_NON_COPYABLE(KSlabHeapBase);
    YUZU_NON_MOVEABLE(KSlabHeapBase);

private:
    size_t m_obj_size{};
    uintptr_t m_peak{};
    uintptr_t m_start{};
    uintptr_t m_end{};

private:
    void UpdatePeakImpl(uintptr_t obj) {
        static_assert(std::atomic_ref<uintptr_t>::is_always_lock_free);
        std::atomic_ref<uintptr_t> peak_ref(m_peak);

        const uintptr_t alloc_peak = obj + this->GetObjectSize();
        uintptr_t cur_peak = m_peak;
        do {
            if (alloc_peak <= cur_peak) {
                break;
            }
        } while (!peak_ref.compare_exchange_strong(cur_peak, alloc_peak));
    }

public:
    constexpr KSlabHeapBase() = default;

    bool Contains(uintptr_t address) const {
        return m_start <= address && address < m_end;
    }

    void Initialize(size_t obj_size, void* memory, size_t memory_size) {
        // Ensure we don't initialize a slab using null memory.
        ASSERT(memory != nullptr);

        // Set our object size.
        m_obj_size = obj_size;

        // Initialize the base allocator.
        KSlabHeapImpl::Initialize();

        // Set our tracking variables.
        const size_t num_obj = (memory_size / obj_size);
        m_start = reinterpret_cast<uintptr_t>(memory);
        m_end = m_start + num_obj * obj_size;
        m_peak = m_start;

        // Free the objects.
        u8* cur = reinterpret_cast<u8*>(m_end);

        for (size_t i = 0; i < num_obj; i++) {
            cur -= obj_size;
            KSlabHeapImpl::Free(cur);
        }
    }

    size_t GetSlabHeapSize() const {
        return (m_end - m_start) / this->GetObjectSize();
    }

    size_t GetObjectSize() const {
        return m_obj_size;
    }

    void* Allocate() {
        void* obj = KSlabHeapImpl::Allocate();

        return obj;
    }

    void Free(void* obj) {
        // Don't allow freeing an object that wasn't allocated from this heap.
        const bool contained = this->Contains(reinterpret_cast<uintptr_t>(obj));
        ASSERT(contained);
        KSlabHeapImpl::Free(obj);
    }

    size_t GetObjectIndex(const void* obj) const {
        if constexpr (SupportDynamicExpansion) {
            if (!this->Contains(reinterpret_cast<uintptr_t>(obj))) {
                return std::numeric_limits<size_t>::max();
            }
        }

        return (reinterpret_cast<uintptr_t>(obj) - m_start) / this->GetObjectSize();
    }

    size_t GetPeakIndex() const {
        return this->GetObjectIndex(reinterpret_cast<const void*>(m_peak));
    }

    uintptr_t GetSlabHeapAddress() const {
        return m_start;
    }

    size_t GetNumRemaining() const {
        // Only calculate the number of remaining objects under debug configuration.
        return 0;
    }
};

template <typename T>
class KSlabHeap final : public KSlabHeapBase<false> {
private:
    using BaseHeap = KSlabHeapBase<false>;

public:
    constexpr KSlabHeap() = default;

    void Initialize(void* memory, size_t memory_size) {
        BaseHeap::Initialize(sizeof(T), memory, memory_size);
    }

    T* Allocate() {
        T* obj = static_cast<T*>(BaseHeap::Allocate());

        if (obj != nullptr) [[likely]] {
            std::construct_at(obj);
        }
        return obj;
    }

    T* Allocate(KernelCore& kernel) {
        T* obj = static_cast<T*>(BaseHeap::Allocate());

        if (obj != nullptr) [[likely]] {
            std::construct_at(obj, kernel);
        }
        return obj;
    }

    void Free(T* obj) {
        BaseHeap::Free(obj);
    }

    size_t GetObjectIndex(const T* obj) const {
        return BaseHeap::GetObjectIndex(obj);
    }
};

} // namespace Kernel

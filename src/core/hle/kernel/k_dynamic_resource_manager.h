// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_funcs.h"
#include "core/hle/kernel/k_dynamic_slab_heap.h"
#include "core/hle/kernel/k_memory_block.h"
#include "core/hle/kernel/k_page_group.h"

namespace Kernel {

template <typename T, bool ClearNode = false>
class KDynamicResourceManager {
    YUZU_NON_COPYABLE(KDynamicResourceManager);
    YUZU_NON_MOVEABLE(KDynamicResourceManager);

public:
    using DynamicSlabType = KDynamicSlabHeap<T, ClearNode>;

public:
    constexpr KDynamicResourceManager() = default;

    constexpr size_t GetSize() const {
        return m_slab_heap->GetSize();
    }
    constexpr size_t GetUsed() const {
        return m_slab_heap->GetUsed();
    }
    constexpr size_t GetPeak() const {
        return m_slab_heap->GetPeak();
    }
    constexpr size_t GetCount() const {
        return m_slab_heap->GetCount();
    }

    void Initialize(KDynamicPageManager* page_allocator, DynamicSlabType* slab_heap) {
        m_page_allocator = page_allocator;
        m_slab_heap = slab_heap;
    }

    T* Allocate() const {
        return m_slab_heap->Allocate(m_page_allocator);
    }

    void Free(T* t) const {
        m_slab_heap->Free(t);
    }

private:
    KDynamicPageManager* m_page_allocator{};
    DynamicSlabType* m_slab_heap{};
};

class KBlockInfoManager : public KDynamicResourceManager<KBlockInfo> {};
class KMemoryBlockSlabManager : public KDynamicResourceManager<KMemoryBlock> {};

using KBlockInfoSlabHeap = typename KBlockInfoManager::DynamicSlabType;
using KMemoryBlockSlabHeap = typename KMemoryBlockSlabManager::DynamicSlabType;

} // namespace Kernel

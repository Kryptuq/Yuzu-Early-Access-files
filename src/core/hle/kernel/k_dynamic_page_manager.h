// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>

#include "common/alignment.h"
#include "core/hle/kernel/k_page_bitmap.h"
#include "core/hle/kernel/k_spin_lock.h"
#include "core/hle/kernel/k_typed_address.h"
#include "core/hle/kernel/memory_types.h"
#include "core/hle/kernel/svc_results.h"

namespace Kernel {

class KDynamicPageManager {
public:
    class PageBuffer {
    private:
        u8 m_buffer[PageSize];
    };
    static_assert(sizeof(PageBuffer) == PageSize);

public:
    KDynamicPageManager() = default;

    template <typename T>
    T* GetPointer(KVirtualAddress addr) {
        return reinterpret_cast<T*>(m_backing_memory.data() + (addr - m_address));
    }

    template <typename T>
    const T* GetPointer(KVirtualAddress addr) const {
        return reinterpret_cast<T*>(m_backing_memory.data() + (addr - m_address));
    }

    Result Initialize(KVirtualAddress memory, size_t size, size_t align) {
        // We need to have positive size.
        R_UNLESS(size > 0, ResultOutOfMemory);
        m_backing_memory.resize(size);

        // Set addresses.
        m_address = memory;
        m_aligned_address = Common::AlignDown(GetInteger(memory), align);

        // Calculate extents.
        const size_t managed_size = m_address + size - m_aligned_address;
        const size_t overhead_size = Common::AlignUp(
            KPageBitmap::CalculateManagementOverheadSize(managed_size / sizeof(PageBuffer)),
            sizeof(PageBuffer));
        R_UNLESS(overhead_size < size, ResultOutOfMemory);

        // Set tracking fields.
        m_size = Common::AlignDown(size - overhead_size, sizeof(PageBuffer));
        m_count = m_size / sizeof(PageBuffer);

        // Clear the management region.
        u64* management_ptr = GetPointer<u64>(m_address + size - overhead_size);
        std::memset(management_ptr, 0, overhead_size);

        // Initialize the bitmap.
        const size_t allocatable_region_size =
            (m_address + size - overhead_size) - m_aligned_address;
        ASSERT(allocatable_region_size >= sizeof(PageBuffer));

        m_page_bitmap.Initialize(management_ptr, allocatable_region_size / sizeof(PageBuffer));

        // Free the pages to the bitmap.
        for (size_t i = 0; i < m_count; i++) {
            // Ensure the freed page is all-zero.
            std::memset(GetPointer<PageBuffer>(m_address) + i, 0, PageSize);

            // Set the bit for the free page.
            m_page_bitmap.SetBit((m_address + (i * sizeof(PageBuffer)) - m_aligned_address) /
                                 sizeof(PageBuffer));
        }

        R_SUCCEED();
    }

    KVirtualAddress GetAddress() const {
        return m_address;
    }
    size_t GetSize() const {
        return m_size;
    }
    size_t GetUsed() const {
        return m_used;
    }
    size_t GetPeak() const {
        return m_peak;
    }
    size_t GetCount() const {
        return m_count;
    }

    PageBuffer* Allocate() {
        // Take the lock.
        // TODO(bunnei): We should disable interrupts here via KScopedInterruptDisable.
        KScopedSpinLock lk(m_lock);

        // Find a random free block.
        s64 soffset = m_page_bitmap.FindFreeBlock(true);
        if (soffset < 0) [[unlikely]] {
            return nullptr;
        }

        const size_t offset = static_cast<size_t>(soffset);

        // Update our tracking.
        m_page_bitmap.ClearBit(offset);
        m_peak = std::max(m_peak, (++m_used));

        return GetPointer<PageBuffer>(m_aligned_address) + offset;
    }

    PageBuffer* Allocate(size_t count) {
        // Take the lock.
        // TODO(bunnei): We should disable interrupts here via KScopedInterruptDisable.
        KScopedSpinLock lk(m_lock);

        // Find a random free block.
        s64 soffset = m_page_bitmap.FindFreeRange(count);
        if (soffset < 0) [[likely]] {
            return nullptr;
        }

        const size_t offset = static_cast<size_t>(soffset);

        // Update our tracking.
        m_page_bitmap.ClearRange(offset, count);
        m_used += count;
        m_peak = std::max(m_peak, m_used);

        return GetPointer<PageBuffer>(m_aligned_address) + offset;
    }

    void Free(PageBuffer* pb) {
        // Ensure all pages in the heap are zero.
        std::memset(pb, 0, PageSize);

        // Take the lock.
        // TODO(bunnei): We should disable interrupts here via KScopedInterruptDisable.
        KScopedSpinLock lk(m_lock);

        // Set the bit for the free page.
        size_t offset =
            (reinterpret_cast<uint64_t>(pb) - GetInteger(m_aligned_address)) / sizeof(PageBuffer);
        m_page_bitmap.SetBit(offset);

        // Decrement our used count.
        --m_used;
    }

private:
    KSpinLock m_lock;
    KPageBitmap m_page_bitmap;
    size_t m_used{};
    size_t m_peak{};
    size_t m_count{};
    KVirtualAddress m_address{};
    KVirtualAddress m_aligned_address{};
    size_t m_size{};

    // TODO(bunnei): Back by host memory until we emulate kernel virtual address space.
    std::vector<u8> m_backing_memory;
};

} // namespace Kernel

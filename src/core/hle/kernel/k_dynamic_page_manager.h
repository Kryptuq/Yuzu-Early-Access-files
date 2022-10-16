// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/alignment.h"
#include "common/common_types.h"
#include "core/hle/kernel/k_page_bitmap.h"
#include "core/hle/kernel/k_spin_lock.h"
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

private:
    KSpinLock m_lock;
    KPageBitmap m_page_bitmap;
    size_t m_used{};
    size_t m_peak{};
    size_t m_count{};
    VAddr m_address{};
    size_t m_size{};

    // TODO(bunnei): Back by host memory until we emulate kernel virtual address space.
    std::vector<u8> m_backing_memory;

public:
    KDynamicPageManager() = default;

    template <typename T>
    T* GetPointer(VAddr addr) {
        return reinterpret_cast<T*>(m_backing_memory.data() + (addr - m_address));
    }

    template <typename T>
    const T* GetPointer(VAddr addr) const {
        return reinterpret_cast<T*>(m_backing_memory.data() + (addr - m_address));
    }

    Result Initialize(VAddr addr, size_t sz) {
        // We need to have positive size.
        R_UNLESS(sz > 0, ResultOutOfMemory);
        m_backing_memory.resize(sz);

        // Calculate management overhead.
        const size_t management_size =
            KPageBitmap::CalculateManagementOverheadSize(sz / sizeof(PageBuffer));
        const size_t allocatable_size = sz - management_size;

        // Set tracking fields.
        m_address = addr;
        m_size = Common::AlignDown(allocatable_size, sizeof(PageBuffer));
        m_count = allocatable_size / sizeof(PageBuffer);
        R_UNLESS(m_count > 0, ResultOutOfMemory);

        // Clear the management region.
        u64* management_ptr = GetPointer<u64>(m_address + allocatable_size);
        std::memset(management_ptr, 0, management_size);

        // Initialize the bitmap.
        m_page_bitmap.Initialize(management_ptr, m_count);

        // Free the pages to the bitmap.
        for (size_t i = 0; i < m_count; i++) {
            // Ensure the freed page is all-zero.
            std::memset(GetPointer<PageBuffer>(m_address) + i, 0, PageSize);

            // Set the bit for the free page.
            m_page_bitmap.SetBit(i);
        }

        R_SUCCEED();
    }

    constexpr VAddr GetAddress() const {
        return m_address;
    }
    constexpr size_t GetSize() const {
        return m_size;
    }
    constexpr size_t GetUsed() const {
        return m_used;
    }
    constexpr size_t GetPeak() const {
        return m_peak;
    }
    constexpr size_t GetCount() const {
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

        return GetPointer<PageBuffer>(m_address) + offset;
    }

    void Free(PageBuffer* pb) {
        // Ensure all pages in the heap are zero.
        std::memset(pb, 0, PageSize);

        // Take the lock.
        // TODO(bunnei): We should disable interrupts here via KScopedInterruptDisable.
        KScopedSpinLock lk(m_lock);

        // Set the bit for the free page.
        size_t offset = (reinterpret_cast<uintptr_t>(pb) - m_address) / sizeof(PageBuffer);
        m_page_bitmap.SetBit(offset);

        // Decrement our used count.
        --m_used;
    }
};

} // namespace Kernel

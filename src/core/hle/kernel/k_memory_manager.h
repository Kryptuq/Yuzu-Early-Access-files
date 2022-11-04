// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <tuple>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/hle/kernel/k_light_lock.h"
#include "core/hle/kernel/k_memory_layout.h"
#include "core/hle/kernel/k_page_heap.h"
#include "core/hle/result.h"

namespace Core {
class System;
}

namespace Kernel {

class KPageGroup;

class KMemoryManager {
public:
    enum class Pool : u32 {
        Application = 0,
        Applet = 1,
        System = 2,
        SystemNonSecure = 3,

        Count,

        Shift = 4,
        Mask = (0xF << Shift),

        // Aliases.
        Unsafe = Application,
        Secure = System,
    };

    enum class Direction : u32 {
        FromFront = 0,
        FromBack = 1,
        Shift = 0,
        Mask = (0xF << Shift),
    };

    static constexpr size_t MaxManagerCount = 10;

    explicit KMemoryManager(Core::System& system);

    void Initialize(VAddr management_region, size_t management_region_size);

    Result InitializeOptimizedMemory(u64 process_id, Pool pool);
    void FinalizeOptimizedMemory(u64 process_id, Pool pool);

    PAddr AllocateAndOpenContinuous(size_t num_pages, size_t align_pages, u32 option);
    Result AllocateAndOpen(KPageGroup* out, size_t num_pages, u32 option);
    Result AllocateForProcess(KPageGroup* out, size_t num_pages, u32 option, u64 process_id,
                              u8 fill_pattern);

    Pool GetPool(PAddr address) const {
        return this->GetManager(address).GetPool();
    }

    void Open(PAddr address, size_t num_pages) {
        // Repeatedly open references until we've done so for all pages.
        while (num_pages) {
            auto& manager = this->GetManager(address);
            const size_t cur_pages = std::min(num_pages, manager.GetPageOffsetToEnd(address));

            {
                KScopedLightLock lk(m_pool_locks[static_cast<size_t>(manager.GetPool())]);
                manager.Open(address, cur_pages);
            }

            num_pages -= cur_pages;
            address += cur_pages * PageSize;
        }
    }

    void OpenFirst(PAddr address, size_t num_pages) {
        // Repeatedly open references until we've done so for all pages.
        while (num_pages) {
            auto& manager = this->GetManager(address);
            const size_t cur_pages = std::min(num_pages, manager.GetPageOffsetToEnd(address));

            {
                KScopedLightLock lk(m_pool_locks[static_cast<size_t>(manager.GetPool())]);
                manager.OpenFirst(address, cur_pages);
            }

            num_pages -= cur_pages;
            address += cur_pages * PageSize;
        }
    }

    void Close(PAddr address, size_t num_pages) {
        // Repeatedly close references until we've done so for all pages.
        while (num_pages) {
            auto& manager = this->GetManager(address);
            const size_t cur_pages = std::min(num_pages, manager.GetPageOffsetToEnd(address));

            {
                KScopedLightLock lk(m_pool_locks[static_cast<size_t>(manager.GetPool())]);
                manager.Close(address, cur_pages);
            }

            num_pages -= cur_pages;
            address += cur_pages * PageSize;
        }
    }

    size_t GetSize() {
        size_t total = 0;
        for (size_t i = 0; i < m_num_managers; i++) {
            total += m_managers[i].GetSize();
        }
        return total;
    }

    size_t GetSize(Pool pool) {
        constexpr Direction GetSizeDirection = Direction::FromFront;
        size_t total = 0;
        for (auto* manager = this->GetFirstManager(pool, GetSizeDirection); manager != nullptr;
             manager = this->GetNextManager(manager, GetSizeDirection)) {
            total += manager->GetSize();
        }
        return total;
    }

    size_t GetFreeSize() {
        size_t total = 0;
        for (size_t i = 0; i < m_num_managers; i++) {
            KScopedLightLock lk(m_pool_locks[static_cast<size_t>(m_managers[i].GetPool())]);
            total += m_managers[i].GetFreeSize();
        }
        return total;
    }

    size_t GetFreeSize(Pool pool) {
        KScopedLightLock lk(m_pool_locks[static_cast<size_t>(pool)]);

        constexpr Direction GetSizeDirection = Direction::FromFront;
        size_t total = 0;
        for (auto* manager = this->GetFirstManager(pool, GetSizeDirection); manager != nullptr;
             manager = this->GetNextManager(manager, GetSizeDirection)) {
            total += manager->GetFreeSize();
        }
        return total;
    }

    void DumpFreeList(Pool pool) {
        KScopedLightLock lk(m_pool_locks[static_cast<size_t>(pool)]);

        constexpr Direction DumpDirection = Direction::FromFront;
        for (auto* manager = this->GetFirstManager(pool, DumpDirection); manager != nullptr;
             manager = this->GetNextManager(manager, DumpDirection)) {
            manager->DumpFreeList();
        }
    }

public:
    static size_t CalculateManagementOverheadSize(size_t region_size) {
        return Impl::CalculateManagementOverheadSize(region_size);
    }

    static constexpr u32 EncodeOption(Pool pool, Direction dir) {
        return (static_cast<u32>(pool) << static_cast<u32>(Pool::Shift)) |
               (static_cast<u32>(dir) << static_cast<u32>(Direction::Shift));
    }

    static constexpr Pool GetPool(u32 option) {
        return static_cast<Pool>((option & static_cast<u32>(Pool::Mask)) >>
                                 static_cast<u32>(Pool::Shift));
    }

    static constexpr Direction GetDirection(u32 option) {
        return static_cast<Direction>((option & static_cast<u32>(Direction::Mask)) >>
                                      static_cast<u32>(Direction::Shift));
    }

    static constexpr std::tuple<Pool, Direction> DecodeOption(u32 option) {
        return std::make_tuple(GetPool(option), GetDirection(option));
    }

private:
    class Impl {
    public:
        static size_t CalculateManagementOverheadSize(size_t region_size);

        static constexpr size_t CalculateOptimizedProcessOverheadSize(size_t region_size) {
            return (Common::AlignUp((region_size / PageSize), Common::BitSize<u64>()) /
                    Common::BitSize<u64>()) *
                   sizeof(u64);
        }

    public:
        Impl() = default;

        size_t Initialize(PAddr address, size_t size, VAddr management, VAddr management_end,
                          Pool p);

        PAddr AllocateBlock(s32 index, bool random) {
            return m_heap.AllocateBlock(index, random);
        }
        PAddr AllocateAligned(s32 index, size_t num_pages, size_t align_pages) {
            return m_heap.AllocateAligned(index, num_pages, align_pages);
        }
        void Free(PAddr addr, size_t num_pages) {
            m_heap.Free(addr, num_pages);
        }

        void SetInitialUsedHeapSize(size_t reserved_size) {
            m_heap.SetInitialUsedSize(reserved_size);
        }

        void InitializeOptimizedMemory() {
            UNIMPLEMENTED();
        }

        void TrackUnoptimizedAllocation(PAddr block, size_t num_pages);
        void TrackOptimizedAllocation(PAddr block, size_t num_pages);

        bool ProcessOptimizedAllocation(PAddr block, size_t num_pages, u8 fill_pattern);

        constexpr Pool GetPool() const {
            return m_pool;
        }
        constexpr size_t GetSize() const {
            return m_heap.GetSize();
        }
        constexpr PAddr GetEndAddress() const {
            return m_heap.GetEndAddress();
        }

        size_t GetFreeSize() const {
            return m_heap.GetFreeSize();
        }

        void DumpFreeList() const {
            UNIMPLEMENTED();
        }

        constexpr size_t GetPageOffset(PAddr address) const {
            return m_heap.GetPageOffset(address);
        }
        constexpr size_t GetPageOffsetToEnd(PAddr address) const {
            return m_heap.GetPageOffsetToEnd(address);
        }

        constexpr void SetNext(Impl* n) {
            m_next = n;
        }
        constexpr void SetPrev(Impl* n) {
            m_prev = n;
        }
        constexpr Impl* GetNext() const {
            return m_next;
        }
        constexpr Impl* GetPrev() const {
            return m_prev;
        }

        void OpenFirst(PAddr address, size_t num_pages) {
            size_t index = this->GetPageOffset(address);
            const size_t end = index + num_pages;
            while (index < end) {
                const RefCount ref_count = (++m_page_reference_counts[index]);
                ASSERT(ref_count == 1);

                index++;
            }
        }

        void Open(PAddr address, size_t num_pages) {
            size_t index = this->GetPageOffset(address);
            const size_t end = index + num_pages;
            while (index < end) {
                const RefCount ref_count = (++m_page_reference_counts[index]);
                ASSERT(ref_count > 1);

                index++;
            }
        }

        void Close(PAddr address, size_t num_pages) {
            size_t index = this->GetPageOffset(address);
            const size_t end = index + num_pages;

            size_t free_start = 0;
            size_t free_count = 0;
            while (index < end) {
                ASSERT(m_page_reference_counts[index] > 0);
                const RefCount ref_count = (--m_page_reference_counts[index]);

                // Keep track of how many zero refcounts we see in a row, to minimize calls to free.
                if (ref_count == 0) {
                    if (free_count > 0) {
                        free_count++;
                    } else {
                        free_start = index;
                        free_count = 1;
                    }
                } else {
                    if (free_count > 0) {
                        this->Free(m_heap.GetAddress() + free_start * PageSize, free_count);
                        free_count = 0;
                    }
                }

                index++;
            }

            if (free_count > 0) {
                this->Free(m_heap.GetAddress() + free_start * PageSize, free_count);
            }
        }

    private:
        using RefCount = u16;

        KPageHeap m_heap;
        std::vector<RefCount> m_page_reference_counts;
        VAddr m_management_region{};
        Pool m_pool{};
        Impl* m_next{};
        Impl* m_prev{};
    };

private:
    Impl& GetManager(PAddr address) {
        return m_managers[m_memory_layout.GetPhysicalLinearRegion(address).GetAttributes()];
    }

    const Impl& GetManager(PAddr address) const {
        return m_managers[m_memory_layout.GetPhysicalLinearRegion(address).GetAttributes()];
    }

    constexpr Impl* GetFirstManager(Pool pool, Direction dir) {
        return dir == Direction::FromBack ? m_pool_managers_tail[static_cast<size_t>(pool)]
                                          : m_pool_managers_head[static_cast<size_t>(pool)];
    }

    constexpr Impl* GetNextManager(Impl* cur, Direction dir) {
        if (dir == Direction::FromBack) {
            return cur->GetPrev();
        } else {
            return cur->GetNext();
        }
    }

    Result AllocatePageGroupImpl(KPageGroup* out, size_t num_pages, Pool pool, Direction dir,
                                 bool unoptimized, bool random);

private:
    template <typename T>
    using PoolArray = std::array<T, static_cast<size_t>(Pool::Count)>;

    Core::System& m_system;
    const KMemoryLayout& m_memory_layout;
    PoolArray<KLightLock> m_pool_locks;
    std::array<Impl*, MaxManagerCount> m_pool_managers_head{};
    std::array<Impl*, MaxManagerCount> m_pool_managers_tail{};
    std::array<Impl, MaxManagerCount> m_managers;
    size_t m_num_managers{};
    PoolArray<u64> m_optimized_process_ids{};
    PoolArray<bool> m_has_optimized_process{};
};

} // namespace Kernel

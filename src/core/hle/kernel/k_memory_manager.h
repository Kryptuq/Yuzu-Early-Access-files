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

class KPageLinkedList;

class KMemoryManager final {
public:
    YUZU_NON_COPYABLE(KMemoryManager);
    YUZU_NON_MOVEABLE(KMemoryManager);

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

    explicit KMemoryManager(Core::System& system_);

    void Initialize(VAddr management_region, size_t management_region_size);

    constexpr size_t GetSize(Pool pool) const {
        constexpr Direction GetSizeDirection = Direction::FromFront;
        size_t total = 0;
        for (auto* manager = this->GetFirstManager(pool, GetSizeDirection); manager != nullptr;
             manager = this->GetNextManager(manager, GetSizeDirection)) {
            total += manager->GetSize();
        }
        return total;
    }

    PAddr AllocateAndOpenContinuous(size_t num_pages, size_t align_pages, u32 option);
    ResultCode AllocateAndOpen(KPageLinkedList* out, size_t num_pages, u32 option);
    ResultCode AllocateAndOpenForProcess(KPageLinkedList* out, size_t num_pages, u32 option,
                                         u64 process_id, u8 fill_pattern);

    static constexpr size_t MaxManagerCount = 10;

    void Close(PAddr address, size_t num_pages);
    void Close(const KPageLinkedList& pg);

    void Open(PAddr address, size_t num_pages);
    void Open(const KPageLinkedList& pg);

public:
    static size_t CalculateManagementOverheadSize(size_t region_size) {
        return Impl::CalculateManagementOverheadSize(region_size);
    }

    static constexpr u32 EncodeOption(Pool pool, Direction dir) {
        return (static_cast<u32>(pool) << static_cast<u32>(Pool::Shift)) |
               (static_cast<u32>(dir) << static_cast<u32>(Direction::Shift));
    }

    static constexpr Pool GetPool(u32 option) {
        return static_cast<Pool>((static_cast<u32>(option) & static_cast<u32>(Pool::Mask)) >>
                                 static_cast<u32>(Pool::Shift));
    }

    static constexpr Direction GetDirection(u32 option) {
        return static_cast<Direction>(
            (static_cast<u32>(option) & static_cast<u32>(Direction::Mask)) >>
            static_cast<u32>(Direction::Shift));
    }

    static constexpr std::tuple<Pool, Direction> DecodeOption(u32 option) {
        return std::make_tuple(GetPool(option), GetDirection(option));
    }

private:
    class Impl final {
    public:
        YUZU_NON_COPYABLE(Impl);
        YUZU_NON_MOVEABLE(Impl);

        Impl() = default;
        ~Impl() = default;

        size_t Initialize(PAddr address, size_t size, VAddr management, VAddr management_end,
                          Pool p);

        VAddr AllocateBlock(s32 index, bool random) {
            return heap.AllocateBlock(index, random);
        }

        void Free(VAddr addr, size_t num_pages) {
            heap.Free(addr, num_pages);
        }

        void SetInitialUsedHeapSize(size_t reserved_size) {
            heap.SetInitialUsedSize(reserved_size);
        }

        constexpr Pool GetPool() const {
            return pool;
        }

        constexpr size_t GetSize() const {
            return heap.GetSize();
        }

        constexpr VAddr GetAddress() const {
            return heap.GetAddress();
        }

        constexpr VAddr GetEndAddress() const {
            return heap.GetEndAddress();
        }

        constexpr size_t GetPageOffset(PAddr address) const {
            return heap.GetPageOffset(address);
        }

        constexpr size_t GetPageOffsetToEnd(PAddr address) const {
            return heap.GetPageOffsetToEnd(address);
        }

        constexpr void SetNext(Impl* n) {
            next = n;
        }

        constexpr void SetPrev(Impl* n) {
            prev = n;
        }

        constexpr Impl* GetNext() const {
            return next;
        }

        constexpr Impl* GetPrev() const {
            return prev;
        }

        void OpenFirst(PAddr address, size_t num_pages) {
            size_t index = this->GetPageOffset(address);
            const size_t end = index + num_pages;
            while (index < end) {
                const RefCount ref_count = (++page_reference_counts[index]);
                ASSERT(ref_count == 1);

                index++;
            }
        }

        void Open(PAddr address, size_t num_pages) {
            size_t index = this->GetPageOffset(address);
            const size_t end = index + num_pages;
            while (index < end) {
                const RefCount ref_count = (++page_reference_counts[index]);
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
                ASSERT(page_reference_counts[index] > 0);
                const RefCount ref_count = (--page_reference_counts[index]);

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
                        this->Free(heap.GetAddress() + free_start * PageSize, free_count);
                        free_count = 0;
                    }
                }

                index++;
            }

            if (free_count > 0) {
                this->Free(heap.GetAddress() + free_start * PageSize, free_count);
            }
        }

        static size_t CalculateManagementOverheadSize(size_t region_size);

        static constexpr size_t CalculateOptimizedProcessOverheadSize(size_t region_size) {
            return (Common::AlignUp((region_size / PageSize), Common::BitSize<u64>()) /
                    Common::BitSize<u64>()) *
                   sizeof(u64);
        }

    private:
        using RefCount = u16;

        KPageHeap heap;
        std::vector<RefCount> page_reference_counts;
        VAddr management_region{};
        Pool pool{};
        Impl* next{};
        Impl* prev{};
    };

private:
    Impl& GetManager(const KMemoryLayout& memory_layout, PAddr address) {
        return managers[memory_layout.GetPhysicalLinearRegion(address).GetAttributes()];
    }

    const Impl& GetManager(const KMemoryLayout& memory_layout, PAddr address) const {
        return managers[memory_layout.GetPhysicalLinearRegion(address).GetAttributes()];
    }

    constexpr Impl* GetFirstManager(Pool pool, Direction dir) const {
        return dir == Direction::FromBack ? pool_managers_tail[static_cast<size_t>(pool)]
                                          : pool_managers_head[static_cast<size_t>(pool)];
    }

    constexpr Impl* GetNextManager(Impl* cur, Direction dir) const {
        if (dir == Direction::FromBack) {
            return cur->GetPrev();
        } else {
            return cur->GetNext();
        }
    }

    ResultCode AllocatePageGroupImpl(KPageLinkedList* out, size_t num_pages, Pool pool,
                                     Direction dir, bool random);

private:
    Core::System& system;
    std::array<KLightLock, static_cast<size_t>(Pool::Count)> pool_locks;
    std::array<Impl*, MaxManagerCount> pool_managers_head{};
    std::array<Impl*, MaxManagerCount> pool_managers_tail{};
    std::array<Impl, MaxManagerCount> managers;
    size_t num_managers{};
};

} // namespace Kernel

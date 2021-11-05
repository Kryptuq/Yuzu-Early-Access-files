// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <mutex>
#include <tuple>

#include "common/common_types.h"
#include "core/hle/kernel/k_page_heap.h"
#include "core/hle/result.h"

namespace Kernel {

class KPageLinkedList;

class KMemoryManager final : NonCopyable {
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

    KMemoryManager() = default;

    constexpr std::size_t GetSize(Pool pool) const {
        return managers[static_cast<std::size_t>(pool)].GetSize();
    }

    void InitializeManager(Pool pool, u64 start_address, u64 end_address);

    VAddr AllocateAndOpenContinuous(size_t num_pages, size_t align_pages, u32 option);
    ResultCode Allocate(KPageLinkedList& page_list, std::size_t num_pages, Pool pool,
                        Direction dir = Direction::FromFront);
    ResultCode Free(KPageLinkedList& page_list, std::size_t num_pages, Pool pool,
                    Direction dir = Direction::FromFront);

    static constexpr std::size_t MaxManagerCount = 10;

public:
    static std::size_t CalculateManagementOverheadSize(std::size_t region_size) {
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
    class Impl final : NonCopyable {
    private:
        using RefCount = u16;

    private:
        KPageHeap heap;
        Pool pool{};

    public:
        static std::size_t CalculateManagementOverheadSize(std::size_t region_size);

        static constexpr std::size_t CalculateOptimizedProcessOverheadSize(
            std::size_t region_size) {
            return (Common::AlignUp((region_size / PageSize), Common::BitSize<u64>()) /
                    Common::BitSize<u64>()) *
                   sizeof(u64);
        }

    public:
        Impl() = default;

        std::size_t Initialize(Pool new_pool, u64 start_address, u64 end_address);

        VAddr AllocateBlock(s32 index, bool random) {
            return heap.AllocateBlock(index, random);
        }

        void Free(VAddr addr, std::size_t num_pages) {
            heap.Free(addr, num_pages);
        }

        constexpr std::size_t GetSize() const {
            return heap.GetSize();
        }

        constexpr VAddr GetAddress() const {
            return heap.GetAddress();
        }

        constexpr VAddr GetEndAddress() const {
            return heap.GetEndAddress();
        }
    };

private:
    std::array<std::mutex, static_cast<std::size_t>(Pool::Count)> pool_locks;
    std::array<Impl, MaxManagerCount> managers;
};

} // namespace Kernel

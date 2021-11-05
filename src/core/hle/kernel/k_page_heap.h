// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <vector>

#include "common/alignment.h"
#include "common/common_types.h"
#include "core/hle/kernel/k_page_bitmap.h"
#include "core/hle/kernel/memory_types.h"

namespace Kernel {

class KPageHeap final : NonCopyable {
public:
    static constexpr s32 GetAlignedBlockIndex(std::size_t num_pages, std::size_t align_pages) {
        const auto target_pages{std::max(num_pages, align_pages)};
        for (std::size_t i = 0; i < NumMemoryBlockPageShifts; i++) {
            if (target_pages <=
                (static_cast<std::size_t>(1) << MemoryBlockPageShifts[i]) / PageSize) {
                return static_cast<s32>(i);
            }
        }
        return -1;
    }

    static constexpr s32 GetBlockIndex(std::size_t num_pages) {
        for (s32 i{static_cast<s32>(NumMemoryBlockPageShifts) - 1}; i >= 0; i--) {
            if (num_pages >= (static_cast<std::size_t>(1) << MemoryBlockPageShifts[i]) / PageSize) {
                return i;
            }
        }
        return -1;
    }

    static constexpr std::size_t GetBlockSize(std::size_t index) {
        return static_cast<std::size_t>(1) << MemoryBlockPageShifts[index];
    }

    static constexpr std::size_t GetBlockNumPages(std::size_t index) {
        return GetBlockSize(index) / PageSize;
    }

private:
    static constexpr std::size_t NumMemoryBlockPageShifts{7};
    static constexpr std::array<std::size_t, NumMemoryBlockPageShifts> MemoryBlockPageShifts{
        0xC, 0x10, 0x15, 0x16, 0x19, 0x1D, 0x1E,
    };

    class Block final : NonCopyable {
    private:
        KPageBitmap bitmap;
        VAddr heap_address{};
        uintptr_t end_offset{};
        std::size_t block_shift{};
        std::size_t next_block_shift{};

    public:
        Block() = default;

        constexpr std::size_t GetShift() const {
            return block_shift;
        }
        constexpr std::size_t GetNextShift() const {
            return next_block_shift;
        }
        constexpr std::size_t GetSize() const {
            return static_cast<std::size_t>(1) << GetShift();
        }
        constexpr std::size_t GetNumPages() const {
            return GetSize() / PageSize;
        }
        constexpr std::size_t GetNumFreeBlocks() const {
            return bitmap.GetNumBits();
        }
        constexpr std::size_t GetNumFreePages() const {
            return GetNumFreeBlocks() * GetNumPages();
        }

        u64* Initialize(VAddr addr, std::size_t size, std::size_t bs, std::size_t nbs,
                        u64* bit_storage) {
            // Set shifts
            block_shift = bs;
            next_block_shift = nbs;

            // Align up the address
            VAddr end{addr + size};
            const auto align{(next_block_shift != 0) ? (1ULL << next_block_shift)
                                                     : (1ULL << block_shift)};
            addr = Common::AlignDown((addr), align);
            end = Common::AlignUp((end), align);

            heap_address = addr;
            end_offset = (end - addr) / (1ULL << block_shift);
            return bitmap.Initialize(bit_storage, end_offset);
        }

        VAddr PushBlock(VAddr address) {
            // Set the bit for the free block
            std::size_t offset{(address - heap_address) >> GetShift()};
            bitmap.SetBit(offset);

            // If we have a next shift, try to clear the blocks below and return the address
            if (GetNextShift()) {
                const auto diff{1ULL << (GetNextShift() - GetShift())};
                offset = Common::AlignDown(offset, diff);
                if (bitmap.ClearRange(offset, diff)) {
                    return heap_address + (offset << GetShift());
                }
            }

            // We couldn't coalesce, or we're already as big as possible
            return 0;
        }

        VAddr PopBlock(bool random) {
            // Find a free block
            const s64 soffset{bitmap.FindFreeBlock(random)};
            if (soffset < 0) {
                return 0;
            }
            const auto offset{static_cast<std::size_t>(soffset)};

            // Update our tracking and return it
            bitmap.ClearBit(offset);
            return heap_address + (offset << GetShift());
        }

    public:
        static constexpr std::size_t CalculateManagementOverheadSize(std::size_t region_size,
                                                                     std::size_t cur_block_shift,
                                                                     std::size_t next_block_shift) {
            const auto cur_block_size{(1ULL << cur_block_shift)};
            const auto next_block_size{(1ULL << next_block_shift)};
            const auto align{(next_block_shift != 0) ? next_block_size : cur_block_size};
            return KPageBitmap::CalculateManagementOverheadSize(
                (align * 2 + Common::AlignUp(region_size, align)) / cur_block_size);
        }
    };

public:
    KPageHeap() = default;

    constexpr VAddr GetAddress() const {
        return heap_address;
    }
    constexpr std::size_t GetSize() const {
        return heap_size;
    }
    constexpr VAddr GetEndAddress() const {
        return GetAddress() + GetSize();
    }
    constexpr std::size_t GetPageOffset(VAddr block) const {
        return (block - GetAddress()) / PageSize;
    }

    void Initialize(VAddr heap_address, std::size_t heap_size, std::size_t metadata_size);
    VAddr AllocateBlock(s32 index, bool random);
    void Free(VAddr addr, std::size_t num_pages);

    void UpdateUsedSize() {
        used_size = heap_size - (GetNumFreePages() * PageSize);
    }

    static std::size_t CalculateManagementOverheadSize(std::size_t region_size);

private:
    constexpr std::size_t GetNumFreePages() const {
        std::size_t num_free{};

        for (const auto& block : blocks) {
            num_free += block.GetNumFreePages();
        }

        return num_free;
    }

    void FreeBlock(VAddr block, s32 index);

    VAddr heap_address{};
    std::size_t heap_size{};
    std::size_t used_size{};
    std::array<Block, NumMemoryBlockPageShifts> blocks{};
    std::vector<u64> metadata;
};

} // namespace Kernel

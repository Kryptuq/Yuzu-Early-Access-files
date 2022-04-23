// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <vector>

#include "common/alignment.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/hle/kernel/k_page_bitmap.h"
#include "core/hle/kernel/memory_types.h"

namespace Kernel {

class KPageHeap final {
public:
    YUZU_NON_COPYABLE(KPageHeap);
    YUZU_NON_MOVEABLE(KPageHeap);

    KPageHeap() = default;
    ~KPageHeap() = default;

    constexpr PAddr GetAddress() const {
        return m_heap_address;
    }
    constexpr size_t GetSize() const {
        return m_heap_size;
    }
    constexpr PAddr GetEndAddress() const {
        return this->GetAddress() + this->GetSize();
    }
    constexpr size_t GetPageOffset(PAddr block) const {
        return (block - this->GetAddress()) / PageSize;
    }
    constexpr size_t GetPageOffsetToEnd(PAddr block) const {
        return (this->GetEndAddress() - block) / PageSize;
    }

    void Initialize(PAddr heap_address, size_t heap_size, VAddr management_address,
                    size_t management_size) {
        return this->Initialize(heap_address, heap_size, management_address, management_size,
                                MemoryBlockPageShifts.data(), NumMemoryBlockPageShifts);
    }

    size_t GetFreeSize() const {
        return this->GetNumFreePages() * PageSize;
    }

    void SetInitialUsedSize(size_t reserved_size) {
        // Check that the reserved size is valid.
        const size_t free_size = this->GetNumFreePages() * PageSize;
        ASSERT(m_heap_size >= free_size + reserved_size);

        // Set the initial used size.
        m_initial_used_size = m_heap_size - free_size - reserved_size;
    }

    PAddr AllocateBlock(s32 index, bool random);
    void Free(PAddr addr, size_t num_pages);

    static size_t CalculateManagementOverheadSize(size_t region_size) {
        return CalculateManagementOverheadSize(region_size, MemoryBlockPageShifts.data(),
                                               NumMemoryBlockPageShifts);
    }

    static constexpr s32 GetAlignedBlockIndex(size_t num_pages, size_t align_pages) {
        const size_t target_pages = std::max(num_pages, align_pages);
        for (size_t i = 0; i < NumMemoryBlockPageShifts; i++) {
            if (target_pages <= (size_t(1) << MemoryBlockPageShifts[i]) / PageSize) {
                return static_cast<s32>(i);
            }
        }
        return -1;
    }

    static constexpr s32 GetBlockIndex(size_t num_pages) {
        for (s32 i = static_cast<s32>(NumMemoryBlockPageShifts) - 1; i >= 0; i--) {
            if (num_pages >= (size_t(1) << MemoryBlockPageShifts[i]) / PageSize) {
                return i;
            }
        }
        return -1;
    }

    static constexpr size_t GetBlockSize(size_t index) {
        return size_t(1) << MemoryBlockPageShifts[index];
    }

    static constexpr size_t GetBlockNumPages(size_t index) {
        return GetBlockSize(index) / PageSize;
    }

private:
    class Block final {
    public:
        YUZU_NON_COPYABLE(Block);
        YUZU_NON_MOVEABLE(Block);

        Block() = default;
        ~Block() = default;

        constexpr size_t GetShift() const {
            return m_block_shift;
        }
        constexpr size_t GetNextShift() const {
            return m_next_block_shift;
        }
        constexpr size_t GetSize() const {
            return u64(1) << this->GetShift();
        }
        constexpr size_t GetNumPages() const {
            return this->GetSize() / PageSize;
        }
        constexpr size_t GetNumFreeBlocks() const {
            return m_bitmap.GetNumBits();
        }
        constexpr size_t GetNumFreePages() const {
            return this->GetNumFreeBlocks() * this->GetNumPages();
        }

        u64* Initialize(PAddr addr, size_t size, size_t bs, size_t nbs, u64* bit_storage) {
            // Set shifts.
            m_block_shift = bs;
            m_next_block_shift = nbs;

            // Align up the address.
            PAddr end = addr + size;
            const size_t align = (m_next_block_shift != 0) ? (u64(1) << m_next_block_shift)
                                                           : (u64(1) << m_block_shift);
            addr = Common::AlignDown(addr, align);
            end = Common::AlignUp(end, align);

            m_heap_address = addr;
            m_end_offset = (end - addr) / (u64(1) << m_block_shift);
            return m_bitmap.Initialize(bit_storage, m_end_offset);
        }

        PAddr PushBlock(PAddr address) {
            // Set the bit for the free block.
            size_t offset = (address - m_heap_address) >> this->GetShift();
            m_bitmap.SetBit(offset);

            // If we have a next shift, try to clear the blocks below this one and return the new
            // address.
            if (this->GetNextShift()) {
                const size_t diff = u64(1) << (this->GetNextShift() - this->GetShift());
                offset = Common::AlignDown(offset, diff);
                if (m_bitmap.ClearRange(offset, diff)) {
                    return m_heap_address + (offset << this->GetShift());
                }
            }

            // We couldn't coalesce, or we're already as big as possible.
            return {};
        }

        PAddr PopBlock(bool random) {
            // Find a free block.
            s64 soffset = m_bitmap.FindFreeBlock(random);
            if (soffset < 0) {
                return {};
            }
            const size_t offset = static_cast<size_t>(soffset);

            // Update our tracking and return it.
            m_bitmap.ClearBit(offset);
            return m_heap_address + (offset << this->GetShift());
        }

    public:
        static constexpr size_t CalculateManagementOverheadSize(size_t region_size,
                                                                size_t cur_block_shift,
                                                                size_t next_block_shift) {
            const size_t cur_block_size = (u64(1) << cur_block_shift);
            const size_t next_block_size = (u64(1) << next_block_shift);
            const size_t align = (next_block_shift != 0) ? next_block_size : cur_block_size;
            return KPageBitmap::CalculateManagementOverheadSize(
                (align * 2 + Common::AlignUp(region_size, align)) / cur_block_size);
        }

    private:
        KPageBitmap m_bitmap;
        PAddr m_heap_address{};
        uintptr_t m_end_offset{};
        size_t m_block_shift{};
        size_t m_next_block_shift{};
    };

private:
    void Initialize(PAddr heap_address, size_t heap_size, VAddr management_address,
                    size_t management_size, const size_t* block_shifts, size_t num_block_shifts);
    size_t GetNumFreePages() const;

    void FreeBlock(PAddr block, s32 index);

    static constexpr size_t NumMemoryBlockPageShifts{7};
    static constexpr std::array<size_t, NumMemoryBlockPageShifts> MemoryBlockPageShifts{
        0xC, 0x10, 0x15, 0x16, 0x19, 0x1D, 0x1E,
    };

private:
    static size_t CalculateManagementOverheadSize(size_t region_size, const size_t* block_shifts,
                                                  size_t num_block_shifts);

private:
    PAddr m_heap_address{};
    size_t m_heap_size{};
    size_t m_initial_used_size{};
    size_t m_num_blocks{};
    std::array<Block, NumMemoryBlockPageShifts> m_blocks{};
    std::vector<u64> m_management_data;
};

} // namespace Kernel

// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/hle/kernel/k_page_heap.h"

namespace Kernel {

void KPageHeap::Initialize(PAddr address, size_t size, VAddr management_address,
                           size_t management_size, const size_t* block_shifts,
                           size_t num_block_shifts) {
    // Check our assumptions.
    ASSERT(Common::IsAligned(address, PageSize));
    ASSERT(Common::IsAligned(size, PageSize));
    ASSERT(0 < num_block_shifts && num_block_shifts <= NumMemoryBlockPageShifts);
    const VAddr management_end = management_address + management_size;

    // Set our members.
    m_heap_address = address;
    m_heap_size = size;
    m_num_blocks = num_block_shifts;

    // Setup bitmaps.
    m_management_data.resize(management_size / sizeof(u64));
    u64* cur_bitmap_storage{m_management_data.data()};
    for (size_t i = 0; i < num_block_shifts; i++) {
        const size_t cur_block_shift = block_shifts[i];
        const size_t next_block_shift = (i != num_block_shifts - 1) ? block_shifts[i + 1] : 0;
        cur_bitmap_storage = m_blocks[i].Initialize(m_heap_address, m_heap_size, cur_block_shift,
                                                    next_block_shift, cur_bitmap_storage);
    }

    // Ensure we didn't overextend our bounds.
    ASSERT(VAddr(cur_bitmap_storage) <= management_end);
}

size_t KPageHeap::GetNumFreePages() const {
    size_t num_free = 0;

    for (size_t i = 0; i < m_num_blocks; i++) {
        num_free += m_blocks[i].GetNumFreePages();
    }

    return num_free;
}

PAddr KPageHeap::AllocateByLinearSearch(s32 index) {
    const size_t needed_size = m_blocks[index].GetSize();

    for (s32 i = index; i < static_cast<s32>(m_num_blocks); i++) {
        if (const PAddr addr = m_blocks[i].PopBlock(false); addr != 0) {
            if (const size_t allocated_size = m_blocks[i].GetSize(); allocated_size > needed_size) {
                this->Free(addr + needed_size, (allocated_size - needed_size) / PageSize);
            }
            return addr;
        }
    }

    return 0;
}

PAddr KPageHeap::AllocateByRandom(s32 index, size_t num_pages, size_t align_pages) {
    // Get the size and required alignment.
    const size_t needed_size = num_pages * PageSize;
    const size_t align_size = align_pages * PageSize;

    // Determine meta-alignment of our desired alignment size.
    const size_t align_shift = std::countr_zero(align_size);

    // Decide on a block to allocate from.
    constexpr size_t MinimumPossibleAlignmentsForRandomAllocation = 4;
    {
        // By default, we'll want to look at all blocks larger than our current one.
        s32 max_blocks = static_cast<s32>(m_num_blocks);

        // Determine the maximum block we should try to allocate from.
        size_t possible_alignments = 0;
        for (s32 i = index; i < max_blocks; ++i) {
            // Add the possible alignments from blocks at the current size.
            possible_alignments += (1 + ((m_blocks[i].GetSize() - needed_size) >> align_shift)) *
                                   m_blocks[i].GetNumFreeBlocks();

            // If there are enough possible alignments, we don't need to look at larger blocks.
            if (possible_alignments >= MinimumPossibleAlignmentsForRandomAllocation) {
                max_blocks = i + 1;
                break;
            }
        }

        // If we have any possible alignments which require a larger block, we need to pick one.
        if (possible_alignments > 0 && index + 1 < max_blocks) {
            // Select a random alignment from the possibilities.
            const size_t rnd = m_rng.GenerateRandom(possible_alignments);

            // Determine which block corresponds to the random alignment we chose.
            possible_alignments = 0;
            for (s32 i = index; i < max_blocks; ++i) {
                // Add the possible alignments from blocks at the current size.
                possible_alignments +=
                    (1 + ((m_blocks[i].GetSize() - needed_size) >> align_shift)) *
                    m_blocks[i].GetNumFreeBlocks();

                // If the current block gets us to our random choice, use the current block.
                if (rnd < possible_alignments) {
                    index = i;
                    break;
                }
            }
        }
    }

    // Pop a block from the index we selected.
    if (PAddr addr = m_blocks[index].PopBlock(true); addr != 0) {
        // Determine how much size we have left over.
        if (const size_t leftover_size = m_blocks[index].GetSize() - needed_size;
            leftover_size > 0) {
            // Determine how many valid alignments we can have.
            const size_t possible_alignments = 1 + (leftover_size >> align_shift);

            // Select a random valid alignment.
            const size_t random_offset = m_rng.GenerateRandom(possible_alignments) << align_shift;

            // Free memory before the random offset.
            if (random_offset != 0) {
                this->Free(addr, random_offset / PageSize);
            }

            // Advance our block by the random offset.
            addr += random_offset;

            // Free memory after our allocated block.
            if (random_offset != leftover_size) {
                this->Free(addr + needed_size, (leftover_size - random_offset) / PageSize);
            }
        }

        // Return the block we allocated.
        return addr;
    }

    return 0;
}

void KPageHeap::FreeBlock(PAddr block, s32 index) {
    do {
        block = m_blocks[index++].PushBlock(block);
    } while (block != 0);
}

void KPageHeap::Free(PAddr addr, size_t num_pages) {
    // Freeing no pages is a no-op.
    if (num_pages == 0) {
        return;
    }

    // Find the largest block size that we can free, and free as many as possible.
    s32 big_index = static_cast<s32>(m_num_blocks) - 1;
    const PAddr start = addr;
    const PAddr end = addr + num_pages * PageSize;
    PAddr before_start = start;
    PAddr before_end = start;
    PAddr after_start = end;
    PAddr after_end = end;
    while (big_index >= 0) {
        const size_t block_size = m_blocks[big_index].GetSize();
        const PAddr big_start = Common::AlignUp(start, block_size);
        const PAddr big_end = Common::AlignDown(end, block_size);
        if (big_start < big_end) {
            // Free as many big blocks as we can.
            for (auto block = big_start; block < big_end; block += block_size) {
                this->FreeBlock(block, big_index);
            }
            before_end = big_start;
            after_start = big_end;
            break;
        }
        big_index--;
    }
    ASSERT(big_index >= 0);

    // Free space before the big blocks.
    for (s32 i = big_index - 1; i >= 0; i--) {
        const size_t block_size = m_blocks[i].GetSize();
        while (before_start + block_size <= before_end) {
            before_end -= block_size;
            this->FreeBlock(before_end, i);
        }
    }

    // Free space after the big blocks.
    for (s32 i = big_index - 1; i >= 0; i--) {
        const size_t block_size = m_blocks[i].GetSize();
        while (after_start + block_size <= after_end) {
            this->FreeBlock(after_start, i);
            after_start += block_size;
        }
    }
}

size_t KPageHeap::CalculateManagementOverheadSize(size_t region_size, const size_t* block_shifts,
                                                  size_t num_block_shifts) {
    size_t overhead_size = 0;
    for (size_t i = 0; i < num_block_shifts; i++) {
        const size_t cur_block_shift = block_shifts[i];
        const size_t next_block_shift = (i != num_block_shifts - 1) ? block_shifts[i + 1] : 0;
        overhead_size += KPageHeap::Block::CalculateManagementOverheadSize(
            region_size, cur_block_shift, next_block_shift);
    }
    return Common::AlignUp(overhead_size, PageSize);
}

} // namespace Kernel

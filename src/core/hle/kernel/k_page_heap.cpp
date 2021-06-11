// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "core/hle/kernel/k_page_heap.h"
#include "core/memory.h"

namespace Kernel {

void KPageHeap::Initialize(VAddr address, std::size_t size, std::size_t metadata_size) {
    // Check our assumptions
    ASSERT(Common::IsAligned((address), PageSize));
    ASSERT(Common::IsAligned(size, PageSize));

    // Set our members
    heap_address = address;
    heap_size = size;

    // Setup bitmaps
    metadata.resize(metadata_size / sizeof(u64));
    u64* cur_bitmap_storage{metadata.data()};
    for (std::size_t i = 0; i < MemoryBlockPageShifts.size(); i++) {
        const std::size_t cur_block_shift{MemoryBlockPageShifts[i]};
        const std::size_t next_block_shift{
            (i != MemoryBlockPageShifts.size() - 1) ? MemoryBlockPageShifts[i + 1] : 0};
        cur_bitmap_storage = blocks[i].Initialize(heap_address, heap_size, cur_block_shift,
                                                  next_block_shift, cur_bitmap_storage);
    }
}

VAddr KPageHeap::AllocateBlock(s32 index, bool random) {
    const std::size_t needed_size{blocks[index].GetSize()};

    for (s32 i{index}; i < static_cast<s32>(MemoryBlockPageShifts.size()); i++) {
        if (const VAddr addr{blocks[i].PopBlock(random)}; addr) {
            if (const std::size_t allocated_size{blocks[i].GetSize()};
                allocated_size > needed_size) {
                Free(addr + needed_size, (allocated_size - needed_size) / PageSize);
            }
            return addr;
        }
    }

    return 0;
}

void KPageHeap::FreeBlock(VAddr block, s32 index) {
    do {
        block = blocks[index++].PushBlock(block);
    } while (block != 0);
}

void KPageHeap::Free(VAddr addr, std::size_t num_pages) {
    // Freeing no pages is a no-op
    if (num_pages == 0) {
        return;
    }

    // Find the largest block size that we can free, and free as many as possible
    s32 big_index{static_cast<s32>(MemoryBlockPageShifts.size()) - 1};
    const VAddr start{addr};
    const VAddr end{(num_pages * PageSize) + addr};
    VAddr before_start{start};
    VAddr before_end{start};
    VAddr after_start{end};
    VAddr after_end{end};
    while (big_index >= 0) {
        const std::size_t block_size{blocks[big_index].GetSize()};
        const VAddr big_start{Common::AlignUp((start), block_size)};
        const VAddr big_end{Common::AlignDown((end), block_size)};
        if (big_start < big_end) {
            // Free as many big blocks as we can
            for (auto block{big_start}; block < big_end; block += block_size) {
                FreeBlock(block, big_index);
            }
            before_end = big_start;
            after_start = big_end;
            break;
        }
        big_index--;
    }
    ASSERT(big_index >= 0);

    // Free space before the big blocks
    for (s32 i{big_index - 1}; i >= 0; i--) {
        const std::size_t block_size{blocks[i].GetSize()};
        while (before_start + block_size <= before_end) {
            before_end -= block_size;
            FreeBlock(before_end, i);
        }
    }

    // Free space after the big blocks
    for (s32 i{big_index - 1}; i >= 0; i--) {
        const std::size_t block_size{blocks[i].GetSize()};
        while (after_start + block_size <= after_end) {
            FreeBlock(after_start, i);
            after_start += block_size;
        }
    }
}

std::size_t KPageHeap::CalculateManagementOverheadSize(std::size_t region_size) {
    std::size_t overhead_size = 0;
    for (std::size_t i = 0; i < MemoryBlockPageShifts.size(); i++) {
        const std::size_t cur_block_shift{MemoryBlockPageShifts[i]};
        const std::size_t next_block_shift{
            (i != MemoryBlockPageShifts.size() - 1) ? MemoryBlockPageShifts[i + 1] : 0};
        overhead_size += KPageHeap::Block::CalculateManagementOverheadSize(
            region_size, cur_block_shift, next_block_shift);
    }
    return Common::AlignUp(overhead_size, PageSize);
}

} // namespace Kernel

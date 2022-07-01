// SPDX-License-Identifier: GPLv3 or later
// Copyright © 2021 Skyline Team and Contributors (https://github.com/skyline-emu/)

#include "common/address_space.h"
#include "common/assert.h"

#define MAP_MEMBER(returnType)                                                                     \
    template <typename VaType, VaType UnmappedVa, typename PaType, PaType UnmappedPa,              \
              bool PaContigSplit, size_t AddressSpaceBits, typename ExtraBlockInfo>                \
    requires AddressSpaceValid<VaType, AddressSpaceBits> returnType FlatAddressSpaceMap<           \
        VaType, UnmappedVa, PaType, UnmappedPa, PaContigSplit, AddressSpaceBits, ExtraBlockInfo>
#define MAP_MEMBER_CONST()                                                                         \
    template <typename VaType, VaType UnmappedVa, typename PaType, PaType UnmappedPa,              \
              bool PaContigSplit, size_t AddressSpaceBits, typename ExtraBlockInfo>                \
    requires AddressSpaceValid<VaType, AddressSpaceBits> FlatAddressSpaceMap<                      \
        VaType, UnmappedVa, PaType, UnmappedPa, PaContigSplit, AddressSpaceBits, ExtraBlockInfo>

#define MM_MEMBER(returnType)                                                                      \
    template <typename VaType, VaType UnmappedVa, size_t AddressSpaceBits>                         \
    requires AddressSpaceValid<VaType, AddressSpaceBits> returnType                                \
        FlatMemoryManager<VaType, UnmappedVa, AddressSpaceBits>

#define ALLOC_MEMBER(returnType)                                                                   \
    template <typename VaType, VaType UnmappedVa, size_t AddressSpaceBits>                         \
    requires AddressSpaceValid<VaType, AddressSpaceBits> returnType                                \
        FlatAllocator<VaType, UnmappedVa, AddressSpaceBits>
#define ALLOC_MEMBER_CONST()                                                                       \
    template <typename VaType, VaType UnmappedVa, size_t AddressSpaceBits>                         \
    requires AddressSpaceValid<VaType, AddressSpaceBits>                                           \
        FlatAllocator<VaType, UnmappedVa, AddressSpaceBits>

namespace Common {
MAP_MEMBER_CONST()::FlatAddressSpaceMap(VaType vaLimit_,
                                        std::function<void(VaType, VaType)> unmapCallback_)
    : unmapCallback(std::move(unmapCallback_)), vaLimit(vaLimit_) {
    if (vaLimit > VaMaximum)
        UNREACHABLE_MSG("Invalid VA limit!");
}

MAP_MEMBER(void)::MapLocked(VaType virt, PaType phys, VaType size, ExtraBlockInfo extraInfo) {
    VaType virtEnd{virt + size};

    if (virtEnd > vaLimit)
        UNREACHABLE_MSG("Trying to map a block past the VA limit: virtEnd: 0x{:X}, vaLimit: 0x{:X}",
                        virtEnd, vaLimit);

    auto blockEndSuccessor{std::lower_bound(blocks.begin(), blocks.end(), virtEnd)};
    if (blockEndSuccessor == blocks.begin())
        UNREACHABLE_MSG("Trying to map a block before the VA start: virtEnd: 0x{:X}", virtEnd);

    auto blockEndPredecessor{std::prev(blockEndSuccessor)};

    if (blockEndSuccessor != blocks.end()) {
        // We have blocks in front of us, if one is directly in front then we don't have to add a
        // tail
        if (blockEndSuccessor->virt != virtEnd) {
            PaType tailPhys{[&]() -> PaType {
                if constexpr (!PaContigSplit) {
                    return blockEndPredecessor
                        ->phys; // Always propagate unmapped regions rather than calculating offset
                } else {
                    if (blockEndPredecessor->Unmapped())
                        return blockEndPredecessor->phys; // Always propagate unmapped regions
                                                          // rather than calculating offset
                    else
                        return blockEndPredecessor->phys + virtEnd - blockEndPredecessor->virt;
                }
            }()};

            if (blockEndPredecessor->virt >= virt) {
                // If this block's start would be overlapped by the map then reuse it as a tail
                // block
                blockEndPredecessor->virt = virtEnd;
                blockEndPredecessor->phys = tailPhys;
                blockEndPredecessor->extraInfo = blockEndPredecessor->extraInfo;

                // No longer predecessor anymore
                blockEndSuccessor = blockEndPredecessor--;
            } else {
                // Else insert a new one and we're done
                blocks.insert(blockEndSuccessor,
                              {Block(virt, phys, extraInfo),
                               Block(virtEnd, tailPhys, blockEndPredecessor->extraInfo)});
                if (unmapCallback)
                    unmapCallback(virt, size);

                return;
            }
        }
    } else {
        // blockEndPredecessor will always be unmapped as blocks has to be terminated by an unmapped
        // chunk
        if (blockEndPredecessor != blocks.begin() && blockEndPredecessor->virt >= virt) {
            // Move the unmapped block start backwards
            blockEndPredecessor->virt = virtEnd;

            // No longer predecessor anymore
            blockEndSuccessor = blockEndPredecessor--;
        } else {
            // Else insert a new one and we're done
            blocks.insert(blockEndSuccessor,
                          {Block(virt, phys, extraInfo), Block(virtEnd, UnmappedPa, {})});
            if (unmapCallback)
                unmapCallback(virt, size);

            return;
        }
    }

    auto blockStartSuccessor{blockEndSuccessor};

    // Walk the block vector to find the start successor as this is more efficient than another
    // binary search in most scenarios
    while (std::prev(blockStartSuccessor)->virt >= virt)
        blockStartSuccessor--;

    // Check that the start successor is either the end block or something in between
    if (blockStartSuccessor->virt > virtEnd) {
        UNREACHABLE_MSG("Unsorted block in AS map: virt: 0x{:X}", blockStartSuccessor->virt);
    } else if (blockStartSuccessor->virt == virtEnd) {
        // We need to create a new block as there are none spare that we would overwrite
        blocks.insert(blockStartSuccessor, Block(virt, phys, extraInfo));
    } else {
        // Erase overwritten blocks
        if (auto eraseStart{std::next(blockStartSuccessor)}; eraseStart != blockEndSuccessor)
            blocks.erase(eraseStart, blockEndSuccessor);

        // Reuse a block that would otherwise be overwritten as a start block
        blockStartSuccessor->virt = virt;
        blockStartSuccessor->phys = phys;
        blockStartSuccessor->extraInfo = extraInfo;
    }

    if (unmapCallback)
        unmapCallback(virt, size);
}

MAP_MEMBER(void)::UnmapLocked(VaType virt, VaType size) {
    VaType virtEnd{virt + size};

    if (virtEnd > vaLimit)
        UNREACHABLE_MSG("Trying to map a block past the VA limit: virtEnd: 0x{:X}, vaLimit: 0x{:X}",
                        virtEnd, vaLimit);

    auto blockEndSuccessor{std::lower_bound(blocks.begin(), blocks.end(), virtEnd)};
    if (blockEndSuccessor == blocks.begin())
        UNREACHABLE_MSG("Trying to unmap a block before the VA start: virtEnd: 0x{:X}", virtEnd);

    auto blockEndPredecessor{std::prev(blockEndSuccessor)};

    auto walkBackToPredecessor{[&](auto iter) {
        while (iter->virt >= virt)
            iter--;

        return iter;
    }};

    auto eraseBlocksWithEndUnmapped{[&](auto unmappedEnd) {
        auto blockStartPredecessor{walkBackToPredecessor(unmappedEnd)};
        auto blockStartSuccessor{std::next(blockStartPredecessor)};

        auto eraseEnd{[&]() {
            if (blockStartPredecessor->Unmapped()) {
                // If the start predecessor is unmapped then we can erase everything in our region
                // and be done
                return std::next(unmappedEnd);
            } else {
                // Else reuse the end predecessor as the start of our unmapped region then erase all
                // up to it
                unmappedEnd->virt = virt;
                return unmappedEnd;
            }
        }()};

        // We can't have two unmapped regions after each other
        if (eraseEnd != blocks.end() &&
            (eraseEnd == blockStartSuccessor ||
             (blockStartPredecessor->Unmapped() && eraseEnd->Unmapped())))
            UNREACHABLE_MSG("Multiple contiguous unmapped regions are unsupported!");

        blocks.erase(blockStartSuccessor, eraseEnd);
    }};

    // We can avoid any splitting logic if these are the case
    if (blockEndPredecessor->Unmapped()) {
        if (blockEndPredecessor->virt > virt)
            eraseBlocksWithEndUnmapped(blockEndPredecessor);

        if (unmapCallback)
            unmapCallback(virt, size);

        return; // The region is unmapped, bail out early
    } else if (blockEndSuccessor->virt == virtEnd && blockEndSuccessor->Unmapped()) {
        eraseBlocksWithEndUnmapped(blockEndSuccessor);

        if (unmapCallback)
            unmapCallback(virt, size);

        return; // The region is unmapped here and doesn't need splitting, bail out early
    } else if (blockEndSuccessor == blocks.end()) {
        // This should never happen as the end should always follow an unmapped block
        UNREACHABLE_MSG("Unexpected Memory Manager state!");
    } else if (blockEndSuccessor->virt != virtEnd) {
        // If one block is directly in front then we don't have to add a tail

        // The previous block is mapped so we will need to add a tail with an offset
        PaType tailPhys{[&]() {
            if constexpr (PaContigSplit)
                return blockEndPredecessor->phys + virtEnd - blockEndPredecessor->virt;
            else
                return blockEndPredecessor->phys;
        }()};

        if (blockEndPredecessor->virt >= virt) {
            // If this block's start would be overlapped by the unmap then reuse it as a tail block
            blockEndPredecessor->virt = virtEnd;
            blockEndPredecessor->phys = tailPhys;

            // No longer predecessor anymore
            blockEndSuccessor = blockEndPredecessor--;
        } else {
            blocks.insert(blockEndSuccessor,
                          {Block(virt, UnmappedPa, {}),
                           Block(virtEnd, tailPhys, blockEndPredecessor->extraInfo)});
            if (unmapCallback)
                unmapCallback(virt, size);

            return; // The previous block is mapped and ends before
        }
    }

    // Walk the block vector to find the start predecessor as this is more efficient than another
    // binary search in most scenarios
    auto blockStartPredecessor{walkBackToPredecessor(blockEndSuccessor)};
    auto blockStartSuccessor{std::next(blockStartPredecessor)};

    if (blockStartSuccessor->virt > virtEnd) {
        UNREACHABLE_MSG("Unsorted block in AS map: virt: 0x{:X}", blockStartSuccessor->virt);
    } else if (blockStartSuccessor->virt == virtEnd) {
        // There are no blocks between the start and the end that would let us skip inserting a new
        // one for head

        // The previous block is may be unmapped, if so we don't need to insert any unmaps after it
        if (blockStartPredecessor->Mapped())
            blocks.insert(blockStartSuccessor, Block(virt, UnmappedPa, {}));
    } else if (blockStartPredecessor->Unmapped()) {
        // If the previous block is unmapped
        blocks.erase(blockStartSuccessor, blockEndPredecessor);
    } else {
        // Erase overwritten blocks, skipping the first one as we have written the unmapped start
        // block there
        if (auto eraseStart{std::next(blockStartSuccessor)}; eraseStart != blockEndSuccessor)
            blocks.erase(eraseStart, blockEndSuccessor);

        // Add in the unmapped block header
        blockStartSuccessor->virt = virt;
        blockStartSuccessor->phys = UnmappedPa;
    }

    if (unmapCallback)
        unmapCallback(virt, size);
}

ALLOC_MEMBER_CONST()::FlatAllocator(VaType vaStart_, VaType vaLimit_)
    : Base(vaLimit_), currentLinearAllocEnd(vaStart_), vaStart(vaStart_) {}

ALLOC_MEMBER(VaType)::Allocate(VaType size) {
    std::scoped_lock lock(this->blockMutex);

    VaType allocStart{UnmappedVa};
    VaType allocEnd{currentLinearAllocEnd + size};

    // Avoid searching backwards in the address space if possible
    if (allocEnd >= currentLinearAllocEnd && allocEnd <= this->vaLimit) {
        auto allocEndSuccessor{
            std::lower_bound(this->blocks.begin(), this->blocks.end(), allocEnd)};
        if (allocEndSuccessor == this->blocks.begin())
            UNREACHABLE_MSG("First block in AS map is invalid!");

        auto allocEndPredecessor{std::prev(allocEndSuccessor)};
        if (allocEndPredecessor->virt <= currentLinearAllocEnd) {
            allocStart = currentLinearAllocEnd;
        } else {
            // Skip over fixed any mappings in front of us
            while (allocEndSuccessor != this->blocks.end()) {
                if (allocEndSuccessor->virt - allocEndPredecessor->virt < size ||
                    allocEndPredecessor->Mapped()) {
                    allocStart = allocEndPredecessor->virt;
                    break;
                }

                allocEndPredecessor = allocEndSuccessor++;

                // Use the VA limit to calculate if we can fit in the final block since it has no
                // successor
                if (allocEndSuccessor == this->blocks.end()) {
                    allocEnd = allocEndPredecessor->virt + size;

                    if (allocEnd >= allocEndPredecessor->virt && allocEnd <= this->vaLimit)
                        allocStart = allocEndPredecessor->virt;
                }
            }
        }
    }

    if (allocStart != UnmappedVa) {
        currentLinearAllocEnd = allocStart + size;
    } else { // If linear allocation overflows the AS then find a gap
        if (this->blocks.size() <= 2)
            UNREACHABLE_MSG("Unexpected allocator state!");

        auto searchPredecessor{this->blocks.begin()};
        auto searchSuccessor{std::next(searchPredecessor)};

        while (searchSuccessor != this->blocks.end() &&
               (searchSuccessor->virt - searchPredecessor->virt < size ||
                searchPredecessor->Mapped())) {
            searchPredecessor = searchSuccessor++;
        }

        if (searchSuccessor != this->blocks.end())
            allocStart = searchPredecessor->virt;
        else
            return {}; // AS is full
    }

    this->MapLocked(allocStart, true, size, {});
    return allocStart;
}

ALLOC_MEMBER(void)::AllocateFixed(VaType virt, VaType size) {
    this->Map(virt, true, size);
}

ALLOC_MEMBER(void)::Free(VaType virt, VaType size) {
    this->Unmap(virt, size);
}
} // namespace Common

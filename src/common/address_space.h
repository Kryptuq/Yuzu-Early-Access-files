// Copyright © 2021 Skyline Team and Contributors (https://github.com/skyline-emu/)
// Licensed under GPLv3 or any later version
// Refer to the license.txt file included.

#pragma once

#include <concepts>
#include <functional>
#include <mutex>
#include <vector>

#include "common/common_types.h"

namespace Common {
template <typename VaType, size_t AddressSpaceBits>
concept AddressSpaceValid = std::is_unsigned_v<VaType> && sizeof(VaType) * 8 >= AddressSpaceBits;

struct EmptyStruct {};

/**
 * @brief FlatAddressSpaceMap provides a generic VA->PA mapping implementation using a sorted vector
 */
template <typename VaType, VaType UnmappedVa, typename PaType, PaType UnmappedPa,
          bool PaContigSplit, size_t AddressSpaceBits, typename ExtraBlockInfo = EmptyStruct>
requires AddressSpaceValid<VaType, AddressSpaceBits>
class FlatAddressSpaceMap {
private:
    std::function<void(VaType, VaType)>
        unmapCallback{}; //!< Callback called when the mappings in an region have changed

protected:
    /**
     * @brief Represents a block of memory in the AS, the physical mapping is contiguous until
     * another block with a different phys address is hit
     */
    struct Block {
        VaType virt{UnmappedVa}; //!< VA of the block
        PaType phys{UnmappedPa}; //!< PA of the block, will increase 1-1 with VA until a new block
                                 //!< is encountered
        [[no_unique_address]] ExtraBlockInfo extraInfo;

        Block() = default;

        Block(VaType virt_, PaType phys_, ExtraBlockInfo extraInfo_)
            : virt(virt_), phys(phys_), extraInfo(extraInfo_) {}

        constexpr bool Valid() {
            return virt != UnmappedVa;
        }

        constexpr bool Mapped() {
            return phys != UnmappedPa;
        }

        constexpr bool Unmapped() {
            return phys == UnmappedPa;
        }

        bool operator<(const VaType& pVirt) const {
            return virt < pVirt;
        }
    };

    std::mutex blockMutex;
    std::vector<Block> blocks{Block{}};

    /**
     * @brief Maps a PA range into the given AS region
     * @note blockMutex MUST be locked when calling this
     */
    void MapLocked(VaType virt, PaType phys, VaType size, ExtraBlockInfo extraInfo);

    /**
     * @brief Unmaps the given range and merges it with other unmapped regions
     * @note blockMutex MUST be locked when calling this
     */
    void UnmapLocked(VaType virt, VaType size);

public:
    static constexpr VaType VaMaximum{(1ULL << (AddressSpaceBits - 1)) +
                                      ((1ULL << (AddressSpaceBits - 1)) -
                                       1)}; //!< The maximum VA that this AS can technically reach

    VaType vaLimit{VaMaximum}; //!< A soft limit on the maximum VA of the AS

    FlatAddressSpaceMap(VaType vaLimit, std::function<void(VaType, VaType)> unmapCallback = {});

    FlatAddressSpaceMap() = default;

    void Map(VaType virt, PaType phys, VaType size, ExtraBlockInfo extraInfo = {}) {
        std::scoped_lock lock(blockMutex);
        MapLocked(virt, phys, size, extraInfo);
    }

    void Unmap(VaType virt, VaType size) {
        std::scoped_lock lock(blockMutex);
        UnmapLocked(virt, size);
    }
};

/**
 * @brief FlatMemoryManager specialises FlatAddressSpaceMap to work as an allocator, with an
 * initial, fast linear pass and a subsequent slower pass that iterates until it finds a free block
 */
template <typename VaType, VaType UnmappedVa, size_t AddressSpaceBits>
requires AddressSpaceValid<VaType, AddressSpaceBits>
class FlatAllocator
    : public FlatAddressSpaceMap<VaType, UnmappedVa, bool, false, false, AddressSpaceBits> {
private:
    using Base = FlatAddressSpaceMap<VaType, UnmappedVa, bool, false, false, AddressSpaceBits>;

    VaType currentLinearAllocEnd; //!< The end address for the initial linear allocation pass, once
                                  //!< this reaches the AS limit the slower allocation path will be
                                  //!< used

public:
    VaType vaStart; //!< The base VA of the allocator, no allocations will be below this

    FlatAllocator(VaType vaStart, VaType vaLimit = Base::VaMaximum);

    /**
     * @brief Allocates a region in the AS of the given size and returns its address
     */
    VaType Allocate(VaType size);

    /**
     * @brief Marks the given region in the AS as allocated
     */
    void AllocateFixed(VaType virt, VaType size);

    /**
     * @brief Frees an AS region so it can be used again
     */
    void Free(VaType virt, VaType size);
};
} // namespace Common

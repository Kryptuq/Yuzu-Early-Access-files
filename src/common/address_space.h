// SPDX-FileCopyrightText: 2021 Skyline Team and Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <concepts>
#include <functional>
#include <mutex>
#include <vector>

#include "common/common_types.h"

namespace Common {
template <typename VaType, size_t AddressSpaceBits>
concept AddressSpaceValid = std::is_unsigned_v<VaType> && sizeof(VaType) * 8 >=
AddressSpaceBits;

struct EmptyStruct {};

/**
 * @brief FlatAddressSpaceMap provides a generic VA->PA mapping implementation using a sorted vector
 */
template <typename VaType, VaType UnmappedVa, typename PaType, PaType UnmappedPa,
          bool PaContigSplit, size_t AddressSpaceBits, typename ExtraBlockInfo = EmptyStruct>
    requires AddressSpaceValid<VaType, AddressSpaceBits>
class FlatAddressSpaceMap {
public:
    /// The maximum VA that this AS can technically reach
    static constexpr VaType VaMaximum{(1ULL << (AddressSpaceBits - 1)) +
                                      ((1ULL << (AddressSpaceBits - 1)) - 1)};

    explicit FlatAddressSpaceMap(VaType va_limit,
                                 std::function<void(VaType, VaType)> unmap_callback = {});

    FlatAddressSpaceMap() = default;

    void Map(VaType virt, PaType phys, VaType size, ExtraBlockInfo extra_info = {}) {
        std::scoped_lock lock(block_mutex);
        MapLocked(virt, phys, size, extra_info);
    }

    void Unmap(VaType virt, VaType size) {
        std::scoped_lock lock(block_mutex);
        UnmapLocked(virt, size);
    }

    VaType GetVALimit() const {
        return va_limit;
    }

protected:
    /**
     * @brief Represents a block of memory in the AS, the physical mapping is contiguous until
     * another block with a different phys address is hit
     */
    struct Block {
        /// VA of the block
        VaType virt{UnmappedVa};
        /// PA of the block, will increase 1-1 with VA until a new block is encountered
        PaType phys{UnmappedPa};
        [[no_unique_address]] ExtraBlockInfo extra_info;

        Block() = default;

        Block(VaType virt_, PaType phys_, ExtraBlockInfo extra_info_)
            : virt(virt_), phys(phys_), extra_info(extra_info_) {}

        bool Valid() const {
            return virt != UnmappedVa;
        }

        bool Mapped() const {
            return phys != UnmappedPa;
        }

        bool Unmapped() const {
            return phys == UnmappedPa;
        }

        bool operator<(const VaType& p_virt) const {
            return virt < p_virt;
        }
    };

    /**
     * @brief Maps a PA range into the given AS region
     * @note block_mutex MUST be locked when calling this
     */
    void MapLocked(VaType virt, PaType phys, VaType size, ExtraBlockInfo extra_info);

    /**
     * @brief Unmaps the given range and merges it with other unmapped regions
     * @note block_mutex MUST be locked when calling this
     */
    void UnmapLocked(VaType virt, VaType size);

    std::mutex block_mutex;
    std::vector<Block> blocks{Block{}};

    /// a soft limit on the maximum VA of the AS
    VaType va_limit{VaMaximum};

private:
    /// Callback called when the mappings in an region have changed
    std::function<void(VaType, VaType)> unmap_callback{};
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

public:
    explicit FlatAllocator(VaType virt_start, VaType va_limit = Base::VaMaximum);

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

    VaType GetVAStart() const {
        return virt_start;
    }

private:
    /// The base VA of the allocator, no allocations will be below this
    VaType virt_start;

    /**
     * The end address for the initial linear allocation pass
     * Once this reaches the AS limit the slower allocation path will be used
     */
    VaType current_linear_alloc_end;
};
} // namespace Common

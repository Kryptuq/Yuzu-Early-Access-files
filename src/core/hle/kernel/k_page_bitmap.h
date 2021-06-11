// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <bit>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/bit_util.h"
#include "common/common_types.h"
#include "common/tiny_mt.h"
#include "core/hle/kernel/k_system_control.h"

namespace Kernel {

class KPageBitmap {
private:
    class RandomBitGenerator {
    private:
        Common::TinyMT rng{};
        u32 entropy{};
        u32 bits_available{};

    private:
        void RefreshEntropy() {
            entropy = rng.GenerateRandomU32();
            bits_available = static_cast<u32>(Common::BitSize<decltype(entropy)>());
        }

        bool GenerateRandomBit() {
            if (bits_available == 0) {
                this->RefreshEntropy();
            }

            const bool rnd_bit = (entropy & 1) != 0;
            entropy >>= 1;
            --bits_available;
            return rnd_bit;
        }

    public:
        RandomBitGenerator() {
            rng.Initialize(static_cast<u32>(KSystemControl::GenerateRandomU64()));
        }

        std::size_t SelectRandomBit(u64 bitmap) {
            u64 selected = 0;

            u64 cur_num_bits = Common::BitSize<decltype(bitmap)>() / 2;
            u64 cur_mask = (1ULL << cur_num_bits) - 1;

            while (cur_num_bits) {
                const u64 low = (bitmap >> 0) & cur_mask;
                const u64 high = (bitmap >> cur_num_bits) & cur_mask;

                bool choose_low;
                if (high == 0) {
                    // If only low val is set, choose low.
                    choose_low = true;
                } else if (low == 0) {
                    // If only high val is set, choose high.
                    choose_low = false;
                } else {
                    // If both are set, choose random.
                    choose_low = this->GenerateRandomBit();
                }

                // If we chose low, proceed with low.
                if (choose_low) {
                    bitmap = low;
                    selected += 0;
                } else {
                    bitmap = high;
                    selected += cur_num_bits;
                }

                // Proceed.
                cur_num_bits /= 2;
                cur_mask >>= cur_num_bits;
            }

            return selected;
        }
    };

public:
    static constexpr std::size_t MaxDepth = 4;

private:
    std::array<u64*, MaxDepth> bit_storages{};
    RandomBitGenerator rng{};
    std::size_t num_bits{};
    std::size_t used_depths{};

public:
    KPageBitmap() = default;

    constexpr std::size_t GetNumBits() const {
        return num_bits;
    }
    constexpr s32 GetHighestDepthIndex() const {
        return static_cast<s32>(used_depths) - 1;
    }

    u64* Initialize(u64* storage, std::size_t size) {
        // Initially, everything is un-set.
        num_bits = 0;

        // Calculate the needed bitmap depth.
        used_depths = static_cast<std::size_t>(GetRequiredDepth(size));
        ASSERT(used_depths <= MaxDepth);

        // Set the bitmap pointers.
        for (s32 depth = this->GetHighestDepthIndex(); depth >= 0; depth--) {
            bit_storages[depth] = storage;
            size = Common::AlignUp(size, Common::BitSize<u64>()) / Common::BitSize<u64>();
            storage += size;
        }

        return storage;
    }

    s64 FindFreeBlock(bool random) {
        uintptr_t offset = 0;
        s32 depth = 0;

        if (random) {
            do {
                const u64 v = bit_storages[depth][offset];
                if (v == 0) {
                    // If depth is bigger than zero, then a previous level indicated a block was
                    // free.
                    ASSERT(depth == 0);
                    return -1;
                }
                offset = offset * Common::BitSize<u64>() + rng.SelectRandomBit(v);
                ++depth;
            } while (depth < static_cast<s32>(used_depths));
        } else {
            do {
                const u64 v = bit_storages[depth][offset];
                if (v == 0) {
                    // If depth is bigger than zero, then a previous level indicated a block was
                    // free.
                    ASSERT(depth == 0);
                    return -1;
                }
                offset = offset * Common::BitSize<u64>() + std::countr_zero(v);
                ++depth;
            } while (depth < static_cast<s32>(used_depths));
        }

        return static_cast<s64>(offset);
    }

    void SetBit(std::size_t offset) {
        this->SetBit(this->GetHighestDepthIndex(), offset);
        num_bits++;
    }

    void ClearBit(std::size_t offset) {
        this->ClearBit(this->GetHighestDepthIndex(), offset);
        num_bits--;
    }

    bool ClearRange(std::size_t offset, std::size_t count) {
        s32 depth = this->GetHighestDepthIndex();
        u64* bits = bit_storages[depth];
        std::size_t bit_ind = offset / Common::BitSize<u64>();
        if (count < Common::BitSize<u64>()) {
            const std::size_t shift = offset % Common::BitSize<u64>();
            ASSERT(shift + count <= Common::BitSize<u64>());
            // Check that all the bits are set.
            const u64 mask = ((u64(1) << count) - 1) << shift;
            u64 v = bits[bit_ind];
            if ((v & mask) != mask) {
                return false;
            }

            // Clear the bits.
            v &= ~mask;
            bits[bit_ind] = v;
            if (v == 0) {
                this->ClearBit(depth - 1, bit_ind);
            }
        } else {
            ASSERT(offset % Common::BitSize<u64>() == 0);
            ASSERT(count % Common::BitSize<u64>() == 0);
            // Check that all the bits are set.
            std::size_t remaining = count;
            std::size_t i = 0;
            do {
                if (bits[bit_ind + i++] != ~u64(0)) {
                    return false;
                }
                remaining -= Common::BitSize<u64>();
            } while (remaining > 0);

            // Clear the bits.
            remaining = count;
            i = 0;
            do {
                bits[bit_ind + i] = 0;
                this->ClearBit(depth - 1, bit_ind + i);
                i++;
                remaining -= Common::BitSize<u64>();
            } while (remaining > 0);
        }

        num_bits -= count;
        return true;
    }

private:
    void SetBit(s32 depth, std::size_t offset) {
        while (depth >= 0) {
            std::size_t ind = offset / Common::BitSize<u64>();
            std::size_t which = offset % Common::BitSize<u64>();
            const u64 mask = u64(1) << which;

            u64* bit = std::addressof(bit_storages[depth][ind]);
            u64 v = *bit;
            ASSERT((v & mask) == 0);
            *bit = v | mask;
            if (v) {
                break;
            }
            offset = ind;
            depth--;
        }
    }

    void ClearBit(s32 depth, std::size_t offset) {
        while (depth >= 0) {
            std::size_t ind = offset / Common::BitSize<u64>();
            std::size_t which = offset % Common::BitSize<u64>();
            const u64 mask = u64(1) << which;

            u64* bit = std::addressof(bit_storages[depth][ind]);
            u64 v = *bit;
            ASSERT((v & mask) != 0);
            v &= ~mask;
            *bit = v;
            if (v) {
                break;
            }
            offset = ind;
            depth--;
        }
    }

private:
    static constexpr s32 GetRequiredDepth(std::size_t region_size) {
        s32 depth = 0;
        while (true) {
            region_size /= Common::BitSize<u64>();
            depth++;
            if (region_size == 0) {
                return depth;
            }
        }
    }

public:
    static constexpr std::size_t CalculateManagementOverheadSize(std::size_t region_size) {
        std::size_t overhead_bits = 0;
        for (s32 depth = GetRequiredDepth(region_size) - 1; depth >= 0; depth--) {
            region_size =
                Common::AlignUp(region_size, Common::BitSize<u64>()) / Common::BitSize<u64>();
            overhead_bits += region_size;
        }
        return overhead_bits * sizeof(u64);
    }
};

} // namespace Kernel

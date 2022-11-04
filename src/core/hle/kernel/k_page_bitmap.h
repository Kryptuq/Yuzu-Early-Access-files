// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
public:
    class RandomBitGenerator {
    public:
        RandomBitGenerator() {
            m_rng.Initialize(static_cast<u32>(KSystemControl::GenerateRandomU64()));
        }

        u64 SelectRandomBit(u64 bitmap) {
            u64 selected = 0;

            for (size_t cur_num_bits = Common::BitSize<decltype(bitmap)>() / 2; cur_num_bits != 0;
                 cur_num_bits /= 2) {
                const u64 high = (bitmap >> cur_num_bits);
                const u64 low = (bitmap & (~(UINT64_C(0xFFFFFFFFFFFFFFFF) << cur_num_bits)));

                // Choose high if we have high and (don't have low or select high randomly).
                if (high && (low == 0 || this->GenerateRandomBit())) {
                    bitmap = high;
                    selected += cur_num_bits;
                } else {
                    bitmap = low;
                    selected += 0;
                }
            }

            return selected;
        }

        u64 GenerateRandom(u64 max) {
            // Determine the number of bits we need.
            const u64 bits_needed = 1 + (Common::BitSize<decltype(max)>() - std::countl_zero(max));

            // Generate a random value of the desired bitwidth.
            const u64 rnd = this->GenerateRandomBits(static_cast<u32>(bits_needed));

            // Adjust the value to be in range.
            return rnd - ((rnd / max) * max);
        }

    private:
        void RefreshEntropy() {
            m_entropy = m_rng.GenerateRandomU32();
            m_bits_available = static_cast<u32>(Common::BitSize<decltype(m_entropy)>());
        }

        bool GenerateRandomBit() {
            if (m_bits_available == 0) {
                this->RefreshEntropy();
            }

            const bool rnd_bit = (m_entropy & 1) != 0;
            m_entropy >>= 1;
            --m_bits_available;
            return rnd_bit;
        }

        u64 GenerateRandomBits(u32 num_bits) {
            u64 result = 0;

            // Iteratively add random bits to our result.
            while (num_bits > 0) {
                // Ensure we have random bits to take from.
                if (m_bits_available == 0) {
                    this->RefreshEntropy();
                }

                // Determine how many bits to take this round.
                const auto cur_bits = std::min(num_bits, m_bits_available);

                // Generate mask for our current bits.
                const u64 mask = (static_cast<u64>(1) << cur_bits) - 1;

                // Add bits to output from our entropy.
                result <<= cur_bits;
                result |= (m_entropy & mask);

                // Remove bits from our entropy.
                m_entropy >>= cur_bits;
                m_bits_available -= cur_bits;

                // Advance.
                num_bits -= cur_bits;
            }

            return result;
        }

    private:
        Common::TinyMT m_rng;
        u32 m_entropy{};
        u32 m_bits_available{};
    };

public:
    static constexpr size_t MaxDepth = 4;

public:
    KPageBitmap() = default;

    constexpr size_t GetNumBits() const {
        return m_num_bits;
    }
    constexpr s32 GetHighestDepthIndex() const {
        return static_cast<s32>(m_used_depths) - 1;
    }

    u64* Initialize(u64* storage, size_t size) {
        // Initially, everything is un-set.
        m_num_bits = 0;

        // Calculate the needed bitmap depth.
        m_used_depths = static_cast<size_t>(GetRequiredDepth(size));
        ASSERT(m_used_depths <= MaxDepth);

        // Set the bitmap pointers.
        for (s32 depth = this->GetHighestDepthIndex(); depth >= 0; depth--) {
            m_bit_storages[depth] = storage;
            size = Common::AlignUp(size, Common::BitSize<u64>()) / Common::BitSize<u64>();
            storage += size;
            m_end_storages[depth] = storage;
        }

        return storage;
    }

    s64 FindFreeBlock(bool random) {
        uintptr_t offset = 0;
        s32 depth = 0;

        if (random) {
            do {
                const u64 v = m_bit_storages[depth][offset];
                if (v == 0) {
                    // If depth is bigger than zero, then a previous level indicated a block was
                    // free.
                    ASSERT(depth == 0);
                    return -1;
                }
                offset = offset * Common::BitSize<u64>() + m_rng.SelectRandomBit(v);
                ++depth;
            } while (depth < static_cast<s32>(m_used_depths));
        } else {
            do {
                const u64 v = m_bit_storages[depth][offset];
                if (v == 0) {
                    // If depth is bigger than zero, then a previous level indicated a block was
                    // free.
                    ASSERT(depth == 0);
                    return -1;
                }
                offset = offset * Common::BitSize<u64>() + std::countr_zero(v);
                ++depth;
            } while (depth < static_cast<s32>(m_used_depths));
        }

        return static_cast<s64>(offset);
    }

    s64 FindFreeRange(size_t count) {
        // Check that it is possible to find a range.
        const u64* const storage_start = m_bit_storages[m_used_depths - 1];
        const u64* const storage_end = m_end_storages[m_used_depths - 1];

        // If we don't have a storage to iterate (or want more blocks than fit in a single storage),
        // we can't find a free range.
        if (!(storage_start < storage_end && count <= Common::BitSize<u64>())) {
            return -1;
        }

        // Walk the storages to select a random free range.
        const size_t options_per_storage = std::max<size_t>(Common::BitSize<u64>() / count, 1);
        const size_t num_entries = std::max<size_t>(storage_end - storage_start, 1);

        const u64 free_mask = (static_cast<u64>(1) << count) - 1;

        size_t num_valid_options = 0;
        s64 chosen_offset = -1;
        for (size_t storage_index = 0; storage_index < num_entries; ++storage_index) {
            u64 storage = storage_start[storage_index];
            for (size_t option = 0; option < options_per_storage; ++option) {
                if ((storage & free_mask) == free_mask) {
                    // We've found a new valid option.
                    ++num_valid_options;

                    // Select the Kth valid option with probability 1/K. This leads to an overall
                    // uniform distribution.
                    if (num_valid_options == 1 || m_rng.GenerateRandom(num_valid_options) == 0) {
                        // This is our first option, so select it.
                        chosen_offset = storage_index * Common::BitSize<u64>() + option * count;
                    }
                }
                storage >>= count;
            }
        }

        // Return the random offset we chose.*/
        return chosen_offset;
    }

    void SetBit(size_t offset) {
        this->SetBit(this->GetHighestDepthIndex(), offset);
        m_num_bits++;
    }

    void ClearBit(size_t offset) {
        this->ClearBit(this->GetHighestDepthIndex(), offset);
        m_num_bits--;
    }

    bool ClearRange(size_t offset, size_t count) {
        s32 depth = this->GetHighestDepthIndex();
        u64* bits = m_bit_storages[depth];
        size_t bit_ind = offset / Common::BitSize<u64>();
        if (count < Common::BitSize<u64>()) [[likely]] {
            const size_t shift = offset % Common::BitSize<u64>();
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
            size_t remaining = count;
            size_t i = 0;
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

        m_num_bits -= count;
        return true;
    }

private:
    void SetBit(s32 depth, size_t offset) {
        while (depth >= 0) {
            size_t ind = offset / Common::BitSize<u64>();
            size_t which = offset % Common::BitSize<u64>();
            const u64 mask = u64(1) << which;

            u64* bit = std::addressof(m_bit_storages[depth][ind]);
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

    void ClearBit(s32 depth, size_t offset) {
        while (depth >= 0) {
            size_t ind = offset / Common::BitSize<u64>();
            size_t which = offset % Common::BitSize<u64>();
            const u64 mask = u64(1) << which;

            u64* bit = std::addressof(m_bit_storages[depth][ind]);
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
    static constexpr s32 GetRequiredDepth(size_t region_size) {
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
    static constexpr size_t CalculateManagementOverheadSize(size_t region_size) {
        size_t overhead_bits = 0;
        for (s32 depth = GetRequiredDepth(region_size) - 1; depth >= 0; depth--) {
            region_size =
                Common::AlignUp(region_size, Common::BitSize<u64>()) / Common::BitSize<u64>();
            overhead_bits += region_size;
        }
        return overhead_bits * sizeof(u64);
    }

private:
    std::array<u64*, MaxDepth> m_bit_storages{};
    std::array<u64*, MaxDepth> m_end_storages{};
    RandomBitGenerator m_rng;
    size_t m_num_bits{};
    size_t m_used_depths{};
};

} // namespace Kernel

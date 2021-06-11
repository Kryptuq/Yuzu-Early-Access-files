/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <array>
#include <bit>

#include "common/alignment.h"
#include "common/bit_util.h"
#include "common/common_types.h"

namespace Common {

namespace impl {

template <typename Storage, size_t N>
class BitSet {

public:
    constexpr BitSet() = default;

    constexpr void SetBit(size_t i) {
        this->words[i / FlagsPerWord] |= GetBitMask(i % FlagsPerWord);
    }

    constexpr void ClearBit(size_t i) {
        this->words[i / FlagsPerWord] &= ~GetBitMask(i % FlagsPerWord);
    }

    constexpr size_t CountLeadingZero() const {
        for (size_t i = 0; i < NumWords; i++) {
            if (this->words[i]) {
                return FlagsPerWord * i + CountLeadingZeroImpl(this->words[i]);
            }
        }
        return FlagsPerWord * NumWords;
    }

    constexpr size_t GetNextSet(size_t n) const {
        for (size_t i = (n + 1) / FlagsPerWord; i < NumWords; i++) {
            Storage word = this->words[i];
            if (!IsAligned(n + 1, FlagsPerWord)) {
                word &= GetBitMask(n % FlagsPerWord) - 1;
            }
            if (word) {
                return FlagsPerWord * i + CountLeadingZeroImpl(word);
            }
        }
        return FlagsPerWord * NumWords;
    }

private:
    static_assert(std::is_unsigned_v<Storage>);
    static_assert(sizeof(Storage) <= sizeof(u64));

    static constexpr size_t FlagsPerWord = BitSize<Storage>();
    static constexpr size_t NumWords = AlignUp(N, FlagsPerWord) / FlagsPerWord;

    static constexpr auto CountLeadingZeroImpl(Storage word) {
        return std::countl_zero(static_cast<unsigned long long>(word)) -
               (BitSize<unsigned long long>() - FlagsPerWord);
    }

    static constexpr Storage GetBitMask(size_t bit) {
        return Storage(1) << (FlagsPerWord - 1 - bit);
    }

    std::array<Storage, NumWords> words{};
};

} // namespace impl

template <size_t N>
using BitSet8 = impl::BitSet<u8, N>;

template <size_t N>
using BitSet16 = impl::BitSet<u16, N>;

template <size_t N>
using BitSet32 = impl::BitSet<u32, N>;

template <size_t N>
using BitSet64 = impl::BitSet<u64, N>;

} // namespace Common

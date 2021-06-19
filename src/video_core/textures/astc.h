// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <bit>
#include "common/common_types.h"

namespace Tegra::Texture::ASTC {

enum class IntegerEncoding { JustBits, Quint, Trit };

struct IntegerEncodedValue {
    constexpr IntegerEncodedValue() = default;

    constexpr IntegerEncodedValue(IntegerEncoding encoding_, u32 num_bits_)
        : encoding{encoding_}, num_bits{num_bits_} {}

    constexpr bool MatchesEncoding(const IntegerEncodedValue& other) const {
        return encoding == other.encoding && num_bits == other.num_bits;
    }

    // Returns the number of bits required to encode num_vals values.
    u32 GetBitLength(u32 num_vals) const {
        u32 total_bits = num_bits * num_vals;
        if (encoding == IntegerEncoding::Trit) {
            total_bits += (num_vals * 8 + 4) / 5;
        } else if (encoding == IntegerEncoding::Quint) {
            total_bits += (num_vals * 7 + 2) / 3;
        }
        return total_bits;
    }

    IntegerEncoding encoding{};
    u32 num_bits = 0;
    u32 bit_value = 0;
    union {
        u32 quint_value = 0;
        u32 trit_value;
    };
};

// Returns a new instance of this struct that corresponds to the
// can take no more than mav_value values
constexpr IntegerEncodedValue CreateEncoding(u32 mav_value) {
    while (mav_value > 0) {
        u32 check = mav_value + 1;

        // Is mav_value a power of two?
        if (!(check & (check - 1))) {
            return IntegerEncodedValue(IntegerEncoding::JustBits, std::popcount(mav_value));
        }

        // Is mav_value of the type 3*2^n - 1?
        if ((check % 3 == 0) && !((check / 3) & ((check / 3) - 1))) {
            return IntegerEncodedValue(IntegerEncoding::Trit, std::popcount(check / 3 - 1));
        }

        // Is mav_value of the type 5*2^n - 1?
        if ((check % 5 == 0) && !((check / 5) & ((check / 5) - 1))) {
            return IntegerEncodedValue(IntegerEncoding::Quint, std::popcount(check / 5 - 1));
        }

        // Apparently it can't be represented with a bounded integer sequence...
        // just iterate.
        mav_value--;
    }
    return IntegerEncodedValue(IntegerEncoding::JustBits, 0);
}

constexpr std::array<IntegerEncodedValue, 256> MakeEncodedValues() {
    std::array<IntegerEncodedValue, 256> encodings{};
    for (std::size_t i = 0; i < encodings.size(); ++i) {
        encodings[i] = CreateEncoding(static_cast<u32>(i));
    }
    return encodings;
}

constexpr std::array<IntegerEncodedValue, 256> ASTC_ENCODINGS_VALUES = MakeEncodedValues();

// Replicates low num_bits such that [(to_bit - 1):(to_bit - 1 - from_bit)]
// is the same as [(num_bits - 1):0] and repeats all the way down.
template <typename IntType>
constexpr IntType Replicate(IntType val, u32 num_bits, u32 to_bit) {
    if (num_bits == 0 || to_bit == 0) {
        return 0;
    }
    const IntType v = val & static_cast<IntType>((1 << num_bits) - 1);
    IntType res = v;
    u32 reslen = num_bits;
    while (reslen < to_bit) {
        u32 comp = 0;
        if (num_bits > to_bit - reslen) {
            u32 newshift = to_bit - reslen;
            comp = num_bits - newshift;
            num_bits = newshift;
        }
        res = static_cast<IntType>(res << num_bits);
        res = static_cast<IntType>(res | (v >> comp));
        reslen += num_bits;
    }
    return res;
}

constexpr std::size_t NumReplicateEntries(u32 num_bits) {
    return std::size_t(1) << num_bits;
}

template <typename IntType, u32 num_bits, u32 to_bit>
constexpr auto MakeReplicateTable() {
    std::array<IntType, NumReplicateEntries(num_bits)> table{};
    for (IntType value = 0; value < static_cast<IntType>(std::size(table)); ++value) {
        table[value] = Replicate(value, num_bits, to_bit);
    }
    return table;
}

constexpr auto REPLICATE_6_BIT_TO_8_TABLE = MakeReplicateTable<u32, 6, 8>();
constexpr auto REPLICATE_7_BIT_TO_8_TABLE = MakeReplicateTable<u32, 7, 8>();
constexpr auto REPLICATE_8_BIT_TO_8_TABLE = MakeReplicateTable<u32, 8, 8>();

void Decompress(std::span<const uint8_t> data, uint32_t width, uint32_t height, uint32_t depth,
                uint32_t block_width, uint32_t block_height, std::span<uint8_t> output);

} // namespace Tegra::Texture::ASTC

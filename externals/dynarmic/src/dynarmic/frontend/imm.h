/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "dynarmic/common/assert.h"
#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/common/math_util.h"

namespace Dynarmic {

/**
 * Imm represents an immediate value in an AArch32/AArch64 instruction.
 * Imm is used during translation as a typesafe way of passing around immediates of fixed sizes.
 */
template<size_t bit_size_>
class Imm {
public:
    static constexpr size_t bit_size = bit_size_;

    explicit Imm(u32 value)
            : value(value) {
        ASSERT_MSG((Common::Bits<0, bit_size - 1>(value) == value), "More bits in value than expected");
    }

    template<typename T = u32>
    T ZeroExtend() const {
        static_assert(Common::BitSize<T>() >= bit_size);
        return static_cast<T>(value);
    }

    template<typename T = s32>
    T SignExtend() const {
        static_assert(Common::BitSize<T>() >= bit_size);
        return Common::SignExtend<bit_size, T>(value);
    }

    template<size_t bit>
    bool Bit() const {
        static_assert(bit < bit_size);
        return Common::Bit<bit>(value);
    }

    template<size_t begin_bit, size_t end_bit, typename T = u32>
    T Bits() const {
        static_assert(begin_bit <= end_bit && end_bit < bit_size);
        static_assert(Common::BitSize<T>() >= end_bit - begin_bit + 1);
        return static_cast<T>(Common::Bits<begin_bit, end_bit>(value));
    }

    bool operator==(Imm other) const {
        return value == other.value;
    }

    bool operator!=(Imm other) const {
        return !operator==(other);
    }

    bool operator<(Imm other) const {
        return value < other.value;
    }

    bool operator<=(Imm other) const {
        return value <= other.value;
    }

    bool operator>(Imm other) const {
        return value > other.value;
    }

    bool operator>=(Imm other) const {
        return value >= other.value;
    }

private:
    static_assert(bit_size != 0, "Cannot have a zero-sized immediate");
    static_assert(bit_size <= 32, "Cannot have an immediate larger than the instruction size");

    u32 value;
};

template<size_t bit_size>
bool operator==(u32 a, Imm<bit_size> b) {
    return Imm<bit_size>{a} == b;
}

template<size_t bit_size>
bool operator==(Imm<bit_size> a, u32 b) {
    return Imm<bit_size>{b} == a;
}

template<size_t bit_size>
bool operator!=(u32 a, Imm<bit_size> b) {
    return !operator==(a, b);
}

template<size_t bit_size>
bool operator!=(Imm<bit_size> a, u32 b) {
    return !operator==(a, b);
}

template<size_t bit_size>
bool operator<(u32 a, Imm<bit_size> b) {
    return Imm<bit_size>{a} < b;
}

template<size_t bit_size>
bool operator<(Imm<bit_size> a, u32 b) {
    return a < Imm<bit_size>{b};
}

template<size_t bit_size>
bool operator<=(u32 a, Imm<bit_size> b) {
    return !operator<(b, a);
}

template<size_t bit_size>
bool operator<=(Imm<bit_size> a, u32 b) {
    return !operator<(b, a);
}

template<size_t bit_size>
bool operator>(u32 a, Imm<bit_size> b) {
    return operator<(b, a);
}

template<size_t bit_size>
bool operator>(Imm<bit_size> a, u32 b) {
    return operator<(b, a);
}

template<size_t bit_size>
bool operator>=(u32 a, Imm<bit_size> b) {
    return !operator<(a, b);
}

template<size_t bit_size>
bool operator>=(Imm<bit_size> a, u32 b) {
    return !operator<(a, b);
}

/**
 * Concatenate immediates together.
 * Left to right corresponds to most significant imm to least significant imm.
 * This is equivalent to a:b:...:z in ASL.
 */
template<size_t first_bit_size, size_t... rest_bit_sizes>
auto concatenate(Imm<first_bit_size> first, Imm<rest_bit_sizes>... rest) {
    if constexpr (sizeof...(rest) == 0) {
        return first;
    } else {
        const auto concat_rest = concatenate(rest...);
        const u32 value = (first.ZeroExtend() << concat_rest.bit_size) | concat_rest.ZeroExtend();
        return Imm<Common::Sum(first_bit_size, rest_bit_sizes...)>{value};
    }
}

/// Expands an Advanced SIMD modified immediate.
u64 AdvSIMDExpandImm(bool op, Imm<4> cmode, Imm<8> imm8);

}  // namespace Dynarmic

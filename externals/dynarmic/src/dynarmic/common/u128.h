/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <tuple>
#include <type_traits>

#include <mcl/bit/bit_field.hpp>
#include <mcl/bitsizeof.hpp>
#include <mcl/stdint.hpp>

namespace Dynarmic {

struct u128 {
    u128() = default;
    u128(const u128&) = default;
    u128(u128&&) = default;
    u128& operator=(const u128&) = default;
    u128& operator=(u128&&) = default;

    u128(u64 lower_, u64 upper_)
            : lower(lower_), upper(upper_) {}

    template<typename T>
    /* implicit */ u128(T value)
            : lower(value), upper(0) {
        static_assert(std::is_integral_v<T>);
        static_assert(mcl::bitsizeof<T> <= mcl::bitsizeof<u64>);
    }

    u64 lower = 0;
    u64 upper = 0;

    template<size_t bit_position>
    bool Bit() const {
        static_assert(bit_position < 128);
        if constexpr (bit_position < 64) {
            return mcl::bit::get_bit<bit_position>(lower);
        } else {
            return mcl::bit::get_bit<bit_position - 64>(upper);
        }
    }
};

static_assert(mcl::bitsizeof<u128> == 128);
static_assert(std::is_standard_layout_v<u128>);
static_assert(std::is_trivially_copyable_v<u128>);

u128 Multiply64To128(u64 a, u64 b);

inline u128 operator+(u128 a, u128 b) {
    u128 result;
    result.lower = a.lower + b.lower;
    result.upper = a.upper + b.upper + (a.lower > result.lower);
    return result;
}

inline u128 operator-(u128 a, u128 b) {
    u128 result;
    result.lower = a.lower - b.lower;
    result.upper = a.upper - b.upper - (a.lower < result.lower);
    return result;
}

inline bool operator<(u128 a, u128 b) {
    return std::tie(a.upper, a.lower) < std::tie(b.upper, b.lower);
}

inline bool operator>(u128 a, u128 b) {
    return operator<(b, a);
}

inline bool operator<=(u128 a, u128 b) {
    return !operator>(a, b);
}

inline bool operator>=(u128 a, u128 b) {
    return !operator<(a, b);
}

inline bool operator==(u128 a, u128 b) {
    return std::tie(a.upper, a.lower) == std::tie(b.upper, b.lower);
}

inline bool operator!=(u128 a, u128 b) {
    return !operator==(a, b);
}

u128 operator<<(u128 operand, int amount);
u128 operator>>(u128 operand, int amount);

/// LSB is a "sticky-bit".
/// If a 1 is shifted off, the LSB would be set.
u128 StickyLogicalShiftRight(u128 operand, int amount);

}  // namespace Dynarmic

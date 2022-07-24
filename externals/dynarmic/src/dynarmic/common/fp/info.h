/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mcl/bit/bit_count.hpp>
#include <mcl/stdint.hpp>

namespace Dynarmic::FP {

template<typename FPT>
struct FPInfo {};

template<>
struct FPInfo<u16> {
    static constexpr size_t total_width = 16;
    static constexpr size_t exponent_width = 5;
    static constexpr size_t explicit_mantissa_width = 10;
    static constexpr size_t mantissa_width = explicit_mantissa_width + 1;

    static constexpr u32 implicit_leading_bit = u32(1) << explicit_mantissa_width;
    static constexpr u32 sign_mask = 0x8000;
    static constexpr u32 exponent_mask = 0x7C00;
    static constexpr u32 mantissa_mask = 0x3FF;
    static constexpr u32 mantissa_msb = 0x200;

    static constexpr int exponent_min = -14;
    static constexpr int exponent_max = 15;
    static constexpr int exponent_bias = 15;

    static constexpr u16 Zero(bool sign) {
        return sign ? static_cast<u16>(sign_mask) : u16{0};
    }

    static constexpr u16 Infinity(bool sign) {
        return static_cast<u16>(exponent_mask | Zero(sign));
    }

    static constexpr u16 MaxNormal(bool sign) {
        return static_cast<u16>((exponent_mask - 1) | Zero(sign));
    }

    static constexpr u16 DefaultNaN() {
        return static_cast<u16>(exponent_mask | (u32(1) << (explicit_mantissa_width - 1)));
    }
};

template<>
struct FPInfo<u32> {
    static constexpr size_t total_width = 32;
    static constexpr size_t exponent_width = 8;
    static constexpr size_t explicit_mantissa_width = 23;
    static constexpr size_t mantissa_width = explicit_mantissa_width + 1;

    static constexpr u32 implicit_leading_bit = u32(1) << explicit_mantissa_width;
    static constexpr u32 sign_mask = 0x80000000;
    static constexpr u32 exponent_mask = 0x7F800000;
    static constexpr u32 mantissa_mask = 0x007FFFFF;
    static constexpr u32 mantissa_msb = 0x00400000;

    static constexpr int exponent_min = -126;
    static constexpr int exponent_max = 127;
    static constexpr int exponent_bias = 127;

    static constexpr u32 Zero(bool sign) {
        return sign ? sign_mask : 0;
    }

    static constexpr u32 Infinity(bool sign) {
        return exponent_mask | Zero(sign);
    }

    static constexpr u32 MaxNormal(bool sign) {
        return (exponent_mask - 1) | Zero(sign);
    }

    static constexpr u32 DefaultNaN() {
        return exponent_mask | (u32(1) << (explicit_mantissa_width - 1));
    }
};

template<>
struct FPInfo<u64> {
    static constexpr size_t total_width = 64;
    static constexpr size_t exponent_width = 11;
    static constexpr size_t explicit_mantissa_width = 52;
    static constexpr size_t mantissa_width = explicit_mantissa_width + 1;

    static constexpr u64 implicit_leading_bit = u64(1) << explicit_mantissa_width;
    static constexpr u64 sign_mask = 0x8000'0000'0000'0000;
    static constexpr u64 exponent_mask = 0x7FF0'0000'0000'0000;
    static constexpr u64 mantissa_mask = 0x000F'FFFF'FFFF'FFFF;
    static constexpr u64 mantissa_msb = 0x0008'0000'0000'0000;

    static constexpr int exponent_min = -1022;
    static constexpr int exponent_max = 1023;
    static constexpr int exponent_bias = 1023;

    static constexpr u64 Zero(bool sign) {
        return sign ? sign_mask : 0;
    }

    static constexpr u64 Infinity(bool sign) {
        return exponent_mask | Zero(sign);
    }

    static constexpr u64 MaxNormal(bool sign) {
        return (exponent_mask - 1) | Zero(sign);
    }

    static constexpr u64 DefaultNaN() {
        return exponent_mask | (u64(1) << (explicit_mantissa_width - 1));
    }
};

/// value = (sign ? -1 : +1) * 2^exponent * value
/// @note We do not handle denormals. Denormals will static_assert.
template<typename FPT, bool sign, int exponent, FPT value>
constexpr FPT FPValue() {
    if constexpr (value == 0) {
        return FPInfo<FPT>::Zero(sign);
    }

    constexpr int point_position = static_cast<int>(FPInfo<FPT>::explicit_mantissa_width);
    constexpr int highest_bit = mcl::bit::highest_set_bit(value);
    constexpr int offset = point_position - highest_bit;
    constexpr int normalized_exponent = exponent - offset + point_position;
    static_assert(offset >= 0);
    static_assert(normalized_exponent >= FPInfo<FPT>::exponent_min && normalized_exponent <= FPInfo<FPT>::exponent_max);

    constexpr FPT mantissa = (value << offset) & FPInfo<FPT>::mantissa_mask;
    constexpr FPT biased_exponent = static_cast<FPT>(normalized_exponent + FPInfo<FPT>::exponent_bias);
    return FPT(FPInfo<FPT>::Zero(sign) | mantissa | (biased_exponent << FPInfo<FPT>::explicit_mantissa_width));
}

}  // namespace Dynarmic::FP

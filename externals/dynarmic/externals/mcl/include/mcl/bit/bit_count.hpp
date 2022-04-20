// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <bitset>

#include "mcl/bitsizeof.hpp"
#include "mcl/concepts/bit_integral.hpp"
#include "mcl/stdint.hpp"

namespace mcl::bit {

template<BitIntegral T>
inline size_t count_ones(T x) {
    return std::bitset<bitsizeof<T>>(x).count();
}

template<BitIntegral T>
constexpr size_t count_leading_zeros(T x) {
    size_t result = bitsizeof<T>;
    while (x != 0) {
        x >>= 1;
        result--;
    }
    return result;
}

template<BitIntegral T>
constexpr int highest_set_bit(T x) {
    int result = -1;
    while (x != 0) {
        x >>= 1;
        result++;
    }
    return result;
}

template<BitIntegral T>
constexpr size_t lowest_set_bit(T x) {
    if (x == 0) {
        return bitsizeof<T>;
    }

    size_t result = 0;
    while ((x & 1) == 0) {
        x >>= 1;
        result++;
    }
    return result;
}

}  // namespace mcl::bit

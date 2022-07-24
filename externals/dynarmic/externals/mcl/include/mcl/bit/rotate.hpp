// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/bitsizeof.hpp"
#include "mcl/concepts/bit_integral.hpp"
#include "mcl/stdint.hpp"

namespace mcl::bit {

template<BitIntegral T>
constexpr T rotate_right(T x, size_t amount)
{
    amount %= bitsizeof<T>;
    if (amount == 0) {
        return x;
    }
    return static_cast<T>((x >> amount) | (x << (bitsizeof<T> - amount)));
}

template<BitIntegral T>
constexpr T rotate_left(T x, size_t amount)
{
    amount %= bitsizeof<T>;
    if (amount == 0) {
        return x;
    }
    return static_cast<T>((x << amount) | (x >> (bitsizeof<T> - amount)));
}

}  // namespace mcl::bit

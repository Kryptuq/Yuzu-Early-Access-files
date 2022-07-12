// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

// Reference: http://jonkagstrom.com/bit-mixer-construction/

#pragma once

#include <functional>

#include "mcl/bit/rotate.hpp"
#include "mcl/stdint.hpp"

namespace mcl::hash {

constexpr size_t xmrx(size_t x)
{
    x ^= x >> 32;
    x *= 0xff51afd7ed558ccd;
    x ^= bit::rotate_right(x, 47) ^ bit::rotate_right(x, 23);
    return x;
}

template<typename T>
struct avalanche_xmrx {
    size_t operator()(const T& value)
    {
        return xmrx(std::hash<T>{}(value));
    }
};

}  // namespace mcl::hash

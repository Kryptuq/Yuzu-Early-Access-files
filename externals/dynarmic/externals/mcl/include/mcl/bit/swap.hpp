// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/concepts/bit_integral.hpp"

namespace mcl::bit {

constexpr u16 swap_bytes_16(u16 value)
{
    return static_cast<u16>(u32{value} >> 8 | u32{value} << 8);
}

constexpr u32 swap_bytes_32(u32 value)
{
    return ((value & 0xff000000u) >> 24)
         | ((value & 0x00ff0000u) >> 8)
         | ((value & 0x0000ff00u) << 8)
         | ((value & 0x000000ffu) << 24);
}

constexpr u64 swap_bytes_64(u64 value)
{
    return ((value & 0xff00000000000000ull) >> 56)
         | ((value & 0x00ff000000000000ull) >> 40)
         | ((value & 0x0000ff0000000000ull) >> 24)
         | ((value & 0x000000ff00000000ull) >> 8)
         | ((value & 0x00000000ff000000ull) << 8)
         | ((value & 0x0000000000ff0000ull) << 24)
         | ((value & 0x000000000000ff00ull) << 40)
         | ((value & 0x00000000000000ffull) << 56);
}

constexpr u32 swap_halves_32(u32 value)
{
    return ((value & 0xffff0000u) >> 16)
         | ((value & 0x0000ffffu) << 16);
}

constexpr u64 swap_halves_64(u64 value)
{
    return ((value & 0xffff000000000000ull) >> 48)
         | ((value & 0x0000ffff00000000ull) >> 16)
         | ((value & 0x00000000ffff0000ull) << 16)
         | ((value & 0x000000000000ffffull) << 48);
}

constexpr u64 swap_words_64(u64 value)
{
    return ((value & 0xffffffff00000000ull) >> 32)
         | ((value & 0x00000000ffffffffull) << 32);
}

}  // namespace mcl::bit

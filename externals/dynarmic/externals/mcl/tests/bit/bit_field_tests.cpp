// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#include <array>
#include <tuple>

#include <catch2/catch_test_macros.hpp>
#include <mcl/bit/bit_field.hpp>
#include <mcl/stdint.hpp>

TEST_CASE("mcl::bit::ones", "[bit]")
{
    const std::array cases{
        std::make_tuple<size_t, u8>(0, 0x00),
        std::make_tuple<size_t, u8>(1, 0x01),
        std::make_tuple<size_t, u8>(2, 0x03),
        std::make_tuple<size_t, u8>(3, 0x07),
        std::make_tuple<size_t, u8>(4, 0x0f),
        std::make_tuple<size_t, u8>(5, 0x1f),
        std::make_tuple<size_t, u8>(6, 0x3f),
        std::make_tuple<size_t, u8>(7, 0x7f),
        std::make_tuple<size_t, u8>(8, 0xff),
    };

    for (const auto [count, expected] : cases) {
        REQUIRE(mcl::bit::ones<u8>(count) == expected);
        REQUIRE(mcl::bit::ones<u16>(count) == expected);
        REQUIRE(mcl::bit::ones<u32>(count) == expected);
        REQUIRE(mcl::bit::ones<u64>(count) == expected);
        REQUIRE(mcl::bit::ones<uptr>(count) == expected);
        REQUIRE(mcl::bit::ones<size_t>(count) == expected);
    }
}

static_assert(mcl::bit::ones<3, u8>() == 0x7);
static_assert(mcl::bit::ones<15, u16>() == 0x7fff);
static_assert(mcl::bit::ones<16, u16>() == 0xffff);
static_assert(mcl::bit::ones<31, u32>() == 0x7fff'ffff);
static_assert(mcl::bit::ones<32, u32>() == 0xffff'ffff);
static_assert(mcl::bit::ones<63, u64>() == 0x7fff'ffff'ffff'ffff);
static_assert(mcl::bit::ones<64, u64>() == 0xffff'ffff'ffff'ffff);

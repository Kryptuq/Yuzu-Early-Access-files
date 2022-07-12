// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#include <unordered_map>

#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>
#include <mcl/container/ihmap.hpp>
#include <mcl/stdint.hpp>

TEST_CASE("mcl::ihmap", "[ihmap]")
{
    mcl::ihmap<u64, u64> double_map;

    constexpr int count = 100000;

    REQUIRE(double_map.empty());

    for (int i = 0; i < count; ++i) {
        double_map[i] = i * 2;
        REQUIRE(double_map.size() == i + 1);
    }

    for (int i = 0; i < count; ++i) {
        REQUIRE(double_map[i] == i * 2);
        REQUIRE(double_map.contains(i));
    }

    for (int i = 0; i < count; ++i) {
        auto iter = double_map.find(i);
        REQUIRE(iter->first == i);
        REQUIRE(iter->second == i * 2);
    }

    for (int i = count; i < count * 2; ++i) {
        REQUIRE(!double_map.contains(i));
    }

    for (int i = 0; i < count; ++i) {
        auto result = double_map.try_emplace(i, 0);
        REQUIRE(!result.second);
    }

    for (auto [k, v] : double_map) {
        REQUIRE(k * 2 == v);
    }

    std::unordered_map<u64, size_t> indexes_count;
    for (auto [k, v] : double_map) {
        (void)v;
        indexes_count[k]++;
    }
    for (auto [k, v] : indexes_count) {
        (void)k;
        REQUIRE(v == 1);
    }

    REQUIRE(!double_map.empty());
    double_map.clear();
    REQUIRE(double_map.empty());

    for (auto [k, v] : double_map) {
        REQUIRE(false);
    }
}

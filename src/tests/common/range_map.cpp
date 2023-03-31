// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

#include "common/range_map.h"

enum class MappedEnum : u32 {
    Invalid = 0,
    Valid_1 = 1,
    Valid_2 = 2,
    Valid_3 = 3,
};

TEST_CASE("Range Map: Setup", "[video_core]") {
    Common::RangeMap<u64, MappedEnum> my_map(MappedEnum::Invalid);
    my_map.Map(3000, 3500, MappedEnum::Valid_1);
    my_map.Unmap(3200, 3600);
    my_map.Map(4000, 4500, MappedEnum::Valid_2);
    my_map.Map(4200, 4400, MappedEnum::Valid_2);
    my_map.Map(4200, 4400, MappedEnum::Valid_1);
    REQUIRE(my_map.GetContinuousSizeFrom(4200) == 200);
    REQUIRE(my_map.GetContinuousSizeFrom(3000) == 200);
    REQUIRE(my_map.GetContinuousSizeFrom(2900) == 0);

    REQUIRE(my_map.GetValueAt(2900) == MappedEnum::Invalid);
    REQUIRE(my_map.GetValueAt(3100) == MappedEnum::Valid_1);
    REQUIRE(my_map.GetValueAt(3000) == MappedEnum::Valid_1);
    REQUIRE(my_map.GetValueAt(3200) == MappedEnum::Invalid);

    REQUIRE(my_map.GetValueAt(4199) == MappedEnum::Valid_2);
    REQUIRE(my_map.GetValueAt(4200) == MappedEnum::Valid_1);
    REQUIRE(my_map.GetValueAt(4400) == MappedEnum::Valid_2);
    REQUIRE(my_map.GetValueAt(4500) == MappedEnum::Invalid);
    REQUIRE(my_map.GetValueAt(4600) == MappedEnum::Invalid);

    my_map.Unmap(0, 6000);
    for (u64 address = 0; address < 10000; address += 1000) {
        REQUIRE(my_map.GetContinuousSizeFrom(address) == 0);
    }

    my_map.Map(1000, 3000, MappedEnum::Valid_1);
    my_map.Map(4000, 5000, MappedEnum::Valid_1);
    my_map.Map(2500, 4100, MappedEnum::Valid_1);
    REQUIRE(my_map.GetContinuousSizeFrom(1000) == 4000);

    my_map.Map(1000, 3000, MappedEnum::Valid_1);
    my_map.Map(4000, 5000, MappedEnum::Valid_2);
    my_map.Map(2500, 4100, MappedEnum::Valid_3);
    REQUIRE(my_map.GetContinuousSizeFrom(1000) == 1500);
    REQUIRE(my_map.GetContinuousSizeFrom(2500) == 1600);
    REQUIRE(my_map.GetContinuousSizeFrom(4100) == 900);
    REQUIRE(my_map.GetValueAt(900) == MappedEnum::Invalid);
    REQUIRE(my_map.GetValueAt(1000) == MappedEnum::Valid_1);
    REQUIRE(my_map.GetValueAt(2500) == MappedEnum::Valid_3);
    REQUIRE(my_map.GetValueAt(4100) == MappedEnum::Valid_2);
    REQUIRE(my_map.GetValueAt(5000) == MappedEnum::Invalid);

    my_map.Map(2000, 6000, MappedEnum::Valid_3);
    REQUIRE(my_map.GetContinuousSizeFrom(1000) == 1000);
    REQUIRE(my_map.GetContinuousSizeFrom(3000) == 3000);
    REQUIRE(my_map.GetValueAt(1000) == MappedEnum::Valid_1);
    REQUIRE(my_map.GetValueAt(1999) == MappedEnum::Valid_1);
    REQUIRE(my_map.GetValueAt(1500) == MappedEnum::Valid_1);
    REQUIRE(my_map.GetValueAt(2001) == MappedEnum::Valid_3);
    REQUIRE(my_map.GetValueAt(5999) == MappedEnum::Valid_3);
    REQUIRE(my_map.GetValueAt(6000) == MappedEnum::Invalid);
}

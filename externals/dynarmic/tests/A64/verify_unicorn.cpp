/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <array>

#include <catch.hpp>

#include "../rand_int.h"
#include "../unicorn_emu/a64_unicorn.h"
#include "./testenv.h"

using namespace Dynarmic;

TEST_CASE("Unicorn: Sanity test", "[a64]") {
    A64TestEnv env;

    env.code_mem.emplace_back(0x8b020020);  // ADD X0, X1, X2
    env.code_mem.emplace_back(0x14000000);  // B .

    constexpr A64Unicorn::RegisterArray regs{
        0, 1, 2, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0};

    A64Unicorn unicorn{env};

    unicorn.SetRegisters(regs);
    unicorn.SetPC(0);

    env.ticks_left = 2;
    unicorn.Run();

    REQUIRE(unicorn.GetRegisters()[0] == 3);
    REQUIRE(unicorn.GetRegisters()[1] == 1);
    REQUIRE(unicorn.GetRegisters()[2] == 2);
    REQUIRE(unicorn.GetPC() == 4);
}

TEST_CASE("Unicorn: Ensure 0xFFFF'FFFF'FFFF'FFFF is readable", "[a64]") {
    A64TestEnv env;

    env.code_mem.emplace_back(0x385fed99);  // LDRB W25, [X12, #0xfffffffffffffffe]!
    env.code_mem.emplace_back(0x14000000);  // B .

    A64Unicorn::RegisterArray regs{};
    regs[12] = 1;

    A64Unicorn unicorn{env};

    unicorn.SetRegisters(regs);
    unicorn.SetPC(0);

    env.ticks_left = 2;
    unicorn.Run();

    REQUIRE(unicorn.GetPC() == 4);
}

TEST_CASE("Unicorn: Ensure is able to read across page boundaries", "[a64]") {
    A64TestEnv env;

    env.code_mem.emplace_back(0xb85f93d9);  // LDUR W25, [X30, #0xfffffffffffffff9]
    env.code_mem.emplace_back(0x14000000);  // B .

    A64Unicorn::RegisterArray regs{};
    regs[30] = 4;

    A64Unicorn unicorn{env};

    unicorn.SetRegisters(regs);
    unicorn.SetPC(0);

    env.ticks_left = 2;
    unicorn.Run();

    REQUIRE(unicorn.GetPC() == 4);
}

// SPDX-FileCopyrightText: Copyright (c) 2022 merryhime <https://mary.rs>
// SPDX-License-Identifier: MIT

#include <cstdint>
#include <cstdio>

#include <catch2/catch_test_macros.hpp>

#include "oaknut/code_block.hpp"
#include "oaknut/oaknut.hpp"
#include "rand_int.hpp"

using namespace oaknut;
using namespace oaknut::util;

TEST_CASE("Basic Test")
{
    CodeBlock mem{4096};
    CodeGenerator code{mem.ptr()};

    mem.unprotect();

    code.MOV(W0, 42);
    code.RET();

    mem.protect();
    mem.invalidate_all();

    int result = ((int (*)())mem.ptr())();
    REQUIRE(result == 42);
}

TEST_CASE("Fibonacci")
{
    CodeBlock mem{4096};
    CodeGenerator code{mem.ptr()};

    mem.unprotect();

    auto fib = code.ptr<int (*)(int)>();
    Label start, end, zero, recurse;

    code.l(start);
    code.STP(X29, X30, SP, PRE_INDEXED, -32);
    code.STP(X20, X19, SP, 16);
    code.MOV(X29, SP);
    code.MOV(W19, W0);
    code.SUBS(W0, W0, 1);
    code.B(LT, zero);
    code.B(NE, recurse);
    code.MOV(W0, 1);
    code.B(end);

    code.l(zero);
    code.MOV(W0, WZR);
    code.B(end);

    code.l(recurse);
    code.BL(start);
    code.MOV(W20, W0);
    code.SUB(W0, W19, 2);
    code.BL(start);
    code.ADD(W0, W0, W20);

    code.l(end);
    code.LDP(X20, X19, SP, 16);
    code.LDP(X29, X30, SP, POST_INDEXED, 32);
    code.RET();

    mem.protect();
    mem.invalidate_all();

    REQUIRE(fib(0) == 0);
    REQUIRE(fib(1) == 1);
    REQUIRE(fib(5) == 5);
    REQUIRE(fib(9) == 34);
}

TEST_CASE("Immediate generation (32-bit)")
{
    CodeBlock mem{4096};

    for (int i = 0; i < 0x100000; i++) {
        const std::uint32_t value = RandInt<std::uint32_t>(0, 0xffffffff);

        CodeGenerator code{mem.ptr()};

        auto f = code.ptr<std::uint64_t (*)()>();
        mem.unprotect();
        code.MOV(W0, value);
        code.RET();
        mem.protect();
        mem.invalidate_all();

        REQUIRE(f() == value);
    }
}

TEST_CASE("Immediate generation (64-bit)")
{
    CodeBlock mem{4096};

    for (int i = 0; i < 0x100000; i++) {
        const std::uint64_t value = RandInt<std::uint64_t>(0, 0xffffffff'ffffffff);

        CodeGenerator code{mem.ptr()};

        auto f = code.ptr<std::uint64_t (*)()>();
        mem.unprotect();
        code.MOV(X0, value);
        code.RET();
        mem.protect();
        mem.invalidate_all();

        REQUIRE(f() == value);
    }
}

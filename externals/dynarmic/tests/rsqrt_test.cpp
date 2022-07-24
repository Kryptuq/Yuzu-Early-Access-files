/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <catch2/catch.hpp>
#include <fmt/printf.h>
#include <mcl/stdint.hpp>

#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/common/fp/op/FPRSqrtEstimate.h"

extern "C" u32 rsqrt_inaccurate(u32);
extern "C" u32 rsqrt_full(u32);
extern "C" u32 rsqrt_full_gpr(u32);
extern "C" u32 rsqrt_full_nb(u32);
extern "C" u32 rsqrt_full_nb2(u32);
extern "C" u32 rsqrt_full_nb_gpr(u32);
extern "C" u32 rsqrt_newton(u32);
extern "C" u32 rsqrt_hack(u32);

using namespace Dynarmic;

extern "C" u32 rsqrt_fallback(u32 value) {
    FP::FPCR fpcr;
    FP::FPSR fpsr;
    return FP::FPRSqrtEstimate(value, fpcr, fpsr);
}
extern "C" u32 _rsqrt_fallback(u32 value) {
    return rsqrt_fallback(value);
}

void Test(u32 value) {
    FP::FPCR fpcr;
    FP::FPSR fpsr;

    const u32 expect = FP::FPRSqrtEstimate(value, fpcr, fpsr);
    const u32 full = rsqrt_full(value);
    const u32 full_gpr = rsqrt_full_gpr(value);
    const u32 newton = rsqrt_newton(value);
    const u32 hack = rsqrt_hack(value);

    if (expect != full || expect != full_gpr || expect != newton || expect != hack) {
        fmt::print("{:08x} = {:08x} : {:08x} : {:08x} : {:08x} : {:08x}\n", value, expect, full, full_gpr, newton, hack);

        REQUIRE(expect == full);
        REQUIRE(expect == full_gpr);
        REQUIRE(expect == newton);
        REQUIRE(expect == hack);
    }
}

TEST_CASE("RSqrt Tests", "[fp][.]") {
    Test(0x00000000);
    Test(0x80000000);
    Test(0x7f8b7201);
    Test(0x7f800000);
    Test(0x7fc00000);
    Test(0xff800000);
    Test(0xffc00000);
    Test(0xff800001);

    for (u64 i = 0; i < 0x1'0000'0000; i++) {
        const u32 value = static_cast<u32>(i);
        Test(value);
    }
}

TEST_CASE("Benchmark RSqrt", "[fp][.]") {
    BENCHMARK("Inaccurate") {
        u64 total = 0;
        for (u64 i = 0; i < 0x1'0000'0000; i += 0x1234) {
            const u32 value = static_cast<u32>(i);
            total += rsqrt_inaccurate(value);
        }
        return total;
    };

    BENCHMARK("Full divss") {
        u64 total = 0;
        for (u64 i = 0; i < 0x1'0000'0000; i += 0x1234) {
            const u32 value = static_cast<u32>(i);
            total += rsqrt_full(value);
        }
        return total;
    };

    BENCHMARK("Full divss (GPR)") {
        u64 total = 0;
        for (u64 i = 0; i < 0x1'0000'0000; i += 0x1234) {
            const u32 value = static_cast<u32>(i);
            total += rsqrt_full_gpr(value);
        }
        return total;
    };

    BENCHMARK("Full divss (NB)") {
        u64 total = 0;
        for (u64 i = 0; i < 0x1'0000'0000; i += 0x1234) {
            const u32 value = static_cast<u32>(i);
            total += rsqrt_full_nb(value);
        }
        return total;
    };

    BENCHMARK("Full divss (NB2)") {
        u64 total = 0;
        for (u64 i = 0; i < 0x1'0000'0000; i += 0x1234) {
            const u32 value = static_cast<u32>(i);
            total += rsqrt_full_nb2(value);
        }
        return total;
    };

    BENCHMARK("Full divss (NB + GPR)") {
        u64 total = 0;
        for (u64 i = 0; i < 0x1'0000'0000; i += 0x1234) {
            const u32 value = static_cast<u32>(i);
            total += rsqrt_full_nb_gpr(value);
        }
        return total;
    };

    BENCHMARK("One Newton iteration") {
        u64 total = 0;
        for (u64 i = 0; i < 0x1'0000'0000; i += 0x1234) {
            const u32 value = static_cast<u32>(i);
            total += rsqrt_newton(value);
        }
        return total;
    };

    BENCHMARK("Ugly Hack") {
        u64 total = 0;
        for (u64 i = 0; i < 0x1'0000'0000; i += 0x1234) {
            const u32 value = static_cast<u32>(i);
            total += rsqrt_hack(value);
        }
        return total;
    };

    BENCHMARK("Softfloat") {
        u64 total = 0;
        for (u64 i = 0; i < 0x1'0000'0000; i += 0x1234) {
            const u32 value = static_cast<u32>(i);
            total += rsqrt_fallback(value);
        }
        return total;
    };
}

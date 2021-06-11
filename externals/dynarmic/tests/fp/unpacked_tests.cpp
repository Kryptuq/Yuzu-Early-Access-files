/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <tuple>
#include <vector>

#include <catch.hpp>

#include "../rand_int.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/common/fp/unpacked.h"

using namespace Dynarmic;
using namespace Dynarmic::FP;

TEST_CASE("FPUnpack Tests", "[fp]") {
    const static std::vector<std::tuple<u32, std::tuple<FPType, bool, FPUnpacked>, u32>> test_cases{
        {0x00000000, {FPType::Zero, false, ToNormalized(false, 0, 0)}, 0},
        {0x7F800000, {FPType::Infinity, false, ToNormalized(false, 1000000, 1)}, 0},
        {0xFF800000, {FPType::Infinity, true, ToNormalized(true, 1000000, 1)}, 0},
        {0x7F800001, {FPType::SNaN, false, ToNormalized(false, 0, 0)}, 0},
        {0xFF800001, {FPType::SNaN, true, ToNormalized(true, 0, 0)}, 0},
        {0x7FC00001, {FPType::QNaN, false, ToNormalized(false, 0, 0)}, 0},
        {0xFFC00001, {FPType::QNaN, true, ToNormalized(true, 0, 0)}, 0},
        {0x00000001, {FPType::Nonzero, false, ToNormalized(false, -149, 1)}, 0},        // Smallest single precision denormal is 2^-149.
        {0x3F7FFFFF, {FPType::Nonzero, false, ToNormalized(false, -24, 0xFFFFFF)}, 0},  // 1.0 - epsilon
    };

    const FPCR fpcr;
    for (const auto& [input, expected_output, expected_fpsr] : test_cases) {
        FPSR fpsr;
        const auto output = FPUnpack<u32>(input, fpcr, fpsr);

        INFO("Input: " << std::hex << input);
        INFO("Output Sign: " << std::get<2>(output).sign);
        INFO("Output Exponent: " << std::get<2>(output).exponent);
        INFO("Output Mantissa: " << std::hex << std::get<2>(output).mantissa);
        INFO("Expected Sign: " << std::get<2>(expected_output).sign);
        INFO("Expected Exponent: " << std::get<2>(expected_output).exponent);
        INFO("Expected Mantissa: " << std::hex << std::get<2>(expected_output).mantissa);

        REQUIRE(output == expected_output);
        REQUIRE(fpsr.Value() == expected_fpsr);
    }
}

TEST_CASE("FPRound Tests", "[fp]") {
    const static std::vector<std::tuple<u32, std::tuple<FPType, bool, FPUnpacked>, u32>> test_cases{
        {0x7F800000, {FPType::Infinity, false, ToNormalized(false, 1000000, 1)}, 0x14},
        {0xFF800000, {FPType::Infinity, true, ToNormalized(true, 1000000, 1)}, 0x14},
        {0x00000001, {FPType::Nonzero, false, ToNormalized(false, -149, 1)}, 0},            // Smallest single precision denormal is 2^-149.
        {0x3F7FFFFF, {FPType::Nonzero, false, ToNormalized(false, -24, 0xFFFFFF)}, 0},      // 1.0 - epsilon
        {0x3F800000, {FPType::Nonzero, false, ToNormalized(false, -28, 0xFFFFFFF)}, 0x10},  // rounds to 1.0
    };

    const FPCR fpcr;
    for (const auto& [expected_output, input, expected_fpsr] : test_cases) {
        FPSR fpsr;
        const auto output = FPRound<u32>(std::get<2>(input), fpcr, fpsr);

        INFO("Expected Output: " << std::hex << expected_output);
        REQUIRE(output == expected_output);
        REQUIRE(fpsr.Value() == expected_fpsr);
    }
}

TEST_CASE("FPUnpack<->FPRound Round-trip Tests", "[fp]") {
    const FPCR fpcr;
    for (size_t count = 0; count < 100000; count++) {
        FPSR fpsr;
        const u32 input = RandInt(0, 1) == 0 ? RandInt<u32>(0x00000001, 0x7F800000) : RandInt<u32>(0x80000001, 0xFF800000);
        const auto intermediate = std::get<2>(FPUnpack<u32>(input, fpcr, fpsr));
        const u32 output = FPRound<u32>(intermediate, fpcr, fpsr);

        INFO("Count: " << count);
        INFO("Intermediate Values: " << std::hex << intermediate.sign << ';' << intermediate.exponent << ';' << intermediate.mantissa);
        REQUIRE(input == output);
    }
}

TEST_CASE("FPRound (near zero, round to posinf)", "[fp]") {
    const FPUnpacked input = {false, -353, 0x0a98d25ace5b2000};

    FPSR fpsr;
    FPCR fpcr;
    fpcr.RMode(RoundingMode::TowardsPlusInfinity);

    const u32 output = FPRound<u32>(input, fpcr, fpsr);

    REQUIRE(output == 0x00000001);
}

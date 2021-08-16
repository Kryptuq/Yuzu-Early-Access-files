/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "../rand_int.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/common/fp/mantissa_util.h"
#include "dynarmic/common/safe_ops.h"

using namespace Dynarmic;
using namespace Dynarmic::FP;

TEST_CASE("ResidualErrorOnRightShift", "[fp]") {
    const std::vector<std::tuple<u32, int, ResidualError>> test_cases{
        {0x00000001, 1, ResidualError::Half},
        {0x00000002, 1, ResidualError::Zero},
        {0x00000001, 2, ResidualError::LessThanHalf},
        {0x00000002, 2, ResidualError::Half},
        {0x00000003, 2, ResidualError::GreaterThanHalf},
        {0x00000004, 2, ResidualError::Zero},
        {0x00000005, 2, ResidualError::LessThanHalf},
        {0x00000006, 2, ResidualError::Half},
        {0x00000007, 2, ResidualError::GreaterThanHalf},
    };

    for (auto [mantissa, shift, expected_result] : test_cases) {
        const ResidualError result = ResidualErrorOnRightShift(mantissa, shift);
        REQUIRE(result == expected_result);
    }
}

TEST_CASE("ResidualErrorOnRightShift Randomized", "[fp]") {
    for (size_t test = 0; test < 100000; test++) {
        const u64 mantissa = Common::SignExtend<32, u64>(RandInt<u32>(0, 0xFFFFFFFF));
        const int shift = RandInt<int>(-60, 60);

        const ResidualError result = ResidualErrorOnRightShift(mantissa, shift);

        const u64 calculated_error = Safe::ArithmeticShiftRightDouble(mantissa, u64(0), shift);
        const ResidualError expected_result = [&] {
            constexpr u64 half_error = 0x8000'0000'0000'0000ull;
            if (calculated_error == 0) {
                return ResidualError::Zero;
            }
            if (calculated_error < half_error) {
                return ResidualError::LessThanHalf;
            }
            if (calculated_error == half_error) {
                return ResidualError::Half;
            }
            return ResidualError::GreaterThanHalf;
        }();

        INFO(std::hex << "mantissa " << mantissa << " shift " << shift << " calculated_error " << calculated_error);
        REQUIRE(result == expected_result);
    }
}

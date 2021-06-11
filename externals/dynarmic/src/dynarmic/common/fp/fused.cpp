/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/fp/fused.h"

#include "dynarmic/common/fp/mantissa_util.h"
#include "dynarmic/common/fp/unpacked.h"
#include "dynarmic/common/u128.h"

namespace Dynarmic::FP {

constexpr size_t product_point_position = normalized_point_position * 2;

static FPUnpacked ReduceMantissa(bool sign, int exponent, const u128& mantissa) {
    constexpr int point_position_correction = normalized_point_position - (product_point_position - 64);
    // We round-to-odd here when reducing the bitwidth of the mantissa so that subsequent roundings are accurate.
    return {sign, exponent + point_position_correction, mantissa.upper | static_cast<u64>(mantissa.lower != 0)};
}

FPUnpacked FusedMulAdd(FPUnpacked addend, FPUnpacked op1, FPUnpacked op2) {
    const bool product_sign = op1.sign != op2.sign;
    const auto [product_exponent, product_value] = [op1, op2] {
        int exponent = op1.exponent + op2.exponent;
        u128 value = Multiply64To128(op1.mantissa, op2.mantissa);
        if (value.Bit<product_point_position + 1>()) {
            value = value >> 1;
            exponent++;
        }
        return std::make_tuple(exponent, value);
    }();

    if (product_value == 0) {
        return addend;
    }

    if (addend.mantissa == 0) {
        return ReduceMantissa(product_sign, product_exponent, product_value);
    }

    const int exp_diff = product_exponent - addend.exponent;

    if (product_sign == addend.sign) {
        // Addition

        if (exp_diff <= 0) {
            // addend > product
            const u64 result = addend.mantissa + StickyLogicalShiftRight(product_value, normalized_point_position - exp_diff).lower;
            return FPUnpacked{addend.sign, addend.exponent, result};
        }

        // addend < product
        const u128 result = product_value + StickyLogicalShiftRight(addend.mantissa, exp_diff - normalized_point_position);
        return ReduceMantissa(product_sign, product_exponent, result);
    }

    // Subtraction

    const u128 addend_long = u128(addend.mantissa) << normalized_point_position;

    bool result_sign;
    u128 result;
    int result_exponent;

    if (exp_diff == 0 && product_value > addend_long) {
        result_sign = product_sign;
        result_exponent = product_exponent;
        result = product_value - addend_long;
    } else if (exp_diff <= 0) {
        result_sign = !product_sign;
        result_exponent = addend.exponent;
        result = addend_long - StickyLogicalShiftRight(product_value, -exp_diff);
    } else {
        result_sign = product_sign;
        result_exponent = product_exponent;
        result = product_value - StickyLogicalShiftRight(addend_long, exp_diff);
    }

    if (result.upper == 0) {
        return FPUnpacked{result_sign, result_exponent, result.lower};
    }

    const int required_shift = normalized_point_position - Common::HighestSetBit(result.upper);
    result = result << required_shift;
    result_exponent -= required_shift;
    return ReduceMantissa(result_sign, result_exponent, result);
}

}  // namespace Dynarmic::FP

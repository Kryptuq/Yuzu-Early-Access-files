/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <mcl/bit/bit_field.hpp>
#include <mcl/bitsizeof.hpp>
#include <mcl/stdint.hpp>

namespace Dynarmic::FP {

enum class ResidualError {
    Zero,
    LessThanHalf,
    Half,
    GreaterThanHalf,
};

inline ResidualError ResidualErrorOnRightShift(u64 mantissa, int shift_amount) {
    if (shift_amount <= 0 || mantissa == 0) {
        return ResidualError::Zero;
    }

    if (shift_amount > static_cast<int>(mcl::bitsizeof<u64>)) {
        return mcl::bit::most_significant_bit(mantissa) ? ResidualError::GreaterThanHalf : ResidualError::LessThanHalf;
    }

    const size_t half_bit_position = static_cast<size_t>(shift_amount - 1);
    const u64 half = static_cast<u64>(1) << half_bit_position;
    const u64 error_mask = mcl::bit::ones<u64>(static_cast<size_t>(shift_amount));
    const u64 error = mantissa & error_mask;

    if (error == 0) {
        return ResidualError::Zero;
    }
    if (error < half) {
        return ResidualError::LessThanHalf;
    }
    if (error == half) {
        return ResidualError::Half;
    }
    return ResidualError::GreaterThanHalf;
}

}  // namespace Dynarmic::FP

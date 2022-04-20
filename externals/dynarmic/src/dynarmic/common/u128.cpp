/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/u128.h"

#include <mcl/bitsizeof.hpp>
#include <mcl/stdint.hpp>

namespace Dynarmic {

u128 Multiply64To128(u64 a, u64 b) {
    const u32 a0 = static_cast<u32>(a);
    const u32 b0 = static_cast<u32>(b);
    const u32 a1 = static_cast<u32>(a >> 32);
    const u32 b1 = static_cast<u32>(b >> 32);

    // result = (c2 << 64) + (c1 << 32) + c0
    // c2 = a1 * b1
    // c1 = a1 * b0 + a0 * b1
    // c0 = a0 * b0
    // noting that these operations may overflow

    const u64 c0 = static_cast<u64>(a0) * b0;
    const u64 c1_0 = static_cast<u64>(a1) * b0;
    const u64 c1_1 = static_cast<u64>(a0) * b1;
    const u64 c2 = static_cast<u64>(a1) * b1;

    const u64 c1 = c1_0 + c1_1;
    const u64 c1_overflow = c1 < c1_0;

    const u64 lower = c0 + (c1 << 32);
    const u64 lower_overflow = lower < c0;

    const u64 upper = lower_overflow + (c1 >> 32) + (c1_overflow << 32) + c2;

    u128 result;
    result.lower = lower;
    result.upper = upper;
    return result;
}

u128 operator<<(u128 operand, int amount) {
    if (amount < 0) {
        return operand >> -amount;
    }

    if (amount == 0) {
        return operand;
    }

    if (amount < 64) {
        u128 result;
        result.lower = (operand.lower << amount);
        result.upper = (operand.upper << amount) | (operand.lower >> (64 - amount));
        return result;
    }

    if (amount < 128) {
        u128 result;
        result.upper = operand.lower << (amount - 64);
        return result;
    }

    return {};
}

u128 operator>>(u128 operand, int amount) {
    if (amount < 0) {
        return operand << -amount;
    }

    if (amount == 0) {
        return operand;
    }

    if (amount < 64) {
        u128 result;
        result.lower = (operand.lower >> amount) | (operand.upper << (64 - amount));
        result.upper = (operand.upper >> amount);
        return result;
    }

    if (amount < 128) {
        u128 result;
        result.lower = operand.upper >> (amount - 64);
        return result;
    }

    return {};
}

u128 StickyLogicalShiftRight(u128 operand, int amount) {
    if (amount < 0) {
        return operand << -amount;
    }

    if (amount == 0) {
        return operand;
    }

    if (amount < 64) {
        u128 result;
        result.lower = (operand.lower >> amount) | (operand.upper << (64 - amount));
        result.upper = (operand.upper >> amount);
        // Sticky bit
        if ((operand.lower << (64 - amount)) != 0) {
            result.lower |= 1;
        }
        return result;
    }

    if (amount == 64) {
        u128 result;
        result.lower = operand.upper;
        // Sticky bit
        if (operand.lower != 0) {
            result.lower |= 1;
        }
        return result;
    }

    if (amount < 128) {
        u128 result;
        result.lower = operand.upper >> (amount - 64);
        // Sticky bit
        if (operand.lower != 0) {
            result.lower |= 1;
        }
        if ((operand.upper << (128 - amount)) != 0) {
            result.lower |= 1;
        }
        return result;
    }

    if (operand.lower != 0 || operand.upper != 0) {
        return u128(1);
    }
    return {};
}

}  // namespace Dynarmic

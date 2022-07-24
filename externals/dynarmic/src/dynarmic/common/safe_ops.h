/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <type_traits>

#include <mcl/bitsizeof.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/common/u128.h"

namespace Dynarmic::Safe {

template<typename T>
T LogicalShiftLeft(T value, int shift_amount);
template<typename T>
T LogicalShiftRight(T value, int shift_amount);
template<typename T>
T ArithmeticShiftLeft(T value, int shift_amount);
template<typename T>
T ArithmeticShiftRight(T value, int shift_amount);

template<typename T>
T LogicalShiftLeft(T value, int shift_amount) {
    static_assert(std::is_integral_v<T>);

    if (shift_amount >= static_cast<int>(mcl::bitsizeof<T>)) {
        return 0;
    }

    if (shift_amount < 0) {
        return LogicalShiftRight(value, -shift_amount);
    }

    auto unsigned_value = static_cast<std::make_unsigned_t<T>>(value);
    return static_cast<T>(unsigned_value << shift_amount);
}

template<>
inline u128 LogicalShiftLeft(u128 value, int shift_amount) {
    return value << shift_amount;
}

template<typename T>
T LogicalShiftRight(T value, int shift_amount) {
    static_assert(std::is_integral_v<T>);

    if (shift_amount >= static_cast<int>(mcl::bitsizeof<T>)) {
        return 0;
    }

    if (shift_amount < 0) {
        return LogicalShiftLeft(value, -shift_amount);
    }

    auto unsigned_value = static_cast<std::make_unsigned_t<T>>(value);
    return static_cast<T>(unsigned_value >> shift_amount);
}

template<>
inline u128 LogicalShiftRight(u128 value, int shift_amount) {
    return value >> shift_amount;
}

template<typename T>
T LogicalShiftRightDouble(T top, T bottom, int shift_amount) {
    return LogicalShiftLeft(top, int(mcl::bitsizeof<T>) - shift_amount) | LogicalShiftRight(bottom, shift_amount);
}

template<typename T>
T ArithmeticShiftLeft(T value, int shift_amount) {
    static_assert(std::is_integral_v<T>);

    if (shift_amount >= static_cast<int>(mcl::bitsizeof<T>)) {
        return 0;
    }

    if (shift_amount < 0) {
        return ArithmeticShiftRight(value, -shift_amount);
    }

    auto unsigned_value = static_cast<std::make_unsigned_t<T>>(value);
    return static_cast<T>(unsigned_value << shift_amount);
}

template<typename T>
T ArithmeticShiftRight(T value, int shift_amount) {
    static_assert(std::is_integral_v<T>);

    if (shift_amount >= static_cast<int>(mcl::bitsizeof<T>)) {
        return mcl::bit::most_significant_bit(value) ? ~static_cast<T>(0) : 0;
    }

    if (shift_amount < 0) {
        return ArithmeticShiftLeft(value, -shift_amount);
    }

    auto signed_value = static_cast<std::make_signed_t<T>>(value);
    return static_cast<T>(signed_value >> shift_amount);
}

template<typename T>
T ArithmeticShiftRightDouble(T top, T bottom, int shift_amount) {
    return ArithmeticShiftLeft(top, int(mcl::bitsizeof<T>) - shift_amount) | LogicalShiftRight(bottom, shift_amount);
}

template<typename T>
T Negate(T value) {
    return static_cast<T>(~static_cast<std::uintmax_t>(value) + 1);
}

}  // namespace Dynarmic::Safe

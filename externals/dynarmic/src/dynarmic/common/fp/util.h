/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <optional>

#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/info.h"

namespace Dynarmic::FP {

/// Is floating point value a zero?
template<typename FPT>
inline bool IsZero(FPT value, FPCR fpcr) {
    if (fpcr.FZ()) {
        return (value & FPInfo<FPT>::exponent_mask) == 0;
    }
    return (value & ~FPInfo<FPT>::sign_mask) == 0;
}

/// Is floating point value an infinity?
template<typename FPT>
constexpr bool IsInf(FPT value) {
    return (value & ~FPInfo<FPT>::sign_mask) == FPInfo<FPT>::Infinity(false);
}

/// Is floating point value a QNaN?
template<typename FPT>
constexpr bool IsQNaN(FPT value) {
    constexpr FPT qnan_bits = FPInfo<FPT>::exponent_mask | FPInfo<FPT>::mantissa_msb;
    return (value & qnan_bits) == qnan_bits;
}

/// Is floating point value a SNaN?
template<typename FPT>
constexpr bool IsSNaN(FPT value) {
    constexpr FPT qnan_bits = FPInfo<FPT>::exponent_mask | FPInfo<FPT>::mantissa_msb;
    constexpr FPT snan_bits = FPInfo<FPT>::exponent_mask;
    return (value & qnan_bits) == snan_bits && (value & FPInfo<FPT>::mantissa_mask) != 0;
}

/// Is floating point value a NaN?
template<typename FPT>
constexpr bool IsNaN(FPT value) {
    return IsQNaN(value) || IsSNaN(value);
}

/// Given a single argument, return the NaN value which would be returned by an ARM processor.
/// If the argument isn't a NaN, returns std::nullopt.
template<typename FPT>
constexpr std::optional<FPT> ProcessNaNs(FPT a) {
    if (IsSNaN(a)) {
        return a | FPInfo<FPT>::mantissa_msb;
    } else if (IsQNaN(a)) {
        return a;
    }
    return std::nullopt;
}

/// Given a pair of arguments, return the NaN value which would be returned by an ARM processor.
/// If neither argument is a NaN, returns std::nullopt.
template<typename FPT>
constexpr std::optional<FPT> ProcessNaNs(FPT a, FPT b) {
    if (IsSNaN(a)) {
        return a | FPInfo<FPT>::mantissa_msb;
    } else if (IsSNaN(b)) {
        return b | FPInfo<FPT>::mantissa_msb;
    } else if (IsQNaN(a)) {
        return a;
    } else if (IsQNaN(b)) {
        return b;
    }
    return std::nullopt;
}

/// Given three arguments, return the NaN value which would be returned by an ARM processor.
/// If none of the arguments is a NaN, returns std::nullopt.
template<typename FPT>
constexpr std::optional<FPT> ProcessNaNs(FPT a, FPT b, FPT c) {
    if (IsSNaN(a)) {
        return a | FPInfo<FPT>::mantissa_msb;
    } else if (IsSNaN(b)) {
        return b | FPInfo<FPT>::mantissa_msb;
    } else if (IsSNaN(c)) {
        return c | FPInfo<FPT>::mantissa_msb;
    } else if (IsQNaN(a)) {
        return a;
    } else if (IsQNaN(b)) {
        return b;
    } else if (IsQNaN(c)) {
        return c;
    }
    return std::nullopt;
}

}  // namespace Dynarmic::FP

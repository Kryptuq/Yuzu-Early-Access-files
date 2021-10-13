/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <tuple>

#include "dynarmic/common/common_types.h"
#include "dynarmic/common/fp/fpcr.h"

namespace Dynarmic::FP {

class FPSR;
enum class RoundingMode;

enum class FPType {
    Nonzero,
    Zero,
    Infinity,
    QNaN,
    SNaN,
};

constexpr size_t normalized_point_position = 62;

/// value = (sign ? -1 : +1) * mantissa/(2^62) * 2^exponent
/// 63rd bit of mantissa is always set (unless value is zero)
struct FPUnpacked {
    bool sign;
    int exponent;
    u64 mantissa;
};

inline bool operator==(const FPUnpacked& a, const FPUnpacked& b) {
    return std::tie(a.sign, a.exponent, a.mantissa) == std::tie(b.sign, b.exponent, b.mantissa);
}

/// return value = (sign ? -1 : +1) * value * 2^exponent
constexpr FPUnpacked ToNormalized(bool sign, int exponent, u64 value) {
    if (value == 0) {
        return {sign, 0, 0};
    }

    const int highest_bit = Common::HighestSetBit(value);
    const int offset = static_cast<int>(normalized_point_position) - highest_bit;
    value <<= offset;
    exponent -= offset - static_cast<int>(normalized_point_position);
    return {sign, exponent, value};
}

template<typename FPT>
std::tuple<FPType, bool, FPUnpacked> FPUnpackBase(FPT op, FPCR fpcr, FPSR& fpsr);

template<typename FPT>
std::tuple<FPType, bool, FPUnpacked> FPUnpack(FPT op, FPCR fpcr, FPSR& fpsr) {
    fpcr.AHP(false);
    return FPUnpackBase(op, fpcr, fpsr);
}

template<typename FPT>
std::tuple<FPType, bool, FPUnpacked> FPUnpackCV(FPT op, FPCR fpcr, FPSR& fpsr) {
    fpcr.FZ16(false);
    return FPUnpackBase(op, fpcr, fpsr);
}

template<typename FPT>
FPT FPRoundBase(FPUnpacked op, FPCR fpcr, RoundingMode rounding, FPSR& fpsr);

template<typename FPT>
FPT FPRound(FPUnpacked op, FPCR fpcr, RoundingMode rounding, FPSR& fpsr) {
    fpcr.AHP(false);
    return FPRoundBase<FPT>(op, fpcr, rounding, fpsr);
}

template<typename FPT>
FPT FPRoundCV(FPUnpacked op, FPCR fpcr, RoundingMode rounding, FPSR& fpsr) {
    fpcr.FZ16(false);
    return FPRoundBase<FPT>(op, fpcr, rounding, fpsr);
}

template<typename FPT>
FPT FPRound(FPUnpacked op, FPCR fpcr, FPSR& fpsr) {
    return FPRound<FPT>(op, fpcr, fpcr.RMode(), fpsr);
}

}  // namespace Dynarmic::FP

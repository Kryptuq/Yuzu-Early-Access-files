/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/fp/op/FPRoundInt.h"

#include "dynarmic/common/assert.h"
#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/common/fp/info.h"
#include "dynarmic/common/fp/mantissa_util.h"
#include "dynarmic/common/fp/process_exception.h"
#include "dynarmic/common/fp/process_nan.h"
#include "dynarmic/common/fp/rounding_mode.h"
#include "dynarmic/common/fp/unpacked.h"
#include "dynarmic/common/safe_ops.h"

namespace Dynarmic::FP {

template<typename FPT>
u64 FPRoundInt(FPT op, FPCR fpcr, RoundingMode rounding, bool exact, FPSR& fpsr) {
    ASSERT(rounding != RoundingMode::ToOdd);

    auto [type, sign, value] = FPUnpack<FPT>(op, fpcr, fpsr);

    if (type == FPType::SNaN || type == FPType::QNaN) {
        return FPProcessNaN(type, op, fpcr, fpsr);
    }

    if (type == FPType::Infinity) {
        return FPInfo<FPT>::Infinity(sign);
    }

    if (type == FPType::Zero) {
        return FPInfo<FPT>::Zero(sign);
    }

    // Reshift decimal point back to bit zero.
    const int exponent = value.exponent - normalized_point_position;

    if (exponent >= 0) {
        // Guaranteed to be an integer
        return op;
    }

    u64 int_result = sign ? Safe::Negate<u64>(value.mantissa) : static_cast<u64>(value.mantissa);
    const ResidualError error = ResidualErrorOnRightShift(int_result, -exponent);
    int_result = Safe::ArithmeticShiftLeft(int_result, exponent);

    bool round_up = false;
    switch (rounding) {
    case RoundingMode::ToNearest_TieEven:
        round_up = error > ResidualError::Half || (error == ResidualError::Half && Common::Bit<0>(int_result));
        break;
    case RoundingMode::TowardsPlusInfinity:
        round_up = error != ResidualError::Zero;
        break;
    case RoundingMode::TowardsMinusInfinity:
        round_up = false;
        break;
    case RoundingMode::TowardsZero:
        round_up = error != ResidualError::Zero && Common::MostSignificantBit(int_result);
        break;
    case RoundingMode::ToNearest_TieAwayFromZero:
        round_up = error > ResidualError::Half || (error == ResidualError::Half && !Common::MostSignificantBit(int_result));
        break;
    case RoundingMode::ToOdd:
        UNREACHABLE();
    }

    if (round_up) {
        int_result++;
    }

    const bool new_sign = Common::MostSignificantBit(int_result);
    const u64 abs_int_result = new_sign ? Safe::Negate<u64>(int_result) : static_cast<u64>(int_result);

    const FPT result = int_result == 0
                         ? FPInfo<FPT>::Zero(sign)
                         : FPRound<FPT>(FPUnpacked{new_sign, normalized_point_position, abs_int_result}, fpcr, RoundingMode::TowardsZero, fpsr);

    if (error != ResidualError::Zero && exact) {
        FPProcessException(FPExc::Inexact, fpcr, fpsr);
    }

    return result;
}

template u64 FPRoundInt<u16>(u16 op, FPCR fpcr, RoundingMode rounding, bool exact, FPSR& fpsr);
template u64 FPRoundInt<u32>(u32 op, FPCR fpcr, RoundingMode rounding, bool exact, FPSR& fpsr);
template u64 FPRoundInt<u64>(u64 op, FPCR fpcr, RoundingMode rounding, bool exact, FPSR& fpsr);

}  // namespace Dynarmic::FP

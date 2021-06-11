/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/fp/op/FPRSqrtEstimate.h"

#include "dynarmic/common/common_types.h"
#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/common/fp/info.h"
#include "dynarmic/common/fp/process_exception.h"
#include "dynarmic/common/fp/process_nan.h"
#include "dynarmic/common/fp/unpacked.h"
#include "dynarmic/common/math_util.h"
#include "dynarmic/common/safe_ops.h"

namespace Dynarmic::FP {

template<typename FPT>
FPT FPRSqrtEstimate(FPT op, FPCR fpcr, FPSR& fpsr) {
    const auto [type, sign, value] = FPUnpack<FPT>(op, fpcr, fpsr);

    if (type == FPType::SNaN || type == FPType::QNaN) {
        return FPProcessNaN(type, op, fpcr, fpsr);
    }

    if (type == FPType::Zero) {
        FPProcessException(FPExc::DivideByZero, fpcr, fpsr);
        return FPInfo<FPT>::Infinity(sign);
    }

    if (sign) {
        FPProcessException(FPExc::InvalidOp, fpcr, fpsr);
        return FPInfo<FPT>::DefaultNaN();
    }

    if (type == FPType::Infinity) {
        // Note: Just +Inf reaches here, negatives are handled in the case above.
        return FPInfo<FPT>::Zero(false);
    }

    const int result_exponent = (-(value.exponent + 1)) >> 1;
    const bool was_exponent_odd = (value.exponent) % 2 == 0;

    const u64 scaled = Safe::LogicalShiftRight(value.mantissa, normalized_point_position - (was_exponent_odd ? 7 : 8));
    const u64 estimate = Common::RecipSqrtEstimate(scaled);

    const FPT bits_exponent = static_cast<FPT>(result_exponent + FPInfo<FPT>::exponent_bias);
    const FPT bits_mantissa = static_cast<FPT>(estimate << (FPInfo<FPT>::explicit_mantissa_width - 8));
    return (bits_exponent << FPInfo<FPT>::explicit_mantissa_width) | (bits_mantissa & FPInfo<FPT>::mantissa_mask);
}

template u16 FPRSqrtEstimate<u16>(u16 op, FPCR fpcr, FPSR& fpsr);
template u32 FPRSqrtEstimate<u32>(u32 op, FPCR fpcr, FPSR& fpsr);
template u64 FPRSqrtEstimate<u64>(u64 op, FPCR fpcr, FPSR& fpsr);

}  // namespace Dynarmic::FP

/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/fp/op/FPRecipEstimate.h"

#include <tuple>

#include <mcl/assert.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/common/fp/info.h"
#include "dynarmic/common/fp/process_exception.h"
#include "dynarmic/common/fp/process_nan.h"
#include "dynarmic/common/fp/unpacked.h"
#include "dynarmic/common/math_util.h"

namespace Dynarmic::FP {

template<typename FPT>
FPT FPRecipEstimate(FPT op, FPCR fpcr, FPSR& fpsr) {
    FPType type;
    bool sign;
    FPUnpacked value;
    std::tie(type, sign, value) = FPUnpack<FPT>(op, fpcr, fpsr);

    if (type == FPType::SNaN || type == FPType::QNaN) {
        return FPProcessNaN(type, op, fpcr, fpsr);
    }

    if (type == FPType::Infinity) {
        return FPInfo<FPT>::Zero(sign);
    }

    if (type == FPType::Zero) {
        FPProcessException(FPExc::DivideByZero, fpcr, fpsr);
        return FPInfo<FPT>::Infinity(sign);
    }

    if (value.exponent < FPInfo<FPT>::exponent_min - 2) {
        const bool overflow_to_inf = [&] {
            switch (fpcr.RMode()) {
            case RoundingMode::ToNearest_TieEven:
                return true;
            case RoundingMode::TowardsPlusInfinity:
                return !sign;
            case RoundingMode::TowardsMinusInfinity:
                return sign;
            case RoundingMode::TowardsZero:
                return false;
            default:
                UNREACHABLE();
            }
        }();

        FPProcessException(FPExc::Overflow, fpcr, fpsr);
        FPProcessException(FPExc::Inexact, fpcr, fpsr);
        return overflow_to_inf ? FPInfo<FPT>::Infinity(sign) : FPInfo<FPT>::MaxNormal(sign);
    }

    if ((fpcr.FZ() && !std::is_same_v<FPT, u16>) || (fpcr.FZ16() && std::is_same_v<FPT, u16>)) {
        if (value.exponent >= -FPInfo<FPT>::exponent_min) {
            fpsr.UFC(true);
            return FPInfo<FPT>::Zero(sign);
        }
    }

    const u64 scaled = value.mantissa >> (normalized_point_position - 8);
    u64 estimate = static_cast<u64>(Common::RecipEstimate(scaled)) << (FPInfo<FPT>::explicit_mantissa_width - 8);
    int result_exponent = -(value.exponent + 1);
    if (result_exponent < FPInfo<FPT>::exponent_min) {
        switch (result_exponent) {
        case (FPInfo<FPT>::exponent_min - 1):
            estimate |= FPInfo<FPT>::implicit_leading_bit;
            estimate >>= 1;
            break;
        case (FPInfo<FPT>::exponent_min - 2):
            estimate |= FPInfo<FPT>::implicit_leading_bit;
            estimate >>= 2;
            result_exponent++;
            break;
        default:
            UNREACHABLE();
        }
    }

    const FPT bits_sign = FPInfo<FPT>::Zero(sign);
    const FPT bits_exponent = static_cast<FPT>(result_exponent + FPInfo<FPT>::exponent_bias);
    const FPT bits_mantissa = static_cast<FPT>(estimate);
    return FPT((bits_exponent << FPInfo<FPT>::explicit_mantissa_width) | (bits_mantissa & FPInfo<FPT>::mantissa_mask) | bits_sign);
}

template u16 FPRecipEstimate<u16>(u16 op, FPCR fpcr, FPSR& fpsr);
template u32 FPRecipEstimate<u32>(u32 op, FPCR fpcr, FPSR& fpsr);
template u64 FPRecipEstimate<u64>(u64 op, FPCR fpcr, FPSR& fpsr);

}  // namespace Dynarmic::FP

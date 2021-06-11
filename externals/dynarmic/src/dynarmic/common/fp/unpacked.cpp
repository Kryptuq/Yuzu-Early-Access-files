/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/fp/unpacked.h"

#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/common/fp/info.h"
#include "dynarmic/common/fp/mantissa_util.h"
#include "dynarmic/common/fp/process_exception.h"
#include "dynarmic/common/fp/rounding_mode.h"
#include "dynarmic/common/safe_ops.h"

namespace Dynarmic::FP {

template<typename FPT>
std::tuple<FPType, bool, FPUnpacked> FPUnpackBase(FPT op, FPCR fpcr, [[maybe_unused]] FPSR& fpsr) {
    constexpr size_t sign_bit = FPInfo<FPT>::exponent_width + FPInfo<FPT>::explicit_mantissa_width;
    constexpr size_t exponent_high_bit = FPInfo<FPT>::exponent_width + FPInfo<FPT>::explicit_mantissa_width - 1;
    constexpr size_t exponent_low_bit = FPInfo<FPT>::explicit_mantissa_width;
    constexpr size_t mantissa_high_bit = FPInfo<FPT>::explicit_mantissa_width - 1;
    constexpr size_t mantissa_low_bit = 0;
    constexpr int denormal_exponent = FPInfo<FPT>::exponent_min - int(FPInfo<FPT>::explicit_mantissa_width);

    constexpr bool is_half_precision = std::is_same_v<FPT, u16>;
    const bool sign = Common::Bit<sign_bit>(op);
    const FPT exp_raw = Common::Bits<exponent_low_bit, exponent_high_bit>(op);
    const FPT frac_raw = Common::Bits<mantissa_low_bit, mantissa_high_bit>(op);

    if (exp_raw == 0) {
        if constexpr (is_half_precision) {
            if (frac_raw == 0 || fpcr.FZ16()) {
                return {FPType::Zero, sign, {sign, 0, 0}};
            }
            return {FPType::Nonzero, sign, ToNormalized(sign, denormal_exponent, frac_raw)};
        } else {
            if (frac_raw == 0 || fpcr.FZ()) {
                if (frac_raw != 0) {
                    FPProcessException(FPExc::InputDenorm, fpcr, fpsr);
                }
                return {FPType::Zero, sign, {sign, 0, 0}};
            }

            return {FPType::Nonzero, sign, ToNormalized(sign, denormal_exponent, frac_raw)};
        }
    }

    const bool exp_all_ones = exp_raw == Common::Ones<FPT>(FPInfo<FPT>::exponent_width);
    const bool ahp_disabled = is_half_precision && !fpcr.AHP();
    if ((exp_all_ones && !is_half_precision) || (exp_all_ones && ahp_disabled)) {
        if (frac_raw == 0) {
            return {FPType::Infinity, sign, ToNormalized(sign, 1000000, 1)};
        }

        const bool is_quiet = Common::Bit<mantissa_high_bit>(frac_raw);
        return {is_quiet ? FPType::QNaN : FPType::SNaN, sign, {sign, 0, 0}};
    }

    const int exp = static_cast<int>(exp_raw) - FPInfo<FPT>::exponent_bias;
    const u64 frac = static_cast<u64>(frac_raw | FPInfo<FPT>::implicit_leading_bit) << (normalized_point_position - FPInfo<FPT>::explicit_mantissa_width);
    return {FPType::Nonzero, sign, {sign, exp, frac}};
}

template std::tuple<FPType, bool, FPUnpacked> FPUnpackBase<u16>(u16 op, FPCR fpcr, FPSR& fpsr);
template std::tuple<FPType, bool, FPUnpacked> FPUnpackBase<u32>(u32 op, FPCR fpcr, FPSR& fpsr);
template std::tuple<FPType, bool, FPUnpacked> FPUnpackBase<u64>(u64 op, FPCR fpcr, FPSR& fpsr);

template<size_t F>
std::tuple<bool, int, u64, ResidualError> Normalize(FPUnpacked op, int extra_right_shift = 0) {
    const int highest_set_bit = Common::HighestSetBit(op.mantissa);
    const int shift_amount = highest_set_bit - static_cast<int>(F) + extra_right_shift;
    const u64 mantissa = Safe::LogicalShiftRight(op.mantissa, shift_amount);
    const ResidualError error = ResidualErrorOnRightShift(op.mantissa, shift_amount);
    const int exponent = op.exponent + highest_set_bit - normalized_point_position;
    return std::make_tuple(op.sign, exponent, mantissa, error);
}

template<typename FPT>
FPT FPRoundBase(FPUnpacked op, FPCR fpcr, RoundingMode rounding, FPSR& fpsr) {
    ASSERT(op.mantissa != 0);
    ASSERT(rounding != RoundingMode::ToNearest_TieAwayFromZero);

    constexpr int minimum_exp = FPInfo<FPT>::exponent_min;
    constexpr size_t E = FPInfo<FPT>::exponent_width;
    constexpr size_t F = FPInfo<FPT>::explicit_mantissa_width;
    constexpr bool isFP16 = FPInfo<FPT>::total_width == 16;

    auto [sign, exponent, mantissa, error] = Normalize<F>(op);

    if (((!isFP16 && fpcr.FZ()) || (isFP16 && fpcr.FZ16())) && exponent < minimum_exp) {
        fpsr.UFC(true);
        return FPInfo<FPT>::Zero(sign);
    }

    int biased_exp = std::max<int>(exponent - minimum_exp + 1, 0);
    if (biased_exp == 0) {
        std::tie(sign, exponent, mantissa, error) = Normalize<F>(op, minimum_exp - exponent);
    }

    if (biased_exp == 0 && (error != ResidualError::Zero || fpcr.UFE())) {
        FPProcessException(FPExc::Underflow, fpcr, fpsr);
    }

    bool round_up = false, overflow_to_inf = false;
    switch (rounding) {
    case RoundingMode::ToNearest_TieEven: {
        round_up = (error > ResidualError::Half) || (error == ResidualError::Half && Common::Bit<0>(mantissa));
        overflow_to_inf = true;
        break;
    }
    case RoundingMode::TowardsPlusInfinity:
        round_up = error != ResidualError::Zero && !sign;
        overflow_to_inf = !sign;
        break;
    case RoundingMode::TowardsMinusInfinity:
        round_up = error != ResidualError::Zero && sign;
        overflow_to_inf = sign;
        break;
    default:
        break;
    }

    if (round_up) {
        if ((mantissa & FPInfo<FPT>::mantissa_mask) == FPInfo<FPT>::mantissa_mask) {
            // Overflow on rounding up is going to happen
            if (mantissa == FPInfo<FPT>::mantissa_mask) {
                // Rounding up from denormal to normal
                mantissa++;
                biased_exp++;
            } else {
                // Rounding up to next exponent
                mantissa = (mantissa + 1) / 2;
                biased_exp++;
            }
        } else {
            mantissa++;
        }
    }

    if (error != ResidualError::Zero && rounding == RoundingMode::ToOdd) {
        mantissa = Common::ModifyBit<0>(mantissa, true);
    }

    FPT result = 0;
#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4127)  // C4127: conditional expression is constant
#endif
    if (!isFP16 || !fpcr.AHP()) {
#ifdef _MSC_VER
#    pragma warning(pop)
#endif
        constexpr int max_biased_exp = (1 << E) - 1;
        if (biased_exp >= max_biased_exp) {
            result = overflow_to_inf ? FPInfo<FPT>::Infinity(sign) : FPInfo<FPT>::MaxNormal(sign);
            FPProcessException(FPExc::Overflow, fpcr, fpsr);
            FPProcessException(FPExc::Inexact, fpcr, fpsr);
        } else {
            result = sign ? 1 : 0;
            result <<= E;
            result += FPT(biased_exp);
            result <<= F;
            result |= static_cast<FPT>(mantissa) & FPInfo<FPT>::mantissa_mask;
            if (error != ResidualError::Zero) {
                FPProcessException(FPExc::Inexact, fpcr, fpsr);
            }
        }
    } else {
        constexpr int max_biased_exp = (1 << E);
        if (biased_exp >= max_biased_exp) {
            result = sign ? 0xFFFF : 0x7FFF;
            FPProcessException(FPExc::InvalidOp, fpcr, fpsr);
        } else {
            result = sign ? 1 : 0;
            result <<= E;
            result += FPT(biased_exp);
            result <<= F;
            result |= static_cast<FPT>(mantissa) & FPInfo<FPT>::mantissa_mask;
            if (error != ResidualError::Zero) {
                FPProcessException(FPExc::Inexact, fpcr, fpsr);
            }
        }
    }
    return result;
}

template u16 FPRoundBase<u16>(FPUnpacked op, FPCR fpcr, RoundingMode rounding, FPSR& fpsr);
template u32 FPRoundBase<u32>(FPUnpacked op, FPCR fpcr, RoundingMode rounding, FPSR& fpsr);
template u64 FPRoundBase<u64>(FPUnpacked op, FPCR fpcr, RoundingMode rounding, FPSR& fpsr);

}  // namespace Dynarmic::FP

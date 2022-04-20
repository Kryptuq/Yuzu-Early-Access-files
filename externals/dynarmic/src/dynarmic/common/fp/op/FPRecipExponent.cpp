/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/fp/op/FPRecipExponent.h"

#include <mcl/bit/bit_field.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/common/fp/info.h"
#include "dynarmic/common/fp/process_nan.h"
#include "dynarmic/common/fp/unpacked.h"

namespace Dynarmic::FP {
namespace {
template<typename FPT>
FPT DetermineExponentValue(size_t value) {
    if constexpr (sizeof(FPT) == sizeof(u32)) {
        return static_cast<FPT>(mcl::bit::get_bits<23, 30>(value));
    } else if constexpr (sizeof(FPT) == sizeof(u64)) {
        return static_cast<FPT>(mcl::bit::get_bits<52, 62>(value));
    } else {
        return static_cast<FPT>(mcl::bit::get_bits<10, 14>(value));
    }
}
}  // Anonymous namespace

template<typename FPT>
FPT FPRecipExponent(FPT op, FPCR fpcr, FPSR& fpsr) {
    const auto [type, sign, value] = FPUnpack<FPT>(op, fpcr, fpsr);
    (void)value;

    if (type == FPType::SNaN || type == FPType::QNaN) {
        return FPProcessNaN(type, op, fpcr, fpsr);
    }

    const FPT sign_bits = FPInfo<FPT>::Zero(sign);
    const FPT exponent = DetermineExponentValue<FPT>(op);

    // Zero and denormals
    if (exponent == 0) {
        const FPT max_exponent = mcl::bit::ones<FPT>(FPInfo<FPT>::exponent_width) - 1;
        return FPT(sign_bits | (max_exponent << FPInfo<FPT>::explicit_mantissa_width));
    }

    // Infinities and normals
    const FPT negated_exponent = FPT(~exponent);
    const FPT adjusted_exponent = FPT(negated_exponent << FPInfo<FPT>::explicit_mantissa_width) & FPInfo<FPT>::exponent_mask;
    return FPT(sign_bits | adjusted_exponent);
}

template u16 FPRecipExponent<u16>(u16 op, FPCR fpcr, FPSR& fpsr);
template u32 FPRecipExponent<u32>(u32 op, FPCR fpcr, FPSR& fpsr);
template u64 FPRecipExponent<u64>(u64 op, FPCR fpcr, FPSR& fpsr);

}  // namespace Dynarmic::FP

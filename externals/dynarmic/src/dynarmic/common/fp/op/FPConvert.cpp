/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/fp/op/FPConvert.h"

#include <mcl/bit/bit_field.hpp>
#include <mcl/bitsizeof.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/common/fp/info.h"
#include "dynarmic/common/fp/process_exception.h"
#include "dynarmic/common/fp/unpacked.h"

namespace Dynarmic::FP {
namespace {
template<typename FPT_TO, typename FPT_FROM>
FPT_TO FPConvertNaN(FPT_FROM op) {
    const bool sign = mcl::bit::get_bit<mcl::bitsizeof<FPT_FROM> - 1>(op);
    const u64 frac = [op] {
        if constexpr (sizeof(FPT_FROM) == sizeof(u64)) {
            return mcl::bit::get_bits<0, 50>(op);
        } else if constexpr (sizeof(FPT_FROM) == sizeof(u32)) {
            return u64{mcl::bit::get_bits<0, 21>(op)} << 29;
        } else {
            return u64{mcl::bit::get_bits<0, 8>(op)} << 42;
        }
    }();

    const size_t dest_bit_size = mcl::bitsizeof<FPT_TO>;
    const u64 shifted_sign = u64{sign} << (dest_bit_size - 1);
    const u64 exponent = mcl::bit::ones<u64>(dest_bit_size - FPInfo<FPT_TO>::explicit_mantissa_width);

    if constexpr (sizeof(FPT_TO) == sizeof(u64)) {
        return FPT_TO(shifted_sign | exponent << 51 | frac);
    } else if constexpr (sizeof(FPT_TO) == sizeof(u32)) {
        return FPT_TO(shifted_sign | exponent << 22 | mcl::bit::get_bits<29, 50>(frac));
    } else {
        return FPT_TO(shifted_sign | exponent << 9 | mcl::bit::get_bits<42, 50>(frac));
    }
}
}  // Anonymous namespace

template<typename FPT_TO, typename FPT_FROM>
FPT_TO FPConvert(FPT_FROM op, FPCR fpcr, RoundingMode rounding_mode, FPSR& fpsr) {
    const auto [type, sign, value] = FPUnpackCV<FPT_FROM>(op, fpcr, fpsr);
    const bool is_althp = mcl::bitsizeof<FPT_TO> == 16 && fpcr.AHP();

    if (type == FPType::SNaN || type == FPType::QNaN) {
        std::uintmax_t result{};

        if (is_althp) {
            result = FPInfo<FPT_TO>::Zero(sign);
        } else if (fpcr.DN()) {
            result = FPInfo<FPT_TO>::DefaultNaN();
        } else {
            result = FPConvertNaN<FPT_TO>(op);
        }

        if (type == FPType::SNaN || is_althp) {
            FPProcessException(FPExc::InvalidOp, fpcr, fpsr);
        }

        return FPT_TO(result);
    }

    if (type == FPType::Infinity) {
        if (is_althp) {
            FPProcessException(FPExc::InvalidOp, fpcr, fpsr);
            return FPT_TO(u32{sign} << 15 | 0b111111111111111);
        }

        return FPInfo<FPT_TO>::Infinity(sign);
    }

    if (type == FPType::Zero) {
        return FPInfo<FPT_TO>::Zero(sign);
    }

    return FPRoundCV<FPT_TO>(value, fpcr, rounding_mode, fpsr);
}

template u16 FPConvert<u16, u32>(u32 op, FPCR fpcr, RoundingMode rounding_mode, FPSR& fpsr);
template u16 FPConvert<u16, u64>(u64 op, FPCR fpcr, RoundingMode rounding_mode, FPSR& fpsr);
template u32 FPConvert<u32, u16>(u16 op, FPCR fpcr, RoundingMode rounding_mode, FPSR& fpsr);
template u32 FPConvert<u32, u64>(u64 op, FPCR fpcr, RoundingMode rounding_mode, FPSR& fpsr);
template u64 FPConvert<u64, u16>(u16 op, FPCR fpcr, RoundingMode rounding_mode, FPSR& fpsr);
template u64 FPConvert<u64, u32>(u32 op, FPCR fpcr, RoundingMode rounding_mode, FPSR& fpsr);

}  // namespace Dynarmic::FP

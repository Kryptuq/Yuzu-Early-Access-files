/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::FMOV_float(Imm<2> type, Vec Vn, Vec Vd) {
    const auto datasize = FPGetDataSize(type);
    if (!datasize) {
        return UnallocatedEncoding();
    }

    const IR::U16U32U64 operand = V_scalar(*datasize, Vn);

    V_scalar(*datasize, Vd, operand);
    return true;
}

bool TranslatorVisitor::FABS_float(Imm<2> type, Vec Vn, Vec Vd) {
    const auto datasize = FPGetDataSize(type);
    if (!datasize) {
        return UnallocatedEncoding();
    }

    const IR::U16U32U64 operand = V_scalar(*datasize, Vn);
    const IR::U16U32U64 result = ir.FPAbs(operand);
    V_scalar(*datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FNEG_float(Imm<2> type, Vec Vn, Vec Vd) {
    const auto datasize = FPGetDataSize(type);
    if (!datasize) {
        return UnallocatedEncoding();
    }

    const IR::U16U32U64 operand = V_scalar(*datasize, Vn);
    const IR::U16U32U64 result = ir.FPNeg(operand);
    V_scalar(*datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FSQRT_float(Imm<2> type, Vec Vn, Vec Vd) {
    const auto datasize = FPGetDataSize(type);
    if (!datasize || *datasize == 16) {
        return UnallocatedEncoding();
    }

    const IR::U32U64 operand = V_scalar(*datasize, Vn);
    const IR::U32U64 result = ir.FPSqrt(operand);
    V_scalar(*datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FMOV_float_imm(Imm<2> type, Imm<8> imm8, Vec Vd) {
    const auto datasize = FPGetDataSize(type);
    if (!datasize) {
        return UnallocatedEncoding();
    }

    IR::UAny result = [&]() -> IR::UAny {
        switch (*datasize) {
        case 16: {
            const u16 sign = imm8.Bit<7>() ? 1 : 0;
            const u16 exp = (imm8.Bit<6>() ? 0b0'1100 : 0b1'0000) | imm8.Bits<4, 5, u16>();
            const u16 fract = imm8.Bits<0, 3, u16>() << 6;
            return ir.Imm16((sign << 15) | (exp << 10) | fract);
        }
        case 32: {
            const u32 sign = imm8.Bit<7>() ? 1 : 0;
            const u32 exp = (imm8.Bit<6>() ? 0b0111'1100 : 0b1000'0000) | imm8.Bits<4, 5, u32>();
            const u32 fract = imm8.Bits<0, 3, u32>() << 19;
            return ir.Imm32((sign << 31) | (exp << 23) | fract);
        }
        case 64:
        default: {
            const u64 sign = imm8.Bit<7>() ? 1 : 0;
            const u64 exp = (imm8.Bit<6>() ? 0b011'1111'1100 : 0b100'0000'0000) | imm8.Bits<4, 5, u64>();
            const u64 fract = imm8.Bits<0, 3, u64>() << 48;
            return ir.Imm64((sign << 63) | (exp << 52) | fract);
        }
        }
    }();

    V_scalar(*datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FCVT_float(Imm<2> type, Imm<2> opc, Vec Vn, Vec Vd) {
    if (type == opc) {
        return UnallocatedEncoding();
    }

    const auto srcsize = FPGetDataSize(type);
    const auto dstsize = FPGetDataSize(opc);

    if (!srcsize || !dstsize) {
        return UnallocatedEncoding();
    }

    const IR::UAny operand = V_scalar(*srcsize, Vn);
    const auto rounding_mode = ir.current_location->FPCR().RMode();

    IR::UAny result;
    switch (*srcsize) {
    case 16:
        switch (*dstsize) {
        case 32:
            result = ir.FPHalfToSingle(operand, rounding_mode);
            break;
        case 64:
            result = ir.FPHalfToDouble(operand, rounding_mode);
            break;
        }
        break;
    case 32:
        switch (*dstsize) {
        case 16:
            result = ir.FPSingleToHalf(operand, rounding_mode);
            break;
        case 64:
            result = ir.FPSingleToDouble(operand, rounding_mode);
            break;
        }
        break;
    case 64:
        switch (*dstsize) {
        case 16:
            result = ir.FPDoubleToHalf(operand, rounding_mode);
            break;
        case 32:
            result = ir.FPDoubleToSingle(operand, rounding_mode);
            break;
        }
        break;
    }

    V_scalar(*dstsize, Vd, result);

    return true;
}

static bool FloatingPointRoundToIntegral(TranslatorVisitor& v, Imm<2> type, Vec Vn, Vec Vd, FP::RoundingMode rounding_mode, bool exact) {
    const auto datasize = FPGetDataSize(type);
    if (!datasize) {
        return v.UnallocatedEncoding();
    }

    const IR::U16U32U64 operand = v.V_scalar(*datasize, Vn);
    const IR::U16U32U64 result = v.ir.FPRoundInt(operand, rounding_mode, exact);
    v.V_scalar(*datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FRINTN_float(Imm<2> type, Vec Vn, Vec Vd) {
    return FloatingPointRoundToIntegral(*this, type, Vn, Vd, FP::RoundingMode::ToNearest_TieEven, false);
}

bool TranslatorVisitor::FRINTP_float(Imm<2> type, Vec Vn, Vec Vd) {
    return FloatingPointRoundToIntegral(*this, type, Vn, Vd, FP::RoundingMode::TowardsPlusInfinity, false);
}

bool TranslatorVisitor::FRINTM_float(Imm<2> type, Vec Vn, Vec Vd) {
    return FloatingPointRoundToIntegral(*this, type, Vn, Vd, FP::RoundingMode::TowardsMinusInfinity, false);
}

bool TranslatorVisitor::FRINTZ_float(Imm<2> type, Vec Vn, Vec Vd) {
    return FloatingPointRoundToIntegral(*this, type, Vn, Vd, FP::RoundingMode::TowardsZero, false);
}

bool TranslatorVisitor::FRINTA_float(Imm<2> type, Vec Vn, Vec Vd) {
    return FloatingPointRoundToIntegral(*this, type, Vn, Vd, FP::RoundingMode::ToNearest_TieAwayFromZero, false);
}

bool TranslatorVisitor::FRINTX_float(Imm<2> type, Vec Vn, Vec Vd) {
    return FloatingPointRoundToIntegral(*this, type, Vn, Vd, ir.current_location->FPCR().RMode(), true);
}

bool TranslatorVisitor::FRINTI_float(Imm<2> type, Vec Vn, Vec Vd) {
    return FloatingPointRoundToIntegral(*this, type, Vn, Vd, ir.current_location->FPCR().RMode(), false);
}

}  // namespace Dynarmic::A64

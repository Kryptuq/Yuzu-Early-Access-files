/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/fp/rounding_mode.h"
#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
enum class ComparisonType {
    EQ,
    GE,
    GT,
    LE,
    LT,
};

bool CompareAgainstZero(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vn, Vec Vd, ComparisonType type) {
    if (size == 0b11 && !Q) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand = v.V(datasize, Vn);
    const IR::U128 zero = v.ir.ZeroVector();
    IR::U128 result = [&] {
        switch (type) {
        case ComparisonType::EQ:
            return v.ir.VectorEqual(esize, operand, zero);
        case ComparisonType::GE:
            return v.ir.VectorGreaterEqualSigned(esize, operand, zero);
        case ComparisonType::GT:
            return v.ir.VectorGreaterSigned(esize, operand, zero);
        case ComparisonType::LE:
            return v.ir.VectorLessEqualSigned(esize, operand, zero);
        case ComparisonType::LT:
        default:
            return v.ir.VectorLessSigned(esize, operand, zero);
        }
    }();

    if (datasize == 64) {
        result = v.ir.VectorZeroUpper(result);
    }

    v.V(datasize, Vd, result);
    return true;
}

bool FPCompareAgainstZero(TranslatorVisitor& v, bool Q, bool sz, Vec Vn, Vec Vd, ComparisonType type) {
    if (sz && !Q) {
        return v.ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand = v.V(datasize, Vn);
    const IR::U128 zero = v.ir.ZeroVector();
    const IR::U128 result = [&] {
        switch (type) {
        case ComparisonType::EQ:
            return v.ir.FPVectorEqual(esize, operand, zero);
        case ComparisonType::GE:
            return v.ir.FPVectorGreaterEqual(esize, operand, zero);
        case ComparisonType::GT:
            return v.ir.FPVectorGreater(esize, operand, zero);
        case ComparisonType::LE:
            return v.ir.FPVectorGreaterEqual(esize, zero, operand);
        case ComparisonType::LT:
            return v.ir.FPVectorGreater(esize, zero, operand);
        }

        UNREACHABLE();
    }();

    v.V(datasize, Vd, result);
    return true;
}

enum class Signedness {
    Signed,
    Unsigned
};

bool IntegerConvertToFloat(TranslatorVisitor& v, bool Q, bool sz, Vec Vn, Vec Vd, Signedness signedness) {
    if (sz && !Q) {
        return v.ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = sz ? 64 : 32;
    const FP::RoundingMode rounding_mode = v.ir.current_location->FPCR().RMode();

    const IR::U128 operand = v.V(datasize, Vn);
    const IR::U128 result = signedness == Signedness::Signed
                              ? v.ir.FPVectorFromSignedFixed(esize, operand, 0, rounding_mode)
                              : v.ir.FPVectorFromUnsignedFixed(esize, operand, 0, rounding_mode);

    v.V(datasize, Vd, result);
    return true;
}

bool FloatConvertToInteger(TranslatorVisitor& v, bool Q, bool sz, Vec Vn, Vec Vd, Signedness signedness, FP::RoundingMode rounding_mode) {
    if (sz && !Q) {
        return v.ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = sz ? 64 : 32;

    const IR::U128 operand = v.V(datasize, Vn);
    const IR::U128 result = signedness == Signedness::Signed
                              ? v.ir.FPVectorToSignedFixed(esize, operand, 0, rounding_mode)
                              : v.ir.FPVectorToUnsignedFixed(esize, operand, 0, rounding_mode);

    v.V(datasize, Vd, result);
    return true;
}

bool FloatRoundToIntegral(TranslatorVisitor& v, bool Q, bool sz, Vec Vn, Vec Vd, FP::RoundingMode rounding_mode, bool exact) {
    if (sz && !Q) {
        return v.ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = sz ? 64 : 32;

    const IR::U128 operand = v.V(datasize, Vn);
    const IR::U128 result = v.ir.FPVectorRoundInt(esize, operand, rounding_mode, exact);

    v.V(datasize, Vd, result);
    return true;
}

bool FloatRoundToIntegralHalfPrecision(TranslatorVisitor& v, bool Q, Vec Vn, Vec Vd, FP::RoundingMode rounding_mode, bool exact) {
    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 16;

    const IR::U128 operand = v.V(datasize, Vn);
    const IR::U128 result = v.ir.FPVectorRoundInt(esize, operand, rounding_mode, exact);

    v.V(datasize, Vd, result);
    return true;
}

bool SaturatedNarrow(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vn, Vec Vd, IR::U128 (IR::IREmitter::*fn)(size_t, const IR::U128&)) {
    if (size == 0b11) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = 64;
    const size_t part = Q ? 1 : 0;

    const IR::U128 operand = v.V(2 * datasize, Vn);
    const IR::U128 result = (v.ir.*fn)(2 * esize, operand);

    v.Vpart(datasize, Vd, part, result);
    return true;
}

enum class PairedAddLongExtraBehavior {
    None,
    Accumulate,
};

bool PairedAddLong(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vn, Vec Vd, Signedness sign, PairedAddLongExtraBehavior behavior) {
    if (size == 0b11) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand = v.V(datasize, Vn);
    IR::U128 result = [&] {
        if (sign == Signedness::Signed) {
            return v.ir.VectorPairedAddSignedWiden(esize, operand);
        }

        return v.ir.VectorPairedAddUnsignedWiden(esize, operand);
    }();

    if (behavior == PairedAddLongExtraBehavior::Accumulate) {
        result = v.ir.VectorAdd(esize * 2, v.V(datasize, Vd), result);
    }

    if (datasize == 64) {
        result = v.ir.VectorZeroUpper(result);
    }

    v.V(datasize, Vd, result);
    return true;
}

}  // Anonymous namespace

bool TranslatorVisitor::CLS_asimd(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    if (size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 shifted = ir.VectorArithmeticShiftRight(esize, operand, static_cast<u8>(esize));
    const IR::U128 xored = ir.VectorEor(operand, shifted);
    const IR::U128 clz = ir.VectorCountLeadingZeros(esize, xored);
    IR::U128 result = ir.VectorSub(esize, clz, ir.VectorBroadcast(esize, I(esize, 1)));

    if (datasize == 64) {
        result = ir.VectorZeroUpper(result);
    }

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::CLZ_asimd(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    if (size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand = V(datasize, Vn);
    IR::U128 result = ir.VectorCountLeadingZeros(esize, operand);

    if (datasize == 64) {
        result = ir.VectorZeroUpper(result);
    }

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::CNT(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    if (size != 0b00) {
        return ReservedValue();
    }
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 result = ir.VectorPopulationCount(operand);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::CMGE_zero_2(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return CompareAgainstZero(*this, Q, size, Vn, Vd, ComparisonType::GE);
}

bool TranslatorVisitor::CMGT_zero_2(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return CompareAgainstZero(*this, Q, size, Vn, Vd, ComparisonType::GT);
}

bool TranslatorVisitor::CMEQ_zero_2(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return CompareAgainstZero(*this, Q, size, Vn, Vd, ComparisonType::EQ);
}

bool TranslatorVisitor::CMLE_2(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return CompareAgainstZero(*this, Q, size, Vn, Vd, ComparisonType::LE);
}

bool TranslatorVisitor::CMLT_2(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return CompareAgainstZero(*this, Q, size, Vn, Vd, ComparisonType::LT);
}

bool TranslatorVisitor::ABS_2(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    if (!Q && size == 0b11) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();

    const IR::U128 data = V(datasize, Vn);
    const IR::U128 result = ir.VectorAbs(esize, data);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::XTN(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    if (size == 0b11) {
        return ReservedValue();
    }
    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = 64;
    const size_t part = Q ? 1 : 0;

    const IR::U128 operand = V(2 * datasize, Vn);
    const IR::U128 result = ir.VectorNarrow(2 * esize, operand);

    Vpart(datasize, Vd, part, result);
    return true;
}

bool TranslatorVisitor::FABS_1(bool Q, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 16;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 result = ir.FPVectorAbs(esize, operand);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FABS_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = sz ? 64 : 32;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 result = ir.FPVectorAbs(esize, operand);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FCMEQ_zero_3(bool Q, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 zero = ir.ZeroVector();
    const IR::U128 result = ir.FPVectorEqual(16, operand, zero);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FCMEQ_zero_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FPCompareAgainstZero(*this, Q, sz, Vn, Vd, ComparisonType::EQ);
}

bool TranslatorVisitor::FCMGE_zero_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FPCompareAgainstZero(*this, Q, sz, Vn, Vd, ComparisonType::GE);
}

bool TranslatorVisitor::FCMGT_zero_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FPCompareAgainstZero(*this, Q, sz, Vn, Vd, ComparisonType::GT);
}

bool TranslatorVisitor::FCMLE_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FPCompareAgainstZero(*this, Q, sz, Vn, Vd, ComparisonType::LE);
}

bool TranslatorVisitor::FCMLT_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FPCompareAgainstZero(*this, Q, sz, Vn, Vd, ComparisonType::LT);
}

bool TranslatorVisitor::FCVTL(bool Q, bool sz, Vec Vn, Vec Vd) {
    const size_t esize = sz ? 32 : 16;
    const size_t datasize = 64;
    const size_t num_elements = datasize / esize;

    const IR::U128 part = Vpart(64, Vn, Q);
    const auto rounding_mode = ir.current_location->FPCR().RMode();
    IR::U128 result = ir.ZeroVector();

    for (size_t i = 0; i < num_elements; i++) {
        IR::U16U32U64 element = ir.VectorGetElement(esize, part, i);

        if (esize == 16) {
            element = ir.FPHalfToSingle(element, rounding_mode);
        } else if (esize == 32) {
            element = ir.FPSingleToDouble(element, rounding_mode);
        }

        result = ir.VectorSetElement(2 * esize, result, i, element);
    }

    V(128, Vd, result);
    return true;
}

bool TranslatorVisitor::FCVTN(bool Q, bool sz, Vec Vn, Vec Vd) {
    const size_t datasize = 64;
    const size_t esize = sz ? 32 : 16;
    const size_t num_elements = datasize / esize;

    const IR::U128 operand = V(128, Vn);
    const auto rounding_mode = ir.current_location->FPCR().RMode();
    IR::U128 result = ir.ZeroVector();

    for (size_t i = 0; i < num_elements; i++) {
        IR::U16U32U64 element = ir.VectorGetElement(2 * esize, operand, i);

        if (esize == 16) {
            element = ir.FPSingleToHalf(element, rounding_mode);
        } else if (esize == 32) {
            element = ir.FPDoubleToSingle(element, rounding_mode);
        }

        result = ir.VectorSetElement(esize, result, i, element);
    }

    Vpart(datasize, Vd, Q, result);
    return true;
}

bool TranslatorVisitor::FCVTNS_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatConvertToInteger(*this, Q, sz, Vn, Vd, Signedness::Signed, FP::RoundingMode::ToNearest_TieEven);
}

bool TranslatorVisitor::FCVTMS_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatConvertToInteger(*this, Q, sz, Vn, Vd, Signedness::Signed, FP::RoundingMode::TowardsMinusInfinity);
}

bool TranslatorVisitor::FCVTAS_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatConvertToInteger(*this, Q, sz, Vn, Vd, Signedness::Signed, FP::RoundingMode::ToNearest_TieAwayFromZero);
}

bool TranslatorVisitor::FCVTPS_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatConvertToInteger(*this, Q, sz, Vn, Vd, Signedness::Signed, FP::RoundingMode::TowardsPlusInfinity);
}

bool TranslatorVisitor::FCVTXN_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    if (!sz) {
        return UnallocatedEncoding();
    }

    const size_t part = Q ? 1 : 0;
    const auto operand = ir.GetQ(Vn);
    auto result = ir.ZeroVector();

    for (size_t e = 0; e < 2; ++e) {
        const IR::U64 element = ir.VectorGetElement(64, operand, e);
        const IR::U32 converted = ir.FPDoubleToSingle(element, FP::RoundingMode::ToOdd);

        result = ir.VectorSetElement(32, result, e, converted);
    }

    Vpart(64, Vd, part, result);
    return true;
}

bool TranslatorVisitor::FCVTZS_int_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatConvertToInteger(*this, Q, sz, Vn, Vd, Signedness::Signed, FP::RoundingMode::TowardsZero);
}

bool TranslatorVisitor::FCVTNU_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatConvertToInteger(*this, Q, sz, Vn, Vd, Signedness::Unsigned, FP::RoundingMode::ToNearest_TieEven);
}

bool TranslatorVisitor::FCVTMU_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatConvertToInteger(*this, Q, sz, Vn, Vd, Signedness::Unsigned, FP::RoundingMode::TowardsMinusInfinity);
}

bool TranslatorVisitor::FCVTAU_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatConvertToInteger(*this, Q, sz, Vn, Vd, Signedness::Unsigned, FP::RoundingMode::ToNearest_TieAwayFromZero);
}

bool TranslatorVisitor::FCVTPU_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatConvertToInteger(*this, Q, sz, Vn, Vd, Signedness::Unsigned, FP::RoundingMode::TowardsPlusInfinity);
}

bool TranslatorVisitor::FCVTZU_int_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatConvertToInteger(*this, Q, sz, Vn, Vd, Signedness::Unsigned, FP::RoundingMode::TowardsZero);
}

bool TranslatorVisitor::FRINTN_1(bool Q, Vec Vn, Vec Vd) {
    return FloatRoundToIntegralHalfPrecision(*this, Q, Vn, Vd, FP::RoundingMode::ToNearest_TieEven, false);
}

bool TranslatorVisitor::FRINTN_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatRoundToIntegral(*this, Q, sz, Vn, Vd, FP::RoundingMode::ToNearest_TieEven, false);
}

bool TranslatorVisitor::FRINTM_1(bool Q, Vec Vn, Vec Vd) {
    return FloatRoundToIntegralHalfPrecision(*this, Q, Vn, Vd, FP::RoundingMode::TowardsMinusInfinity, false);
}

bool TranslatorVisitor::FRINTM_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatRoundToIntegral(*this, Q, sz, Vn, Vd, FP::RoundingMode::TowardsMinusInfinity, false);
}

bool TranslatorVisitor::FRINTP_1(bool Q, Vec Vn, Vec Vd) {
    return FloatRoundToIntegralHalfPrecision(*this, Q, Vn, Vd, FP::RoundingMode::TowardsPlusInfinity, false);
}

bool TranslatorVisitor::FRINTP_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatRoundToIntegral(*this, Q, sz, Vn, Vd, FP::RoundingMode::TowardsPlusInfinity, false);
}

bool TranslatorVisitor::FRINTZ_1(bool Q, Vec Vn, Vec Vd) {
    return FloatRoundToIntegralHalfPrecision(*this, Q, Vn, Vd, FP::RoundingMode::TowardsZero, false);
}

bool TranslatorVisitor::FRINTZ_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatRoundToIntegral(*this, Q, sz, Vn, Vd, FP::RoundingMode::TowardsZero, false);
}

bool TranslatorVisitor::FRINTA_1(bool Q, Vec Vn, Vec Vd) {
    return FloatRoundToIntegralHalfPrecision(*this, Q, Vn, Vd, FP::RoundingMode::ToNearest_TieAwayFromZero, false);
}

bool TranslatorVisitor::FRINTA_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatRoundToIntegral(*this, Q, sz, Vn, Vd, FP::RoundingMode::ToNearest_TieAwayFromZero, false);
}

bool TranslatorVisitor::FRINTX_1(bool Q, Vec Vn, Vec Vd) {
    return FloatRoundToIntegralHalfPrecision(*this, Q, Vn, Vd, ir.current_location->FPCR().RMode(), true);
}

bool TranslatorVisitor::FRINTX_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatRoundToIntegral(*this, Q, sz, Vn, Vd, ir.current_location->FPCR().RMode(), true);
}

bool TranslatorVisitor::FRINTI_1(bool Q, Vec Vn, Vec Vd) {
    return FloatRoundToIntegralHalfPrecision(*this, Q, Vn, Vd, ir.current_location->FPCR().RMode(), false);
}

bool TranslatorVisitor::FRINTI_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FloatRoundToIntegral(*this, Q, sz, Vn, Vd, ir.current_location->FPCR().RMode(), false);
}

bool TranslatorVisitor::FRECPE_3(bool Q, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 16;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 result = ir.FPVectorRecipEstimate(esize, operand);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FRECPE_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = sz ? 64 : 32;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 result = ir.FPVectorRecipEstimate(esize, operand);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FSQRT_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = sz ? 64 : 32;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 result = ir.FPVectorSqrt(esize, operand);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FRSQRTE_3(bool Q, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 16;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 result = ir.FPVectorRSqrtEstimate(esize, operand);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FRSQRTE_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = sz ? 64 : 32;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 result = ir.FPVectorRSqrtEstimate(esize, operand);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FNEG_1(bool Q, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 mask = ir.VectorBroadcast(64, I(64, 0x8000800080008000));
    const IR::U128 result = ir.VectorEor(operand, mask);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FNEG_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = sz ? 64 : 32;
    const size_t mask_value = esize == 64 ? 0x8000000000000000 : 0x8000000080000000;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 mask = Q ? ir.VectorBroadcast(esize, I(esize, mask_value)) : ir.VectorBroadcastLower(esize, I(esize, mask_value));
    const IR::U128 result = ir.VectorEor(operand, mask);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::NEG_2(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }
    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 zero = ir.ZeroVector();
    const IR::U128 result = ir.VectorSub(esize, zero, operand);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SQXTUN_2(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return SaturatedNarrow(*this, Q, size, Vn, Vd, &IR::IREmitter::VectorSignedSaturatedNarrowToUnsigned);
}

bool TranslatorVisitor::SQXTN_2(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return SaturatedNarrow(*this, Q, size, Vn, Vd, &IR::IREmitter::VectorSignedSaturatedNarrowToSigned);
}

bool TranslatorVisitor::UQXTN_2(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return SaturatedNarrow(*this, Q, size, Vn, Vd, &IR::IREmitter::VectorUnsignedSaturatedNarrow);
}

bool TranslatorVisitor::NOT(bool Q, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand = V(datasize, Vn);
    IR::U128 result = ir.VectorNot(operand);

    if (datasize == 64) {
        result = ir.VectorZeroUpper(result);
    }

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::RBIT_asimd(bool Q, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 data = V(datasize, Vn);
    const IR::U128 result = ir.VectorReverseBits(data);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::REV16_asimd(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    if (size != 0) {
        return UnallocatedEncoding();
    }

    const size_t datasize = Q ? 128 : 64;
    constexpr size_t esize = 16;

    const IR::U128 data = V(datasize, Vn);
    const IR::U128 result = ir.VectorOr(ir.VectorLogicalShiftRight(esize, data, 8),
                                        ir.VectorLogicalShiftLeft(esize, data, 8));

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::REV32_asimd(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    const u32 zext_size = size.ZeroExtend();

    if (zext_size > 1) {
        return UnallocatedEncoding();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 16 << zext_size;
    const u8 shift = static_cast<u8>(8 << zext_size);

    const IR::U128 data = V(datasize, Vn);

    // TODO: Consider factoring byte swapping code out into its own opcode.
    //       Technically the rest of the following code can be a PSHUFB
    //       in the presence of SSSE3.
    IR::U128 result = ir.VectorOr(ir.VectorLogicalShiftRight(esize, data, shift),
                                  ir.VectorLogicalShiftLeft(esize, data, shift));

    // If dealing with 8-bit elements we'll need to shuffle the bytes in each halfword
    // e.g. Assume the following numbers point out bytes in a 32-bit word, we're essentially
    //      changing [3, 2, 1, 0] to [2, 3, 0, 1]
    if (zext_size == 0) {
        result = ir.VectorShuffleLowHalfwords(result, 0b10110001);
        result = ir.VectorShuffleHighHalfwords(result, 0b10110001);
    }

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::REV64_asimd(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    const u32 zext_size = size.ZeroExtend();

    if (zext_size >= 3) {
        return UnallocatedEncoding();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 16 << zext_size;
    const u8 shift = static_cast<u8>(8 << zext_size);

    const IR::U128 data = V(datasize, Vn);

    // TODO: Consider factoring byte swapping code out into its own opcode.
    //       Technically the rest of the following code can be a PSHUFB
    //       in the presence of SSSE3.
    IR::U128 result = ir.VectorOr(ir.VectorLogicalShiftRight(esize, data, shift),
                                  ir.VectorLogicalShiftLeft(esize, data, shift));

    switch (zext_size) {
    case 0:  // 8-bit elements
        result = ir.VectorShuffleLowHalfwords(result, 0b00011011);
        result = ir.VectorShuffleHighHalfwords(result, 0b00011011);
        break;
    case 1:  // 16-bit elements
        result = ir.VectorShuffleLowHalfwords(result, 0b01001110);
        result = ir.VectorShuffleHighHalfwords(result, 0b01001110);
        break;
    }

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SQABS_2(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 result = ir.VectorSignedSaturatedAbs(esize, operand);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SQNEG_2(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 result = ir.VectorSignedSaturatedNeg(esize, operand);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SUQADD_2(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vd);
    const IR::U128 result = ir.VectorSignedSaturatedAccumulateUnsigned(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::USQADD_2(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vd);
    const IR::U128 result = ir.VectorUnsignedSaturatedAccumulateSigned(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SADALP(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return PairedAddLong(*this, Q, size, Vn, Vd, Signedness::Signed, PairedAddLongExtraBehavior::Accumulate);
}

bool TranslatorVisitor::SADDLP(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return PairedAddLong(*this, Q, size, Vn, Vd, Signedness::Signed, PairedAddLongExtraBehavior::None);
}

bool TranslatorVisitor::UADALP(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return PairedAddLong(*this, Q, size, Vn, Vd, Signedness::Unsigned, PairedAddLongExtraBehavior::Accumulate);
}

bool TranslatorVisitor::UADDLP(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return PairedAddLong(*this, Q, size, Vn, Vd, Signedness::Unsigned, PairedAddLongExtraBehavior::None);
}

bool TranslatorVisitor::URECPE(bool Q, bool sz, Vec Vn, Vec Vd) {
    if (sz) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 result = ir.VectorUnsignedRecipEstimate(operand);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::URSQRTE(bool Q, bool sz, Vec Vn, Vec Vd) {
    if (sz) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 result = ir.VectorUnsignedRecipSqrtEstimate(operand);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SCVTF_int_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return IntegerConvertToFloat(*this, Q, sz, Vn, Vd, Signedness::Signed);
}

bool TranslatorVisitor::UCVTF_int_4(bool Q, bool sz, Vec Vn, Vec Vd) {
    return IntegerConvertToFloat(*this, Q, sz, Vn, Vd, Signedness::Unsigned);
}

bool TranslatorVisitor::SHLL(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    if (size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();

    const IR::U128 operand = ir.VectorZeroExtend(esize, Vpart(64, Vn, Q));
    const IR::U128 result = ir.VectorLogicalShiftLeft(esize * 2, operand, static_cast<u8>(esize));

    V(128, Vd, result);
    return true;
}

}  // namespace Dynarmic::A64

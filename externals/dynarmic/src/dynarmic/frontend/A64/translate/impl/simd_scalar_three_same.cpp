/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <optional>

#include <mcl/bit/bit_field.hpp>

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
enum class ComparisonType {
    EQ,
    GE,
    GT,
    HI,
    HS,
    LE,
    LT,
};

enum class ComparisonVariant {
    Register,
    Zero,
};

enum class Signedness {
    Signed,
    Unsigned,
};

bool RoundingShiftLeft(TranslatorVisitor& v, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, Signedness sign) {
    if (size != 0b11) {
        return v.ReservedValue();
    }

    const IR::U128 operand1 = v.V(64, Vn);
    const IR::U128 operand2 = v.V(64, Vm);
    const IR::U128 result = [&] {
        if (sign == Signedness::Signed) {
            return v.ir.VectorRoundingShiftLeftSigned(64, operand1, operand2);
        }

        return v.ir.VectorRoundingShiftLeftUnsigned(64, operand1, operand2);
    }();

    v.V(64, Vd, result);
    return true;
}

bool ScalarCompare(TranslatorVisitor& v, Imm<2> size, std::optional<Vec> Vm, Vec Vn, Vec Vd, ComparisonType type, ComparisonVariant variant) {
    if (size != 0b11) {
        return v.ReservedValue();
    }

    const size_t esize = 64;
    const size_t datasize = 64;

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = variant == ComparisonVariant::Register ? v.V(datasize, *Vm) : v.ir.ZeroVector();

    const IR::U128 result = [&] {
        switch (type) {
        case ComparisonType::EQ:
            return v.ir.VectorEqual(esize, operand1, operand2);
        case ComparisonType::GE:
            return v.ir.VectorGreaterEqualSigned(esize, operand1, operand2);
        case ComparisonType::GT:
            return v.ir.VectorGreaterSigned(esize, operand1, operand2);
        case ComparisonType::HI:
            return v.ir.VectorGreaterUnsigned(esize, operand1, operand2);
        case ComparisonType::HS:
            return v.ir.VectorGreaterEqualUnsigned(esize, operand1, operand2);
        case ComparisonType::LE:
            return v.ir.VectorLessEqualSigned(esize, operand1, operand2);
        case ComparisonType::LT:
        default:
            return v.ir.VectorLessSigned(esize, operand1, operand2);
        }
    }();

    v.V_scalar(datasize, Vd, v.ir.VectorGetElement(esize, result, 0));
    return true;
}

enum class FPComparisonType {
    EQ,
    GE,
    AbsoluteGE,
    GT,
    AbsoluteGT
};

bool ScalarFPCompareRegister(TranslatorVisitor& v, bool sz, Vec Vm, Vec Vn, Vec Vd, FPComparisonType type) {
    const size_t esize = sz ? 64 : 32;
    const size_t datasize = esize;

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = v.V(datasize, Vm);
    const IR::U128 result = [&] {
        switch (type) {
        case FPComparisonType::EQ:
            return v.ir.FPVectorEqual(esize, operand1, operand2);
        case FPComparisonType::GE:
            return v.ir.FPVectorGreaterEqual(esize, operand1, operand2);
        case FPComparisonType::AbsoluteGE:
            return v.ir.FPVectorGreaterEqual(esize,
                                             v.ir.FPVectorAbs(esize, operand1),
                                             v.ir.FPVectorAbs(esize, operand2));
        case FPComparisonType::GT:
            return v.ir.FPVectorGreater(esize, operand1, operand2);
        case FPComparisonType::AbsoluteGT:
            return v.ir.FPVectorGreater(esize,
                                        v.ir.FPVectorAbs(esize, operand1),
                                        v.ir.FPVectorAbs(esize, operand2));
        }

        UNREACHABLE();
    }();

    v.V_scalar(datasize, Vd, v.ir.VectorGetElement(esize, result, 0));
    return true;
}
}  // Anonymous namespace

bool TranslatorVisitor::SQADD_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    const size_t esize = 8 << size.ZeroExtend<size_t>();

    const IR::UAny operand1 = V_scalar(esize, Vn);
    const IR::UAny operand2 = V_scalar(esize, Vm);
    const auto result = ir.SignedSaturatedAdd(operand1, operand2);
    ir.OrQC(result.overflow);
    V_scalar(esize, Vd, result.result);
    return true;
}

bool TranslatorVisitor::SQDMULH_vec_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b00 || size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();

    const IR::UAny operand1 = V_scalar(esize, Vn);
    const IR::UAny operand2 = V_scalar(esize, Vm);
    const auto result = ir.SignedSaturatedDoublingMultiplyReturnHigh(operand1, operand2);

    ir.OrQC(result.overflow);

    V_scalar(esize, Vd, result.result);
    return true;
}

bool TranslatorVisitor::SQRDMULH_vec_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b00 || size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();

    const IR::U128 operand1 = ir.ZeroExtendToQuad(ir.VectorGetElement(esize, V(128, Vn), 0));
    const IR::U128 operand2 = ir.ZeroExtendToQuad(ir.VectorGetElement(esize, V(128, Vm), 0));
    const IR::UpperAndLower multiply = ir.VectorSignedSaturatedDoublingMultiply(esize, operand1, operand2);
    const IR::U128 result = ir.VectorAdd(esize, multiply.upper, ir.VectorLogicalShiftRight(esize, multiply.lower, static_cast<u8>(esize - 1)));

    V_scalar(esize, Vd, ir.VectorGetElement(esize, result, 0));
    return true;
}

bool TranslatorVisitor::SQSUB_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    const size_t esize = 8 << size.ZeroExtend<size_t>();

    const IR::UAny operand1 = V_scalar(esize, Vn);
    const IR::UAny operand2 = V_scalar(esize, Vm);
    const auto result = ir.SignedSaturatedSub(operand1, operand2);
    ir.OrQC(result.overflow);
    V_scalar(esize, Vd, result.result);
    return true;
}

bool TranslatorVisitor::UQADD_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    const size_t esize = 8 << size.ZeroExtend();

    const IR::UAny operand1 = V_scalar(esize, Vn);
    const IR::UAny operand2 = V_scalar(esize, Vm);
    const auto result = ir.UnsignedSaturatedAdd(operand1, operand2);
    ir.OrQC(result.overflow);
    V_scalar(esize, Vd, result.result);
    return true;
}

bool TranslatorVisitor::UQSUB_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    const size_t esize = 8 << size.ZeroExtend();

    const IR::UAny operand1 = V_scalar(esize, Vn);
    const IR::UAny operand2 = V_scalar(esize, Vm);
    const auto result = ir.UnsignedSaturatedSub(operand1, operand2);
    ir.OrQC(result.overflow);
    V_scalar(esize, Vd, result.result);
    return true;
}

bool TranslatorVisitor::ADD_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size != 0b11) {
        return ReservedValue();
    }
    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = esize;

    const IR::U64 operand1 = V_scalar(datasize, Vn);
    const IR::U64 operand2 = V_scalar(datasize, Vm);
    const IR::U64 result = ir.Add(operand1, operand2);
    V_scalar(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::CMEQ_reg_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return ScalarCompare(*this, size, Vm, Vn, Vd, ComparisonType::EQ, ComparisonVariant::Register);
}

bool TranslatorVisitor::CMEQ_zero_1(Imm<2> size, Vec Vn, Vec Vd) {
    return ScalarCompare(*this, size, {}, Vn, Vd, ComparisonType::EQ, ComparisonVariant::Zero);
}

bool TranslatorVisitor::CMGE_reg_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return ScalarCompare(*this, size, Vm, Vn, Vd, ComparisonType::GE, ComparisonVariant::Register);
}

bool TranslatorVisitor::CMGE_zero_1(Imm<2> size, Vec Vn, Vec Vd) {
    return ScalarCompare(*this, size, {}, Vn, Vd, ComparisonType::GE, ComparisonVariant::Zero);
}

bool TranslatorVisitor::CMGT_reg_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return ScalarCompare(*this, size, Vm, Vn, Vd, ComparisonType::GT, ComparisonVariant::Register);
}

bool TranslatorVisitor::CMGT_zero_1(Imm<2> size, Vec Vn, Vec Vd) {
    return ScalarCompare(*this, size, {}, Vn, Vd, ComparisonType::GT, ComparisonVariant::Zero);
}

bool TranslatorVisitor::CMLE_1(Imm<2> size, Vec Vn, Vec Vd) {
    return ScalarCompare(*this, size, {}, Vn, Vd, ComparisonType::LE, ComparisonVariant::Zero);
}

bool TranslatorVisitor::CMLT_1(Imm<2> size, Vec Vn, Vec Vd) {
    return ScalarCompare(*this, size, {}, Vn, Vd, ComparisonType::LT, ComparisonVariant::Zero);
}

bool TranslatorVisitor::CMHI_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return ScalarCompare(*this, size, Vm, Vn, Vd, ComparisonType::HI, ComparisonVariant::Register);
}

bool TranslatorVisitor::CMHS_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return ScalarCompare(*this, size, Vm, Vn, Vd, ComparisonType::HS, ComparisonVariant::Register);
}

bool TranslatorVisitor::CMTST_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size != 0b11) {
        return ReservedValue();
    }

    const IR::U128 operand1 = V(64, Vn);
    const IR::U128 operand2 = V(64, Vm);
    const IR::U128 anded = ir.VectorAnd(operand1, operand2);
    const IR::U128 result = ir.VectorNot(ir.VectorEqual(64, anded, ir.ZeroVector()));

    V(64, Vd, result);
    return true;
}

bool TranslatorVisitor::FABD_2(bool sz, Vec Vm, Vec Vn, Vec Vd) {
    const size_t esize = sz ? 64 : 32;

    const IR::U32U64 operand1 = V_scalar(esize, Vn);
    const IR::U32U64 operand2 = V_scalar(esize, Vm);
    const IR::U32U64 result = ir.FPAbs(ir.FPSub(operand1, operand2));

    V_scalar(esize, Vd, result);
    return true;
}

bool TranslatorVisitor::FMULX_vec_2(bool sz, Vec Vm, Vec Vn, Vec Vd) {
    const size_t esize = sz ? 64 : 32;

    const IR::U32U64 operand1 = V_scalar(esize, Vn);
    const IR::U32U64 operand2 = V_scalar(esize, Vm);
    const IR::U32U64 result = ir.FPMulX(operand1, operand2);

    V_scalar(esize, Vd, result);
    return true;
}

bool TranslatorVisitor::FRECPS_1(Vec Vm, Vec Vn, Vec Vd) {
    const size_t esize = 16;

    const IR::U16 operand1 = V_scalar(esize, Vn);
    const IR::U16 operand2 = V_scalar(esize, Vm);
    const IR::U16 result = ir.FPRecipStepFused(operand1, operand2);

    V_scalar(esize, Vd, result);
    return true;
}

bool TranslatorVisitor::FRECPS_2(bool sz, Vec Vm, Vec Vn, Vec Vd) {
    const size_t esize = sz ? 64 : 32;

    const IR::U32U64 operand1 = V_scalar(esize, Vn);
    const IR::U32U64 operand2 = V_scalar(esize, Vm);
    const IR::U32U64 result = ir.FPRecipStepFused(operand1, operand2);

    V_scalar(esize, Vd, result);
    return true;
}

bool TranslatorVisitor::FRSQRTS_1(Vec Vm, Vec Vn, Vec Vd) {
    const size_t esize = 16;

    const IR::U16 operand1 = V_scalar(esize, Vn);
    const IR::U16 operand2 = V_scalar(esize, Vm);
    const IR::U16 result = ir.FPRSqrtStepFused(operand1, operand2);

    V_scalar(esize, Vd, result);
    return true;
}

bool TranslatorVisitor::FRSQRTS_2(bool sz, Vec Vm, Vec Vn, Vec Vd) {
    const size_t esize = sz ? 64 : 32;

    const IR::U32U64 operand1 = V_scalar(esize, Vn);
    const IR::U32U64 operand2 = V_scalar(esize, Vm);
    const IR::U32U64 result = ir.FPRSqrtStepFused(operand1, operand2);

    V_scalar(esize, Vd, result);
    return true;
}

bool TranslatorVisitor::FACGE_2(bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return ScalarFPCompareRegister(*this, sz, Vm, Vn, Vd, FPComparisonType::AbsoluteGE);
}

bool TranslatorVisitor::FACGT_2(bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return ScalarFPCompareRegister(*this, sz, Vm, Vn, Vd, FPComparisonType::AbsoluteGT);
}

bool TranslatorVisitor::FCMEQ_reg_1(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 lhs = V(128, Vn);
    const IR::U128 rhs = V(128, Vm);
    const IR::U128 result = ir.FPVectorEqual(16, lhs, rhs);

    V_scalar(16, Vd, ir.VectorGetElement(16, result, 0));
    return true;
}

bool TranslatorVisitor::FCMEQ_reg_2(bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return ScalarFPCompareRegister(*this, sz, Vm, Vn, Vd, FPComparisonType::EQ);
}

bool TranslatorVisitor::FCMGE_reg_2(bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return ScalarFPCompareRegister(*this, sz, Vm, Vn, Vd, FPComparisonType::GE);
}

bool TranslatorVisitor::FCMGT_reg_2(bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return ScalarFPCompareRegister(*this, sz, Vm, Vn, Vd, FPComparisonType::GT);
}

bool TranslatorVisitor::SQSHL_reg_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    const size_t esize = 8U << size.ZeroExtend();

    const IR::U128 operand1 = ir.ZeroExtendToQuad(ir.VectorGetElement(esize, V(128, Vn), 0));
    const IR::U128 operand2 = ir.ZeroExtendToQuad(ir.VectorGetElement(esize, V(128, Vm), 0));
    const IR::U128 result = ir.VectorSignedSaturatedShiftLeft(esize, operand1, operand2);

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SRSHL_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return RoundingShiftLeft(*this, size, Vm, Vn, Vd, Signedness::Signed);
}

bool TranslatorVisitor::SSHL_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size != 0b11) {
        return ReservedValue();
    }

    const IR::U128 operand1 = V(64, Vn);
    const IR::U128 operand2 = V(64, Vm);
    const IR::U128 result = ir.VectorArithmeticVShift(64, operand1, operand2);

    V(64, Vd, result);
    return true;
}

bool TranslatorVisitor::SUB_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size != 0b11) {
        return ReservedValue();
    }
    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = esize;

    const IR::U64 operand1 = V_scalar(datasize, Vn);
    const IR::U64 operand2 = V_scalar(datasize, Vm);
    const IR::U64 result = ir.Sub(operand1, operand2);
    V_scalar(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::UQSHL_reg_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    const size_t esize = 8U << size.ZeroExtend();

    const IR::U128 operand1 = ir.ZeroExtendToQuad(ir.VectorGetElement(esize, V(128, Vn), 0));
    const IR::U128 operand2 = ir.ZeroExtendToQuad(ir.VectorGetElement(esize, V(128, Vm), 0));
    const IR::U128 result = ir.VectorUnsignedSaturatedShiftLeft(esize, operand1, operand2);

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::URSHL_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return RoundingShiftLeft(*this, size, Vm, Vn, Vd, Signedness::Unsigned);
}

bool TranslatorVisitor::USHL_1(Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size != 0b11) {
        return ReservedValue();
    }

    const IR::U128 operand1 = V(64, Vn);
    const IR::U128 operand2 = V(64, Vm);
    const IR::U128 result = ir.VectorLogicalVShift(64, operand1, operand2);

    V(64, Vd, result);
    return true;
}

}  // namespace Dynarmic::A64

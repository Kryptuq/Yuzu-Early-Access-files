/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
enum class Operation {
    Add,
    Subtract,
};

enum class ExtraBehavior {
    None,
    Round
};

bool HighNarrowingOperation(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, Operation op, ExtraBehavior behavior) {
    if (size == 0b11) {
        return v.ReservedValue();
    }

    const size_t part = Q;
    const size_t esize = 8 << size.ZeroExtend();
    const size_t doubled_esize = 2 * esize;

    const IR::U128 operand1 = v.ir.GetQ(Vn);
    const IR::U128 operand2 = v.ir.GetQ(Vm);
    IR::U128 wide = [&] {
        if (op == Operation::Add) {
            return v.ir.VectorAdd(doubled_esize, operand1, operand2);
        }
        return v.ir.VectorSub(doubled_esize, operand1, operand2);
    }();

    if (behavior == ExtraBehavior::Round) {
        const u64 round_const = 1ULL << (esize - 1);
        const IR::U128 round_operand = v.ir.VectorBroadcast(doubled_esize, v.I(doubled_esize, round_const));
        wide = v.ir.VectorAdd(doubled_esize, wide, round_operand);
    }

    const IR::U128 result = v.ir.VectorNarrow(doubled_esize,
                                              v.ir.VectorLogicalShiftRight(doubled_esize, wide, static_cast<u8>(esize)));

    v.Vpart(64, Vd, part, result);
    return true;
}

enum class AbsDiffExtraBehavior {
    None,
    Accumulate
};

bool SignedAbsoluteDifference(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, AbsDiffExtraBehavior behavior) {
    if (size == 0b11) {
        return v.ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = v.V(datasize, Vm);
    const IR::U128 result = [&] {
        const IR::U128 tmp = v.ir.VectorSignedAbsoluteDifference(esize, operand1, operand2);

        if (behavior == AbsDiffExtraBehavior::Accumulate) {
            const IR::U128 d = v.V(datasize, Vd);
            return v.ir.VectorAdd(esize, d, tmp);
        }

        return tmp;
    }();

    v.V(datasize, Vd, result);
    return true;
}

enum class Signedness {
    Signed,
    Unsigned
};

bool RoundingHalvingAdd(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, Signedness sign) {
    if (size == 0b11) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = v.V(datasize, Vm);
    const IR::U128 operand2 = v.V(datasize, Vn);
    const IR::U128 result = sign == Signedness::Signed ? v.ir.VectorRoundingHalvingAddSigned(esize, operand1, operand2)
                                                       : v.ir.VectorRoundingHalvingAddUnsigned(esize, operand1, operand2);

    v.V(datasize, Vd, result);
    return true;
}

bool RoundingShiftLeft(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, Signedness sign) {
    if (size == 0b11 && !Q) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = v.V(datasize, Vm);
    const IR::U128 result = [&] {
        if (sign == Signedness::Signed) {
            return v.ir.VectorRoundingShiftLeftSigned(esize, operand1, operand2);
        }

        return v.ir.VectorRoundingShiftLeftUnsigned(esize, operand1, operand2);
    }();

    v.V(datasize, Vd, result);
    return true;
}

enum class ComparisonType {
    EQ,
    GE,
    AbsoluteGE,
    GT,
    AbsoluteGT
};

bool FPCompareRegister(TranslatorVisitor& v, bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd, ComparisonType type) {
    if (sz && !Q) {
        return v.ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = v.V(datasize, Vm);
    const IR::U128 result = [&] {
        switch (type) {
        case ComparisonType::EQ:
            return v.ir.FPVectorEqual(esize, operand1, operand2);
        case ComparisonType::GE:
            return v.ir.FPVectorGreaterEqual(esize, operand1, operand2);
        case ComparisonType::AbsoluteGE:
            return v.ir.FPVectorGreaterEqual(esize,
                                             v.ir.FPVectorAbs(esize, operand1),
                                             v.ir.FPVectorAbs(esize, operand2));
        case ComparisonType::GT:
            return v.ir.FPVectorGreater(esize, operand1, operand2);
        case ComparisonType::AbsoluteGT:
            return v.ir.FPVectorGreater(esize,
                                        v.ir.FPVectorAbs(esize, operand1),
                                        v.ir.FPVectorAbs(esize, operand2));
        }

        UNREACHABLE();
    }();

    v.V(datasize, Vd, result);
    return true;
}

enum class MinMaxOperation {
    Min,
    Max,
};

bool VectorMinMaxOperation(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, MinMaxOperation operation, Signedness sign) {
    if (size == 0b11) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = v.V(datasize, Vm);
    const IR::U128 result = [&] {
        switch (operation) {
        case MinMaxOperation::Max:
            if (sign == Signedness::Signed) {
                return v.ir.VectorMaxSigned(esize, operand1, operand2);
            }
            return v.ir.VectorMaxUnsigned(esize, operand1, operand2);

        case MinMaxOperation::Min:
            if (sign == Signedness::Signed) {
                return v.ir.VectorMinSigned(esize, operand1, operand2);
            }
            return v.ir.VectorMinUnsigned(esize, operand1, operand2);

        default:
            UNREACHABLE();
        }
    }();

    v.V(datasize, Vd, result);
    return true;
}

bool FPMinMaxOperation(TranslatorVisitor& v, bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd, MinMaxOperation operation) {
    if (sz && !Q) {
        return v.ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = v.V(datasize, Vm);
    const IR::U128 result = [&] {
        if (operation == MinMaxOperation::Min) {
            return v.ir.FPVectorMin(esize, operand1, operand2);
        }

        return v.ir.FPVectorMax(esize, operand1, operand2);
    }();

    v.V(datasize, Vd, result);
    return true;
}

bool FPMinMaxNumericOperation(TranslatorVisitor& v, bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd, IR::U32U64 (IREmitter::*fn)(const IR::U32U64&, const IR::U32U64&)) {
    if (sz && !Q) {
        return v.ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;
    const size_t elements = datasize / esize;

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = v.V(datasize, Vm);
    IR::U128 result = v.ir.ZeroVector();

    for (size_t i = 0; i < elements; i++) {
        const IR::UAny elem1 = v.ir.VectorGetElement(esize, operand1, i);
        const IR::UAny elem2 = v.ir.VectorGetElement(esize, operand2, i);
        const IR::UAny result_elem = (v.ir.*fn)(elem1, elem2);

        result = v.ir.VectorSetElement(esize, result, i, result_elem);
    }

    v.V(datasize, Vd, result);
    return true;
}

bool PairedMinMaxOperation(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, MinMaxOperation operation, Signedness sign) {
    if (size == 0b11) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = v.V(datasize, Vm);
    IR::U128 result = [&] {
        switch (operation) {
        case MinMaxOperation::Max:
            if (sign == Signedness::Signed) {
                return v.ir.VectorPairedMaxSigned(esize, operand1, operand2);
            }
            return v.ir.VectorPairedMaxUnsigned(esize, operand1, operand2);

        case MinMaxOperation::Min:
            if (sign == Signedness::Signed) {
                return v.ir.VectorPairedMinSigned(esize, operand1, operand2);
            }
            return v.ir.VectorPairedMinUnsigned(esize, operand1, operand2);

        default:
            UNREACHABLE();
        }
    }();

    if (datasize == 64) {
        result = v.ir.VectorShuffleWords(result, 0b11011000);
    }

    v.V(datasize, Vd, result);
    return true;
}

bool FPPairedMinMax(TranslatorVisitor& v, bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd, IR::U32U64 (IREmitter::*fn)(const IR::U32U64&, const IR::U32U64&)) {
    if (sz && !Q) {
        return v.ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;
    const size_t elements = datasize / esize;
    const size_t boundary = elements / 2;

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = v.V(datasize, Vm);
    IR::U128 result = v.ir.ZeroVector();

    const auto operation = [&](IR::U128 operand, size_t result_start_index) {
        for (size_t i = 0; i < elements; i += 2, result_start_index++) {
            const IR::UAny elem1 = v.ir.VectorGetElement(esize, operand, i);
            const IR::UAny elem2 = v.ir.VectorGetElement(esize, operand, i + 1);
            const IR::UAny result_elem = (v.ir.*fn)(elem1, elem2);

            result = v.ir.VectorSetElement(esize, result, result_start_index, result_elem);
        }
    };

    operation(operand1, 0);
    operation(operand2, boundary);

    v.V(datasize, Vd, result);
    return true;
}

bool SaturatingArithmeticOperation(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, Operation op, Signedness sign) {
    if (size == 0b11 && !Q) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = v.V(datasize, Vm);

    const IR::U128 result = [&] {
        if (sign == Signedness::Signed) {
            if (op == Operation::Add) {
                return v.ir.VectorSignedSaturatedAdd(esize, operand1, operand2);
            }

            return v.ir.VectorSignedSaturatedSub(esize, operand1, operand2);
        }

        if (op == Operation::Add) {
            return v.ir.VectorUnsignedSaturatedAdd(esize, operand1, operand2);
        }

        return v.ir.VectorUnsignedSaturatedSub(esize, operand1, operand2);
    }();

    v.V(datasize, Vd, result);
    return true;
}

bool SaturatingShiftLeft(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, Signedness sign) {
    if (size == 0b11 && !Q) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = v.V(datasize, Vm);
    const IR::U128 result = [&] {
        if (sign == Signedness::Signed) {
            return v.ir.VectorSignedSaturatedShiftLeft(esize, operand1, operand2);
        }

        return v.ir.VectorUnsignedSaturatedShiftLeft(esize, operand1, operand2);
    }();

    v.V(datasize, Vd, result);
    return true;
}

}  // Anonymous namespace

bool TranslatorVisitor::CMGT_reg_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.VectorGreaterSigned(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::CMGE_reg_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    IR::U128 result = ir.VectorGreaterEqualSigned(esize, operand1, operand2);
    if (datasize == 64) {
        result = ir.VectorZeroUpper(result);
    }
    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SABA(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return SignedAbsoluteDifference(*this, Q, size, Vm, Vn, Vd, AbsDiffExtraBehavior::Accumulate);
}

bool TranslatorVisitor::SABD(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return SignedAbsoluteDifference(*this, Q, size, Vm, Vn, Vd, AbsDiffExtraBehavior::None);
}

bool TranslatorVisitor::SMAX(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return VectorMinMaxOperation(*this, Q, size, Vm, Vn, Vd, MinMaxOperation::Max, Signedness::Signed);
}

bool TranslatorVisitor::SMAXP(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return PairedMinMaxOperation(*this, Q, size, Vm, Vn, Vd, MinMaxOperation::Max, Signedness::Signed);
}

bool TranslatorVisitor::SMIN(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return VectorMinMaxOperation(*this, Q, size, Vm, Vn, Vd, MinMaxOperation::Min, Signedness::Signed);
}

bool TranslatorVisitor::SMINP(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return PairedMinMaxOperation(*this, Q, size, Vm, Vn, Vd, MinMaxOperation::Min, Signedness::Signed);
}

bool TranslatorVisitor::SQDMULH_vec_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b00 || size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.VectorSignedSaturatedDoublingMultiply(esize, operand1, operand2).upper;

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SQRDMULH_vec_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b00 || size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::UpperAndLower multiply = ir.VectorSignedSaturatedDoublingMultiply(esize, operand1, operand2);
    const IR::U128 result = ir.VectorAdd(esize, multiply.upper, ir.VectorLogicalShiftRight(esize, multiply.lower, static_cast<u8>(esize - 1)));

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::ADD_vector(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const auto operand1 = V(datasize, Vn);
    const auto operand2 = V(datasize, Vm);
    const auto result = ir.VectorAdd(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::MLA_vec(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 operand3 = V(datasize, Vd);
    const IR::U128 result = ir.VectorAdd(esize, ir.VectorMultiply(esize, operand1, operand2), operand3);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::MUL_vec(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.VectorMultiply(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::ADDHN(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return HighNarrowingOperation(*this, Q, size, Vm, Vn, Vd, Operation::Add, ExtraBehavior::None);
}

bool TranslatorVisitor::RADDHN(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return HighNarrowingOperation(*this, Q, size, Vm, Vn, Vd, Operation::Add, ExtraBehavior::Round);
}

bool TranslatorVisitor::SUBHN(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return HighNarrowingOperation(*this, Q, size, Vm, Vn, Vd, Operation::Subtract, ExtraBehavior::None);
}

bool TranslatorVisitor::RSUBHN(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return HighNarrowingOperation(*this, Q, size, Vm, Vn, Vd, Operation::Subtract, ExtraBehavior::Round);
}

bool TranslatorVisitor::SHADD(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.VectorHalvingAddSigned(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SHSUB(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.VectorHalvingSubSigned(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SQADD_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return SaturatingArithmeticOperation(*this, Q, size, Vm, Vn, Vd, Operation::Add, Signedness::Signed);
}

bool TranslatorVisitor::SQSUB_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return SaturatingArithmeticOperation(*this, Q, size, Vm, Vn, Vd, Operation::Subtract, Signedness::Signed);
}

bool TranslatorVisitor::SRHADD(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return RoundingHalvingAdd(*this, Q, size, Vm, Vn, Vd, Signedness::Signed);
}

bool TranslatorVisitor::UHADD(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.VectorHalvingAddUnsigned(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::UHSUB(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.VectorHalvingSubUnsigned(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::UQADD_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return SaturatingArithmeticOperation(*this, Q, size, Vm, Vn, Vd, Operation::Add, Signedness::Unsigned);
}

bool TranslatorVisitor::UQSUB_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return SaturatingArithmeticOperation(*this, Q, size, Vm, Vn, Vd, Operation::Subtract, Signedness::Unsigned);
}

bool TranslatorVisitor::URHADD(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return RoundingHalvingAdd(*this, Q, size, Vm, Vn, Vd, Signedness::Unsigned);
}

bool TranslatorVisitor::ADDP_vec(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = Q ? ir.VectorPairedAdd(esize, operand1, operand2) : ir.VectorPairedAddLower(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FABD_4(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = sz ? 64 : 32;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.FPVectorAbs(esize, ir.FPVectorSub(esize, operand1, operand2));

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FACGE_4(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return FPCompareRegister(*this, Q, sz, Vm, Vn, Vd, ComparisonType::AbsoluteGE);
}

bool TranslatorVisitor::FACGT_4(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return FPCompareRegister(*this, Q, sz, Vm, Vn, Vd, ComparisonType::AbsoluteGT);
}

bool TranslatorVisitor::FADD_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.FPVectorAdd(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FMLA_vec_1(bool Q, Vec Vm, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 16;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 operand3 = V(datasize, Vd);
    const IR::U128 result = ir.FPVectorMulAdd(esize, operand3, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FMLA_vec_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 operand3 = V(datasize, Vd);
    const IR::U128 result = ir.FPVectorMulAdd(esize, operand3, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FMLS_vec_1(bool Q, Vec Vm, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 16;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 operand3 = V(datasize, Vd);
    const IR::U128 result = ir.FPVectorMulAdd(esize, operand3, ir.FPVectorNeg(esize, operand1), operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FMLS_vec_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 operand3 = V(datasize, Vd);
    const IR::U128 result = ir.FPVectorMulAdd(esize, operand3, ir.FPVectorNeg(esize, operand1), operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FCMEQ_reg_3(bool Q, Vec Vm, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 lhs = V(datasize, Vn);
    const IR::U128 rhs = V(datasize, Vm);
    const IR::U128 result = ir.FPVectorEqual(16, lhs, rhs);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FCMEQ_reg_4(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return FPCompareRegister(*this, Q, sz, Vm, Vn, Vd, ComparisonType::EQ);
}

bool TranslatorVisitor::FCMGE_reg_4(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return FPCompareRegister(*this, Q, sz, Vm, Vn, Vd, ComparisonType::GE);
}

bool TranslatorVisitor::FCMGT_reg_4(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return FPCompareRegister(*this, Q, sz, Vm, Vn, Vd, ComparisonType::GT);
}

bool TranslatorVisitor::AND_asimd(bool Q, Vec Vm, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;

    const auto operand1 = V(datasize, Vn);
    const auto operand2 = V(datasize, Vm);
    const auto result = ir.VectorAnd(operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::BIC_asimd_reg(bool Q, Vec Vm, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);

    IR::U128 result = ir.VectorAnd(operand1, ir.VectorNot(operand2));
    if (datasize == 64) {
        result = ir.VectorZeroUpper(result);
    }

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::CMHI_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.VectorGreaterUnsigned(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::CMHS_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    IR::U128 result = ir.VectorGreaterEqualUnsigned(esize, operand1, operand2);
    if (datasize == 64) {
        result = ir.VectorZeroUpper(result);
    }

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::CMTST_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 anded = ir.VectorAnd(operand1, operand2);
    const IR::U128 result = ir.VectorNot(ir.VectorEqual(esize, anded, ir.ZeroVector()));

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SQSHL_reg_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return SaturatingShiftLeft(*this, Q, size, Vm, Vn, Vd, Signedness::Signed);
}

bool TranslatorVisitor::SRSHL_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return RoundingShiftLeft(*this, Q, size, Vm, Vn, Vd, Signedness::Signed);
}

bool TranslatorVisitor::SSHL_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.VectorArithmeticVShift(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::UQSHL_reg_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return SaturatingShiftLeft(*this, Q, size, Vm, Vn, Vd, Signedness::Unsigned);
}

bool TranslatorVisitor::URSHL_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return RoundingShiftLeft(*this, Q, size, Vm, Vn, Vd, Signedness::Unsigned);
}

bool TranslatorVisitor::USHL_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.VectorLogicalVShift(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::UMAX(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return VectorMinMaxOperation(*this, Q, size, Vm, Vn, Vd, MinMaxOperation::Max, Signedness::Unsigned);
}

bool TranslatorVisitor::UMAXP(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return PairedMinMaxOperation(*this, Q, size, Vm, Vn, Vd, MinMaxOperation::Max, Signedness::Unsigned);
}

bool TranslatorVisitor::UABA(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 initial_dest = V(datasize, Vd);

    const IR::U128 result = ir.VectorAdd(esize, initial_dest,
                                         ir.VectorUnsignedAbsoluteDifference(esize, operand1, operand2));

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::UABD(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.VectorUnsignedAbsoluteDifference(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::UMIN(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return VectorMinMaxOperation(*this, Q, size, Vm, Vn, Vd, MinMaxOperation::Min, Signedness::Unsigned);
}

bool TranslatorVisitor::UMINP(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return PairedMinMaxOperation(*this, Q, size, Vm, Vn, Vd, MinMaxOperation::Min, Signedness::Unsigned);
}

bool TranslatorVisitor::FSUB_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.FPVectorSub(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FRECPS_3(bool Q, Vec Vm, Vec Vn, Vec Vd) {
    const size_t esize = 16;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.FPVectorRecipStepFused(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FRECPS_4(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.FPVectorRecipStepFused(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FRSQRTS_3(bool Q, Vec Vm, Vec Vn, Vec Vd) {
    const size_t esize = 16;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.FPVectorRSqrtStepFused(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FRSQRTS_4(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.FPVectorRSqrtStepFused(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::ORR_asimd_reg(bool Q, Vec Vm, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;

    const auto operand1 = V(datasize, Vn);
    const auto operand2 = V(datasize, Vm);
    const auto result = ir.VectorOr(operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::ORN_asimd(bool Q, Vec Vm, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;

    const auto operand1 = V(datasize, Vn);
    const auto operand2 = V(datasize, Vm);

    auto result = ir.VectorOr(operand1, ir.VectorNot(operand2));
    if (datasize == 64) {
        result = ir.VectorZeroUpper(result);
    }

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::PMUL(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size != 0b00) {
        return ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.VectorPolynomialMultiply(operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SUB_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const auto operand1 = V(datasize, Vn);
    const auto operand2 = V(datasize, Vm);
    const auto result = ir.VectorSub(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::CMEQ_reg_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);

    IR::U128 result = ir.VectorEqual(esize, operand1, operand2);
    if (datasize == 64) {
        result = ir.VectorZeroUpper(result);
    }

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::MLS_vec(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 operand3 = V(datasize, Vd);

    const IR::U128 result = ir.VectorSub(esize, operand3, ir.VectorMultiply(esize, operand1, operand2));

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::EOR_asimd(bool Q, Vec Vm, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;

    const auto operand1 = V(datasize, Vn);
    const auto operand2 = V(datasize, Vm);
    const auto result = ir.VectorEor(operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FMAX_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return FPMinMaxOperation(*this, Q, sz, Vm, Vn, Vd, MinMaxOperation::Max);
}

bool TranslatorVisitor::FMAXNM_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return FPMinMaxNumericOperation(*this, Q, sz, Vm, Vn, Vd, &IREmitter::FPMaxNumeric);
}

bool TranslatorVisitor::FMAXNMP_vec_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return FPPairedMinMax(*this, Q, sz, Vm, Vn, Vd, &IREmitter::FPMaxNumeric);
}

bool TranslatorVisitor::FMAXP_vec_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return FPPairedMinMax(*this, Q, sz, Vm, Vn, Vd, &IREmitter::FPMax);
}

bool TranslatorVisitor::FMIN_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return FPMinMaxOperation(*this, Q, sz, Vm, Vn, Vd, MinMaxOperation::Min);
}

bool TranslatorVisitor::FMINNM_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return FPMinMaxNumericOperation(*this, Q, sz, Vm, Vn, Vd, &IREmitter::FPMinNumeric);
}

bool TranslatorVisitor::FMINNMP_vec_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return FPPairedMinMax(*this, Q, sz, Vm, Vn, Vd, &IREmitter::FPMinNumeric);
}

bool TranslatorVisitor::FMINP_vec_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    return FPPairedMinMax(*this, Q, sz, Vm, Vn, Vd, &IREmitter::FPMin);
}

bool TranslatorVisitor::FADDP_vec_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = Q ? ir.FPVectorPairedAdd(esize, operand1, operand2) : ir.FPVectorPairedAddLower(esize, operand1, operand2);
    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FMUL_vec_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.FPVectorMul(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FMULX_vec_4(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.FPVectorMulX(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FDIV_2(bool Q, bool sz, Vec Vm, Vec Vn, Vec Vd) {
    if (sz && !Q) {
        return ReservedValue();
    }

    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    IR::U128 result = ir.FPVectorDiv(esize, operand1, operand2);
    if (datasize == 64) {
        result = ir.VectorZeroUpper(result);
    }

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::BIF(bool Q, Vec Vm, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;

    const auto operand1 = V(datasize, Vd);
    const auto operand4 = V(datasize, Vn);
    const auto operand3 = ir.VectorNot(V(datasize, Vm));
    const auto result = ir.VectorEor(operand1, ir.VectorAnd(ir.VectorEor(operand1, operand4), operand3));

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::BIT(bool Q, Vec Vm, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;

    const auto operand1 = V(datasize, Vd);
    const auto operand4 = V(datasize, Vn);
    const auto operand3 = V(datasize, Vm);
    const auto result = ir.VectorEor(operand1, ir.VectorAnd(ir.VectorEor(operand1, operand4), operand3));

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::BSL(bool Q, Vec Vm, Vec Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;

    const auto operand4 = V(datasize, Vn);
    const auto operand1 = V(datasize, Vm);
    const auto operand3 = V(datasize, Vd);
    const auto result = ir.VectorEor(operand1, ir.VectorAnd(ir.VectorEor(operand1, operand4), operand3));

    V(datasize, Vd, result);
    return true;
}

}  // namespace Dynarmic::A64

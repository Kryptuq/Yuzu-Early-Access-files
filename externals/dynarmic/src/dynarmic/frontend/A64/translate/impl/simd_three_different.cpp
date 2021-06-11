/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
enum class AbsoluteDifferenceBehavior {
    None,
    Accumulate
};

enum class Signedness {
    Signed,
    Unsigned
};

bool AbsoluteDifferenceLong(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, AbsoluteDifferenceBehavior behavior, Signedness sign) {
    if (size == 0b11) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = 64;

    const IR::U128 operand1 = v.ir.VectorZeroExtend(esize, v.Vpart(datasize, Vn, Q));
    const IR::U128 operand2 = v.ir.VectorZeroExtend(esize, v.Vpart(datasize, Vm, Q));
    IR::U128 result = sign == Signedness::Signed ? v.ir.VectorSignedAbsoluteDifference(esize, operand1, operand2)
                                                 : v.ir.VectorUnsignedAbsoluteDifference(esize, operand1, operand2);

    if (behavior == AbsoluteDifferenceBehavior::Accumulate) {
        const IR::U128 data = v.V(2 * datasize, Vd);
        result = v.ir.VectorAdd(2 * esize, result, data);
    }

    v.V(2 * datasize, Vd, result);
    return true;
}

enum class MultiplyLongBehavior {
    None,
    Accumulate,
    Subtract
};

bool MultiplyLong(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, MultiplyLongBehavior behavior, Signedness sign) {
    if (size == 0b11) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t doubled_esize = 2 * esize;
    const size_t datasize = 64;
    const size_t doubled_datasize = datasize * 2;

    const auto get_operands = [&] {
        const auto p1 = v.Vpart(datasize, Vn, Q);
        const auto p2 = v.Vpart(datasize, Vm, Q);

        if (sign == Signedness::Signed) {
            return std::make_pair(v.ir.VectorSignExtend(esize, p1),
                                  v.ir.VectorSignExtend(esize, p2));
        }

        return std::make_pair(v.ir.VectorZeroExtend(esize, p1),
                              v.ir.VectorZeroExtend(esize, p2));
    };

    const auto [operand1, operand2] = get_operands();
    IR::U128 result = v.ir.VectorMultiply(doubled_esize, operand1, operand2);

    if (behavior == MultiplyLongBehavior::Accumulate) {
        const IR::U128 addend = v.V(doubled_datasize, Vd);
        result = v.ir.VectorAdd(doubled_esize, addend, result);
    } else if (behavior == MultiplyLongBehavior::Subtract) {
        const IR::U128 minuend = v.V(doubled_datasize, Vd);
        result = v.ir.VectorSub(doubled_esize, minuend, result);
    }

    v.V(doubled_datasize, Vd, result);
    return true;
}

enum class LongOperationBehavior {
    Addition,
    Subtraction
};

bool LongOperation(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, LongOperationBehavior behavior, Signedness sign) {
    if (size == 0b11) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t part = Q ? 1 : 0;

    const auto get_operand = [&](Vec vec) {
        const IR::U128 tmp = v.Vpart(64, vec, part);

        if (sign == Signedness::Signed) {
            return v.ir.VectorSignExtend(esize, tmp);
        }

        return v.ir.VectorZeroExtend(esize, tmp);
    };

    const IR::U128 operand1 = get_operand(Vn);
    const IR::U128 operand2 = get_operand(Vm);
    const IR::U128 result = [&] {
        if (behavior == LongOperationBehavior::Addition) {
            return v.ir.VectorAdd(esize * 2, operand1, operand2);
        }

        return v.ir.VectorSub(esize * 2, operand1, operand2);
    }();

    v.V(128, Vd, result);
    return true;
}

enum class WideOperationBehavior {
    Addition,
    Subtraction
};

bool WideOperation(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, WideOperationBehavior behavior, Signedness sign) {
    if (size == 0b11) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t part = Q ? 1 : 0;

    const IR::U128 operand1 = v.V(128, Vn);
    const IR::U128 operand2 = [&] {
        const IR::U128 tmp = v.Vpart(64, Vm, part);

        if (sign == Signedness::Signed) {
            return v.ir.VectorSignExtend(esize, tmp);
        }

        return v.ir.VectorZeroExtend(esize, tmp);
    }();
    const IR::U128 result = [&] {
        if (behavior == WideOperationBehavior::Addition) {
            return v.ir.VectorAdd(esize * 2, operand1, operand2);
        }

        return v.ir.VectorSub(esize * 2, operand1, operand2);
    }();

    v.V(128, Vd, result);
    return true;
}
}  // Anonymous namespace

bool TranslatorVisitor::PMULL(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b01 || size == 0b10) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = 64;

    const IR::U128 operand1 = Vpart(datasize, Vn, Q);
    const IR::U128 operand2 = Vpart(datasize, Vm, Q);
    const IR::U128 result = ir.VectorPolynomialMultiplyLong(esize, operand1, operand2);

    V(128, Vd, result);
    return true;
}

bool TranslatorVisitor::SABAL(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return AbsoluteDifferenceLong(*this, Q, size, Vm, Vn, Vd, AbsoluteDifferenceBehavior::Accumulate, Signedness::Signed);
}

bool TranslatorVisitor::SABDL(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return AbsoluteDifferenceLong(*this, Q, size, Vm, Vn, Vd, AbsoluteDifferenceBehavior::None, Signedness::Signed);
}

bool TranslatorVisitor::SADDL(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return LongOperation(*this, Q, size, Vm, Vn, Vd, LongOperationBehavior::Addition, Signedness::Signed);
}

bool TranslatorVisitor::SADDW(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return WideOperation(*this, Q, size, Vm, Vn, Vd, WideOperationBehavior::Addition, Signedness::Signed);
}

bool TranslatorVisitor::SMLAL_vec(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return MultiplyLong(*this, Q, size, Vm, Vn, Vd, MultiplyLongBehavior::Accumulate, Signedness::Signed);
}

bool TranslatorVisitor::SMLSL_vec(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return MultiplyLong(*this, Q, size, Vm, Vn, Vd, MultiplyLongBehavior::Subtract, Signedness::Signed);
}

bool TranslatorVisitor::SMULL_vec(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return MultiplyLong(*this, Q, size, Vm, Vn, Vd, MultiplyLongBehavior::None, Signedness::Signed);
}

bool TranslatorVisitor::SSUBW(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return WideOperation(*this, Q, size, Vm, Vn, Vd, WideOperationBehavior::Subtraction, Signedness::Signed);
}

bool TranslatorVisitor::SSUBL(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return LongOperation(*this, Q, size, Vm, Vn, Vd, LongOperationBehavior::Subtraction, Signedness::Signed);
}

bool TranslatorVisitor::UADDL(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return LongOperation(*this, Q, size, Vm, Vn, Vd, LongOperationBehavior::Addition, Signedness::Unsigned);
}

bool TranslatorVisitor::UABAL(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return AbsoluteDifferenceLong(*this, Q, size, Vm, Vn, Vd, AbsoluteDifferenceBehavior::Accumulate, Signedness::Unsigned);
}

bool TranslatorVisitor::UABDL(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return AbsoluteDifferenceLong(*this, Q, size, Vm, Vn, Vd, AbsoluteDifferenceBehavior::None, Signedness::Unsigned);
}

bool TranslatorVisitor::UADDW(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return WideOperation(*this, Q, size, Vm, Vn, Vd, WideOperationBehavior::Addition, Signedness::Unsigned);
}

bool TranslatorVisitor::UMLAL_vec(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return MultiplyLong(*this, Q, size, Vm, Vn, Vd, MultiplyLongBehavior::Accumulate, Signedness::Unsigned);
}

bool TranslatorVisitor::UMLSL_vec(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return MultiplyLong(*this, Q, size, Vm, Vn, Vd, MultiplyLongBehavior::Subtract, Signedness::Unsigned);
}

bool TranslatorVisitor::UMULL_vec(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return MultiplyLong(*this, Q, size, Vm, Vn, Vd, MultiplyLongBehavior::None, Signedness::Unsigned);
}

bool TranslatorVisitor::USUBW(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return WideOperation(*this, Q, size, Vm, Vn, Vd, WideOperationBehavior::Subtraction, Signedness::Unsigned);
}

bool TranslatorVisitor::USUBL(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return LongOperation(*this, Q, size, Vm, Vn, Vd, LongOperationBehavior::Subtraction, Signedness::Unsigned);
}

bool TranslatorVisitor::SQDMULL_vec_2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b00 || size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t part = Q ? 1 : 0;

    const IR::U128 operand1 = Vpart(64, Vn, part);
    const IR::U128 operand2 = Vpart(64, Vm, part);
    const IR::U128 result = ir.VectorSignedSaturatedDoublingMultiplyLong(esize, operand1, operand2);

    V(128, Vd, result);
    return true;
}

}  // namespace Dynarmic::A64

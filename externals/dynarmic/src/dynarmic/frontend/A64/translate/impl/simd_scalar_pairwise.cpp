/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
enum class MinMaxOperation {
    Max,
    MaxNumeric,
    Min,
    MinNumeric,
};

bool FPPairwiseMinMax(TranslatorVisitor& v, bool sz, Vec Vn, Vec Vd, MinMaxOperation operation) {
    const size_t esize = sz ? 64 : 32;

    const IR::U128 operand = v.V(128, Vn);
    const IR::U32U64 element1 = v.ir.VectorGetElement(esize, operand, 0);
    const IR::U32U64 element2 = v.ir.VectorGetElement(esize, operand, 1);
    const IR::U32U64 result = [&] {
        switch (operation) {
        case MinMaxOperation::Max:
            return v.ir.FPMax(element1, element2);
        case MinMaxOperation::MaxNumeric:
            return v.ir.FPMaxNumeric(element1, element2);
        case MinMaxOperation::Min:
            return v.ir.FPMin(element1, element2);
        case MinMaxOperation::MinNumeric:
            return v.ir.FPMinNumeric(element1, element2);
        default:
            UNREACHABLE();
        }
    }();

    v.V(128, Vd, v.ir.ZeroExtendToQuad(result));
    return true;
}
}  // Anonymous namespace

bool TranslatorVisitor::ADDP_pair(Imm<2> size, Vec Vn, Vec Vd) {
    if (size != 0b11) {
        return ReservedValue();
    }

    const IR::U64 operand1 = ir.VectorGetElement(64, V(128, Vn), 0);
    const IR::U64 operand2 = ir.VectorGetElement(64, V(128, Vn), 1);
    const IR::U128 result = ir.ZeroExtendToQuad(ir.Add(operand1, operand2));
    V(128, Vd, result);
    return true;
}

bool TranslatorVisitor::FADDP_pair_2(bool size, Vec Vn, Vec Vd) {
    const size_t esize = size ? 64 : 32;

    const IR::U32U64 operand1 = ir.VectorGetElement(esize, V(128, Vn), 0);
    const IR::U32U64 operand2 = ir.VectorGetElement(esize, V(128, Vn), 1);
    const IR::U128 result = ir.ZeroExtendToQuad(ir.FPAdd(operand1, operand2));
    V(128, Vd, result);
    return true;
}

bool TranslatorVisitor::FMAXNMP_pair_2(bool sz, Vec Vn, Vec Vd) {
    return FPPairwiseMinMax(*this, sz, Vn, Vd, MinMaxOperation::MaxNumeric);
}

bool TranslatorVisitor::FMAXP_pair_2(bool sz, Vec Vn, Vec Vd) {
    return FPPairwiseMinMax(*this, sz, Vn, Vd, MinMaxOperation::Max);
}

bool TranslatorVisitor::FMINNMP_pair_2(bool sz, Vec Vn, Vec Vd) {
    return FPPairwiseMinMax(*this, sz, Vn, Vd, MinMaxOperation::MinNumeric);
}

bool TranslatorVisitor::FMINP_pair_2(bool sz, Vec Vn, Vec Vd) {
    return FPPairwiseMinMax(*this, sz, Vn, Vd, MinMaxOperation::Min);
}
}  // namespace Dynarmic::A64

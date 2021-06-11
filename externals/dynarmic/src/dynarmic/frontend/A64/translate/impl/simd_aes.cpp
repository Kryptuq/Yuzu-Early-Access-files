/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::AESD(Vec Vn, Vec Vd) {
    const IR::U128 operand1 = ir.GetQ(Vd);
    const IR::U128 operand2 = ir.GetQ(Vn);

    const IR::U128 result = ir.AESDecryptSingleRound(ir.VectorEor(operand1, operand2));

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::AESE(Vec Vn, Vec Vd) {
    const IR::U128 operand1 = ir.GetQ(Vd);
    const IR::U128 operand2 = ir.GetQ(Vn);

    const IR::U128 result = ir.AESEncryptSingleRound(ir.VectorEor(operand1, operand2));

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::AESIMC(Vec Vn, Vec Vd) {
    const IR::U128 operand = ir.GetQ(Vn);
    const IR::U128 result = ir.AESInverseMixColumns(operand);

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::AESMC(Vec Vn, Vec Vd) {
    const IR::U128 operand = ir.GetQ(Vn);
    const IR::U128 result = ir.AESMixColumns(operand);

    ir.SetQ(Vd, result);
    return true;
}

}  // namespace Dynarmic::A64

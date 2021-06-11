/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {
namespace {
using DivideFunction = IR::U32U64 (IREmitter::*)(const IR::U32U64&, const IR::U32U64&);

bool DivideOperation(TranslatorVisitor& v, Cond cond, Reg d, Reg m, Reg n, DivideFunction fn) {
    if (d == Reg::PC || m == Reg::PC || n == Reg::PC) {
        return v.UnpredictableInstruction();
    }

    if (!v.ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 operand1 = v.ir.GetRegister(n);
    const IR::U32 operand2 = v.ir.GetRegister(m);
    const IR::U32 result = (v.ir.*fn)(operand1, operand2);

    v.ir.SetRegister(d, result);
    return true;
}
}  // Anonymous namespace

// SDIV<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SDIV(Cond cond, Reg d, Reg m, Reg n) {
    return DivideOperation(*this, cond, d, m, n, &IREmitter::SignedDiv);
}

// UDIV<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UDIV(Cond cond, Reg d, Reg m, Reg n) {
    return DivideOperation(*this, cond, d, m, n, &IREmitter::UnsignedDiv);
}

}  // namespace Dynarmic::A32

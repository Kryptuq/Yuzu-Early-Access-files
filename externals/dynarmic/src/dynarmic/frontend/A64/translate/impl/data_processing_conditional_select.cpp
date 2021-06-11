/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::CSEL(bool sf, Reg Rm, Cond cond, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    const IR::U32U64 operand1 = X(datasize, Rn);
    const IR::U32U64 operand2 = X(datasize, Rm);

    const IR::U32U64 result = ir.ConditionalSelect(cond, operand1, operand2);

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::CSINC(bool sf, Reg Rm, Cond cond, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    const IR::U32U64 operand1 = X(datasize, Rn);
    const IR::U32U64 operand2 = X(datasize, Rm);

    const IR::U32U64 result = ir.ConditionalSelect(cond, operand1, ir.Add(operand2, I(datasize, 1)));

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::CSINV(bool sf, Reg Rm, Cond cond, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    const IR::U32U64 operand1 = X(datasize, Rn);
    const IR::U32U64 operand2 = X(datasize, Rm);

    const IR::U32U64 result = ir.ConditionalSelect(cond, operand1, ir.Not(operand2));

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::CSNEG(bool sf, Reg Rm, Cond cond, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    const IR::U32U64 operand1 = X(datasize, Rn);
    const IR::U32U64 operand2 = X(datasize, Rm);

    const IR::U32U64 result = ir.ConditionalSelect(cond, operand1, ir.Add(ir.Not(operand2), I(datasize, 1)));

    X(datasize, Rd, result);
    return true;
}

}  // namespace Dynarmic::A64

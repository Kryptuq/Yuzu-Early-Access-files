/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::LSLV(bool sf, Reg Rm, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    const IR::U32U64 operand = X(datasize, Rn);
    const IR::U32U64 shift_amount = X(datasize, Rm);

    const IR::U32U64 result = ir.LogicalShiftLeftMasked(operand, shift_amount);

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::LSRV(bool sf, Reg Rm, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    const IR::U32U64 operand = X(datasize, Rn);
    const IR::U32U64 shift_amount = X(datasize, Rm);

    const IR::U32U64 result = ir.LogicalShiftRightMasked(operand, shift_amount);

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::ASRV(bool sf, Reg Rm, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    const IR::U32U64 operand = X(datasize, Rn);
    const IR::U32U64 shift_amount = X(datasize, Rm);

    const IR::U32U64 result = ir.ArithmeticShiftRightMasked(operand, shift_amount);

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::RORV(bool sf, Reg Rm, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    const IR::U32U64 operand = X(datasize, Rn);
    const IR::U32U64 shift_amount = X(datasize, Rm);

    const IR::U32U64 result = ir.RotateRightMasked(operand, shift_amount);

    X(datasize, Rd, result);
    return true;
}

}  // namespace Dynarmic::A64

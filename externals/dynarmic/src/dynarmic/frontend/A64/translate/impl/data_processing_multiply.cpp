/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::MADD(bool sf, Reg Rm, Reg Ra, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    const IR::U32U64 a = X(datasize, Ra);
    const IR::U32U64 m = X(datasize, Rm);
    const IR::U32U64 n = X(datasize, Rn);

    const IR::U32U64 result = ir.Add(a, ir.Mul(n, m));

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::MSUB(bool sf, Reg Rm, Reg Ra, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    const IR::U32U64 a = X(datasize, Ra);
    const IR::U32U64 m = X(datasize, Rm);
    const IR::U32U64 n = X(datasize, Rn);

    const IR::U32U64 result = ir.Sub(a, ir.Mul(n, m));

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::SMADDL(Reg Rm, Reg Ra, Reg Rn, Reg Rd) {
    const IR::U64 a = X(64, Ra);
    const IR::U64 m = ir.SignExtendToLong(X(32, Rm));
    const IR::U64 n = ir.SignExtendToLong(X(32, Rn));

    const IR::U64 result = ir.Add(a, ir.Mul(n, m));

    X(64, Rd, result);
    return true;
}

bool TranslatorVisitor::SMSUBL(Reg Rm, Reg Ra, Reg Rn, Reg Rd) {
    const IR::U64 a = X(64, Ra);
    const IR::U64 m = ir.SignExtendToLong(X(32, Rm));
    const IR::U64 n = ir.SignExtendToLong(X(32, Rn));

    const IR::U64 result = ir.Sub(a, ir.Mul(n, m));

    X(64, Rd, result);
    return true;
}

bool TranslatorVisitor::SMULH(Reg Rm, Reg Rn, Reg Rd) {
    const IR::U64 m = X(64, Rm);
    const IR::U64 n = X(64, Rn);

    const IR::U64 result = ir.SignedMultiplyHigh(n, m);

    X(64, Rd, result);
    return true;
}

bool TranslatorVisitor::UMADDL(Reg Rm, Reg Ra, Reg Rn, Reg Rd) {
    const IR::U64 a = X(64, Ra);
    const IR::U64 m = ir.ZeroExtendToLong(X(32, Rm));
    const IR::U64 n = ir.ZeroExtendToLong(X(32, Rn));

    const IR::U64 result = ir.Add(a, ir.Mul(n, m));

    X(64, Rd, result);
    return true;
}

bool TranslatorVisitor::UMSUBL(Reg Rm, Reg Ra, Reg Rn, Reg Rd) {
    const IR::U64 a = X(64, Ra);
    const IR::U64 m = ir.ZeroExtendToLong(X(32, Rm));
    const IR::U64 n = ir.ZeroExtendToLong(X(32, Rn));

    const IR::U64 result = ir.Sub(a, ir.Mul(n, m));

    X(64, Rd, result);
    return true;
}

bool TranslatorVisitor::UMULH(Reg Rm, Reg Rn, Reg Rd) {
    const IR::U64 m = X(64, Rm);
    const IR::U64 n = X(64, Rn);

    const IR::U64 result = ir.UnsignedMultiplyHigh(n, m);

    X(64, Rd, result);
    return true;
}

}  // namespace Dynarmic::A64

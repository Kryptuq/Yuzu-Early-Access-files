/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
bool FPCompare(TranslatorVisitor& v, Imm<2> type, Vec Vm, Vec Vn, bool exc_on_qnan, bool cmp_with_zero) {
    const auto datasize = FPGetDataSize(type);
    if (!datasize || *datasize == 16) {
        return v.UnallocatedEncoding();
    }

    const IR::U32U64 operand1 = v.V_scalar(*datasize, Vn);
    IR::U32U64 operand2;
    if (cmp_with_zero) {
        operand2 = v.I(*datasize, 0);
    } else {
        operand2 = v.V_scalar(*datasize, Vm);
    }

    const auto nzcv = v.ir.FPCompare(operand1, operand2, exc_on_qnan);
    v.ir.SetNZCV(nzcv);
    return true;
}
}  // Anonymous namespace

bool TranslatorVisitor::FCMP_float(Imm<2> type, Vec Vm, Vec Vn, bool cmp_with_zero) {
    return FPCompare(*this, type, Vm, Vn, false, cmp_with_zero);
}

bool TranslatorVisitor::FCMPE_float(Imm<2> type, Vec Vm, Vec Vn, bool cmp_with_zero) {
    return FPCompare(*this, type, Vm, Vn, true, cmp_with_zero);
}

}  // namespace Dynarmic::A64

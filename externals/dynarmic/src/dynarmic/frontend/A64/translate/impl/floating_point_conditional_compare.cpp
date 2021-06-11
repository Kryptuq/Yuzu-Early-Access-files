/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
bool FPCompare(TranslatorVisitor& v, Imm<2> type, Vec Vm, Cond cond, Vec Vn, Imm<4> nzcv, bool exc_on_qnan) {
    const auto datasize = FPGetDataSize(type);
    if (!datasize || *datasize == 16) {
        return v.UnallocatedEncoding();
    }
    const u32 flags = nzcv.ZeroExtend<u32>() << 28;

    const IR::U32U64 operand1 = v.V_scalar(*datasize, Vn);
    const IR::U32U64 operand2 = v.V_scalar(*datasize, Vm);

    const IR::NZCV then_flags = v.ir.FPCompare(operand1, operand2, exc_on_qnan);
    const IR::NZCV else_flags = v.ir.NZCVFromPackedFlags(v.ir.Imm32(flags));
    v.ir.SetNZCV(v.ir.ConditionalSelect(cond, then_flags, else_flags));
    return true;
}
}  // Anonymous namespace

bool TranslatorVisitor::FCCMP_float(Imm<2> type, Vec Vm, Cond cond, Vec Vn, Imm<4> nzcv) {
    return FPCompare(*this, type, Vm, cond, Vn, nzcv, false);
}

bool TranslatorVisitor::FCCMPE_float(Imm<2> type, Vec Vm, Cond cond, Vec Vn, Imm<4> nzcv) {
    return FPCompare(*this, type, Vm, cond, Vn, nzcv, true);
}

}  // namespace Dynarmic::A64

/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::FMADD_float(Imm<2> type, Vec Vm, Vec Va, Vec Vn, Vec Vd) {
    const auto datasize = FPGetDataSize(type);
    if (!datasize) {
        return UnallocatedEncoding();
    }

    const IR::U16U32U64 operanda = V_scalar(*datasize, Va);
    const IR::U16U32U64 operand1 = V_scalar(*datasize, Vn);
    const IR::U16U32U64 operand2 = V_scalar(*datasize, Vm);
    const IR::U16U32U64 result = ir.FPMulAdd(operanda, operand1, operand2);
    V_scalar(*datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FMSUB_float(Imm<2> type, Vec Vm, Vec Va, Vec Vn, Vec Vd) {
    const auto datasize = FPGetDataSize(type);
    if (!datasize) {
        return UnallocatedEncoding();
    }

    const IR::U16U32U64 operanda = V_scalar(*datasize, Va);
    const IR::U16U32U64 operand1 = V_scalar(*datasize, Vn);
    const IR::U16U32U64 operand2 = V_scalar(*datasize, Vm);
    const IR::U16U32U64 result = ir.FPMulAdd(operanda, ir.FPNeg(operand1), operand2);
    V_scalar(*datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FNMADD_float(Imm<2> type, Vec Vm, Vec Va, Vec Vn, Vec Vd) {
    const auto datasize = FPGetDataSize(type);
    if (!datasize) {
        return UnallocatedEncoding();
    }

    const IR::U16U32U64 operanda = V_scalar(*datasize, Va);
    const IR::U16U32U64 operand1 = V_scalar(*datasize, Vn);
    const IR::U16U32U64 operand2 = V_scalar(*datasize, Vm);
    const IR::U16U32U64 result = ir.FPMulAdd(ir.FPNeg(operanda), ir.FPNeg(operand1), operand2);
    V_scalar(*datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::FNMSUB_float(Imm<2> type, Vec Vm, Vec Va, Vec Vn, Vec Vd) {
    const auto datasize = FPGetDataSize(type);
    if (!datasize) {
        return UnallocatedEncoding();
    }

    const IR::U16U32U64 operanda = V_scalar(*datasize, Va);
    const IR::U16U32U64 operand1 = V_scalar(*datasize, Vn);
    const IR::U16U32U64 operand2 = V_scalar(*datasize, Vm);
    const IR::U16U32U64 result = ir.FPMulAdd(ir.FPNeg(operanda), operand1, operand2);
    V_scalar(*datasize, Vd, result);
    return true;
}

}  // namespace Dynarmic::A64

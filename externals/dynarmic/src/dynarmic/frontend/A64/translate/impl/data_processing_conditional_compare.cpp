/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::CCMN_reg(bool sf, Reg Rm, Cond cond, Reg Rn, Imm<4> nzcv) {
    const size_t datasize = sf ? 64 : 32;
    const u32 flags = nzcv.ZeroExtend<u32>() << 28;

    const IR::U32U64 operand1 = X(datasize, Rn);
    const IR::U32U64 operand2 = X(datasize, Rm);

    const IR::NZCV then_flags = ir.NZCVFrom(ir.AddWithCarry(operand1, operand2, ir.Imm1(0)));
    const IR::NZCV else_flags = ir.NZCVFromPackedFlags(ir.Imm32(flags));
    ir.SetNZCV(ir.ConditionalSelect(cond, then_flags, else_flags));
    return true;
}

bool TranslatorVisitor::CCMP_reg(bool sf, Reg Rm, Cond cond, Reg Rn, Imm<4> nzcv) {
    const size_t datasize = sf ? 64 : 32;
    const u32 flags = nzcv.ZeroExtend<u32>() << 28;

    const IR::U32U64 operand1 = X(datasize, Rn);
    const IR::U32U64 operand2 = X(datasize, Rm);

    const IR::NZCV then_flags = ir.NZCVFrom(ir.SubWithCarry(operand1, operand2, ir.Imm1(1)));
    const IR::NZCV else_flags = ir.NZCVFromPackedFlags(ir.Imm32(flags));
    ir.SetNZCV(ir.ConditionalSelect(cond, then_flags, else_flags));
    return true;
}

bool TranslatorVisitor::CCMN_imm(bool sf, Imm<5> imm5, Cond cond, Reg Rn, Imm<4> nzcv) {
    const size_t datasize = sf ? 64 : 32;
    const u32 flags = nzcv.ZeroExtend<u32>() << 28;

    const IR::U32U64 operand1 = X(datasize, Rn);
    const IR::U32U64 operand2 = I(datasize, imm5.ZeroExtend<u32>());

    const IR::NZCV then_flags = ir.NZCVFrom(ir.AddWithCarry(operand1, operand2, ir.Imm1(0)));
    const IR::NZCV else_flags = ir.NZCVFromPackedFlags(ir.Imm32(flags));
    ir.SetNZCV(ir.ConditionalSelect(cond, then_flags, else_flags));
    return true;
}

bool TranslatorVisitor::CCMP_imm(bool sf, Imm<5> imm5, Cond cond, Reg Rn, Imm<4> nzcv) {
    const size_t datasize = sf ? 64 : 32;
    const u32 flags = nzcv.ZeroExtend<u32>() << 28;

    const IR::U32U64 operand1 = X(datasize, Rn);
    const IR::U32U64 operand2 = I(datasize, imm5.ZeroExtend<u32>());

    const IR::NZCV then_flags = ir.NZCVFrom(ir.SubWithCarry(operand1, operand2, ir.Imm1(1)));
    const IR::NZCV else_flags = ir.NZCVFromPackedFlags(ir.Imm32(flags));
    ir.SetNZCV(ir.ConditionalSelect(cond, then_flags, else_flags));
    return true;
}

}  // namespace Dynarmic::A64

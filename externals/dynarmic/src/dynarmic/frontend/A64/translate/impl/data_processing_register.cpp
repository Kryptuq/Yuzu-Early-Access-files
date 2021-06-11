/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::CLZ_int(bool sf, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    const IR::U32U64 operand = X(datasize, Rn);
    const IR::U32U64 result = ir.CountLeadingZeros(operand);

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::CLS_int(bool sf, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    const IR::U32U64 operand = X(datasize, Rn);
    const IR::U32U64 result = ir.Sub(ir.CountLeadingZeros(ir.Eor(operand, ir.ArithmeticShiftRight(operand, ir.Imm8(u8(datasize))))), I(datasize, 1));

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::RBIT_int(bool sf, Reg Rn, Reg Rd) {
    const auto rbit32 = [this](const IR::U32& operand) {
        // x = (x & 0x55555555) << 1 | ((x >> 1) & 0x55555555);
        const IR::U32 first_lsl = ir.LogicalShiftLeft(ir.And(operand, ir.Imm32(0x55555555)), ir.Imm8(1));
        const IR::U32 first_lsr = ir.And(ir.LogicalShiftRight(operand, ir.Imm8(1)), ir.Imm32(0x55555555));
        const IR::U32 first = ir.Or(first_lsl, first_lsr);

        // x = (x & 0x33333333) << 2 | ((x >> 2) & 0x33333333);
        const IR::U32 second_lsl = ir.LogicalShiftLeft(ir.And(first, ir.Imm32(0x33333333)), ir.Imm8(2));
        const IR::U32 second_lsr = ir.And(ir.LogicalShiftRight(first, ir.Imm8(2)), ir.Imm32(0x33333333));
        const IR::U32 second = ir.Or(second_lsl, second_lsr);

        // x = (x & 0x0F0F0F0F) << 4 | ((x >> 4) & 0x0F0F0F0F);
        const IR::U32 third_lsl = ir.LogicalShiftLeft(ir.And(second, ir.Imm32(0x0F0F0F0F)), ir.Imm8(4));
        const IR::U32 third_lsr = ir.And(ir.LogicalShiftRight(second, ir.Imm8(4)), ir.Imm32(0x0F0F0F0F));
        const IR::U32 third = ir.Or(third_lsl, third_lsr);

        // x = (x << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | (x >> 24);
        const IR::U32 fourth_lsl = ir.Or(ir.LogicalShiftLeft(third, ir.Imm8(24)),
                                         ir.LogicalShiftLeft(ir.And(third, ir.Imm32(0xFF00)), ir.Imm8(8)));
        const IR::U32 fourth_lsr = ir.Or(ir.And(ir.LogicalShiftRight(third, ir.Imm8(8)), ir.Imm32(0xFF00)),
                                         ir.LogicalShiftRight(third, ir.Imm8(24)));
        return ir.Or(fourth_lsl, fourth_lsr);
    };

    const size_t datasize = sf ? 64 : 32;
    const IR::U32U64 operand = X(datasize, Rn);

    if (sf) {
        const IR::U32 lsw = rbit32(ir.LeastSignificantWord(operand));
        const IR::U32 msw = rbit32(ir.MostSignificantWord(operand).result);
        const IR::U64 result = ir.Pack2x32To1x64(msw, lsw);

        X(datasize, Rd, result);
    } else {
        X(datasize, Rd, rbit32(operand));
    }

    return true;
}

bool TranslatorVisitor::REV(bool sf, bool opc_0, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    if (!sf && opc_0)
        return UnallocatedEncoding();

    const IR::U32U64 operand = X(datasize, Rn);

    if (sf) {
        X(datasize, Rd, ir.ByteReverseDual(operand));
    } else {
        X(datasize, Rd, ir.ByteReverseWord(operand));
    }
    return true;
}

bool TranslatorVisitor::REV32_int(Reg Rn, Reg Rd) {
    const IR::U64 operand = ir.GetX(Rn);
    const IR::U32 lo = ir.ByteReverseWord(ir.LeastSignificantWord(operand));
    const IR::U32 hi = ir.ByteReverseWord(ir.MostSignificantWord(operand).result);
    const IR::U64 result = ir.Pack2x32To1x64(lo, hi);
    X(64, Rd, result);
    return true;
}

bool TranslatorVisitor::REV16_int(bool sf, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    if (sf) {
        const IR::U64 operand = X(datasize, Rn);
        const IR::U64 hihalf = ir.And(ir.LogicalShiftRight(operand, ir.Imm8(8)), ir.Imm64(0x00FF00FF00FF00FF));
        const IR::U64 lohalf = ir.And(ir.LogicalShiftLeft(operand, ir.Imm8(8)), ir.Imm64(0xFF00FF00FF00FF00));
        const IR::U64 result = ir.Or(hihalf, lohalf);
        X(datasize, Rd, result);
    } else {
        const IR::U32 operand = X(datasize, Rn);
        const IR::U32 hihalf = ir.And(ir.LogicalShiftRight(operand, ir.Imm8(8)), ir.Imm32(0x00FF00FF));
        const IR::U32 lohalf = ir.And(ir.LogicalShiftLeft(operand, ir.Imm8(8)), ir.Imm32(0xFF00FF00));
        const IR::U32 result = ir.Or(hihalf, lohalf);
        X(datasize, Rd, result);
    }
    return true;
}

bool TranslatorVisitor::UDIV(bool sf, Reg Rm, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    const IR::U32U64 m = X(datasize, Rm);
    const IR::U32U64 n = X(datasize, Rn);

    const IR::U32U64 result = ir.UnsignedDiv(n, m);

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::SDIV(bool sf, Reg Rm, Reg Rn, Reg Rd) {
    const size_t datasize = sf ? 64 : 32;

    const IR::U32U64 m = X(datasize, Rm);
    const IR::U32U64 n = X(datasize, Rn);

    const IR::U32U64 result = ir.SignedDiv(n, m);

    X(datasize, Rd, result);
    return true;
}

}  // namespace Dynarmic::A64

/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"

namespace Dynarmic::A32 {

// MLA{S}<c> <Rd>, <Rn>, <Rm>, <Ra>
bool TranslatorVisitor::arm_MLA(Cond cond, bool S, Reg d, Reg a, Reg m, Reg n) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.Add(ir.Mul(ir.GetRegister(n), ir.GetRegister(m)), ir.GetRegister(a));
    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
    }

    return true;
}

// MLS<c> <Rd>, <Rn>, <Rm>, <Ra>
bool TranslatorVisitor::arm_MLS(Cond cond, Reg d, Reg a, Reg m, Reg n) {
    if (d == Reg::PC || a == Reg::PC || m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 operand1 = ir.GetRegister(n);
    const IR::U32 operand2 = ir.GetRegister(m);
    const IR::U32 operand3 = ir.GetRegister(a);
    const IR::U32 result = ir.Sub(operand3, ir.Mul(operand1, operand2));

    ir.SetRegister(d, result);
    return true;
}

// MUL{S}<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_MUL(Cond cond, bool S, Reg d, Reg m, Reg n) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.Mul(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
    }

    return true;
}

// SMLAL{S}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SMLAL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dLo == dHi) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto n64 = ir.SignExtendWordToLong(ir.GetRegister(n));
    const auto m64 = ir.SignExtendWordToLong(ir.GetRegister(m));
    const auto product = ir.Mul(n64, m64);
    const auto addend = ir.Pack2x32To1x64(ir.GetRegister(dLo), ir.GetRegister(dHi));
    const auto result = ir.Add(product, addend);
    const auto lo = ir.LeastSignificantWord(result);
    const auto hi = ir.MostSignificantWord(result).result;

    ir.SetRegister(dLo, lo);
    ir.SetRegister(dHi, hi);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(hi));
        ir.SetZFlag(ir.IsZero(result));
    }

    return true;
}

// SMULL{S}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SMULL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dLo == dHi) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto n64 = ir.SignExtendWordToLong(ir.GetRegister(n));
    const auto m64 = ir.SignExtendWordToLong(ir.GetRegister(m));
    const auto result = ir.Mul(n64, m64);
    const auto lo = ir.LeastSignificantWord(result);
    const auto hi = ir.MostSignificantWord(result).result;

    ir.SetRegister(dLo, lo);
    ir.SetRegister(dHi, hi);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(hi));
        ir.SetZFlag(ir.IsZero(result));
    }

    return true;
}

// UMAAL<c> <RdLo>, <RdHi>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UMAAL(Cond cond, Reg dHi, Reg dLo, Reg m, Reg n) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dLo == dHi) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto lo64 = ir.ZeroExtendWordToLong(ir.GetRegister(dLo));
    const auto hi64 = ir.ZeroExtendWordToLong(ir.GetRegister(dHi));
    const auto n64 = ir.ZeroExtendWordToLong(ir.GetRegister(n));
    const auto m64 = ir.ZeroExtendWordToLong(ir.GetRegister(m));
    const auto result = ir.Add(ir.Add(ir.Mul(n64, m64), hi64), lo64);

    ir.SetRegister(dLo, ir.LeastSignificantWord(result));
    ir.SetRegister(dHi, ir.MostSignificantWord(result).result);
    return true;
}

// UMLAL{S}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UMLAL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dLo == dHi) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto addend = ir.Pack2x32To1x64(ir.GetRegister(dLo), ir.GetRegister(dHi));
    const auto n64 = ir.ZeroExtendWordToLong(ir.GetRegister(n));
    const auto m64 = ir.ZeroExtendWordToLong(ir.GetRegister(m));
    const auto result = ir.Add(ir.Mul(n64, m64), addend);
    const auto lo = ir.LeastSignificantWord(result);
    const auto hi = ir.MostSignificantWord(result).result;

    ir.SetRegister(dLo, lo);
    ir.SetRegister(dHi, hi);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(hi));
        ir.SetZFlag(ir.IsZero(result));
    }

    return true;
}

// UMULL{S}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UMULL(Cond cond, bool S, Reg dHi, Reg dLo, Reg m, Reg n) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dLo == dHi) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto n64 = ir.ZeroExtendWordToLong(ir.GetRegister(n));
    const auto m64 = ir.ZeroExtendWordToLong(ir.GetRegister(m));
    const auto result = ir.Mul(n64, m64);
    const auto lo = ir.LeastSignificantWord(result);
    const auto hi = ir.MostSignificantWord(result).result;

    ir.SetRegister(dLo, lo);
    ir.SetRegister(dHi, hi);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(hi));
        ir.SetZFlag(ir.IsZero(result));
    }

    return true;
}

// SMLAL<x><y><c> <RdLo>, <RdHi>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SMLALxy(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, bool N, Reg n) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dLo == dHi) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 n32 = ir.GetRegister(n);
    const IR::U32 m32 = ir.GetRegister(m);
    const IR::U32 n16 = N ? ir.ArithmeticShiftRight(n32, ir.Imm8(16), ir.Imm1(0)).result
                          : ir.SignExtendHalfToWord(ir.LeastSignificantHalf(n32));
    const IR::U32 m16 = M ? ir.ArithmeticShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result
                          : ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32));
    const IR::U64 product = ir.SignExtendWordToLong(ir.Mul(n16, m16));
    const auto addend = ir.Pack2x32To1x64(ir.GetRegister(dLo), ir.GetRegister(dHi));
    const auto result = ir.Add(product, addend);

    ir.SetRegister(dLo, ir.LeastSignificantWord(result));
    ir.SetRegister(dHi, ir.MostSignificantWord(result).result);
    return true;
}

// SMLA<x><y><c> <Rd>, <Rn>, <Rm>, <Ra>
bool TranslatorVisitor::arm_SMLAxy(Cond cond, Reg d, Reg a, Reg m, bool M, bool N, Reg n) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 n32 = ir.GetRegister(n);
    const IR::U32 m32 = ir.GetRegister(m);
    const IR::U32 n16 = N ? ir.ArithmeticShiftRight(n32, ir.Imm8(16), ir.Imm1(0)).result
                          : ir.SignExtendHalfToWord(ir.LeastSignificantHalf(n32));
    const IR::U32 m16 = M ? ir.ArithmeticShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result
                          : ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32));
    const IR::U32 product = ir.Mul(n16, m16);
    const auto result = ir.AddWithCarry(product, ir.GetRegister(a), ir.Imm1(0));

    ir.SetRegister(d, result);
    ir.OrQFlag(ir.GetOverflowFrom(result));
    return true;
}

// SMUL<x><y><c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SMULxy(Cond cond, Reg d, Reg m, bool M, bool N, Reg n) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 n32 = ir.GetRegister(n);
    const IR::U32 m32 = ir.GetRegister(m);
    const IR::U32 n16 = N ? ir.ArithmeticShiftRight(n32, ir.Imm8(16), ir.Imm1(0)).result
                          : ir.SignExtendHalfToWord(ir.LeastSignificantHalf(n32));
    const IR::U32 m16 = M ? ir.ArithmeticShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result
                          : ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32));
    const IR::U32 result = ir.Mul(n16, m16);

    ir.SetRegister(d, result);
    return true;
}

// SMLAW<y><c> <Rd>, <Rn>, <Rm>, <Ra>
bool TranslatorVisitor::arm_SMLAWy(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U64 n32 = ir.SignExtendWordToLong(ir.GetRegister(n));
    IR::U32 m32 = ir.GetRegister(m);
    if (M) {
        m32 = ir.LogicalShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result;
    }
    const IR::U64 m16 = ir.SignExtendWordToLong(ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32)));
    const auto product = ir.LeastSignificantWord(ir.LogicalShiftRight(ir.Mul(n32, m16), ir.Imm8(16)));
    const auto result = ir.AddWithCarry(product, ir.GetRegister(a), ir.Imm1(0));

    ir.SetRegister(d, result);
    ir.OrQFlag(ir.GetOverflowFrom(result));
    return true;
}

// SMULW<y><c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SMULWy(Cond cond, Reg d, Reg m, bool M, Reg n) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U64 n32 = ir.SignExtendWordToLong(ir.GetRegister(n));
    IR::U32 m32 = ir.GetRegister(m);
    if (M) {
        m32 = ir.LogicalShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result;
    }
    const IR::U64 m16 = ir.SignExtendWordToLong(ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32)));
    const auto result = ir.LogicalShiftRight(ir.Mul(n32, m16), ir.Imm8(16));

    ir.SetRegister(d, ir.LeastSignificantWord(result));
    return true;
}

// SMMLA{R}<c> <Rd>, <Rn>, <Rm>, <Ra>
bool TranslatorVisitor::arm_SMMLA(Cond cond, Reg d, Reg a, Reg m, bool R, Reg n) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC /* no check for a */) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto n64 = ir.SignExtendWordToLong(ir.GetRegister(n));
    const auto m64 = ir.SignExtendWordToLong(ir.GetRegister(m));
    const auto a64 = ir.Pack2x32To1x64(ir.Imm32(0), ir.GetRegister(a));
    const auto temp = ir.Add(a64, ir.Mul(n64, m64));
    const auto result_carry = ir.MostSignificantWord(temp);
    auto result = result_carry.result;
    if (R) {
        result = ir.AddWithCarry(result, ir.Imm32(0), result_carry.carry);
    }

    ir.SetRegister(d, result);
    return true;
}

// SMMLS{R}<c> <Rd>, <Rn>, <Rm>, <Ra>
bool TranslatorVisitor::arm_SMMLS(Cond cond, Reg d, Reg a, Reg m, bool R, Reg n) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto n64 = ir.SignExtendWordToLong(ir.GetRegister(n));
    const auto m64 = ir.SignExtendWordToLong(ir.GetRegister(m));
    const auto a64 = ir.Pack2x32To1x64(ir.Imm32(0), ir.GetRegister(a));
    const auto temp = ir.Sub(a64, ir.Mul(n64, m64));
    const auto result_carry = ir.MostSignificantWord(temp);
    auto result = result_carry.result;
    if (R) {
        result = ir.AddWithCarry(result, ir.Imm32(0), result_carry.carry);
    }

    ir.SetRegister(d, result);
    return true;
}

// SMMUL{R}<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SMMUL(Cond cond, Reg d, Reg m, bool R, Reg n) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto n64 = ir.SignExtendWordToLong(ir.GetRegister(n));
    const auto m64 = ir.SignExtendWordToLong(ir.GetRegister(m));
    const auto product = ir.Mul(n64, m64);
    const auto result_carry = ir.MostSignificantWord(product);
    auto result = result_carry.result;
    if (R) {
        result = ir.AddWithCarry(result, ir.Imm32(0), result_carry.carry);
    }

    ir.SetRegister(d, result);
    return true;
}

// SMLAD{X}<c> <Rd>, <Rn>, <Rm>, <Ra>
bool TranslatorVisitor::arm_SMLAD(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) {
    if (a == Reg::PC) {
        return arm_SMUAD(cond, d, m, M, n);
    }

    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 n32 = ir.GetRegister(n);
    const IR::U32 m32 = ir.GetRegister(m);
    const IR::U32 n_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(n32));
    const IR::U32 n_hi = ir.ArithmeticShiftRight(n32, ir.Imm8(16), ir.Imm1(0)).result;

    IR::U32 m_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32));
    IR::U32 m_hi = ir.ArithmeticShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result;
    if (M) {
        std::swap(m_lo, m_hi);
    }

    const IR::U32 product_lo = ir.Mul(n_lo, m_lo);
    const IR::U32 product_hi = ir.Mul(n_hi, m_hi);
    const IR::U32 addend = ir.GetRegister(a);

    auto result = ir.AddWithCarry(product_lo, product_hi, ir.Imm1(0));
    ir.OrQFlag(ir.GetOverflowFrom(result));
    result = ir.AddWithCarry(result, addend, ir.Imm1(0));
    ir.SetRegister(d, result);
    ir.OrQFlag(ir.GetOverflowFrom(result));
    return true;
}

// SMLALD{X}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SMLALD(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, Reg n) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dLo == dHi) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 n32 = ir.GetRegister(n);
    const IR::U32 m32 = ir.GetRegister(m);
    const IR::U32 n_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(n32));
    const IR::U32 n_hi = ir.ArithmeticShiftRight(n32, ir.Imm8(16), ir.Imm1(0)).result;

    IR::U32 m_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32));
    IR::U32 m_hi = ir.ArithmeticShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result;
    if (M) {
        std::swap(m_lo, m_hi);
    }

    const IR::U64 product_lo = ir.SignExtendWordToLong(ir.Mul(n_lo, m_lo));
    const IR::U64 product_hi = ir.SignExtendWordToLong(ir.Mul(n_hi, m_hi));
    const auto addend = ir.Pack2x32To1x64(ir.GetRegister(dLo), ir.GetRegister(dHi));
    const auto result = ir.Add(ir.Add(product_lo, product_hi), addend);

    ir.SetRegister(dLo, ir.LeastSignificantWord(result));
    ir.SetRegister(dHi, ir.MostSignificantWord(result).result);
    return true;
}

// SMLSD{X}<c> <Rd>, <Rn>, <Rm>, <Ra>
bool TranslatorVisitor::arm_SMLSD(Cond cond, Reg d, Reg a, Reg m, bool M, Reg n) {
    if (a == Reg::PC) {
        return arm_SMUSD(cond, d, m, M, n);
    }

    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 n32 = ir.GetRegister(n);
    const IR::U32 m32 = ir.GetRegister(m);
    const IR::U32 n_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(n32));
    const IR::U32 n_hi = ir.ArithmeticShiftRight(n32, ir.Imm8(16), ir.Imm1(0)).result;

    IR::U32 m_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32));
    IR::U32 m_hi = ir.ArithmeticShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result;
    if (M) {
        std::swap(m_lo, m_hi);
    }

    const IR::U32 product_lo = ir.Mul(n_lo, m_lo);
    const IR::U32 product_hi = ir.Mul(n_hi, m_hi);
    const IR::U32 addend = ir.GetRegister(a);
    const IR::U32 product = ir.Sub(product_lo, product_hi);
    auto result = ir.AddWithCarry(product, addend, ir.Imm1(0));

    ir.SetRegister(d, result);
    ir.OrQFlag(ir.GetOverflowFrom(result));
    return true;
}

// SMLSLD{X}<c> <RdLo>, <RdHi>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SMLSLD(Cond cond, Reg dHi, Reg dLo, Reg m, bool M, Reg n) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dLo == dHi) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 n32 = ir.GetRegister(n);
    const IR::U32 m32 = ir.GetRegister(m);
    const IR::U32 n_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(n32));
    const IR::U32 n_hi = ir.ArithmeticShiftRight(n32, ir.Imm8(16), ir.Imm1(0)).result;

    IR::U32 m_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32));
    IR::U32 m_hi = ir.ArithmeticShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result;
    if (M) {
        std::swap(m_lo, m_hi);
    }

    const IR::U64 product_lo = ir.SignExtendWordToLong(ir.Mul(n_lo, m_lo));
    const IR::U64 product_hi = ir.SignExtendWordToLong(ir.Mul(n_hi, m_hi));
    const auto addend = ir.Pack2x32To1x64(ir.GetRegister(dLo), ir.GetRegister(dHi));
    const auto result = ir.Add(ir.Sub(product_lo, product_hi), addend);

    ir.SetRegister(dLo, ir.LeastSignificantWord(result));
    ir.SetRegister(dHi, ir.MostSignificantWord(result).result);
    return true;
}

// SMUAD{X}<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SMUAD(Cond cond, Reg d, Reg m, bool M, Reg n) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 n32 = ir.GetRegister(n);
    const IR::U32 m32 = ir.GetRegister(m);
    const IR::U32 n_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(n32));
    const IR::U32 n_hi = ir.ArithmeticShiftRight(n32, ir.Imm8(16), ir.Imm1(0)).result;

    IR::U32 m_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32));
    IR::U32 m_hi = ir.ArithmeticShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result;
    if (M) {
        std::swap(m_lo, m_hi);
    }

    const IR::U32 product_lo = ir.Mul(n_lo, m_lo);
    const IR::U32 product_hi = ir.Mul(n_hi, m_hi);
    const auto result = ir.AddWithCarry(product_lo, product_hi, ir.Imm1(0));

    ir.SetRegister(d, result);
    ir.OrQFlag(ir.GetOverflowFrom(result));
    return true;
}

// SMUSD{X}<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SMUSD(Cond cond, Reg d, Reg m, bool M, Reg n) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const IR::U32 n32 = ir.GetRegister(n);
    const IR::U32 m32 = ir.GetRegister(m);
    const IR::U32 n_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(n32));
    const IR::U32 n_hi = ir.ArithmeticShiftRight(n32, ir.Imm8(16), ir.Imm1(0)).result;

    IR::U32 m_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32));
    IR::U32 m_hi = ir.ArithmeticShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result;
    if (M) {
        std::swap(m_lo, m_hi);
    }

    const IR::U32 product_lo = ir.Mul(n_lo, m_lo);
    const IR::U32 product_hi = ir.Mul(n_hi, m_hi);
    auto result = ir.Sub(product_lo, product_hi);
    ir.SetRegister(d, result);
    return true;
}

}  // namespace Dynarmic::A32

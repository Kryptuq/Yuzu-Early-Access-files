/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {

bool TranslatorVisitor::thumb32_MLA(Reg n, Reg a, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_a = ir.GetRegister(a);
    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Add(ir.Mul(reg_n, reg_m), reg_a);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_MLS(Reg n, Reg a, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_a = ir.GetRegister(a);
    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Sub(reg_a, ir.Mul(reg_n, reg_m));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_MUL(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Mul(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SMLAD(Reg n, Reg a, Reg d, bool X, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
    }

    const IR::U32 n32 = ir.GetRegister(n);
    const IR::U32 m32 = ir.GetRegister(m);
    const IR::U32 n_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(n32));
    const IR::U32 n_hi = ir.ArithmeticShiftRight(n32, ir.Imm8(16), ir.Imm1(0)).result;

    IR::U32 m_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32));
    IR::U32 m_hi = ir.ArithmeticShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result;
    if (X) {
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

bool TranslatorVisitor::thumb32_SMLSD(Reg n, Reg a, Reg d, bool X, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
    }

    const IR::U32 n32 = ir.GetRegister(n);
    const IR::U32 m32 = ir.GetRegister(m);
    const IR::U32 n_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(n32));
    const IR::U32 n_hi = ir.ArithmeticShiftRight(n32, ir.Imm8(16), ir.Imm1(0)).result;

    IR::U32 m_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32));
    IR::U32 m_hi = ir.ArithmeticShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result;
    if (X) {
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

bool TranslatorVisitor::thumb32_SMLAXY(Reg n, Reg a, Reg d, bool N, bool M, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
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

bool TranslatorVisitor::thumb32_SMMLA(Reg n, Reg a, Reg d, bool R, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
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

bool TranslatorVisitor::thumb32_SMMLS(Reg n, Reg a, Reg d, bool R, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
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

bool TranslatorVisitor::thumb32_SMMUL(Reg n, Reg d, bool R, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
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

bool TranslatorVisitor::thumb32_SMUAD(Reg n, Reg d, bool M, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
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

bool TranslatorVisitor::thumb32_SMUSD(Reg n, Reg d, bool M, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
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
    const IR::U32 result = ir.Sub(product_lo, product_hi);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SMULXY(Reg n, Reg d, bool N, bool M, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto n32 = ir.GetRegister(n);
    const auto m32 = ir.GetRegister(m);
    const auto n16 = N ? ir.ArithmeticShiftRight(n32, ir.Imm8(16), ir.Imm1(0)).result
                       : ir.SignExtendHalfToWord(ir.LeastSignificantHalf(n32));
    const auto m16 = M ? ir.ArithmeticShiftRight(m32, ir.Imm8(16), ir.Imm1(0)).result
                       : ir.SignExtendHalfToWord(ir.LeastSignificantHalf(m32));
    const auto result = ir.Mul(n16, m16);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SMLAWY(Reg n, Reg a, Reg d, bool M, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
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

bool TranslatorVisitor::thumb32_SMULWY(Reg n, Reg d, bool M, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
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

bool TranslatorVisitor::thumb32_USAD8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAbsDiffSumS8(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_USADA8(Reg n, Reg a, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || a == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_a = ir.GetRegister(a);
    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto tmp = ir.PackedAbsDiffSumS8(reg_n, reg_m);
    const auto result = ir.AddWithCarry(reg_a, tmp, ir.Imm1(0));

    ir.SetRegister(d, result);
    return true;
}

}  // namespace Dynarmic::A32

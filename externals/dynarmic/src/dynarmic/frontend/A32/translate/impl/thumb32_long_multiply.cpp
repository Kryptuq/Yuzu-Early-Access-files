/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {
namespace {
using DivideFunction = IR::U32U64 (IREmitter::*)(const IR::U32U64&, const IR::U32U64&);

bool DivideOperation(TranslatorVisitor& v, Reg d, Reg m, Reg n, DivideFunction fn) {
    if (d == Reg::PC || m == Reg::PC || n == Reg::PC) {
        return v.UnpredictableInstruction();
    }

    const IR::U32 operand1 = v.ir.GetRegister(n);
    const IR::U32 operand2 = v.ir.GetRegister(m);
    const IR::U32 result = (v.ir.*fn)(operand1, operand2);

    v.ir.SetRegister(d, result);
    return true;
}
}  // Anonymous namespace

bool TranslatorVisitor::thumb32_SDIV(Reg n, Reg d, Reg m) {
    return DivideOperation(*this, d, m, n, &IREmitter::SignedDiv);
}

bool TranslatorVisitor::thumb32_SMLAL(Reg n, Reg dLo, Reg dHi, Reg m) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dHi == dLo) {
        return UnpredictableInstruction();
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
    return true;
}

bool TranslatorVisitor::thumb32_SMLALD(Reg n, Reg dLo, Reg dHi, bool M, Reg m) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dHi == dLo) {
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

    const IR::U64 product_lo = ir.SignExtendWordToLong(ir.Mul(n_lo, m_lo));
    const IR::U64 product_hi = ir.SignExtendWordToLong(ir.Mul(n_hi, m_hi));
    const auto addend = ir.Pack2x32To1x64(ir.GetRegister(dLo), ir.GetRegister(dHi));
    const auto result = ir.Add(ir.Add(product_lo, product_hi), addend);

    ir.SetRegister(dLo, ir.LeastSignificantWord(result));
    ir.SetRegister(dHi, ir.MostSignificantWord(result).result);
    return true;
}

bool TranslatorVisitor::thumb32_SMLALXY(Reg n, Reg dLo, Reg dHi, bool N, bool M, Reg m) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dHi == dLo) {
        return UnpredictableInstruction();
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

bool TranslatorVisitor::thumb32_SMLSLD(Reg n, Reg dLo, Reg dHi, bool M, Reg m) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dHi == dLo) {
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

    const IR::U64 product_lo = ir.SignExtendWordToLong(ir.Mul(n_lo, m_lo));
    const IR::U64 product_hi = ir.SignExtendWordToLong(ir.Mul(n_hi, m_hi));
    const auto addend = ir.Pack2x32To1x64(ir.GetRegister(dLo), ir.GetRegister(dHi));
    const auto result = ir.Add(ir.Sub(product_lo, product_hi), addend);

    ir.SetRegister(dLo, ir.LeastSignificantWord(result));
    ir.SetRegister(dHi, ir.MostSignificantWord(result).result);
    return true;
}

bool TranslatorVisitor::thumb32_SMULL(Reg n, Reg dLo, Reg dHi, Reg m) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dHi == dLo) {
        return UnpredictableInstruction();
    }

    const auto n64 = ir.SignExtendWordToLong(ir.GetRegister(n));
    const auto m64 = ir.SignExtendWordToLong(ir.GetRegister(m));
    const auto result = ir.Mul(n64, m64);
    const auto lo = ir.LeastSignificantWord(result);
    const auto hi = ir.MostSignificantWord(result).result;

    ir.SetRegister(dLo, lo);
    ir.SetRegister(dHi, hi);
    return true;
}

bool TranslatorVisitor::thumb32_UDIV(Reg n, Reg d, Reg m) {
    return DivideOperation(*this, d, m, n, &IREmitter::UnsignedDiv);
}

bool TranslatorVisitor::thumb32_UMLAL(Reg n, Reg dLo, Reg dHi, Reg m) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dHi == dLo) {
        return UnpredictableInstruction();
    }

    const auto n64 = ir.ZeroExtendWordToLong(ir.GetRegister(n));
    const auto m64 = ir.ZeroExtendWordToLong(ir.GetRegister(m));
    const auto product = ir.Mul(n64, m64);
    const auto addend = ir.Pack2x32To1x64(ir.GetRegister(dLo), ir.GetRegister(dHi));
    const auto result = ir.Add(product, addend);
    const auto lo = ir.LeastSignificantWord(result);
    const auto hi = ir.MostSignificantWord(result).result;

    ir.SetRegister(dLo, lo);
    ir.SetRegister(dHi, hi);
    return true;
}

bool TranslatorVisitor::thumb32_UMULL(Reg n, Reg dLo, Reg dHi, Reg m) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dHi == dLo) {
        return UnpredictableInstruction();
    }

    const auto n64 = ir.ZeroExtendWordToLong(ir.GetRegister(n));
    const auto m64 = ir.ZeroExtendWordToLong(ir.GetRegister(m));
    const auto result = ir.Mul(n64, m64);
    const auto lo = ir.LeastSignificantWord(result);
    const auto hi = ir.MostSignificantWord(result).result;

    ir.SetRegister(dLo, lo);
    ir.SetRegister(dHi, hi);
    return true;
}

bool TranslatorVisitor::thumb32_UMAAL(Reg n, Reg dLo, Reg dHi, Reg m) {
    if (dLo == Reg::PC || dHi == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (dHi == dLo) {
        return UnpredictableInstruction();
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

}  // namespace Dynarmic::A32

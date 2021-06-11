/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {
namespace {
IR::U32 Rotate(A32::IREmitter& ir, Reg m, SignExtendRotation rotate) {
    const u8 rotate_by = static_cast<u8>(static_cast<size_t>(rotate) * 8);
    return ir.RotateRight(ir.GetRegister(m), ir.Imm8(rotate_by), ir.Imm1(0)).result;
}

using ShiftFunction = IR::ResultAndCarry<IR::U32> (IREmitter::*)(const IR::U32&, const IR::U8&, const IR::U1&);

bool ShiftInstruction(TranslatorVisitor& v, Reg m, Reg d, Reg s, bool S, ShiftFunction shift_fn) {
    if (d == Reg::PC || m == Reg::PC || s == Reg::PC) {
        return v.UnpredictableInstruction();
    }

    const auto shift_s = v.ir.LeastSignificantByte(v.ir.GetRegister(s));
    const auto apsr_c = v.ir.GetCFlag();
    const auto result_carry = (v.ir.*shift_fn)(v.ir.GetRegister(m), shift_s, apsr_c);

    if (S) {
        v.ir.SetNFlag(v.ir.MostSignificantBit(result_carry.result));
        v.ir.SetZFlag(v.ir.IsZero(result_carry.result));
        v.ir.SetCFlag(result_carry.carry);
    }

    v.ir.SetRegister(d, result_carry.result);
    return true;
}
}  // Anonymous namespace

bool TranslatorVisitor::thumb32_ASR_reg(bool S, Reg m, Reg d, Reg s) {
    return ShiftInstruction(*this, m, d, s, S, &IREmitter::ArithmeticShiftRight);
}

bool TranslatorVisitor::thumb32_LSL_reg(bool S, Reg m, Reg d, Reg s) {
    return ShiftInstruction(*this, m, d, s, S, &IREmitter::LogicalShiftLeft);
}

bool TranslatorVisitor::thumb32_LSR_reg(bool S, Reg m, Reg d, Reg s) {
    return ShiftInstruction(*this, m, d, s, S, &IREmitter::LogicalShiftRight);
}

bool TranslatorVisitor::thumb32_ROR_reg(bool S, Reg m, Reg d, Reg s) {
    return ShiftInstruction(*this, m, d, s, S, &IREmitter::RotateRight);
}

bool TranslatorVisitor::thumb32_SXTB(Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.SignExtendByteToWord(ir.LeastSignificantByte(rotated));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SXTB16(Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto low_byte = ir.And(rotated, ir.Imm32(0x00FF00FF));
    const auto sign_bit = ir.And(rotated, ir.Imm32(0x00800080));
    const auto result = ir.Or(low_byte, ir.Mul(sign_bit, ir.Imm32(0x1FE)));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SXTAB(Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Add(reg_n, ir.SignExtendByteToWord(ir.LeastSignificantByte(rotated)));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SXTAB16(Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto low_byte = ir.And(rotated, ir.Imm32(0x00FF00FF));
    const auto sign_bit = ir.And(rotated, ir.Imm32(0x00800080));
    const auto addend = ir.Or(low_byte, ir.Mul(sign_bit, ir.Imm32(0x1FE)));
    const auto result = ir.PackedAddU16(addend, ir.GetRegister(n)).result;

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SXTH(Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(rotated));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SXTAH(Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Add(reg_n, ir.SignExtendHalfToWord(ir.LeastSignificantHalf(rotated)));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UXTB(Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.ZeroExtendByteToWord(ir.LeastSignificantByte(rotated));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UXTB16(Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.And(rotated, ir.Imm32(0x00FF00FF));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UXTAB(Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Add(reg_n, ir.ZeroExtendByteToWord(ir.LeastSignificantByte(rotated)));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UXTAB16(Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    auto result = ir.And(rotated, ir.Imm32(0x00FF00FF));
    const auto reg_n = ir.GetRegister(n);
    result = ir.PackedAddU16(reg_n, result).result;

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UXTH(Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(rotated));

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UXTAH(Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Add(reg_n, ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(rotated)));

    ir.SetRegister(d, result);
    return true;
}

}  // namespace Dynarmic::A32

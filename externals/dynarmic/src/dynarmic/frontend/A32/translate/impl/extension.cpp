/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {

static IR::U32 Rotate(A32::IREmitter& ir, Reg m, SignExtendRotation rotate) {
    const u8 rotate_by = static_cast<u8>(static_cast<size_t>(rotate) * 8);
    return ir.RotateRight(ir.GetRegister(m), ir.Imm8(rotate_by), ir.Imm1(0)).result;
}

// SXTAB<c> <Rd>, <Rn>, <Rm>{, <rotation>}
bool TranslatorVisitor::arm_SXTAB(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Add(reg_n, ir.SignExtendByteToWord(ir.LeastSignificantByte(rotated)));

    ir.SetRegister(d, result);
    return true;
}

// SXTAB16<c> <Rd>, <Rn>, <Rm>{, <rotation>}
bool TranslatorVisitor::arm_SXTAB16(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto low_byte = ir.And(rotated, ir.Imm32(0x00FF00FF));
    const auto sign_bit = ir.And(rotated, ir.Imm32(0x00800080));
    const auto addend = ir.Or(low_byte, ir.Mul(sign_bit, ir.Imm32(0x1FE)));
    const auto result = ir.PackedAddU16(addend, ir.GetRegister(n)).result;

    ir.SetRegister(d, result);
    return true;
}

// SXTAH<c> <Rd>, <Rn>, <Rm>{, <rotation>}
bool TranslatorVisitor::arm_SXTAH(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Add(reg_n, ir.SignExtendHalfToWord(ir.LeastSignificantHalf(rotated)));

    ir.SetRegister(d, result);
    return true;
}

// SXTB<c> <Rd>, <Rm>{, <rotation>}
bool TranslatorVisitor::arm_SXTB(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.SignExtendByteToWord(ir.LeastSignificantByte(rotated));

    ir.SetRegister(d, result);
    return true;
}

// SXTB16<c> <Rd>, <Rm>{, <rotation>}
bool TranslatorVisitor::arm_SXTB16(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto low_byte = ir.And(rotated, ir.Imm32(0x00FF00FF));
    const auto sign_bit = ir.And(rotated, ir.Imm32(0x00800080));
    const auto result = ir.Or(low_byte, ir.Mul(sign_bit, ir.Imm32(0x1FE)));

    ir.SetRegister(d, result);
    return true;
}

// SXTH<c> <Rd>, <Rm>{, <rotation>}
bool TranslatorVisitor::arm_SXTH(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(rotated));

    ir.SetRegister(d, result);
    return true;
}

// UXTAB<c> <Rd>, <Rn>, <Rm>{, <rotation>}
bool TranslatorVisitor::arm_UXTAB(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Add(reg_n, ir.ZeroExtendByteToWord(ir.LeastSignificantByte(rotated)));

    ir.SetRegister(d, result);
    return true;
}

// UXTAB16<c> <Rd>, <Rn>, <Rm>{, <rotation>}
bool TranslatorVisitor::arm_UXTAB16(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto rotated = Rotate(ir, m, rotate);
    auto result = ir.And(rotated, ir.Imm32(0x00FF00FF));
    const auto reg_n = ir.GetRegister(n);
    result = ir.PackedAddU16(reg_n, result).result;

    ir.SetRegister(d, result);
    return true;
}

// UXTAH<c> <Rd>, <Rn>, <Rm>{, <rotation>}
bool TranslatorVisitor::arm_UXTAH(Cond cond, Reg n, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.Add(reg_n, ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(rotated)));

    ir.SetRegister(d, result);
    return true;
}

// UXTB<c> <Rd>, <Rm>{, <rotation>}
bool TranslatorVisitor::arm_UXTB(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.ZeroExtendByteToWord(ir.LeastSignificantByte(rotated));
    ir.SetRegister(d, result);
    return true;
}

// UXTB16<c> <Rd>, <Rm>{, <rotation>}
bool TranslatorVisitor::arm_UXTB16(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.And(rotated, ir.Imm32(0x00FF00FF));

    ir.SetRegister(d, result);
    return true;
}

// UXTH<c> <Rd>, <Rm>{, <rotation>}
bool TranslatorVisitor::arm_UXTH(Cond cond, Reg d, SignExtendRotation rotate, Reg m) {
    if (d == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto rotated = Rotate(ir, m, rotate);
    const auto result = ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(rotated));

    ir.SetRegister(d, result);
    return true;
}

}  // namespace Dynarmic::A32

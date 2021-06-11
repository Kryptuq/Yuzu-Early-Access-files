/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {

// ADC{S}<c> <Rd>, <Rn>, #<imm>
bool TranslatorVisitor::arm_ADC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = ArmExpandImm(rotate, imm8);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.GetCFlag());
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result.result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }
    return true;
}

// ADC{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_ADC_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag());
    const auto result = ir.AddWithCarry(ir.GetRegister(n), shifted.result, ir.GetCFlag());
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result.result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// ADC{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
bool TranslatorVisitor::arm_ADC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || s == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), shifted.result, ir.GetCFlag());

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// ADD{S}<c> <Rd>, <Rn>, #<const>
bool TranslatorVisitor::arm_ADD_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = ArmExpandImm(rotate, imm8);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(0));
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result.result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// ADD{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_ADD_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag());
    const auto result = ir.AddWithCarry(ir.GetRegister(n), shifted.result, ir.Imm1(0));
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result.result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// ADD{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
bool TranslatorVisitor::arm_ADD_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || s == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), shifted.result, ir.Imm1(0));

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// AND{S}<c> <Rd>, <Rn>, #<const>
bool TranslatorVisitor::arm_AND_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto imm_carry = ArmExpandImm_C(rotate, imm8, ir.GetCFlag());
    const auto result = ir.And(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }

    return true;
}

// AND{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_AND_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, carry_in);
    const auto result = ir.And(ir.GetRegister(n), shifted.result);
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }
    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(shifted.carry);
    }

    return true;
}

// AND{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
bool TranslatorVisitor::arm_AND_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || s == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.And(ir.GetRegister(n), shifted.result);

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(shifted.carry);
    }

    return true;
}

// BIC{S}<c> <Rd>, <Rn>, #<const>
bool TranslatorVisitor::arm_BIC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto imm_carry = ArmExpandImm_C(rotate, imm8, ir.GetCFlag());
    const auto result = ir.And(ir.GetRegister(n), ir.Not(ir.Imm32(imm_carry.imm32)));
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }

    return true;
}

// BIC{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_BIC_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, carry_in);
    const auto result = ir.And(ir.GetRegister(n), ir.Not(shifted.result));
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(shifted.carry);
    }

    return true;
}

// BIC{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
bool TranslatorVisitor::arm_BIC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || s == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.And(ir.GetRegister(n), ir.Not(shifted.result));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(shifted.carry);
    }

    return true;
}

// CMN<c> <Rn>, #<const>
bool TranslatorVisitor::arm_CMN_imm(Cond cond, Reg n, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = ArmExpandImm(rotate, imm8);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(0));

    ir.SetNFlag(ir.MostSignificantBit(result.result));
    ir.SetZFlag(ir.IsZero(result.result));
    ir.SetCFlag(result.carry);
    ir.SetVFlag(result.overflow);
    return true;
}

// CMN<c> <Rn>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_CMN_reg(Cond cond, Reg n, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag());
    const auto result = ir.AddWithCarry(ir.GetRegister(n), shifted.result, ir.Imm1(0));

    ir.SetNFlag(ir.MostSignificantBit(result.result));
    ir.SetZFlag(ir.IsZero(result.result));
    ir.SetCFlag(result.carry);
    ir.SetVFlag(result.overflow);
    return true;
}

// CMN<c> <Rn>, <Rm>, <type> <Rs>
bool TranslatorVisitor::arm_CMN_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
    if (n == Reg::PC || m == Reg::PC || s == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), shifted.result, ir.Imm1(0));

    ir.SetNFlag(ir.MostSignificantBit(result.result));
    ir.SetZFlag(ir.IsZero(result.result));
    ir.SetCFlag(result.carry);
    ir.SetVFlag(result.overflow);
    return true;
}

// CMP<c> <Rn>, #<imm>
bool TranslatorVisitor::arm_CMP_imm(Cond cond, Reg n, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = ArmExpandImm(rotate, imm8);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(1));

    ir.SetNFlag(ir.MostSignificantBit(result.result));
    ir.SetZFlag(ir.IsZero(result.result));
    ir.SetCFlag(result.carry);
    ir.SetVFlag(result.overflow);
    return true;
}

// CMP<c> <Rn>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_CMP_reg(Cond cond, Reg n, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag());
    const auto result = ir.SubWithCarry(ir.GetRegister(n), shifted.result, ir.Imm1(1));

    ir.SetNFlag(ir.MostSignificantBit(result.result));
    ir.SetZFlag(ir.IsZero(result.result));
    ir.SetCFlag(result.carry);
    ir.SetVFlag(result.overflow);
    return true;
}

// CMP<c> <Rn>, <Rm>, <type> <Rs>
bool TranslatorVisitor::arm_CMP_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
    if (n == Reg::PC || m == Reg::PC || s == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), shifted.result, ir.Imm1(1));

    ir.SetNFlag(ir.MostSignificantBit(result.result));
    ir.SetZFlag(ir.IsZero(result.result));
    ir.SetCFlag(result.carry);
    ir.SetVFlag(result.overflow);
    return true;
}

// EOR{S}<c> <Rd>, <Rn>, #<const>
bool TranslatorVisitor::arm_EOR_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto imm_carry = ArmExpandImm_C(rotate, imm8, ir.GetCFlag());
    const auto result = ir.Eor(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }

    return true;
}

// EOR{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_EOR_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, carry_in);
    const auto result = ir.Eor(ir.GetRegister(n), shifted.result);
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(shifted.carry);
    }

    return true;
}

// EOR{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
bool TranslatorVisitor::arm_EOR_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC || s == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.Eor(ir.GetRegister(n), shifted.result);

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(shifted.carry);
    }

    return true;
}

// MOV{S}<c> <Rd>, #<const>
bool TranslatorVisitor::arm_MOV_imm(Cond cond, bool S, Reg d, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto imm_carry = ArmExpandImm_C(rotate, imm8, ir.GetCFlag());
    const auto result = ir.Imm32(imm_carry.imm32);
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }

    return true;
}

// MOV{S}<c> <Rd>, <Rm>
bool TranslatorVisitor::arm_MOV_reg(Cond cond, bool S, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, carry_in);
    const auto result = shifted.result;
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(shifted.carry);
    }

    return true;
}

bool TranslatorVisitor::arm_MOV_rsr(Cond cond, bool S, Reg d, Reg s, ShiftType shift, Reg m) {
    if (d == Reg::PC || m == Reg::PC || s == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = shifted.result;
    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(shifted.carry);
    }

    return true;
}

// MVN{S}<c> <Rd>, #<const>
bool TranslatorVisitor::arm_MVN_imm(Cond cond, bool S, Reg d, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto imm_carry = ArmExpandImm_C(rotate, imm8, ir.GetCFlag());
    const auto result = ir.Not(ir.Imm32(imm_carry.imm32));
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }

    return true;
}

// MVN{S}<c> <Rd>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_MVN_reg(Cond cond, bool S, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, carry_in);
    const auto result = ir.Not(shifted.result);
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(shifted.carry);
    }

    return true;
}

// MVN{S}<c> <Rd>, <Rm>, <type> <Rs>
bool TranslatorVisitor::arm_MVN_rsr(Cond cond, bool S, Reg d, Reg s, ShiftType shift, Reg m) {
    if (d == Reg::PC || m == Reg::PC || s == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.Not(shifted.result);

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(shifted.carry);
    }

    return true;
}

// ORR{S}<c> <Rd>, <Rn>, #<const>
bool TranslatorVisitor::arm_ORR_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto imm_carry = ArmExpandImm_C(rotate, imm8, ir.GetCFlag());
    const auto result = ir.Or(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }

    return true;
}

// ORR{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_ORR_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, carry_in);
    const auto result = ir.Or(ir.GetRegister(n), shifted.result);
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(shifted.carry);
    }

    return true;
}

// ORR{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
bool TranslatorVisitor::arm_ORR_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
    if (n == Reg::PC || m == Reg::PC || s == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.Or(ir.GetRegister(n), shifted.result);

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(shifted.carry);
    }

    return true;
}

// RSB{S}<c> <Rd>, <Rn>, #<const>
bool TranslatorVisitor::arm_RSB_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = ArmExpandImm(rotate, imm8);
    const auto result = ir.SubWithCarry(ir.Imm32(imm32), ir.GetRegister(n), ir.Imm1(1));
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result.result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// RSB{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_RSB_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag());
    const auto result = ir.SubWithCarry(shifted.result, ir.GetRegister(n), ir.Imm1(1));
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result.result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// RSB{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
bool TranslatorVisitor::arm_RSB_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
    if (n == Reg::PC || m == Reg::PC || s == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.SubWithCarry(shifted.result, ir.GetRegister(n), ir.Imm1(1));

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// RSC{S}<c> <Rd>, <Rn>, #<const>
bool TranslatorVisitor::arm_RSC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = ArmExpandImm(rotate, imm8);
    const auto result = ir.SubWithCarry(ir.Imm32(imm32), ir.GetRegister(n), ir.GetCFlag());
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result.result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

bool TranslatorVisitor::arm_RSC_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag());
    const auto result = ir.SubWithCarry(shifted.result, ir.GetRegister(n), ir.GetCFlag());
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result.result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// RSC{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_RSC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
    if (n == Reg::PC || m == Reg::PC || s == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.SubWithCarry(shifted.result, ir.GetRegister(n), ir.GetCFlag());

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// SBC{S}<c> <Rd>, <Rn>, #<const>
bool TranslatorVisitor::arm_SBC_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = ArmExpandImm(rotate, imm8);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.GetCFlag());
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result.result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// SBC{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_SBC_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag());
    const auto result = ir.SubWithCarry(ir.GetRegister(n), shifted.result, ir.GetCFlag());
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result.result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// SBC{S}<c> <Rd>, <Rn>, <Rm>, <type> <Rs>
bool TranslatorVisitor::arm_SBC_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
    if (n == Reg::PC || m == Reg::PC || s == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), shifted.result, ir.GetCFlag());

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// SUB{S}<c> <Rd>, <Rn>, #<const>
bool TranslatorVisitor::arm_SUB_imm(Cond cond, bool S, Reg n, Reg d, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = ArmExpandImm(rotate, imm8);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(1));
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result.result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// SUB{S}<c> <Rd>, <Rn>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_SUB_reg(Cond cond, bool S, Reg n, Reg d, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag());
    const auto result = ir.SubWithCarry(ir.GetRegister(n), shifted.result, ir.Imm1(1));
    if (d == Reg::PC) {
        if (S) {
            // This is UNPREDICTABLE when in user-mode.
            return UnpredictableInstruction();
        }

        ir.ALUWritePC(result.result);
        ir.SetTerm(IR::Term::ReturnToDispatch{});
        return false;
    }

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

bool TranslatorVisitor::arm_SUB_rsr(Cond cond, bool S, Reg n, Reg d, Reg s, ShiftType shift, Reg m) {
    if (n == Reg::PC || m == Reg::PC || s == Reg::PC || d == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), shifted.result, ir.Imm1(1));

    ir.SetRegister(d, result.result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result.result));
        ir.SetZFlag(ir.IsZero(result.result));
        ir.SetCFlag(result.carry);
        ir.SetVFlag(result.overflow);
    }

    return true;
}

// TEQ<c> <Rn>, #<const>
bool TranslatorVisitor::arm_TEQ_imm(Cond cond, Reg n, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto imm_carry = ArmExpandImm_C(rotate, imm8, ir.GetCFlag());
    const auto result = ir.Eor(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));

    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(imm_carry.carry);
    return true;
}

// TEQ<c> <Rn>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_TEQ_reg(Cond cond, Reg n, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, carry_in);
    const auto result = ir.Eor(ir.GetRegister(n), shifted.result);

    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(shifted.carry);
    return true;
}

// TEQ<c> <Rn>, <Rm>, <type> <Rs>
bool TranslatorVisitor::arm_TEQ_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
    if (n == Reg::PC || m == Reg::PC || s == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.Eor(ir.GetRegister(n), shifted.result);

    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(shifted.carry);
    return true;
}

// TST<c> <Rn>, #<const>
bool TranslatorVisitor::arm_TST_imm(Cond cond, Reg n, int rotate, Imm<8> imm8) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto imm_carry = ArmExpandImm_C(rotate, imm8, ir.GetCFlag());
    const auto result = ir.And(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));

    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(imm_carry.carry);
    return true;
}

// TST<c> <Rn>, <Rm>{, <shift>}
bool TranslatorVisitor::arm_TST_reg(Cond cond, Reg n, Imm<5> imm5, ShiftType shift, Reg m) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitImmShift(ir.GetRegister(m), shift, imm5, carry_in);
    const auto result = ir.And(ir.GetRegister(n), shifted.result);

    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(shifted.carry);
    return true;
}

// TST<c> <Rn>, <Rm>, <type> <Rs>
bool TranslatorVisitor::arm_TST_rsr(Cond cond, Reg n, Reg s, ShiftType shift, Reg m) {
    if (n == Reg::PC || m == Reg::PC || s == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto shift_n = ir.LeastSignificantByte(ir.GetRegister(s));
    const auto carry_in = ir.GetCFlag();
    const auto shifted = EmitRegShift(ir.GetRegister(m), shift, shift_n, carry_in);
    const auto result = ir.And(ir.GetRegister(n), shifted.result);

    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(shifted.carry);
    return true;
}

}  // namespace Dynarmic::A32

/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {

bool TranslatorVisitor::thumb32_TST_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, ir.GetCFlag());
    const auto result = ir.And(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));

    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(imm_carry.carry);
    return true;
}

bool TranslatorVisitor::thumb32_AND_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    ASSERT_MSG(!(d == Reg::PC && S), "Decode error");
    if ((d == Reg::PC && !S) || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, ir.GetCFlag());
    const auto result = ir.And(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

bool TranslatorVisitor::thumb32_BIC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, ir.GetCFlag());
    const auto result = ir.And(ir.GetRegister(n), ir.Not(ir.Imm32(imm_carry.imm32)));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

bool TranslatorVisitor::thumb32_MOV_imm(Imm<1> i, bool S, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, ir.GetCFlag());
    const auto result = ir.Imm32(imm_carry.imm32);

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

bool TranslatorVisitor::thumb32_ORR_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    ASSERT_MSG(n != Reg::PC, "Decode error");
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, ir.GetCFlag());
    const auto result = ir.Or(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

bool TranslatorVisitor::thumb32_MVN_imm(Imm<1> i, bool S, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, ir.GetCFlag());
    const auto result = ir.Imm32(~imm_carry.imm32);

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

bool TranslatorVisitor::thumb32_ORN_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    ASSERT_MSG(n != Reg::PC, "Decode error");
    if (d == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, ir.GetCFlag());
    const auto result = ir.Or(ir.GetRegister(n), ir.Imm32(~imm_carry.imm32));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

bool TranslatorVisitor::thumb32_TEQ_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, ir.GetCFlag());
    const auto result = ir.Eor(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));

    ir.SetNFlag(ir.MostSignificantBit(result));
    ir.SetZFlag(ir.IsZero(result));
    ir.SetCFlag(imm_carry.carry);
    return true;
}

bool TranslatorVisitor::thumb32_EOR_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    ASSERT_MSG(!(d == Reg::PC && S), "Decode error");
    if ((d == Reg::PC && !S) || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm_carry = ThumbExpandImm_C(i, imm3, imm8, ir.GetCFlag());
    const auto result = ir.Eor(ir.GetRegister(n), ir.Imm32(imm_carry.imm32));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetNFlag(ir.MostSignificantBit(result));
        ir.SetZFlag(ir.IsZero(result));
        ir.SetCFlag(imm_carry.carry);
    }
    return true;
}

bool TranslatorVisitor::thumb32_CMN_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm32 = ThumbExpandImm(i, imm3, imm8);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(0));

    ir.SetCpsrNZCV(ir.NZCVFrom(result));
    return true;
}

bool TranslatorVisitor::thumb32_ADD_imm_1(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    ASSERT_MSG(!(d == Reg::PC && S), "Decode error");
    if ((d == Reg::PC && !S) || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm32 = ThumbExpandImm(i, imm3, imm8);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(0));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetCpsrNZCV(ir.NZCVFrom(result));
    }
    return true;
}

bool TranslatorVisitor::thumb32_ADC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm32 = ThumbExpandImm(i, imm3, imm8);
    const auto result = ir.AddWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.GetCFlag());

    ir.SetRegister(d, result);
    if (S) {
        ir.SetCpsrNZCV(ir.NZCVFrom(result));
    }
    return true;
}

bool TranslatorVisitor::thumb32_SBC_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm32 = ThumbExpandImm(i, imm3, imm8);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.GetCFlag());

    ir.SetRegister(d, result);
    if (S) {
        ir.SetCpsrNZCV(ir.NZCVFrom(result));
    }
    return true;
}

bool TranslatorVisitor::thumb32_CMP_imm(Imm<1> i, Reg n, Imm<3> imm3, Imm<8> imm8) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm32 = ThumbExpandImm(i, imm3, imm8);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(1));

    ir.SetCpsrNZCV(ir.NZCVFrom(result));
    return true;
}

bool TranslatorVisitor::thumb32_SUB_imm_1(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    ASSERT_MSG(!(d == Reg::PC && S), "Decode error");
    if ((d == Reg::PC && !S) || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm32 = ThumbExpandImm(i, imm3, imm8);
    const auto result = ir.SubWithCarry(ir.GetRegister(n), ir.Imm32(imm32), ir.Imm1(1));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetCpsrNZCV(ir.NZCVFrom(result));
    }
    return true;
}

bool TranslatorVisitor::thumb32_RSB_imm(Imm<1> i, bool S, Reg n, Imm<3> imm3, Reg d, Imm<8> imm8) {
    if (d == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto imm32 = ThumbExpandImm(i, imm3, imm8);
    const auto result = ir.SubWithCarry(ir.Imm32(imm32), ir.GetRegister(n), ir.Imm1(1));

    ir.SetRegister(d, result);
    if (S) {
        ir.SetCpsrNZCV(ir.NZCVFrom(result));
    }
    return true;
}

}  // namespace Dynarmic::A32

/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {

// SADD8<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SADD8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedAddS8(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

// SADD16<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SADD16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedAddS16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

// SASX<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SASX(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedAddSubS16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

// SSAX<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SSAX(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedSubAddS16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

// SSUB8<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SSUB8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedSubS8(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

// SSUB16<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SSUB16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedSubS16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

// UADD8<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UADD8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedAddU8(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

// UADD16<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UADD16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedAddU16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

// UASX<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UASX(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedAddSubU16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

// USAX<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_USAX(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedSubAddU16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

// USAD8<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_USAD8(Cond cond, Reg d, Reg m, Reg n) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedAbsDiffSumS8(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// USADA8<c> <Rd>, <Rn>, <Rm>, <Ra>
bool TranslatorVisitor::arm_USADA8(Cond cond, Reg d, Reg a, Reg m, Reg n) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto tmp = ir.PackedAbsDiffSumS8(ir.GetRegister(n), ir.GetRegister(m));
    const auto result = ir.AddWithCarry(ir.GetRegister(a), tmp, ir.Imm1(0));
    ir.SetRegister(d, result);
    return true;
}

// USUB8<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_USUB8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedSubU8(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

// USUB16<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_USUB16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedSubU16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

// Parallel Add/Subtract (Saturating) instructions

// QADD8<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_QADD8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedSaturatedAddS8(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// QADD16<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_QADD16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedSaturatedAddS16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// QSUB8<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_QSUB8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedSaturatedSubS8(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// QSUB16<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_QSUB16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedSaturatedSubS16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// UQADD8<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UQADD8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedSaturatedAddU8(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// UQADD16<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UQADD16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedSaturatedAddU16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// UQSUB8<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UQSUB8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedSaturatedSubU8(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// UQSUB16<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UQSUB16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedSaturatedSubU16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// Parallel Add/Subtract (Halving) instructions

// SHADD8<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SHADD8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedHalvingAddS8(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// SHADD16<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SHADD16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedHalvingAddS16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// SHASX<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SHASX(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedHalvingAddSubS16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// SHSAX<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SHSAX(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedHalvingSubAddS16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// SHSUB8<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SHSUB8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedHalvingSubS8(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// SHSUB16<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_SHSUB16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedHalvingSubS16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// UHADD8<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UHADD8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedHalvingAddU8(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// UHADD16<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UHADD16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedHalvingAddU16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// UHASX<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UHASX(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedHalvingAddSubU16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// UHSAX<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UHSAX(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedHalvingSubAddU16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// UHSUB8<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UHSUB8(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedHalvingSubU8(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

// UHSUB16<c> <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_UHSUB16(Cond cond, Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto result = ir.PackedHalvingSubU16(ir.GetRegister(n), ir.GetRegister(m));
    ir.SetRegister(d, result);
    return true;
}

}  // namespace Dynarmic::A32

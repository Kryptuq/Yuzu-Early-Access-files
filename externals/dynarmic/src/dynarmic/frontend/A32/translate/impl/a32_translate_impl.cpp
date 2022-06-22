/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"

#include <mcl/assert.hpp>

#include "dynarmic/interface/A32/config.h"

namespace Dynarmic::A32 {

bool TranslatorVisitor::ArmConditionPassed(Cond cond) {
    return IsConditionPassed(*this, cond);
}

bool TranslatorVisitor::ThumbConditionPassed() {
    const Cond cond = ir.current_location.IT().Cond();
    return IsConditionPassed(*this, cond);
}

bool TranslatorVisitor::VFPConditionPassed(Cond cond) {
    if (ir.current_location.TFlag()) {
        ASSERT(cond == Cond::AL);
        return true;
    }
    return ArmConditionPassed(cond);
}

bool TranslatorVisitor::InterpretThisInstruction() {
    ir.SetTerm(IR::Term::Interpret(ir.current_location));
    return false;
}

bool TranslatorVisitor::UnpredictableInstruction() {
    return RaiseException(Exception::UnpredictableInstruction);
}

bool TranslatorVisitor::UndefinedInstruction() {
    return RaiseException(Exception::UndefinedInstruction);
}

bool TranslatorVisitor::DecodeError() {
    return RaiseException(Exception::DecodeError);
}

bool TranslatorVisitor::RaiseException(Exception exception) {
    ir.UpdateUpperLocationDescriptor();
    ir.BranchWritePC(ir.Imm32(ir.current_location.PC() + static_cast<u32>(current_instruction_size)));
    ir.ExceptionRaised(exception);
    ir.SetTerm(IR::Term::CheckHalt{IR::Term::ReturnToDispatch{}});
    return false;
}

bool TranslatorVisitor::MemoryInstructionContinues() {
    if (options.check_halt_on_memory_access) {
        ir.SetTerm(IR::Term::LinkBlock{ir.current_location.AdvancePC(static_cast<s32>(current_instruction_size))});
        return false;
    }

    return true;
}

IR::UAny TranslatorVisitor::I(size_t bitsize, u64 value) {
    switch (bitsize) {
    case 8:
        return ir.Imm8(static_cast<u8>(value));
    case 16:
        return ir.Imm16(static_cast<u16>(value));
    case 32:
        return ir.Imm32(static_cast<u32>(value));
    case 64:
        return ir.Imm64(value);
    default:
        ASSERT_FALSE("Imm - get: Invalid bitsize");
    }
}

IR::ResultAndCarry<IR::U32> TranslatorVisitor::EmitImmShift(IR::U32 value, ShiftType type, Imm<3> imm3, Imm<2> imm2, IR::U1 carry_in) {
    return EmitImmShift(value, type, concatenate(imm3, imm2), carry_in);
}

IR::ResultAndCarry<IR::U32> TranslatorVisitor::EmitImmShift(IR::U32 value, ShiftType type, Imm<5> imm5, IR::U1 carry_in) {
    u8 imm5_value = imm5.ZeroExtend<u8>();
    switch (type) {
    case ShiftType::LSL:
        return ir.LogicalShiftLeft(value, ir.Imm8(imm5_value), carry_in);
    case ShiftType::LSR:
        imm5_value = imm5_value ? imm5_value : 32;
        return ir.LogicalShiftRight(value, ir.Imm8(imm5_value), carry_in);
    case ShiftType::ASR:
        imm5_value = imm5_value ? imm5_value : 32;
        return ir.ArithmeticShiftRight(value, ir.Imm8(imm5_value), carry_in);
    case ShiftType::ROR:
        if (imm5_value) {
            return ir.RotateRight(value, ir.Imm8(imm5_value), carry_in);
        } else {
            return ir.RotateRightExtended(value, carry_in);
        }
    }
    UNREACHABLE();
}

IR::ResultAndCarry<IR::U32> TranslatorVisitor::EmitRegShift(IR::U32 value, ShiftType type, IR::U8 amount, IR::U1 carry_in) {
    switch (type) {
    case ShiftType::LSL:
        return ir.LogicalShiftLeft(value, amount, carry_in);
    case ShiftType::LSR:
        return ir.LogicalShiftRight(value, amount, carry_in);
    case ShiftType::ASR:
        return ir.ArithmeticShiftRight(value, amount, carry_in);
    case ShiftType::ROR:
        return ir.RotateRight(value, amount, carry_in);
    }
    UNREACHABLE();
}

}  // namespace Dynarmic::A32

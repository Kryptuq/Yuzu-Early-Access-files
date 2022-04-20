/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <mcl/bit/bit_count.hpp>
#include <mcl/bit/bit_field.hpp>

#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"

namespace Dynarmic::A32 {

bool TranslatorVisitor::arm_LDRBT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

bool TranslatorVisitor::arm_LDRHT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

bool TranslatorVisitor::arm_LDRSBT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

bool TranslatorVisitor::arm_LDRSHT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

bool TranslatorVisitor::arm_LDRT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

bool TranslatorVisitor::arm_STRBT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

bool TranslatorVisitor::arm_STRHT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

bool TranslatorVisitor::arm_STRT() {
    // System instructions unimplemented
    return UndefinedInstruction();
}

static IR::U32 GetAddress(A32::IREmitter& ir, bool P, bool U, bool W, Reg n, IR::U32 offset) {
    const bool index = P;
    const bool add = U;
    const bool wback = !P || W;

    const IR::U32 offset_addr = add ? ir.Add(ir.GetRegister(n), offset) : ir.Sub(ir.GetRegister(n), offset);
    const IR::U32 address = index ? offset_addr : ir.GetRegister(n);

    if (wback) {
        ir.SetRegister(n, offset_addr);
    }

    return address;
}

// LDR <Rt>, [PC, #+/-<imm>]
bool TranslatorVisitor::arm_LDR_lit(Cond cond, bool U, Reg t, Imm<12> imm12) {
    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const bool add = U;
    const u32 base = ir.AlignPC(4);
    const u32 address = add ? (base + imm12.ZeroExtend()) : (base - imm12.ZeroExtend());
    const auto data = ir.ReadMemory32(ir.Imm32(address), IR::AccType::NORMAL);

    if (t == Reg::PC) {
        ir.LoadWritePC(data);
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    }

    ir.SetRegister(t, data);
    return true;
}

// LDR <Rt>, [<Rn>, #+/-<imm>]{!}
// LDR <Rt>, [<Rn>], #+/-<imm>
bool TranslatorVisitor::arm_LDR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = imm12.ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.ReadMemory32(address, IR::AccType::NORMAL);

    if (t == Reg::PC) {
        ir.LoadWritePC(data);

        if (!P && W && n == Reg::R13) {
            ir.SetTerm(IR::Term::PopRSBHint{});
        } else {
            ir.SetTerm(IR::Term::FastDispatchHint{});
        }

        return false;
    }

    ir.SetRegister(t, data);
    return true;
}

// LDR <Rt>, [<Rn>, #+/-<Rm>]{!}
// LDR <Rt>, [<Rn>], #+/-<Rm>
bool TranslatorVisitor::arm_LDR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto offset = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag()).result;
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.ReadMemory32(address, IR::AccType::NORMAL);

    if (t == Reg::PC) {
        ir.LoadWritePC(data);
        ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    }

    ir.SetRegister(t, data);
    return true;
}

// LDRB <Rt>, [PC, #+/-<imm>]
bool TranslatorVisitor::arm_LDRB_lit(Cond cond, bool U, Reg t, Imm<12> imm12) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = imm12.ZeroExtend();
    const bool add = U;
    const u32 base = ir.AlignPC(4);
    const u32 address = add ? (base + imm32) : (base - imm32);
    const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(ir.Imm32(address), IR::AccType::NORMAL));

    ir.SetRegister(t, data);
    return true;
}

// LDRB <Rt>, [<Rn>, #+/-<imm>]{!}
// LDRB <Rt>, [<Rn>], #+/-<imm>
bool TranslatorVisitor::arm_LDRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t) {
        return UnpredictableInstruction();
    }

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = imm12.ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(address, IR::AccType::NORMAL));

    ir.SetRegister(t, data);
    return true;
}

// LDRB <Rt>, [<Rn>, #+/-<Rm>]{!}
// LDRB <Rt>, [<Rn>], #+/-<Rm>
bool TranslatorVisitor::arm_LDRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto offset = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag()).result;
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.ZeroExtendByteToWord(ir.ReadMemory8(address, IR::AccType::NORMAL));

    ir.SetRegister(t, data);
    return true;
}

// LDRD <Rt>, <Rt2>, [PC, #+/-<imm>]
bool TranslatorVisitor::arm_LDRD_lit(Cond cond, bool U, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (RegNumber(t) % 2 == 1) {
        return UnpredictableInstruction();
    }

    if (t + 1 == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const Reg t2 = t + 1;
    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const bool add = U;

    const u32 base = ir.AlignPC(4);
    const u32 address = add ? (base + imm32) : (base - imm32);

    // NOTE: If alignment is exactly off by 4, each word is an atomic access.
    const IR::U64 data = ir.ReadMemory64(ir.Imm32(address), IR::AccType::ATOMIC);

    if (ir.current_location.EFlag()) {
        ir.SetRegister(t, ir.MostSignificantWord(data).result);
        ir.SetRegister(t2, ir.LeastSignificantWord(data));
    } else {
        ir.SetRegister(t, ir.LeastSignificantWord(data));
        ir.SetRegister(t2, ir.MostSignificantWord(data).result);
    }
    return true;
}

// LDRD <Rt>, [<Rn>, #+/-<imm>]{!}
// LDRD <Rt>, [<Rn>], #+/-<imm>
bool TranslatorVisitor::arm_LDRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (RegNumber(t) % 2 == 1) {
        return UnpredictableInstruction();
    }

    if (!P && W) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == t || n == t + 1)) {
        return UnpredictableInstruction();
    }

    if (t + 1 == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const Reg t2 = t + 1;
    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();

    const auto offset = ir.Imm32(imm32);
    const auto address = GetAddress(ir, P, U, W, n, offset);

    // NOTE: If alignment is exactly off by 4, each word is an atomic access.
    const IR::U64 data = ir.ReadMemory64(address, IR::AccType::ATOMIC);

    if (ir.current_location.EFlag()) {
        ir.SetRegister(t, ir.MostSignificantWord(data).result);
        ir.SetRegister(t2, ir.LeastSignificantWord(data));
    } else {
        ir.SetRegister(t, ir.LeastSignificantWord(data));
        ir.SetRegister(t2, ir.MostSignificantWord(data).result);
    }
    return true;
}

// LDRD <Rt>, [<Rn>, #+/-<Rm>]{!}
// LDRD <Rt>, [<Rn>], #+/-<Rm>
bool TranslatorVisitor::arm_LDRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    if (RegNumber(t) % 2 == 1) {
        return UnpredictableInstruction();
    }

    if (!P && W) {
        return UnpredictableInstruction();
    }

    if (t + 1 == Reg::PC || m == Reg::PC || m == t || m == t + 1) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t || n == t + 1)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const Reg t2 = t + 1;
    const auto offset = ir.GetRegister(m);
    const auto address = GetAddress(ir, P, U, W, n, offset);

    // NOTE: If alignment is exactly off by 4, each word is an atomic access.
    const IR::U64 data = ir.ReadMemory64(address, IR::AccType::ATOMIC);

    if (ir.current_location.EFlag()) {
        ir.SetRegister(t, ir.MostSignificantWord(data).result);
        ir.SetRegister(t2, ir.LeastSignificantWord(data));
    } else {
        ir.SetRegister(t, ir.LeastSignificantWord(data));
        ir.SetRegister(t2, ir.MostSignificantWord(data).result);
    }
    return true;
}

// LDRH <Rt>, [PC, #-/+<imm>]
bool TranslatorVisitor::arm_LDRH_lit(Cond cond, bool P, bool U, bool W, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (P == W) {
        return UnpredictableInstruction();
    }

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const bool add = U;
    const u32 base = ir.AlignPC(4);
    const u32 address = add ? (base + imm32) : (base - imm32);
    const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(ir.Imm32(address), IR::AccType::NORMAL));

    ir.SetRegister(t, data);
    return true;
}

// LDRH <Rt>, [<Rn>, #+/-<imm>]{!}
// LDRH <Rt>, [<Rn>], #+/-<imm>
bool TranslatorVisitor::arm_LDRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t) {
        return UnpredictableInstruction();
    }

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(address, IR::AccType::NORMAL));

    ir.SetRegister(t, data);
    return true;
}

// LDRH <Rt>, [<Rn>, #+/-<Rm>]{!}
// LDRH <Rt>, [<Rn>], #+/-<Rm>
bool TranslatorVisitor::arm_LDRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto offset = ir.GetRegister(m);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.ZeroExtendHalfToWord(ir.ReadMemory16(address, IR::AccType::NORMAL));

    ir.SetRegister(t, data);
    return true;
}

// LDRSB <Rt>, [PC, #+/-<imm>]
bool TranslatorVisitor::arm_LDRSB_lit(Cond cond, bool U, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const bool add = U;

    const u32 base = ir.AlignPC(4);
    const u32 address = add ? (base + imm32) : (base - imm32);
    const auto data = ir.SignExtendByteToWord(ir.ReadMemory8(ir.Imm32(address), IR::AccType::NORMAL));

    ir.SetRegister(t, data);
    return true;
}

// LDRSB <Rt>, [<Rn>, #+/-<imm>]{!}
// LDRSB <Rt>, [<Rn>], #+/-<imm>
bool TranslatorVisitor::arm_LDRSB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t) {
        return UnpredictableInstruction();
    }

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.SignExtendByteToWord(ir.ReadMemory8(address, IR::AccType::NORMAL));

    ir.SetRegister(t, data);
    return true;
}

// LDRSB <Rt>, [<Rn>, #+/-<Rm>]{!}
// LDRSB <Rt>, [<Rn>], #+/-<Rm>
bool TranslatorVisitor::arm_LDRSB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto offset = ir.GetRegister(m);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.SignExtendByteToWord(ir.ReadMemory8(address, IR::AccType::NORMAL));

    ir.SetRegister(t, data);
    return true;
}

// LDRSH <Rt>, [PC, #-/+<imm>]
bool TranslatorVisitor::arm_LDRSH_lit(Cond cond, bool U, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const bool add = U;
    const u32 base = ir.AlignPC(4);
    const u32 address = add ? (base + imm32) : (base - imm32);
    const auto data = ir.SignExtendHalfToWord(ir.ReadMemory16(ir.Imm32(address), IR::AccType::NORMAL));

    ir.SetRegister(t, data);
    return true;
}

// LDRSH <Rt>, [<Rn>, #+/-<imm>]{!}
// LDRSH <Rt>, [<Rn>], #+/-<imm>
bool TranslatorVisitor::arm_LDRSH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (n == Reg::PC) {
        return UnpredictableInstruction();
    }

    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if ((!P || W) && n == t) {
        return UnpredictableInstruction();
    }

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.SignExtendHalfToWord(ir.ReadMemory16(address, IR::AccType::NORMAL));

    ir.SetRegister(t, data);
    return true;
}

// LDRSH <Rt>, [<Rn>, #+/-<Rm>]{!}
// LDRSH <Rt>, [<Rn>], #+/-<Rm>
bool TranslatorVisitor::arm_LDRSH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    ASSERT_MSG(!(!P && W), "T form of instruction unimplemented");
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto offset = ir.GetRegister(m);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto data = ir.SignExtendHalfToWord(ir.ReadMemory16(address, IR::AccType::NORMAL));

    ir.SetRegister(t, data);
    return true;
}

// STR <Rt>, [<Rn>, #+/-<imm>]{!}
// STR <Rt>, [<Rn>], #+/-<imm>
bool TranslatorVisitor::arm_STR_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12) {
    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto offset = ir.Imm32(imm12.ZeroExtend());
    const auto address = GetAddress(ir, P, U, W, n, offset);
    ir.WriteMemory32(address, ir.GetRegister(t), IR::AccType::NORMAL);
    return true;
}

// STR <Rt>, [<Rn>, #+/-<Rm>]{!}
// STR <Rt>, [<Rn>], #+/-<Rm>
bool TranslatorVisitor::arm_STR_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m) {
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto offset = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag()).result;
    const auto address = GetAddress(ir, P, U, W, n, offset);
    ir.WriteMemory32(address, ir.GetRegister(t), IR::AccType::NORMAL);
    return true;
}

// STRB <Rt>, [<Rn>, #+/-<imm>]{!}
// STRB <Rt>, [<Rn>], #+/-<imm>
bool TranslatorVisitor::arm_STRB_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<12> imm12) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto offset = ir.Imm32(imm12.ZeroExtend());
    const auto address = GetAddress(ir, P, U, W, n, offset);
    ir.WriteMemory8(address, ir.LeastSignificantByte(ir.GetRegister(t)), IR::AccType::NORMAL);
    return true;
}

// STRB <Rt>, [<Rn>, #+/-<Rm>]{!}
// STRB <Rt>, [<Rn>], #+/-<Rm>
bool TranslatorVisitor::arm_STRB_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<5> imm5, ShiftType shift, Reg m) {
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto offset = EmitImmShift(ir.GetRegister(m), shift, imm5, ir.GetCFlag()).result;
    const auto address = GetAddress(ir, P, U, W, n, offset);
    ir.WriteMemory8(address, ir.LeastSignificantByte(ir.GetRegister(t)), IR::AccType::NORMAL);
    return true;
}

// STRD <Rt>, [<Rn>, #+/-<imm>]{!}
// STRD <Rt>, [<Rn>], #+/-<imm>
bool TranslatorVisitor::arm_STRD_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (size_t(t) % 2 != 0) {
        return UnpredictableInstruction();
    }

    if (!P && W) {
        return UnpredictableInstruction();
    }

    const Reg t2 = t + 1;
    if ((!P || W) && (n == Reg::PC || n == t || n == t2)) {
        return UnpredictableInstruction();
    }

    if (t2 == Reg::PC) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto value_a = ir.GetRegister(t);
    const auto value_b = ir.GetRegister(t2);

    const IR::U64 data = ir.current_location.EFlag() ? ir.Pack2x32To1x64(value_b, value_a)
                                                     : ir.Pack2x32To1x64(value_a, value_b);

    // NOTE: If alignment is exactly off by 4, each word is an atomic access.
    ir.WriteMemory64(address, data, IR::AccType::ATOMIC);
    return true;
}

// STRD <Rt>, [<Rn>, #+/-<Rm>]{!}
// STRD <Rt>, [<Rn>], #+/-<Rm>
bool TranslatorVisitor::arm_STRD_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    if (size_t(t) % 2 != 0) {
        return UnpredictableInstruction();
    }

    if (!P && W) {
        return UnpredictableInstruction();
    }

    const Reg t2 = t + 1;
    if (t2 == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t || n == t2)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto offset = ir.GetRegister(m);
    const auto address = GetAddress(ir, P, U, W, n, offset);
    const auto value_a = ir.GetRegister(t);
    const auto value_b = ir.GetRegister(t2);

    const IR::U64 data = ir.current_location.EFlag() ? ir.Pack2x32To1x64(value_b, value_a)
                                                     : ir.Pack2x32To1x64(value_a, value_b);

    // NOTE: If alignment is exactly off by 4, each word is an atomic access.
    ir.WriteMemory64(address, data, IR::AccType::ATOMIC);
    return true;
}

// STRH <Rt>, [<Rn>, #+/-<imm>]{!}
// STRH <Rt>, [<Rn>], #+/-<imm>
bool TranslatorVisitor::arm_STRH_imm(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Imm<4> imm8a, Imm<4> imm8b) {
    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const u32 imm32 = concatenate(imm8a, imm8b).ZeroExtend();
    const auto offset = ir.Imm32(imm32);
    const auto address = GetAddress(ir, P, U, W, n, offset);

    ir.WriteMemory16(address, ir.LeastSignificantHalf(ir.GetRegister(t)), IR::AccType::NORMAL);
    return true;
}

// STRH <Rt>, [<Rn>, #+/-<Rm>]{!}
// STRH <Rt>, [<Rn>], #+/-<Rm>
bool TranslatorVisitor::arm_STRH_reg(Cond cond, bool P, bool U, bool W, Reg n, Reg t, Reg m) {
    if (t == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    if ((!P || W) && (n == Reg::PC || n == t)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto offset = ir.GetRegister(m);
    const auto address = GetAddress(ir, P, U, W, n, offset);

    ir.WriteMemory16(address, ir.LeastSignificantHalf(ir.GetRegister(t)), IR::AccType::NORMAL);
    return true;
}

static bool LDMHelper(A32::IREmitter& ir, bool W, Reg n, RegList list, IR::U32 start_address, IR::U32 writeback_address) {
    auto address = start_address;
    for (size_t i = 0; i <= 14; i++) {
        if (mcl::bit::get_bit(i, list)) {
            ir.SetRegister(static_cast<Reg>(i), ir.ReadMemory32(address, IR::AccType::ATOMIC));
            address = ir.Add(address, ir.Imm32(4));
        }
    }
    if (W && !mcl::bit::get_bit(RegNumber(n), list)) {
        ir.SetRegister(n, writeback_address);
    }
    if (mcl::bit::get_bit<15>(list)) {
        ir.LoadWritePC(ir.ReadMemory32(address, IR::AccType::ATOMIC));
        if (n == Reg::R13)
            ir.SetTerm(IR::Term::PopRSBHint{});
        else
            ir.SetTerm(IR::Term::FastDispatchHint{});
        return false;
    }
    return true;
}

// LDM <Rn>{!}, <reg_list>
bool TranslatorVisitor::arm_LDM(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || mcl::bit::count_ones(list) < 1) {
        return UnpredictableInstruction();
    }
    if (W && mcl::bit::get_bit(static_cast<size_t>(n), list)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.GetRegister(n);
    const auto writeback_address = ir.Add(start_address, ir.Imm32(u32(mcl::bit::count_ones(list) * 4)));
    return LDMHelper(ir, W, n, list, start_address, writeback_address);
}

// LDMDA <Rn>{!}, <reg_list>
bool TranslatorVisitor::arm_LDMDA(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || mcl::bit::count_ones(list) < 1) {
        return UnpredictableInstruction();
    }
    if (W && mcl::bit::get_bit(static_cast<size_t>(n), list)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(u32(4 * mcl::bit::count_ones(list) - 4)));
    const auto writeback_address = ir.Sub(start_address, ir.Imm32(4));
    return LDMHelper(ir, W, n, list, start_address, writeback_address);
}

// LDMDB <Rn>{!}, <reg_list>
bool TranslatorVisitor::arm_LDMDB(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || mcl::bit::count_ones(list) < 1) {
        return UnpredictableInstruction();
    }
    if (W && mcl::bit::get_bit(static_cast<size_t>(n), list)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(u32(4 * mcl::bit::count_ones(list))));
    const auto writeback_address = start_address;
    return LDMHelper(ir, W, n, list, start_address, writeback_address);
}

// LDMIB <Rn>{!}, <reg_list>
bool TranslatorVisitor::arm_LDMIB(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || mcl::bit::count_ones(list) < 1) {
        return UnpredictableInstruction();
    }
    if (W && mcl::bit::get_bit(static_cast<size_t>(n), list)) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.Add(ir.GetRegister(n), ir.Imm32(4));
    const auto writeback_address = ir.Add(ir.GetRegister(n), ir.Imm32(u32(4 * mcl::bit::count_ones(list))));
    return LDMHelper(ir, W, n, list, start_address, writeback_address);
}

bool TranslatorVisitor::arm_LDM_usr() {
    return InterpretThisInstruction();
}

bool TranslatorVisitor::arm_LDM_eret() {
    return InterpretThisInstruction();
}

static bool STMHelper(A32::IREmitter& ir, bool W, Reg n, RegList list, IR::U32 start_address, IR::U32 writeback_address) {
    auto address = start_address;
    for (size_t i = 0; i <= 14; i++) {
        if (mcl::bit::get_bit(i, list)) {
            ir.WriteMemory32(address, ir.GetRegister(static_cast<Reg>(i)), IR::AccType::ATOMIC);
            address = ir.Add(address, ir.Imm32(4));
        }
    }
    if (W) {
        ir.SetRegister(n, writeback_address);
    }
    if (mcl::bit::get_bit<15>(list)) {
        ir.WriteMemory32(address, ir.Imm32(ir.PC()), IR::AccType::ATOMIC);
    }
    return true;
}

// STM <Rn>{!}, <reg_list>
bool TranslatorVisitor::arm_STM(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || mcl::bit::count_ones(list) < 1) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.GetRegister(n);
    const auto writeback_address = ir.Add(start_address, ir.Imm32(u32(mcl::bit::count_ones(list) * 4)));
    return STMHelper(ir, W, n, list, start_address, writeback_address);
}

// STMDA <Rn>{!}, <reg_list>
bool TranslatorVisitor::arm_STMDA(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || mcl::bit::count_ones(list) < 1) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(u32(4 * mcl::bit::count_ones(list) - 4)));
    const auto writeback_address = ir.Sub(start_address, ir.Imm32(4));
    return STMHelper(ir, W, n, list, start_address, writeback_address);
}

// STMDB <Rn>{!}, <reg_list>
bool TranslatorVisitor::arm_STMDB(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || mcl::bit::count_ones(list) < 1) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.Sub(ir.GetRegister(n), ir.Imm32(u32(4 * mcl::bit::count_ones(list))));
    const auto writeback_address = start_address;
    return STMHelper(ir, W, n, list, start_address, writeback_address);
}

// STMIB <Rn>{!}, <reg_list>
bool TranslatorVisitor::arm_STMIB(Cond cond, bool W, Reg n, RegList list) {
    if (n == Reg::PC || mcl::bit::count_ones(list) < 1) {
        return UnpredictableInstruction();
    }

    if (!ArmConditionPassed(cond)) {
        return true;
    }

    const auto start_address = ir.Add(ir.GetRegister(n), ir.Imm32(4));
    const auto writeback_address = ir.Add(ir.GetRegister(n), ir.Imm32(u32(4 * mcl::bit::count_ones(list))));
    return STMHelper(ir, W, n, list, start_address, writeback_address);
}

bool TranslatorVisitor::arm_STM_usr() {
    return InterpretThisInstruction();
}

}  // namespace Dynarmic::A32

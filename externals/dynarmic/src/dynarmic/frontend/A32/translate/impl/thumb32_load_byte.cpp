/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"
#include "dynarmic/interface/A32/config.h"

namespace Dynarmic::A32 {
static bool PLDHandler(TranslatorVisitor& v, bool W) {
    if (!v.options.hook_hint_instructions) {
        return true;
    }

    const auto exception = W ? Exception::PreloadDataWithIntentToWrite
                             : Exception::PreloadData;
    return v.RaiseException(exception);
}

static bool PLIHandler(TranslatorVisitor& v) {
    if (!v.options.hook_hint_instructions) {
        return true;
    }

    return v.RaiseException(Exception::PreloadInstruction);
}

using ExtensionFunction = IR::U32 (IREmitter::*)(const IR::U8&);

static bool LoadByteLiteral(TranslatorVisitor& v, bool U, Reg t, Imm<12> imm12, ExtensionFunction ext_fn) {
    const u32 imm32 = imm12.ZeroExtend();
    const u32 base = v.ir.AlignPC(4);
    const u32 address = U ? (base + imm32) : (base - imm32);
    const auto data = (v.ir.*ext_fn)(v.ir.ReadMemory8(v.ir.Imm32(address)));

    v.ir.SetRegister(t, data);
    return true;
}

static bool LoadByteRegister(TranslatorVisitor& v, Reg n, Reg t, Imm<2> imm2, Reg m, ExtensionFunction ext_fn) {
    if (m == Reg::PC) {
        return v.UnpredictableInstruction();
    }

    const auto reg_n = v.ir.GetRegister(n);
    const auto reg_m = v.ir.GetRegister(m);
    const auto offset = v.ir.LogicalShiftLeft(reg_m, v.ir.Imm8(imm2.ZeroExtend<u8>()));
    const auto address = v.ir.Add(reg_n, offset);
    const auto data = (v.ir.*ext_fn)(v.ir.ReadMemory8(address));

    v.ir.SetRegister(t, data);
    return true;
}

static bool LoadByteImmediate(TranslatorVisitor& v, Reg n, Reg t, bool P, bool U, bool W, Imm<12> imm12, ExtensionFunction ext_fn) {
    const u32 imm32 = imm12.ZeroExtend();
    const IR::U32 reg_n = v.ir.GetRegister(n);
    const IR::U32 offset_address = U ? v.ir.Add(reg_n, v.ir.Imm32(imm32))
                                     : v.ir.Sub(reg_n, v.ir.Imm32(imm32));
    const IR::U32 address = P ? offset_address : reg_n;
    const IR::U32 data = (v.ir.*ext_fn)(v.ir.ReadMemory8(address));

    v.ir.SetRegister(t, data);
    if (W) {
        v.ir.SetRegister(n, offset_address);
    }
    return true;
}

bool TranslatorVisitor::thumb32_PLD_lit(bool /*U*/, Imm<12> /*imm12*/) {
    return PLDHandler(*this, false);
}

bool TranslatorVisitor::thumb32_PLD_imm8(bool W, Reg /*n*/, Imm<8> /*imm8*/) {
    return PLDHandler(*this, W);
}

bool TranslatorVisitor::thumb32_PLD_imm12(bool W, Reg /*n*/, Imm<12> /*imm12*/) {
    return PLDHandler(*this, W);
}

bool TranslatorVisitor::thumb32_PLD_reg(bool W, Reg /*n*/, Imm<2> /*imm2*/, Reg m) {
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }

    return PLDHandler(*this, W);
}

bool TranslatorVisitor::thumb32_PLI_lit(bool /*U*/, Imm<12> /*imm12*/) {
    return PLIHandler(*this);
}

bool TranslatorVisitor::thumb32_PLI_imm8(Reg /*n*/, Imm<8> /*imm8*/) {
    return PLIHandler(*this);
}

bool TranslatorVisitor::thumb32_PLI_imm12(Reg /*n*/, Imm<12> /*imm12*/) {
    return PLIHandler(*this);
}

bool TranslatorVisitor::thumb32_PLI_reg(Reg /*n*/, Imm<2> /*imm2*/, Reg m) {
    if (m == Reg::PC) {
        return UnpredictableInstruction();
    }

    return PLIHandler(*this);
}

bool TranslatorVisitor::thumb32_LDRB_lit(bool U, Reg t, Imm<12> imm12) {
    return LoadByteLiteral(*this, U, t, imm12,
                           &IREmitter::ZeroExtendByteToWord);
}

bool TranslatorVisitor::thumb32_LDRB_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (t == Reg::PC && W) {
        return UnpredictableInstruction();
    }
    if (W && n == t) {
        return UnpredictableInstruction();
    }
    if (!P && !W) {
        return UndefinedInstruction();
    }

    return LoadByteImmediate(*this, n, t, P, U, W, Imm<12>{imm8.ZeroExtend()},
                             &IREmitter::ZeroExtendByteToWord);
}

bool TranslatorVisitor::thumb32_LDRB_imm12(Reg n, Reg t, Imm<12> imm12) {
    return LoadByteImmediate(*this, n, t, true, true, false, imm12,
                             &IREmitter::ZeroExtendByteToWord);
}

bool TranslatorVisitor::thumb32_LDRB_reg(Reg n, Reg t, Imm<2> imm2, Reg m) {
    return LoadByteRegister(*this, n, t, imm2, m,
                            &IREmitter::ZeroExtendByteToWord);
}

bool TranslatorVisitor::thumb32_LDRBT(Reg n, Reg t, Imm<8> imm8) {
    // TODO: Add an unpredictable instruction path if this
    //       is executed in hypervisor mode if we ever support
    //       privileged execution modes.

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    // Treat it as a normal LDRB, given we don't support
    // execution levels other than EL0 currently.
    return thumb32_LDRB_imm8(n, t, true, true, false, imm8);
}

bool TranslatorVisitor::thumb32_LDRSB_lit(bool U, Reg t, Imm<12> imm12) {
    return LoadByteLiteral(*this, U, t, imm12,
                           &IREmitter::SignExtendByteToWord);
}

bool TranslatorVisitor::thumb32_LDRSB_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (t == Reg::PC && W) {
        return UnpredictableInstruction();
    }
    if (W && n == t) {
        return UnpredictableInstruction();
    }
    if (!P && !W) {
        return UndefinedInstruction();
    }

    return LoadByteImmediate(*this, n, t, P, U, W, Imm<12>{imm8.ZeroExtend()},
                             &IREmitter::SignExtendByteToWord);
}

bool TranslatorVisitor::thumb32_LDRSB_imm12(Reg n, Reg t, Imm<12> imm12) {
    return LoadByteImmediate(*this, n, t, true, true, false, imm12,
                             &IREmitter::SignExtendByteToWord);
}

bool TranslatorVisitor::thumb32_LDRSB_reg(Reg n, Reg t, Imm<2> imm2, Reg m) {
    return LoadByteRegister(*this, n, t, imm2, m,
                            &IREmitter::SignExtendByteToWord);
}

bool TranslatorVisitor::thumb32_LDRSBT(Reg n, Reg t, Imm<8> imm8) {
    // TODO: Add an unpredictable instruction path if this
    //       is executed in hypervisor mode if we ever support
    //       privileged execution modes.

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    // Treat it as a normal LDRSB, given we don't support
    // execution levels other than EL0 currently.
    return thumb32_LDRSB_imm8(n, t, true, true, false, imm8);
}

}  // namespace Dynarmic::A32

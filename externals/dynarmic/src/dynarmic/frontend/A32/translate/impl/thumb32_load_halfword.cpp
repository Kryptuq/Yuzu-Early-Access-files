/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"

namespace Dynarmic::A32 {

using ExtensionFunction = IR::U32 (IREmitter::*)(const IR::U16&);

static bool LoadHalfLiteral(TranslatorVisitor& v, bool U, Reg t, Imm<12> imm12, ExtensionFunction ext_fn) {
    const auto imm32 = imm12.ZeroExtend();
    const auto base = v.ir.AlignPC(4);
    const auto address = U ? (base + imm32) : (base - imm32);
    const auto data = (v.ir.*ext_fn)(v.ir.ReadMemory16(v.ir.Imm32(address), IR::AccType::NORMAL));

    v.ir.SetRegister(t, data);
    return true;
}

static bool LoadHalfRegister(TranslatorVisitor& v, Reg n, Reg t, Imm<2> imm2, Reg m, ExtensionFunction ext_fn) {
    if (m == Reg::PC) {
        return v.UnpredictableInstruction();
    }

    const IR::U32 reg_m = v.ir.GetRegister(m);
    const IR::U32 reg_n = v.ir.GetRegister(n);
    const IR::U32 offset = v.ir.LogicalShiftLeft(reg_m, v.ir.Imm8(imm2.ZeroExtend<u8>()));
    const IR::U32 address = v.ir.Add(reg_n, offset);
    const IR::U32 data = (v.ir.*ext_fn)(v.ir.ReadMemory16(address, IR::AccType::NORMAL));

    v.ir.SetRegister(t, data);
    return true;
}

static bool LoadHalfImmediate(TranslatorVisitor& v, Reg n, Reg t, bool P, bool U, bool W, Imm<12> imm12, ExtensionFunction ext_fn) {
    const u32 imm32 = imm12.ZeroExtend();
    const IR::U32 reg_n = v.ir.GetRegister(n);
    const IR::U32 offset_address = U ? v.ir.Add(reg_n, v.ir.Imm32(imm32))
                                     : v.ir.Sub(reg_n, v.ir.Imm32(imm32));
    const IR::U32 address = P ? offset_address
                              : reg_n;
    const IR::U32 data = (v.ir.*ext_fn)(v.ir.ReadMemory16(address, IR::AccType::NORMAL));

    if (W) {
        v.ir.SetRegister(n, offset_address);
    }

    v.ir.SetRegister(t, data);
    return true;
}

bool TranslatorVisitor::thumb32_LDRH_lit(bool U, Reg t, Imm<12> imm12) {
    return LoadHalfLiteral(*this, U, t, imm12, &IREmitter::ZeroExtendHalfToWord);
}

bool TranslatorVisitor::thumb32_LDRH_reg(Reg n, Reg t, Imm<2> imm2, Reg m) {
    return LoadHalfRegister(*this, n, t, imm2, m, &IREmitter::ZeroExtendHalfToWord);
}

bool TranslatorVisitor::thumb32_LDRH_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (!P && !W) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC && W) {
        return UnpredictableInstruction();
    }
    if (W && n == t) {
        return UnpredictableInstruction();
    }

    return LoadHalfImmediate(*this, n, t, P, U, W, Imm<12>{imm8.ZeroExtend()},
                             &IREmitter::ZeroExtendHalfToWord);
}

bool TranslatorVisitor::thumb32_LDRH_imm12(Reg n, Reg t, Imm<12> imm12) {
    return LoadHalfImmediate(*this, n, t, true, true, false, imm12,
                             &IREmitter::ZeroExtendHalfToWord);
}

bool TranslatorVisitor::thumb32_LDRHT(Reg n, Reg t, Imm<8> imm8) {
    // TODO: Add an unpredictable instruction path if this
    //       is executed in hypervisor mode if we ever support
    //       privileged execution levels.

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    // Treat it as a normal LDRH, given we don't support
    // execution levels other than EL0 currently.
    return thumb32_LDRH_imm8(n, t, true, true, false, imm8);
}

bool TranslatorVisitor::thumb32_LDRSH_lit(bool U, Reg t, Imm<12> imm12) {
    return LoadHalfLiteral(*this, U, t, imm12, &IREmitter::SignExtendHalfToWord);
}

bool TranslatorVisitor::thumb32_LDRSH_reg(Reg n, Reg t, Imm<2> imm2, Reg m) {
    return LoadHalfRegister(*this, n, t, imm2, m, &IREmitter::SignExtendHalfToWord);
}

bool TranslatorVisitor::thumb32_LDRSH_imm8(Reg n, Reg t, bool P, bool U, bool W, Imm<8> imm8) {
    if (!P && !W) {
        return UndefinedInstruction();
    }
    if (t == Reg::PC && W) {
        return UnpredictableInstruction();
    }
    if (W && n == t) {
        return UnpredictableInstruction();
    }

    return LoadHalfImmediate(*this, n, t, P, U, W, Imm<12>{imm8.ZeroExtend()},
                             &IREmitter::SignExtendHalfToWord);
}

bool TranslatorVisitor::thumb32_LDRSH_imm12(Reg n, Reg t, Imm<12> imm12) {
    return LoadHalfImmediate(*this, n, t, true, true, false, imm12,
                             &IREmitter::SignExtendHalfToWord);
}

bool TranslatorVisitor::thumb32_LDRSHT(Reg n, Reg t, Imm<8> imm8) {
    // TODO: Add an unpredictable instruction path if this
    //       is executed in hypervisor mode if we ever support
    //       privileged execution levels.

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    // Treat it as a normal LDRSH, given we don't support
    // execution levels other than EL0 currently.
    return thumb32_LDRSH_imm8(n, t, true, true, false, imm8);
}

}  // namespace Dynarmic::A32

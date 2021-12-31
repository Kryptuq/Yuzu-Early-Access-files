/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"

namespace Dynarmic::A32 {

bool TranslatorVisitor::thumb32_MCRR(bool two, Reg t2, Reg t, size_t coproc_no, size_t opc, CoprocReg CRm) {
    ir.CoprocSendTwoWords(coproc_no, two, opc, CRm, ir.GetRegister(t), ir.GetRegister(t2));
    return true;
}

bool TranslatorVisitor::thumb32_MRRC(bool two, Reg t2, Reg t, size_t coproc_no, size_t opc, CoprocReg CRm) {
    const auto two_words = ir.CoprocGetTwoWords(coproc_no, two, opc, CRm);
    ir.SetRegister(t, ir.LeastSignificantWord(two_words));
    ir.SetRegister(t2, ir.MostSignificantWord(two_words).result);
    return true;
}

bool TranslatorVisitor::thumb32_STC(bool two, bool p, bool u, bool d, bool w, Reg n, CoprocReg CRd, size_t coproc_no, Imm<8> imm8) {
    const u32 imm32 = imm8.ZeroExtend() << 2;
    const bool index = p;
    const bool add = u;
    const bool wback = w;
    const bool has_option = !p && !w && u;
    const IR::U32 reg_n = ir.GetRegister(n);
    const IR::U32 offset_address = add ? ir.Add(reg_n, ir.Imm32(imm32)) : ir.Sub(reg_n, ir.Imm32(imm32));
    const IR::U32 address = index ? offset_address : reg_n;
    ir.CoprocStoreWords(coproc_no, two, d, CRd, address, has_option, imm8.ZeroExtend<u8>());
    if (wback) {
        ir.SetRegister(n, offset_address);
    }
    return true;
}

bool TranslatorVisitor::thumb32_LDC(bool two, bool p, bool u, bool d, bool w, Reg n, CoprocReg CRd, size_t coproc_no, Imm<8> imm8) {
    const u32 imm32 = imm8.ZeroExtend() << 2;
    const bool index = p;
    const bool add = u;
    const bool wback = w;
    const bool has_option = !p && !w && u;
    const IR::U32 reg_n = ir.GetRegister(n);
    const IR::U32 offset_address = add ? ir.Add(reg_n, ir.Imm32(imm32)) : ir.Sub(reg_n, ir.Imm32(imm32));
    const IR::U32 address = index ? offset_address : reg_n;
    ir.CoprocLoadWords(coproc_no, two, d, CRd, address, has_option, imm8.ZeroExtend<u8>());
    if (wback) {
        ir.SetRegister(n, offset_address);
    }
    return true;
}

bool TranslatorVisitor::thumb32_CDP(bool two, size_t opc1, CoprocReg CRn, CoprocReg CRd, size_t coproc_no, size_t opc2, CoprocReg CRm) {
    ir.CoprocInternalOperation(coproc_no, two, opc1, CRd, CRn, CRm, opc2);
    return true;
}

bool TranslatorVisitor::thumb32_MCR(bool two, size_t opc1, CoprocReg CRn, Reg t, size_t coproc_no, size_t opc2, CoprocReg CRm) {
    ir.CoprocSendOneWord(coproc_no, two, opc1, CRn, CRm, opc2, ir.GetRegister(t));
    return true;
}

bool TranslatorVisitor::thumb32_MRC(bool two, size_t opc1, CoprocReg CRn, Reg t, size_t coproc_no, size_t opc2, CoprocReg CRm) {
    const auto word = ir.CoprocGetOneWord(coproc_no, two, opc1, CRn, CRm, opc2);
    if (t != Reg::PC) {
        ir.SetRegister(t, word);
    } else {
        const auto new_cpsr_nzcv = ir.And(word, ir.Imm32(0xF0000000));
        ir.SetCpsrNZCVRaw(new_cpsr_nzcv);
    }
    return true;
}

}  // namespace Dynarmic::A32

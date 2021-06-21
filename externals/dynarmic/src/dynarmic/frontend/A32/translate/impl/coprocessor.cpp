/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {

// CDP{2} <coproc_no>, #<opc1>, <CRd>, <CRn>, <CRm>, #<opc2>
bool TranslatorVisitor::arm_CDP(Cond cond, size_t opc1, CoprocReg CRn, CoprocReg CRd, size_t coproc_no, size_t opc2, CoprocReg CRm) {
    if ((coproc_no & 0b1110) == 0b1010) {
        return arm_UDF();
    }

    const bool two = cond == Cond::NV;

    if (two || ArmConditionPassed(cond)) {
        ir.CoprocInternalOperation(coproc_no, two, opc1, CRd, CRn, CRm, opc2);
    }
    return true;
}

// LDC{2}{L}<c> <coproc_no>, <CRd>, [<Rn>, #+/-<imm32>]{!}
// LDC{2}{L}<c> <coproc_no>, <CRd>, [<Rn>], #+/-<imm32>
// LDC{2}{L}<c> <coproc_no>, <CRd>, [<Rn>], <imm8>
bool TranslatorVisitor::arm_LDC(Cond cond, bool p, bool u, bool d, bool w, Reg n, CoprocReg CRd, size_t coproc_no, Imm<8> imm8) {
    if (!p && !u && !d && !w) {
        return arm_UDF();
    }

    if ((coproc_no & 0b1110) == 0b1010) {
        return arm_UDF();
    }

    const bool two = cond == Cond::NV;

    if (two || ArmConditionPassed(cond)) {
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
    }
    return true;
}

// MCR{2}<c> <coproc_no>, #<opc1>, <Rt>, <CRn>, <CRm>, #<opc2>
bool TranslatorVisitor::arm_MCR(Cond cond, size_t opc1, CoprocReg CRn, Reg t, size_t coproc_no, size_t opc2, CoprocReg CRm) {
    if ((coproc_no & 0b1110) == 0b1010) {
        return arm_UDF();
    }

    if (t == Reg::PC) {
        return UnpredictableInstruction();
    }

    const bool two = cond == Cond::NV;

    if (two || ArmConditionPassed(cond)) {
        ir.CoprocSendOneWord(coproc_no, two, opc1, CRn, CRm, opc2, ir.GetRegister(t));
    }
    return true;
}

// MCRR{2}<c> <coproc_no>, #<opc>, <Rt>, <Rt2>, <CRm>
bool TranslatorVisitor::arm_MCRR(Cond cond, Reg t2, Reg t, size_t coproc_no, size_t opc, CoprocReg CRm) {
    if ((coproc_no & 0b1110) == 0b1010) {
        return arm_UDF();
    }

    if (t == Reg::PC || t2 == Reg::PC) {
        return UnpredictableInstruction();
    }

    const bool two = cond == Cond::NV;

    if (two || ArmConditionPassed(cond)) {
        ir.CoprocSendTwoWords(coproc_no, two, opc, CRm, ir.GetRegister(t), ir.GetRegister(t2));
    }
    return true;
}

// MRC{2}<c> <coproc_no>, #<opc1>, <Rt>, <CRn>, <CRm>, #<opc2>
bool TranslatorVisitor::arm_MRC(Cond cond, size_t opc1, CoprocReg CRn, Reg t, size_t coproc_no, size_t opc2, CoprocReg CRm) {
    if ((coproc_no & 0b1110) == 0b1010) {
        return arm_UDF();
    }

    const bool two = cond == Cond::NV;

    if (two || ArmConditionPassed(cond)) {
        const auto word = ir.CoprocGetOneWord(coproc_no, two, opc1, CRn, CRm, opc2);
        if (t != Reg::PC) {
            ir.SetRegister(t, word);
        } else {
            const auto new_cpsr_nzcv = ir.And(word, ir.Imm32(0xF0000000));
            ir.SetCpsrNZCVRaw(new_cpsr_nzcv);
        }
    }
    return true;
}

// MRRC{2}<c> <coproc_no>, #<opc>, <Rt>, <Rt2>, <CRm>
bool TranslatorVisitor::arm_MRRC(Cond cond, Reg t2, Reg t, size_t coproc_no, size_t opc, CoprocReg CRm) {
    if ((coproc_no & 0b1110) == 0b1010) {
        return arm_UDF();
    }

    if (t == Reg::PC || t2 == Reg::PC || t == t2) {
        return UnpredictableInstruction();
    }

    const bool two = cond == Cond::NV;

    if (two || ArmConditionPassed(cond)) {
        const auto two_words = ir.CoprocGetTwoWords(coproc_no, two, opc, CRm);
        ir.SetRegister(t, ir.LeastSignificantWord(two_words));
        ir.SetRegister(t2, ir.MostSignificantWord(two_words).result);
    }
    return true;
}

// STC{2}{L}<c> <coproc>, <CRd>, [<Rn>, #+/-<imm32>]{!}
// STC{2}{L}<c> <coproc>, <CRd>, [<Rn>], #+/-<imm32>
// STC{2}{L}<c> <coproc>, <CRd>, [<Rn>], <imm8>
bool TranslatorVisitor::arm_STC(Cond cond, bool p, bool u, bool d, bool w, Reg n, CoprocReg CRd, size_t coproc_no, Imm<8> imm8) {
    if ((coproc_no & 0b1110) == 0b1010) {
        return arm_UDF();
    }

    if (!p && !u && !d && !w) {
        return arm_UDF();
    }

    if (n == Reg::PC && w) {
        return UnpredictableInstruction();
    }

    const bool two = cond == Cond::NV;

    if (two || ArmConditionPassed(cond)) {
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
    }
    return true;
}

}  // namespace Dynarmic::A32

/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/bit_util.h"
#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"

namespace Dynarmic::A32 {
static bool ITBlockCheck(const A32::IREmitter& ir) {
    return ir.current_location.IT().IsInITBlock() && !ir.current_location.IT().IsLastInITBlock();
}

static bool TableBranch(TranslatorVisitor& v, Reg n, Reg m, bool half) {
    if (m == Reg::PC) {
        return v.UnpredictableInstruction();
    }
    if (ITBlockCheck(v.ir)) {
        return v.UnpredictableInstruction();
    }

    const auto reg_m = v.ir.GetRegister(m);
    const auto reg_n = v.ir.GetRegister(n);

    IR::U32 halfwords;
    if (half) {
        const auto data = v.ir.ReadMemory16(v.ir.Add(reg_n, v.ir.LogicalShiftLeft(reg_m, v.ir.Imm8(1))), IR::AccType::NORMAL);
        halfwords = v.ir.ZeroExtendToWord(data);
    } else {
        halfwords = v.ir.ZeroExtendToWord(v.ir.ReadMemory8(v.ir.Add(reg_n, reg_m), IR::AccType::NORMAL));
    }

    const auto current_pc = v.ir.Imm32(v.ir.PC());
    const auto branch_value = v.ir.Add(current_pc, v.ir.Add(halfwords, halfwords));

    v.ir.UpdateUpperLocationDescriptor();
    v.ir.BranchWritePC(branch_value);
    v.ir.SetTerm(IR::Term::FastDispatchHint{});
    return false;
}

static bool LoadDualImmediate(TranslatorVisitor& v, bool P, bool U, bool W, Reg n, Reg t, Reg t2, Imm<8> imm8) {
    if (W && (n == t || n == t2)) {
        return v.UnpredictableInstruction();
    }
    if (t == Reg::PC || t2 == Reg::PC || t == t2) {
        return v.UnpredictableInstruction();
    }

    const u32 imm = imm8.ZeroExtend() << 2;
    const IR::U32 reg_n = v.ir.GetRegister(n);
    const IR::U32 offset_address = U ? v.ir.Add(reg_n, v.ir.Imm32(imm))
                                     : v.ir.Sub(reg_n, v.ir.Imm32(imm));
    const IR::U32 address = P ? offset_address : reg_n;

    // NOTE: If alignment is exactly off by 4, each word is an atomic access.
    const IR::U64 data = v.ir.ReadMemory64(address, IR::AccType::ATOMIC);

    if (v.ir.current_location.EFlag()) {
        v.ir.SetRegister(t, v.ir.MostSignificantWord(data).result);
        v.ir.SetRegister(t2, v.ir.LeastSignificantWord(data));
    } else {
        v.ir.SetRegister(t, v.ir.LeastSignificantWord(data));
        v.ir.SetRegister(t2, v.ir.MostSignificantWord(data).result);
    }

    if (W) {
        v.ir.SetRegister(n, offset_address);
    }
    return true;
}

static bool LoadDualLiteral(TranslatorVisitor& v, bool U, bool W, Reg t, Reg t2, Imm<8> imm8) {
    if (t == Reg::PC || t2 == Reg::PC || t == t2) {
        return v.UnpredictableInstruction();
    }
    if (W) {
        return v.UnpredictableInstruction();
    }

    const auto imm = imm8.ZeroExtend() << 2;
    const auto address = U ? v.ir.Add(v.ir.Imm32(v.ir.AlignPC(4)), v.ir.Imm32(imm))
                           : v.ir.Sub(v.ir.Imm32(v.ir.AlignPC(4)), v.ir.Imm32(imm));

    // NOTE: If alignment is exactly off by 4, each word is an atomic access.
    const IR::U64 data = v.ir.ReadMemory64(address, IR::AccType::ATOMIC);

    if (v.ir.current_location.EFlag()) {
        v.ir.SetRegister(t, v.ir.MostSignificantWord(data).result);
        v.ir.SetRegister(t2, v.ir.LeastSignificantWord(data));
    } else {
        v.ir.SetRegister(t, v.ir.LeastSignificantWord(data));
        v.ir.SetRegister(t2, v.ir.MostSignificantWord(data).result);
    }

    return true;
}

static bool StoreDual(TranslatorVisitor& v, bool P, bool U, bool W, Reg n, Reg t, Reg t2, Imm<8> imm8) {
    if (W && (n == t || n == t2)) {
        return v.UnpredictableInstruction();
    }
    if (n == Reg::PC || t == Reg::PC || t2 == Reg::PC) {
        return v.UnpredictableInstruction();
    }

    const u32 imm = imm8.ZeroExtend() << 2;
    const IR::U32 reg_n = v.ir.GetRegister(n);
    const IR::U32 reg_t = v.ir.GetRegister(t);
    const IR::U32 reg_t2 = v.ir.GetRegister(t2);

    const IR::U32 offset_address = U ? v.ir.Add(reg_n, v.ir.Imm32(imm))
                                     : v.ir.Sub(reg_n, v.ir.Imm32(imm));
    const IR::U32 address = P ? offset_address : reg_n;

    const IR::U64 data = v.ir.current_location.EFlag() ? v.ir.Pack2x32To1x64(reg_t2, reg_t)
                                                       : v.ir.Pack2x32To1x64(reg_t, reg_t2);

    // NOTE: If alignment is exactly off by 4, each word is an atomic access.
    v.ir.WriteMemory64(address, data, IR::AccType::ATOMIC);

    if (W) {
        v.ir.SetRegister(n, offset_address);
    }
    return true;
}

bool TranslatorVisitor::thumb32_LDA(Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto address = ir.GetRegister(n);
    ir.SetRegister(t, ir.ReadMemory32(address, IR::AccType::ORDERED));
    return true;
}

bool TranslatorVisitor::thumb32_LDRD_imm_1(bool U, Reg n, Reg t, Reg t2, Imm<8> imm8) {
    return LoadDualImmediate(*this, false, U, true, n, t, t2, imm8);
}

bool TranslatorVisitor::thumb32_LDRD_imm_2(bool U, bool W, Reg n, Reg t, Reg t2, Imm<8> imm8) {
    return LoadDualImmediate(*this, true, U, W, n, t, t2, imm8);
}

bool TranslatorVisitor::thumb32_LDRD_lit_1(bool U, Reg t, Reg t2, Imm<8> imm8) {
    return LoadDualLiteral(*this, U, true, t, t2, imm8);
}

bool TranslatorVisitor::thumb32_LDRD_lit_2(bool U, bool W, Reg t, Reg t2, Imm<8> imm8) {
    return LoadDualLiteral(*this, U, W, t, t2, imm8);
}

bool TranslatorVisitor::thumb32_STRD_imm_1(bool U, Reg n, Reg t, Reg t2, Imm<8> imm8) {
    return StoreDual(*this, false, U, true, n, t, t2, imm8);
}

bool TranslatorVisitor::thumb32_STRD_imm_2(bool U, bool W, Reg n, Reg t, Reg t2, Imm<8> imm8) {
    return StoreDual(*this, true, U, W, n, t, t2, imm8);
}

bool TranslatorVisitor::thumb32_LDREX(Reg n, Reg t, Imm<8> imm8) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto address = ir.Add(ir.GetRegister(n), ir.Imm32(imm8.ZeroExtend() << 2));
    const auto value = ir.ExclusiveReadMemory32(address, IR::AccType::ATOMIC);

    ir.SetRegister(t, value);
    return true;
}

bool TranslatorVisitor::thumb32_LDREXB(Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto address = ir.GetRegister(n);
    const auto value = ir.ZeroExtendToWord(ir.ExclusiveReadMemory8(address, IR::AccType::ATOMIC));

    ir.SetRegister(t, value);
    return true;
}

bool TranslatorVisitor::thumb32_LDREXD(Reg n, Reg t, Reg t2) {
    if (t == Reg::PC || t2 == Reg::PC || t == t2 || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto address = ir.GetRegister(n);
    const auto [lo, hi] = ir.ExclusiveReadMemory64(address, IR::AccType::ATOMIC);

    // DO NOT SWAP hi AND lo IN BIG ENDIAN MODE, THIS IS CORRECT BEHAVIOUR
    ir.SetRegister(t, lo);
    ir.SetRegister(t2, hi);
    return true;
}

bool TranslatorVisitor::thumb32_LDREXH(Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto address = ir.GetRegister(n);
    const auto value = ir.ZeroExtendToWord(ir.ExclusiveReadMemory16(address, IR::AccType::ATOMIC));

    ir.SetRegister(t, value);
    return true;
}

bool TranslatorVisitor::thumb32_STL(Reg n, Reg t) {
    if (t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto address = ir.GetRegister(n);
    ir.WriteMemory32(address, ir.GetRegister(t), IR::AccType::ORDERED);
    return true;
}

bool TranslatorVisitor::thumb32_STREX(Reg n, Reg t, Reg d, Imm<8> imm8) {
    if (d == Reg::PC || t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == n || d == t) {
        return UnpredictableInstruction();
    }

    const auto address = ir.Add(ir.GetRegister(n), ir.Imm32(imm8.ZeroExtend() << 2));
    const auto value = ir.GetRegister(t);
    const auto passed = ir.ExclusiveWriteMemory32(address, value, IR::AccType::ATOMIC);
    ir.SetRegister(d, passed);
    return true;
}

bool TranslatorVisitor::thumb32_STREXB(Reg n, Reg t, Reg d) {
    if (d == Reg::PC || t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == n || d == t) {
        return UnpredictableInstruction();
    }

    const auto address = ir.GetRegister(n);
    const auto value = ir.LeastSignificantByte(ir.GetRegister(t));
    const auto passed = ir.ExclusiveWriteMemory8(address, value, IR::AccType::ATOMIC);
    ir.SetRegister(d, passed);
    return true;
}

bool TranslatorVisitor::thumb32_STREXD(Reg n, Reg t, Reg t2, Reg d) {
    if (d == Reg::PC || t == Reg::PC || t2 == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == n || d == t || d == t2) {
        return UnpredictableInstruction();
    }

    const auto address = ir.GetRegister(n);
    const auto value_lo = ir.GetRegister(t);
    const auto value_hi = ir.GetRegister(t2);
    const auto passed = ir.ExclusiveWriteMemory64(address, value_lo, value_hi, IR::AccType::ATOMIC);
    ir.SetRegister(d, passed);
    return true;
}

bool TranslatorVisitor::thumb32_STREXH(Reg n, Reg t, Reg d) {
    if (d == Reg::PC || t == Reg::PC || n == Reg::PC) {
        return UnpredictableInstruction();
    }
    if (d == n || d == t) {
        return UnpredictableInstruction();
    }

    const auto address = ir.GetRegister(n);
    const auto value = ir.LeastSignificantHalf(ir.GetRegister(t));
    const auto passed = ir.ExclusiveWriteMemory16(address, value, IR::AccType::ATOMIC);
    ir.SetRegister(d, passed);
    return true;
}

bool TranslatorVisitor::thumb32_TBB(Reg n, Reg m) {
    return TableBranch(*this, n, m, false);
}

bool TranslatorVisitor::thumb32_TBH(Reg n, Reg m) {
    return TableBranch(*this, n, m, true);
}

}  // namespace Dynarmic::A32

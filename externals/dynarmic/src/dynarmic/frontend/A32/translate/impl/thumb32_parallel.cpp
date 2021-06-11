/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {
static IR::U32 Pack2x16To1x32(A32::IREmitter& ir, IR::U32 lo, IR::U32 hi) {
    return ir.Or(ir.And(lo, ir.Imm32(0xFFFF)), ir.LogicalShiftLeft(hi, ir.Imm8(16), ir.Imm1(0)).result);
}

static IR::U16 MostSignificantHalf(A32::IREmitter& ir, IR::U32 value) {
    return ir.LeastSignificantHalf(ir.LogicalShiftRight(value, ir.Imm8(16), ir.Imm1(0)).result);
}

bool TranslatorVisitor::thumb32_SADD8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAddS8(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool TranslatorVisitor::thumb32_SADD16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAddS16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool TranslatorVisitor::thumb32_SASX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAddSubS16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool TranslatorVisitor::thumb32_SSAX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSubAddS16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool TranslatorVisitor::thumb32_SSUB8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSubS8(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool TranslatorVisitor::thumb32_SSUB16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSubS16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool TranslatorVisitor::thumb32_UADD8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAddU8(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool TranslatorVisitor::thumb32_UADD16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAddU16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool TranslatorVisitor::thumb32_UASX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedAddSubU16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool TranslatorVisitor::thumb32_USAX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSubAddU16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool TranslatorVisitor::thumb32_USUB8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSubU8(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool TranslatorVisitor::thumb32_USUB16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSubU16(reg_n, reg_m);

    ir.SetRegister(d, result.result);
    ir.SetGEFlags(result.ge);
    return true;
}

bool TranslatorVisitor::thumb32_QADD8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSaturatedAddS8(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_QADD16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSaturatedAddS16(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_QASX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto Rn = ir.GetRegister(n);
    const auto Rm = ir.GetRegister(m);
    const auto Rn_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(Rn));
    const auto Rn_hi = ir.SignExtendHalfToWord(MostSignificantHalf(ir, Rn));
    const auto Rm_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(Rm));
    const auto Rm_hi = ir.SignExtendHalfToWord(MostSignificantHalf(ir, Rm));
    const auto diff = ir.SignedSaturation(ir.Sub(Rn_lo, Rm_hi), 16).result;
    const auto sum = ir.SignedSaturation(ir.Add(Rn_hi, Rm_lo), 16).result;
    const auto result = Pack2x16To1x32(ir, diff, sum);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_QSAX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto Rn = ir.GetRegister(n);
    const auto Rm = ir.GetRegister(m);
    const auto Rn_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(Rn));
    const auto Rn_hi = ir.SignExtendHalfToWord(MostSignificantHalf(ir, Rn));
    const auto Rm_lo = ir.SignExtendHalfToWord(ir.LeastSignificantHalf(Rm));
    const auto Rm_hi = ir.SignExtendHalfToWord(MostSignificantHalf(ir, Rm));
    const auto sum = ir.SignedSaturation(ir.Add(Rn_lo, Rm_hi), 16).result;
    const auto diff = ir.SignedSaturation(ir.Sub(Rn_hi, Rm_lo), 16).result;
    const auto result = Pack2x16To1x32(ir, sum, diff);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_QSUB8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSaturatedSubS8(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_QSUB16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSaturatedSubS16(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UQADD8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSaturatedAddU8(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UQADD16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSaturatedAddU16(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UQASX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto Rn = ir.GetRegister(n);
    const auto Rm = ir.GetRegister(m);
    const auto Rn_lo = ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(Rn));
    const auto Rn_hi = ir.ZeroExtendHalfToWord(MostSignificantHalf(ir, Rn));
    const auto Rm_lo = ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(Rm));
    const auto Rm_hi = ir.ZeroExtendHalfToWord(MostSignificantHalf(ir, Rm));
    const auto diff = ir.UnsignedSaturation(ir.Sub(Rn_lo, Rm_hi), 16).result;
    const auto sum = ir.UnsignedSaturation(ir.Add(Rn_hi, Rm_lo), 16).result;
    const auto result = Pack2x16To1x32(ir, diff, sum);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UQSAX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto Rn = ir.GetRegister(n);
    const auto Rm = ir.GetRegister(m);
    const auto Rn_lo = ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(Rn));
    const auto Rn_hi = ir.ZeroExtendHalfToWord(MostSignificantHalf(ir, Rn));
    const auto Rm_lo = ir.ZeroExtendHalfToWord(ir.LeastSignificantHalf(Rm));
    const auto Rm_hi = ir.ZeroExtendHalfToWord(MostSignificantHalf(ir, Rm));
    const auto sum = ir.UnsignedSaturation(ir.Add(Rn_lo, Rm_hi), 16).result;
    const auto diff = ir.UnsignedSaturation(ir.Sub(Rn_hi, Rm_lo), 16).result;
    const auto result = Pack2x16To1x32(ir, sum, diff);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UQSUB8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSaturatedSubU8(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UQSUB16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedSaturatedSubU16(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SHADD8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedHalvingAddS8(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SHADD16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedHalvingAddS16(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SHASX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedHalvingAddSubS16(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SHSAX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedHalvingSubAddS16(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SHSUB8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedHalvingSubS8(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_SHSUB16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedHalvingSubS16(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UHADD8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedHalvingAddU8(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UHADD16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedHalvingAddU16(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UHASX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedHalvingAddSubU16(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UHSAX(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedHalvingSubAddU16(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UHSUB8(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedHalvingSubU8(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

bool TranslatorVisitor::thumb32_UHSUB16(Reg n, Reg d, Reg m) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return UnpredictableInstruction();
    }

    const auto reg_m = ir.GetRegister(m);
    const auto reg_n = ir.GetRegister(n);
    const auto result = ir.PackedHalvingSubU16(reg_n, reg_m);

    ir.SetRegister(d, result);
    return true;
}

}  // namespace Dynarmic::A32

/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {

// It's considered constrained UNPREDICTABLE behavior if either
// CRC32 instruction variant is executed with a condition code
// that is *not* 0xE (Always execute). ARM defines one of the following
// as being a requirement in this case. Either:
//
// 1. The instruction is undefined.
// 2. The instruction executes as a NOP.
// 3. The instruction executes unconditionally.
// 4. The instruction executes conditionally.
//
// It's also considered constrained UNPREDICTABLE behavior if
// either CRC32 instruction variant is executed with a size specifier
// of 64-bit (sz -> 0b11)
//
// In this case, either:
//
// 1. The instruction is undefined
// 2. The instruction executes as a NOP.
// 3. The instruction executes with the additional decode: size = 32.
//
// In both cases, we treat as unpredictable, to allow
// library users to provide their own intended behavior
// in the unpredictable exception handler.

namespace {
enum class CRCType {
    Castagnoli,
    ISO,
};

bool CRC32Variant(TranslatorVisitor& v, Cond cond, Imm<2> sz, Reg n, Reg d, Reg m, CRCType type) {
    if (d == Reg::PC || n == Reg::PC || m == Reg::PC) {
        return v.UnpredictableInstruction();
    }

    if (sz == 0b11) {
        return v.UnpredictableInstruction();
    }

    if (cond != Cond::AL) {
        return v.UnpredictableInstruction();
    }

    const IR::U32 result = [m, n, sz, type, &v] {
        const IR::U32 accumulator = v.ir.GetRegister(n);
        const IR::U32 data = v.ir.GetRegister(m);

        if (type == CRCType::ISO) {
            switch (sz.ZeroExtend()) {
            case 0b00:
                return v.ir.CRC32ISO8(accumulator, data);
            case 0b01:
                return v.ir.CRC32ISO16(accumulator, data);
            case 0b10:
                return v.ir.CRC32ISO32(accumulator, data);
            }
        } else {
            switch (sz.ZeroExtend()) {
            case 0b00:
                return v.ir.CRC32Castagnoli8(accumulator, data);
            case 0b01:
                return v.ir.CRC32Castagnoli16(accumulator, data);
            case 0b10:
                return v.ir.CRC32Castagnoli32(accumulator, data);
            }
        }

        UNREACHABLE();
    }();

    v.ir.SetRegister(d, result);
    return true;
}
}  // Anonymous namespace

// CRC32{B,H,W}{<q>} <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_CRC32(Cond cond, Imm<2> sz, Reg n, Reg d, Reg m) {
    return CRC32Variant(*this, cond, sz, n, d, m, CRCType::ISO);
}

// CRC32C{B,H,W}{<q>} <Rd>, <Rn>, <Rm>
bool TranslatorVisitor::arm_CRC32C(Cond cond, Imm<2> sz, Reg n, Reg d, Reg m) {
    return CRC32Variant(*this, cond, sz, n, d, m, CRCType::Castagnoli);
}

}  // namespace Dynarmic::A32

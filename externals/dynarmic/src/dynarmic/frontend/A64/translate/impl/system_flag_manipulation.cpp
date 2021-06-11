/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::CFINV() {
    const IR::U32 nzcv = ir.GetNZCVRaw();
    const IR::U32 result = ir.Eor(nzcv, ir.Imm32(0x20000000));

    ir.SetNZCVRaw(result);
    return true;
}

bool TranslatorVisitor::RMIF(Imm<6> lsb, Reg Rn, Imm<4> mask) {
    const u32 mask_value = mask.ZeroExtend();

    // If no bits are to be moved into the NZCV bits, then we
    // just preserve the bits and do no extra work.
    if (mask_value == 0) {
        ir.SetNZCVRaw(ir.GetNZCVRaw());
        return true;
    }

    const IR::U64 tmp_reg = ir.GetX(Rn);
    const IR::U64 rotated = ir.RotateRight(tmp_reg, ir.Imm8(lsb.ZeroExtend<u8>()));
    const IR::U32 shifted = ir.LeastSignificantWord(ir.LogicalShiftLeft(rotated, ir.Imm8(28)));

    // On the other hand, if all mask bits are set, then we move all four
    // relevant bits in the source register to the NZCV bits.
    if (mask_value == 0b1111) {
        ir.SetNZCVRaw(shifted);
        return true;
    }

    // Determine which bits from the PSTATE will be preserved during the operation.
    u32 preservation_mask = 0;
    if ((mask_value & 0b1000) == 0) {
        preservation_mask |= 1U << 31;
    }
    if ((mask_value & 0b0100) == 0) {
        preservation_mask |= 1U << 30;
    }
    if ((mask_value & 0b0010) == 0) {
        preservation_mask |= 1U << 29;
    }
    if ((mask_value & 0b0001) == 0) {
        preservation_mask |= 1U << 28;
    }

    const IR::U32 masked = ir.And(shifted, ir.Imm32(~preservation_mask));
    const IR::U32 nzcv = ir.And(ir.GetNZCVRaw(), ir.Imm32(preservation_mask));
    const IR::U32 result = ir.Or(nzcv, masked);

    ir.SetNZCVRaw(result);
    return true;
}

}  // namespace Dynarmic::A64

/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::AXFlag() {
    const IR::U32 nzcv = ir.GetNZCVRaw();

    const IR::U32 z = ir.And(nzcv, ir.Imm32(0x40000000));
    const IR::U32 c = ir.And(nzcv, ir.Imm32(0x20000000));
    const IR::U32 v = ir.And(nzcv, ir.Imm32(0x10000000));

    const IR::U32 new_z = ir.Or(ir.LogicalShiftLeft(v, ir.Imm8(2)), z);
    const IR::U32 new_c = ir.And(ir.And(c, ir.Not(ir.LogicalShiftLeft(v, ir.Imm8(1)))), ir.Imm32(0x20000000));

    ir.SetNZCVRaw(ir.Or(new_z, new_c));
    return true;
}

bool TranslatorVisitor::XAFlag() {
    const IR::U32 nzcv = ir.GetNZCVRaw();

    const IR::U32 z = ir.And(nzcv, ir.Imm32(0x40000000));
    const IR::U32 c = ir.And(nzcv, ir.Imm32(0x20000000));

    const IR::U32 not_z = ir.And(ir.Not(z), ir.Imm32(0x40000000));
    const IR::U32 not_c = ir.And(ir.Not(c), ir.Imm32(0x20000000));

    const IR::U32 new_n = ir.And(ir.LogicalShiftLeft(not_c, ir.Imm8(2)),
                                 ir.LogicalShiftLeft(not_z, ir.Imm8(1)));
    const IR::U32 new_z = ir.And(z, ir.LogicalShiftLeft(c, ir.Imm8(1)));
    const IR::U32 new_c = ir.Or(c, ir.LogicalShiftRight(z, ir.Imm8(1)));
    const IR::U32 new_v = ir.And(ir.LogicalShiftRight(not_c, ir.Imm8(1)),
                                 ir.LogicalShiftRight(z, ir.Imm8(2)));

    const IR::U32 result = ir.Or(ir.Or(ir.Or(new_n, new_z), new_c), new_v);

    ir.SetNZCVRaw(result);
    return true;
}

}  // namespace Dynarmic::A64

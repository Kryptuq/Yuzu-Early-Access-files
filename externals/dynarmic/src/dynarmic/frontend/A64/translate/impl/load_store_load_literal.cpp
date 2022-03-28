/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::LDR_lit_gen(bool opc_0, Imm<19> imm19, Reg Rt) {
    const size_t size = opc_0 == 0 ? 4 : 8;
    const s64 offset = concatenate(imm19, Imm<2>{0}).SignExtend<s64>();

    const u64 address = ir.PC() + offset;
    const auto data = Mem(ir.Imm64(address), size, IR::AccType::NORMAL);

    X(8 * size, Rt, data);
    return true;
}

bool TranslatorVisitor::LDR_lit_fpsimd(Imm<2> opc, Imm<19> imm19, Vec Vt) {
    if (opc == 0b11) {
        return UnallocatedEncoding();
    }

    const u64 size = 4 << opc.ZeroExtend();
    const u64 offset = imm19.SignExtend<u64>() << 2;
    const IR::U64 address = ir.Imm64(ir.PC() + offset);
    const IR::UAnyU128 data = Mem(address, size, IR::AccType::VEC);

    if (size == 16) {
        V(128, Vt, data);
    } else {
        V(128, Vt, ir.ZeroExtendToQuad(data));
    }
    return true;
}

bool TranslatorVisitor::LDRSW_lit(Imm<19> imm19, Reg Rt) {
    const s64 offset = concatenate(imm19, Imm<2>{0}).SignExtend<s64>();
    const u64 address = ir.PC() + offset;
    const auto data = Mem(ir.Imm64(address), 4, IR::AccType::NORMAL);

    X(64, Rt, ir.SignExtendWordToLong(data));
    return true;
}

bool TranslatorVisitor::PRFM_lit(Imm<19> /*imm19*/, Imm<5> /*prfop*/) {
    // s64 offset = concatenate(imm19, Imm<2>{0}).SignExtend<s64>();
    // u64 address = ir.PC() + offset;
    // Prefetch(address, prfop);
    return true;
}

}  // namespace Dynarmic::A64

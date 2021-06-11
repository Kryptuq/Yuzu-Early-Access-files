/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::ADR(Imm<2> immlo, Imm<19> immhi, Reg Rd) {
    const u64 imm = concatenate(immhi, immlo).SignExtend<u64>();
    const u64 base = ir.PC();
    X(64, Rd, ir.Imm64(base + imm));
    return true;
}

bool TranslatorVisitor::ADRP(Imm<2> immlo, Imm<19> immhi, Reg Rd) {
    const u64 imm = concatenate(immhi, immlo).SignExtend<u64>() << 12;
    const u64 base = ir.PC() & ~u64(0xFFF);
    X(64, Rd, ir.Imm64(base + imm));
    return true;
}

}  // namespace Dynarmic::A64

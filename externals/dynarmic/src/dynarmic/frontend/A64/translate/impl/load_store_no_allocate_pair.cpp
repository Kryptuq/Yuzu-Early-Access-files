/* This file is part of the dynarmic project.
 * Copyright (c) 2019 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

// Given the LDNP and STNP instructions are simply hinting at non-temporal
// accesses, we can treat them as regular LDP and STP instructions for the
// time being since we don't perform data caching.

bool TranslatorVisitor::STNP_LDNP_gen(Imm<1> upper_opc, Imm<1> L, Imm<7> imm7, Reg Rt2, Reg Rn, Reg Rt) {
    return STP_LDP_gen(Imm<2>{upper_opc.ZeroExtend() << 1}, true, false, L, imm7, Rt2, Rn, Rt);
}

bool TranslatorVisitor::STNP_LDNP_fpsimd(Imm<2> opc, Imm<1> L, Imm<7> imm7, Vec Vt2, Reg Rn, Vec Vt) {
    return STP_LDP_fpsimd(opc, true, false, L, imm7, Vt2, Rn, Vt);
}

}  // namespace Dynarmic::A64

/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/x64/block_of_code.h"
#include "dynarmic/backend/x64/emit_x64.h"

namespace Dynarmic::Backend::X64 {

using namespace Xbyak::util;

void EmitX64::EmitSHA256Hash(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const bool part1 = args[3].GetImmediateU1();

    ASSERT(code.HasHostFeature(HostFeature::SHA));

    //      3   2   1   0
    // x =  d   c   b   a
    // y =  h   g   f   e
    // w = wk3 wk2 wk1 wk0

    const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm w = ctx.reg_alloc.UseXmm(args[2]);

    // x64 expects:
    //         3   2   1   0
    // src1 =  c   d   g   h
    // src2 =  a   b   e   f
    // xmm0 =  -   -  wk1 wk0

    code.movaps(xmm0, y);
    code.shufps(xmm0, x, 0b10111011);  // src1
    code.shufps(y, x, 0b00010001);     // src2
    code.movaps(x, xmm0);

    code.movaps(xmm0, w);
    code.sha256rnds2(x, y);

    code.punpckhqdq(xmm0, xmm0);
    code.sha256rnds2(y, x);

    code.shufps(y, x, part1 ? 0b10111011 : 0b00010001);

    ctx.reg_alloc.DefineValue(inst, y);
}

void EmitX64::EmitSHA256MessageSchedule0(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    ASSERT(code.HasHostFeature(HostFeature::SHA));

    const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);

    code.sha256msg1(x, y);

    ctx.reg_alloc.DefineValue(inst, x);
}

void EmitX64::EmitSHA256MessageSchedule1(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    ASSERT(code.HasHostFeature(HostFeature::SHA));

    const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm z = ctx.reg_alloc.UseXmm(args[2]);

    code.movaps(xmm0, z);
    code.palignr(xmm0, y, 4);
    code.paddd(x, xmm0);
    code.sha256msg2(x, z);

    ctx.reg_alloc.DefineValue(inst, x);
}

}  // namespace Dynarmic::Backend::X64

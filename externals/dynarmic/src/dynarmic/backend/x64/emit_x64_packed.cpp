/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/backend/x64/block_of_code.h"
#include "dynarmic/backend/x64/emit_x64.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::Backend::X64 {

using namespace Xbyak::util;

void EmitX64::EmitPackedAddU8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto ge_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetGEFromOp);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);

    code.paddb(xmm_a, xmm_b);

    if (ge_inst) {
        const Xbyak::Xmm xmm_ge = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm ones = ctx.reg_alloc.ScratchXmm();

        code.pcmpeqb(ones, ones);

        code.movdqa(xmm_ge, xmm_a);
        code.pminub(xmm_ge, xmm_b);
        code.pcmpeqb(xmm_ge, xmm_b);
        code.pxor(xmm_ge, ones);

        ctx.reg_alloc.DefineValue(ge_inst, xmm_ge);
        ctx.EraseInstruction(ge_inst);
    }

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitPackedAddS8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto ge_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetGEFromOp);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);

    if (ge_inst) {
        const Xbyak::Xmm xmm_ge = ctx.reg_alloc.ScratchXmm();

        code.pcmpeqb(xmm0, xmm0);

        code.movdqa(xmm_ge, xmm_a);
        code.paddsb(xmm_ge, xmm_b);
        code.pcmpgtb(xmm_ge, xmm0);

        ctx.reg_alloc.DefineValue(ge_inst, xmm_ge);
        ctx.EraseInstruction(ge_inst);
    }

    code.paddb(xmm_a, xmm_b);

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitPackedAddU16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto ge_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetGEFromOp);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);

    code.paddw(xmm_a, xmm_b);

    if (ge_inst) {
        if (code.HasHostFeature(HostFeature::SSE41)) {
            const Xbyak::Xmm xmm_ge = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Xmm ones = ctx.reg_alloc.ScratchXmm();

            code.pcmpeqb(ones, ones);

            code.movdqa(xmm_ge, xmm_a);
            code.pminuw(xmm_ge, xmm_b);
            code.pcmpeqw(xmm_ge, xmm_b);
            code.pxor(xmm_ge, ones);

            ctx.reg_alloc.DefineValue(ge_inst, xmm_ge);
            ctx.EraseInstruction(ge_inst);
        } else {
            const Xbyak::Xmm tmp_a = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Xmm tmp_b = ctx.reg_alloc.ScratchXmm();

            // !(b <= a+b) == b > a+b
            code.movdqa(tmp_a, xmm_a);
            code.movdqa(tmp_b, xmm_b);
            code.paddw(tmp_a, code.XmmBConst<16>(xword, 0x8000));
            code.paddw(tmp_b, code.XmmBConst<16>(xword, 0x8000));
            code.pcmpgtw(tmp_b, tmp_a);  // *Signed* comparison!

            ctx.reg_alloc.DefineValue(ge_inst, tmp_b);
            ctx.EraseInstruction(ge_inst);
        }
    }

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitPackedAddS16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto ge_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetGEFromOp);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);

    if (ge_inst) {
        const Xbyak::Xmm xmm_ge = ctx.reg_alloc.ScratchXmm();

        code.pcmpeqw(xmm0, xmm0);

        code.movdqa(xmm_ge, xmm_a);
        code.paddsw(xmm_ge, xmm_b);
        code.pcmpgtw(xmm_ge, xmm0);

        ctx.reg_alloc.DefineValue(ge_inst, xmm_ge);
        ctx.EraseInstruction(ge_inst);
    }

    code.paddw(xmm_a, xmm_b);

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitPackedSubU8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto ge_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetGEFromOp);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);

    if (ge_inst) {
        const Xbyak::Xmm xmm_ge = ctx.reg_alloc.ScratchXmm();

        code.movdqa(xmm_ge, xmm_a);
        code.pmaxub(xmm_ge, xmm_b);
        code.pcmpeqb(xmm_ge, xmm_a);

        ctx.reg_alloc.DefineValue(ge_inst, xmm_ge);
        ctx.EraseInstruction(ge_inst);
    }

    code.psubb(xmm_a, xmm_b);

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitPackedSubS8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto ge_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetGEFromOp);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);

    if (ge_inst) {
        const Xbyak::Xmm xmm_ge = ctx.reg_alloc.ScratchXmm();

        code.pcmpeqb(xmm0, xmm0);

        code.movdqa(xmm_ge, xmm_a);
        code.psubsb(xmm_ge, xmm_b);
        code.pcmpgtb(xmm_ge, xmm0);

        ctx.reg_alloc.DefineValue(ge_inst, xmm_ge);
        ctx.EraseInstruction(ge_inst);
    }

    code.psubb(xmm_a, xmm_b);

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitPackedSubU16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto ge_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetGEFromOp);

    if (!ge_inst) {
        const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);

        code.psubw(xmm_a, xmm_b);

        ctx.reg_alloc.DefineValue(inst, xmm_a);
        return;
    }

    if (code.HasHostFeature(HostFeature::SSE41)) {
        const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);
        const Xbyak::Xmm xmm_ge = ctx.reg_alloc.ScratchXmm();

        code.movdqa(xmm_ge, xmm_a);
        code.pmaxuw(xmm_ge, xmm_b);  // Requires SSE 4.1
        code.pcmpeqw(xmm_ge, xmm_a);

        code.psubw(xmm_a, xmm_b);

        ctx.reg_alloc.DefineValue(ge_inst, xmm_ge);
        ctx.EraseInstruction(ge_inst);
        ctx.reg_alloc.DefineValue(inst, xmm_a);
        return;
    }

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm xmm_ge = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm ones = ctx.reg_alloc.ScratchXmm();

    // (a >= b) == !(b > a)
    code.pcmpeqb(ones, ones);
    code.paddw(xmm_a, code.XmmBConst<16>(xword, 0x8000));
    code.paddw(xmm_b, code.XmmBConst<16>(xword, 0x8000));
    code.movdqa(xmm_ge, xmm_b);
    code.pcmpgtw(xmm_ge, xmm_a);  // *Signed* comparison!
    code.pxor(xmm_ge, ones);

    code.psubw(xmm_a, xmm_b);

    ctx.reg_alloc.DefineValue(ge_inst, xmm_ge);
    ctx.EraseInstruction(ge_inst);
    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitPackedSubS16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto ge_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetGEFromOp);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);

    if (ge_inst) {
        const Xbyak::Xmm xmm_ge = ctx.reg_alloc.ScratchXmm();

        code.pcmpeqw(xmm0, xmm0);

        code.movdqa(xmm_ge, xmm_a);
        code.psubsw(xmm_ge, xmm_b);
        code.pcmpgtw(xmm_ge, xmm0);

        ctx.reg_alloc.DefineValue(ge_inst, xmm_ge);
        ctx.EraseInstruction(ge_inst);
    }

    code.psubw(xmm_a, xmm_b);

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitPackedHalvingAddU8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (args[0].IsInXmm() || args[1].IsInXmm()) {
        const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseScratchXmm(args[1]);
        const Xbyak::Xmm ones = ctx.reg_alloc.ScratchXmm();

        // Since,
        //   pavg(a, b) == (a + b + 1) >> 1
        // Therefore,
        //   ~pavg(~a, ~b) == (a + b) >> 1

        code.pcmpeqb(ones, ones);
        code.pxor(xmm_a, ones);
        code.pxor(xmm_b, ones);
        code.pavgb(xmm_a, xmm_b);
        code.pxor(xmm_a, ones);

        ctx.reg_alloc.DefineValue(inst, xmm_a);
    } else {
        const Xbyak::Reg32 reg_a = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        const Xbyak::Reg32 reg_b = ctx.reg_alloc.UseGpr(args[1]).cvt32();
        const Xbyak::Reg32 xor_a_b = ctx.reg_alloc.ScratchGpr().cvt32();
        const Xbyak::Reg32 and_a_b = reg_a;
        const Xbyak::Reg32 result = reg_a;

        // This relies on the equality x+y == ((x&y) << 1) + (x^y).
        // Note that x^y always contains the LSB of the result.
        // Since we want to calculate (x+y)/2, we can instead calculate (x&y) + ((x^y)>>1).
        // We mask by 0x7F to remove the LSB so that it doesn't leak into the field below.

        code.mov(xor_a_b, reg_a);
        code.and_(and_a_b, reg_b);
        code.xor_(xor_a_b, reg_b);
        code.shr(xor_a_b, 1);
        code.and_(xor_a_b, 0x7F7F7F7F);
        code.add(result, xor_a_b);

        ctx.reg_alloc.DefineValue(inst, result);
    }
}

void EmitX64::EmitPackedHalvingAddU16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (args[0].IsInXmm() || args[1].IsInXmm()) {
        const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.movdqa(tmp, xmm_a);
        code.pand(xmm_a, xmm_b);
        code.pxor(tmp, xmm_b);
        code.psrlw(tmp, 1);
        code.paddw(xmm_a, tmp);

        ctx.reg_alloc.DefineValue(inst, xmm_a);
    } else {
        const Xbyak::Reg32 reg_a = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        const Xbyak::Reg32 reg_b = ctx.reg_alloc.UseGpr(args[1]).cvt32();
        const Xbyak::Reg32 xor_a_b = ctx.reg_alloc.ScratchGpr().cvt32();
        const Xbyak::Reg32 and_a_b = reg_a;
        const Xbyak::Reg32 result = reg_a;

        // This relies on the equality x+y == ((x&y) << 1) + (x^y).
        // Note that x^y always contains the LSB of the result.
        // Since we want to calculate (x+y)/2, we can instead calculate (x&y) + ((x^y)>>1).
        // We mask by 0x7FFF to remove the LSB so that it doesn't leak into the field below.

        code.mov(xor_a_b, reg_a);
        code.and_(and_a_b, reg_b);
        code.xor_(xor_a_b, reg_b);
        code.shr(xor_a_b, 1);
        code.and_(xor_a_b, 0x7FFF7FFF);
        code.add(result, xor_a_b);

        ctx.reg_alloc.DefineValue(inst, result);
    }
}

void EmitX64::EmitPackedHalvingAddS8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 reg_a = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
    const Xbyak::Reg32 reg_b = ctx.reg_alloc.UseGpr(args[1]).cvt32();
    const Xbyak::Reg32 xor_a_b = ctx.reg_alloc.ScratchGpr().cvt32();
    const Xbyak::Reg32 and_a_b = reg_a;
    const Xbyak::Reg32 result = reg_a;
    const Xbyak::Reg32 carry = ctx.reg_alloc.ScratchGpr().cvt32();

    // This relies on the equality x+y == ((x&y) << 1) + (x^y).
    // Note that x^y always contains the LSB of the result.
    // Since we want to calculate (x+y)/2, we can instead calculate (x&y) + ((x^y)>>1).
    // We mask by 0x7F to remove the LSB so that it doesn't leak into the field below.
    // carry propagates the sign bit from (x^y)>>1 upwards by one.

    code.mov(xor_a_b, reg_a);
    code.and_(and_a_b, reg_b);
    code.xor_(xor_a_b, reg_b);
    code.mov(carry, xor_a_b);
    code.and_(carry, 0x80808080);
    code.shr(xor_a_b, 1);
    code.and_(xor_a_b, 0x7F7F7F7F);
    code.add(result, xor_a_b);
    code.xor_(result, carry);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitPackedHalvingAddS16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    // This relies on the equality x+y == ((x&y) << 1) + (x^y).
    // Note that x^y always contains the LSB of the result.
    // Since we want to calculate (x+y)/2, we can instead calculate (x&y) + ((x^y)>>>1).
    // The arithmetic shift right makes this signed.

    code.movdqa(tmp, xmm_a);
    code.pand(xmm_a, xmm_b);
    code.pxor(tmp, xmm_b);
    code.psraw(tmp, 1);
    code.paddw(xmm_a, tmp);

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitPackedHalvingSubU8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 minuend = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
    const Xbyak::Reg32 subtrahend = ctx.reg_alloc.UseScratchGpr(args[1]).cvt32();

    // This relies on the equality x-y == (x^y) - (((x^y)&y) << 1).
    // Note that x^y always contains the LSB of the result.
    // Since we want to calculate (x+y)/2, we can instead calculate ((x^y)>>1) - ((x^y)&y).

    code.xor_(minuend, subtrahend);
    code.and_(subtrahend, minuend);
    code.shr(minuend, 1);

    // At this point,
    // minuend := (a^b) >> 1
    // subtrahend := (a^b) & b

    // We must now perform a partitioned subtraction.
    // We can do this because minuend contains 7 bit fields.
    // We use the extra bit in minuend as a bit to borrow from; we set this bit.
    // We invert this bit at the end as this tells us if that bit was borrowed from.
    code.or_(minuend, 0x80808080);
    code.sub(minuend, subtrahend);
    code.xor_(minuend, 0x80808080);

    // minuend now contains the desired result.
    ctx.reg_alloc.DefineValue(inst, minuend);
}

void EmitX64::EmitPackedHalvingSubS8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 minuend = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
    const Xbyak::Reg32 subtrahend = ctx.reg_alloc.UseScratchGpr(args[1]).cvt32();

    const Xbyak::Reg32 carry = ctx.reg_alloc.ScratchGpr().cvt32();

    // This relies on the equality x-y == (x^y) - (((x^y)&y) << 1).
    // Note that x^y always contains the LSB of the result.
    // Since we want to calculate (x-y)/2, we can instead calculate ((x^y)>>1) - ((x^y)&y).

    code.xor_(minuend, subtrahend);
    code.and_(subtrahend, minuend);
    code.mov(carry, minuend);
    code.and_(carry, 0x80808080);
    code.shr(minuend, 1);

    // At this point,
    // minuend := (a^b) >> 1
    // subtrahend := (a^b) & b
    // carry := (a^b) & 0x80808080

    // We must now perform a partitioned subtraction.
    // We can do this because minuend contains 7 bit fields.
    // We use the extra bit in minuend as a bit to borrow from; we set this bit.
    // We invert this bit at the end as this tells us if that bit was borrowed from.
    // We then sign extend the result into this bit.
    code.or_(minuend, 0x80808080);
    code.sub(minuend, subtrahend);
    code.xor_(minuend, 0x80808080);
    code.xor_(minuend, carry);

    ctx.reg_alloc.DefineValue(inst, minuend);
}

void EmitX64::EmitPackedHalvingSubU16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm minuend = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm subtrahend = ctx.reg_alloc.UseScratchXmm(args[1]);

    // This relies on the equality x-y == (x^y) - (((x^y)&y) << 1).
    // Note that x^y always contains the LSB of the result.
    // Since we want to calculate (x-y)/2, we can instead calculate ((x^y)>>1) - ((x^y)&y).

    code.pxor(minuend, subtrahend);
    code.pand(subtrahend, minuend);
    code.psrlw(minuend, 1);

    // At this point,
    // minuend := (a^b) >> 1
    // subtrahend := (a^b) & b

    code.psubw(minuend, subtrahend);

    ctx.reg_alloc.DefineValue(inst, minuend);
}

void EmitX64::EmitPackedHalvingSubS16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm minuend = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm subtrahend = ctx.reg_alloc.UseScratchXmm(args[1]);

    // This relies on the equality x-y == (x^y) - (((x^y)&y) << 1).
    // Note that x^y always contains the LSB of the result.
    // Since we want to calculate (x-y)/2, we can instead calculate ((x^y)>>>1) - ((x^y)&y).

    code.pxor(minuend, subtrahend);
    code.pand(subtrahend, minuend);
    code.psraw(minuend, 1);

    // At this point,
    // minuend := (a^b) >>> 1
    // subtrahend := (a^b) & b

    code.psubw(minuend, subtrahend);

    ctx.reg_alloc.DefineValue(inst, minuend);
}

static void EmitPackedSubAdd(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, bool hi_is_sum, bool is_signed, bool is_halving) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto ge_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetGEFromOp);

    const Xbyak::Reg32 reg_a_hi = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
    const Xbyak::Reg32 reg_b_hi = ctx.reg_alloc.UseScratchGpr(args[1]).cvt32();
    const Xbyak::Reg32 reg_a_lo = ctx.reg_alloc.ScratchGpr().cvt32();
    const Xbyak::Reg32 reg_b_lo = ctx.reg_alloc.ScratchGpr().cvt32();
    Xbyak::Reg32 reg_sum, reg_diff;

    if (is_signed) {
        code.movsx(reg_a_lo, reg_a_hi.cvt16());
        code.movsx(reg_b_lo, reg_b_hi.cvt16());
        code.sar(reg_a_hi, 16);
        code.sar(reg_b_hi, 16);
    } else {
        code.movzx(reg_a_lo, reg_a_hi.cvt16());
        code.movzx(reg_b_lo, reg_b_hi.cvt16());
        code.shr(reg_a_hi, 16);
        code.shr(reg_b_hi, 16);
    }

    if (hi_is_sum) {
        code.sub(reg_a_lo, reg_b_hi);
        code.add(reg_a_hi, reg_b_lo);
        reg_diff = reg_a_lo;
        reg_sum = reg_a_hi;
    } else {
        code.add(reg_a_lo, reg_b_hi);
        code.sub(reg_a_hi, reg_b_lo);
        reg_diff = reg_a_hi;
        reg_sum = reg_a_lo;
    }

    if (ge_inst) {
        // The reg_b registers are no longer required.
        const Xbyak::Reg32 ge_sum = reg_b_hi;
        const Xbyak::Reg32 ge_diff = reg_b_lo;

        code.mov(ge_sum, reg_sum);
        code.mov(ge_diff, reg_diff);

        if (!is_signed) {
            code.shl(ge_sum, 15);
            code.sar(ge_sum, 31);
        } else {
            code.not_(ge_sum);
            code.sar(ge_sum, 31);
        }
        code.not_(ge_diff);
        code.sar(ge_diff, 31);
        code.and_(ge_sum, hi_is_sum ? 0xFFFF0000 : 0x0000FFFF);
        code.and_(ge_diff, hi_is_sum ? 0x0000FFFF : 0xFFFF0000);
        code.or_(ge_sum, ge_diff);

        ctx.reg_alloc.DefineValue(ge_inst, ge_sum);
        ctx.EraseInstruction(ge_inst);
    }

    if (is_halving) {
        code.shl(reg_a_lo, 15);
        code.shr(reg_a_hi, 1);
    } else {
        code.shl(reg_a_lo, 16);
    }

    // reg_a_lo now contains the low word and reg_a_hi now contains the high word.
    // Merge them.
    code.shld(reg_a_hi, reg_a_lo, 16);

    ctx.reg_alloc.DefineValue(inst, reg_a_hi);
}

void EmitX64::EmitPackedAddSubU16(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedSubAdd(code, ctx, inst, true, false, false);
}

void EmitX64::EmitPackedAddSubS16(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedSubAdd(code, ctx, inst, true, true, false);
}

void EmitX64::EmitPackedSubAddU16(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedSubAdd(code, ctx, inst, false, false, false);
}

void EmitX64::EmitPackedSubAddS16(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedSubAdd(code, ctx, inst, false, true, false);
}

void EmitX64::EmitPackedHalvingAddSubU16(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedSubAdd(code, ctx, inst, true, false, true);
}

void EmitX64::EmitPackedHalvingAddSubS16(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedSubAdd(code, ctx, inst, true, true, true);
}

void EmitX64::EmitPackedHalvingSubAddU16(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedSubAdd(code, ctx, inst, false, false, true);
}

void EmitX64::EmitPackedHalvingSubAddS16(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedSubAdd(code, ctx, inst, false, true, true);
}

static void EmitPackedOperation(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, void (Xbyak::CodeGenerator::*fn)(const Xbyak::Mmx& mmx, const Xbyak::Operand&)) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);

    (code.*fn)(xmm_a, xmm_b);

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitPackedSaturatedAddU8(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedOperation(code, ctx, inst, &Xbyak::CodeGenerator::paddusb);
}

void EmitX64::EmitPackedSaturatedAddS8(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedOperation(code, ctx, inst, &Xbyak::CodeGenerator::paddsb);
}

void EmitX64::EmitPackedSaturatedSubU8(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedOperation(code, ctx, inst, &Xbyak::CodeGenerator::psubusb);
}

void EmitX64::EmitPackedSaturatedSubS8(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedOperation(code, ctx, inst, &Xbyak::CodeGenerator::psubsb);
}

void EmitX64::EmitPackedSaturatedAddU16(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedOperation(code, ctx, inst, &Xbyak::CodeGenerator::paddusw);
}

void EmitX64::EmitPackedSaturatedAddS16(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedOperation(code, ctx, inst, &Xbyak::CodeGenerator::paddsw);
}

void EmitX64::EmitPackedSaturatedSubU16(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedOperation(code, ctx, inst, &Xbyak::CodeGenerator::psubusw);
}

void EmitX64::EmitPackedSaturatedSubS16(EmitContext& ctx, IR::Inst* inst) {
    EmitPackedOperation(code, ctx, inst, &Xbyak::CodeGenerator::psubsw);
}

void EmitX64::EmitPackedAbsDiffSumS8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    // TODO: Optimize with zero-extension detection
    code.movaps(tmp, code.XmmBConst<8>(xword, 0xFF));
    code.pand(xmm_a, tmp);
    code.pand(xmm_b, tmp);
    code.psadbw(xmm_a, xmm_b);

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitPackedSelect(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const size_t num_args_in_xmm = args[0].IsInXmm() + args[1].IsInXmm() + args[2].IsInXmm();

    if (num_args_in_xmm >= 2) {
        const Xbyak::Xmm ge = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm to = ctx.reg_alloc.UseXmm(args[1]);
        const Xbyak::Xmm from = ctx.reg_alloc.UseScratchXmm(args[2]);

        code.pand(from, ge);
        code.pandn(ge, to);
        code.por(from, ge);

        ctx.reg_alloc.DefineValue(inst, from);
    } else if (code.HasHostFeature(HostFeature::BMI1)) {
        const Xbyak::Reg32 ge = ctx.reg_alloc.UseGpr(args[0]).cvt32();
        const Xbyak::Reg32 to = ctx.reg_alloc.UseScratchGpr(args[1]).cvt32();
        const Xbyak::Reg32 from = ctx.reg_alloc.UseScratchGpr(args[2]).cvt32();

        code.and_(from, ge);
        code.andn(to, ge, to);
        code.or_(from, to);

        ctx.reg_alloc.DefineValue(inst, from);
    } else {
        const Xbyak::Reg32 ge = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        const Xbyak::Reg32 to = ctx.reg_alloc.UseGpr(args[1]).cvt32();
        const Xbyak::Reg32 from = ctx.reg_alloc.UseScratchGpr(args[2]).cvt32();

        code.and_(from, ge);
        code.not_(ge);
        code.and_(ge, to);
        code.or_(from, ge);

        ctx.reg_alloc.DefineValue(inst, from);
    }
}

}  // namespace Dynarmic::Backend::X64

/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <limits>

#include <mp/traits/integer_of_size.h>

#include "dynarmic/backend/x64/block_of_code.h"
#include "dynarmic/backend/x64/emit_x64.h"
#include "dynarmic/common/assert.h"
#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::Backend::X64 {

using namespace Xbyak::util;

namespace {

enum class Op {
    Add,
    Sub,
};

template<Op op, size_t size>
void EmitSignedSaturatedOp(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    const auto overflow_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetOverflowFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    Xbyak::Reg result = ctx.reg_alloc.UseScratchGpr(args[0]).changeBit(size);
    Xbyak::Reg addend = ctx.reg_alloc.UseGpr(args[1]).changeBit(size);
    Xbyak::Reg overflow = ctx.reg_alloc.ScratchGpr().changeBit(size);

    constexpr u64 int_max = static_cast<u64>(std::numeric_limits<mp::signed_integer_of_size<size>>::max());
    if constexpr (size < 64) {
        code.xor_(overflow.cvt32(), overflow.cvt32());
        code.bt(result.cvt32(), size - 1);
        code.adc(overflow.cvt32(), int_max);
    } else {
        code.mov(overflow, int_max);
        code.bt(result, 63);
        code.adc(overflow, 0);
    }

    // overflow now contains 0x7F... if a was positive, or 0x80... if a was negative

    if constexpr (op == Op::Add) {
        code.add(result, addend);
    } else {
        code.sub(result, addend);
    }

    if constexpr (size == 8) {
        code.cmovo(result.cvt32(), overflow.cvt32());
    } else {
        code.cmovo(result, overflow);
    }

    if (overflow_inst) {
        code.seto(overflow.cvt8());

        ctx.reg_alloc.DefineValue(overflow_inst, overflow);
        ctx.EraseInstruction(overflow_inst);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

template<Op op, size_t size>
void EmitUnsignedSaturatedOp(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    const auto overflow_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetOverflowFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    Xbyak::Reg op_result = ctx.reg_alloc.UseScratchGpr(args[0]).changeBit(size);
    Xbyak::Reg addend = ctx.reg_alloc.UseScratchGpr(args[1]).changeBit(size);

    constexpr u64 boundary = op == Op::Add ? std::numeric_limits<mp::unsigned_integer_of_size<size>>::max() : 0;

    if constexpr (op == Op::Add) {
        code.add(op_result, addend);
    } else {
        code.sub(op_result, addend);
    }
    code.mov(addend, boundary);
    if constexpr (size == 8) {
        code.cmovae(addend.cvt32(), op_result.cvt32());
    } else {
        code.cmovae(addend, op_result);
    }

    if (overflow_inst) {
        const Xbyak::Reg overflow = ctx.reg_alloc.ScratchGpr();
        code.setb(overflow.cvt8());

        ctx.reg_alloc.DefineValue(overflow_inst, overflow);
        ctx.EraseInstruction(overflow_inst);
    }

    ctx.reg_alloc.DefineValue(inst, addend);
}

}  // anonymous namespace

void EmitX64::EmitSignedSaturatedAdd8(EmitContext& ctx, IR::Inst* inst) {
    EmitSignedSaturatedOp<Op::Add, 8>(code, ctx, inst);
}

void EmitX64::EmitSignedSaturatedAdd16(EmitContext& ctx, IR::Inst* inst) {
    EmitSignedSaturatedOp<Op::Add, 16>(code, ctx, inst);
}

void EmitX64::EmitSignedSaturatedAdd32(EmitContext& ctx, IR::Inst* inst) {
    EmitSignedSaturatedOp<Op::Add, 32>(code, ctx, inst);
}

void EmitX64::EmitSignedSaturatedAdd64(EmitContext& ctx, IR::Inst* inst) {
    EmitSignedSaturatedOp<Op::Add, 64>(code, ctx, inst);
}

void EmitX64::EmitSignedSaturatedDoublingMultiplyReturnHigh16(EmitContext& ctx, IR::Inst* inst) {
    const auto overflow_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetOverflowFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 x = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
    const Xbyak::Reg32 y = ctx.reg_alloc.UseScratchGpr(args[1]).cvt32();
    const Xbyak::Reg32 tmp = ctx.reg_alloc.ScratchGpr().cvt32();

    code.movsx(x, x.cvt16());
    code.movsx(y, y.cvt16());

    code.imul(x, y);
    code.lea(y, ptr[x.cvt64() + x.cvt64()]);
    code.mov(tmp, x);
    code.shr(tmp, 15);
    code.xor_(y, x);
    code.mov(y, 0x7FFF);
    code.cmovns(y, tmp);

    if (overflow_inst) {
        code.sets(tmp.cvt8());

        ctx.reg_alloc.DefineValue(overflow_inst, tmp);
        ctx.EraseInstruction(overflow_inst);
    }

    ctx.reg_alloc.DefineValue(inst, y);
}

void EmitX64::EmitSignedSaturatedDoublingMultiplyReturnHigh32(EmitContext& ctx, IR::Inst* inst) {
    const auto overflow_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetOverflowFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg64 x = ctx.reg_alloc.UseScratchGpr(args[0]);
    const Xbyak::Reg64 y = ctx.reg_alloc.UseScratchGpr(args[1]);
    const Xbyak::Reg64 tmp = ctx.reg_alloc.ScratchGpr();

    code.movsxd(x, x.cvt32());
    code.movsxd(y, y.cvt32());

    code.imul(x, y);
    code.lea(y, ptr[x + x]);
    code.mov(tmp, x);
    code.shr(tmp, 31);
    code.xor_(y, x);
    code.mov(y.cvt32(), 0x7FFFFFFF);
    code.cmovns(y.cvt32(), tmp.cvt32());

    if (overflow_inst) {
        code.sets(tmp.cvt8());

        ctx.reg_alloc.DefineValue(overflow_inst, tmp);
        ctx.EraseInstruction(overflow_inst);
    }

    ctx.reg_alloc.DefineValue(inst, y);
}

void EmitX64::EmitSignedSaturatedSub8(EmitContext& ctx, IR::Inst* inst) {
    EmitSignedSaturatedOp<Op::Sub, 8>(code, ctx, inst);
}

void EmitX64::EmitSignedSaturatedSub16(EmitContext& ctx, IR::Inst* inst) {
    EmitSignedSaturatedOp<Op::Sub, 16>(code, ctx, inst);
}

void EmitX64::EmitSignedSaturatedSub32(EmitContext& ctx, IR::Inst* inst) {
    EmitSignedSaturatedOp<Op::Sub, 32>(code, ctx, inst);
}

void EmitX64::EmitSignedSaturatedSub64(EmitContext& ctx, IR::Inst* inst) {
    EmitSignedSaturatedOp<Op::Sub, 64>(code, ctx, inst);
}

void EmitX64::EmitSignedSaturation(EmitContext& ctx, IR::Inst* inst) {
    const auto overflow_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetOverflowFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const size_t N = args[1].GetImmediateU8();
    ASSERT(N >= 1 && N <= 32);

    if (N == 32) {
        if (overflow_inst) {
            const auto no_overflow = IR::Value(false);
            overflow_inst->ReplaceUsesWith(no_overflow);
        }
        // TODO: DefineValue directly on Argument
        const Xbyak::Reg64 result = ctx.reg_alloc.ScratchGpr();
        const Xbyak::Reg64 source = ctx.reg_alloc.UseGpr(args[0]);
        code.mov(result.cvt32(), source.cvt32());
        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    const u32 mask = (1u << N) - 1;
    const u32 positive_saturated_value = (1u << (N - 1)) - 1;
    const u32 negative_saturated_value = 1u << (N - 1);

    const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();
    const Xbyak::Reg32 reg_a = ctx.reg_alloc.UseGpr(args[0]).cvt32();
    const Xbyak::Reg32 overflow = ctx.reg_alloc.ScratchGpr().cvt32();

    // overflow now contains a value between 0 and mask if it was originally between {negative,positive}_saturated_value.
    code.lea(overflow, code.ptr[reg_a.cvt64() + negative_saturated_value]);

    // Put the appropriate saturated value in result
    code.mov(result, reg_a);
    code.sar(result, 31);
    code.xor_(result, positive_saturated_value);

    // Do the saturation
    code.cmp(overflow, mask);
    code.cmovbe(result, reg_a);

    if (overflow_inst) {
        code.seta(overflow.cvt8());

        ctx.reg_alloc.DefineValue(overflow_inst, overflow);
        ctx.EraseInstruction(overflow_inst);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitUnsignedSaturatedAdd8(EmitContext& ctx, IR::Inst* inst) {
    EmitUnsignedSaturatedOp<Op::Add, 8>(code, ctx, inst);
}

void EmitX64::EmitUnsignedSaturatedAdd16(EmitContext& ctx, IR::Inst* inst) {
    EmitUnsignedSaturatedOp<Op::Add, 16>(code, ctx, inst);
}

void EmitX64::EmitUnsignedSaturatedAdd32(EmitContext& ctx, IR::Inst* inst) {
    EmitUnsignedSaturatedOp<Op::Add, 32>(code, ctx, inst);
}

void EmitX64::EmitUnsignedSaturatedAdd64(EmitContext& ctx, IR::Inst* inst) {
    EmitUnsignedSaturatedOp<Op::Add, 64>(code, ctx, inst);
}

void EmitX64::EmitUnsignedSaturatedSub8(EmitContext& ctx, IR::Inst* inst) {
    EmitUnsignedSaturatedOp<Op::Sub, 8>(code, ctx, inst);
}

void EmitX64::EmitUnsignedSaturatedSub16(EmitContext& ctx, IR::Inst* inst) {
    EmitUnsignedSaturatedOp<Op::Sub, 16>(code, ctx, inst);
}

void EmitX64::EmitUnsignedSaturatedSub32(EmitContext& ctx, IR::Inst* inst) {
    EmitUnsignedSaturatedOp<Op::Sub, 32>(code, ctx, inst);
}

void EmitX64::EmitUnsignedSaturatedSub64(EmitContext& ctx, IR::Inst* inst) {
    EmitUnsignedSaturatedOp<Op::Sub, 64>(code, ctx, inst);
}

void EmitX64::EmitUnsignedSaturation(EmitContext& ctx, IR::Inst* inst) {
    const auto overflow_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetOverflowFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const size_t N = args[1].GetImmediateU8();
    ASSERT(N <= 31);

    const u32 saturated_value = (1u << N) - 1;

    const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();
    const Xbyak::Reg32 reg_a = ctx.reg_alloc.UseGpr(args[0]).cvt32();
    const Xbyak::Reg32 overflow = ctx.reg_alloc.ScratchGpr().cvt32();

    // Pseudocode: result = clamp(reg_a, 0, saturated_value);
    code.xor_(overflow, overflow);
    code.cmp(reg_a, saturated_value);
    code.mov(result, saturated_value);
    code.cmovle(result, overflow);
    code.cmovbe(result, reg_a);

    if (overflow_inst) {
        code.seta(overflow.cvt8());

        ctx.reg_alloc.DefineValue(overflow_inst, overflow);
        ctx.EraseInstruction(overflow_inst);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

}  // namespace Dynarmic::Backend::X64

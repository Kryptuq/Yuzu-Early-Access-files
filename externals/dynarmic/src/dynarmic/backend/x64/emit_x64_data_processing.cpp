/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <cstddef>
#include <type_traits>

#include "dynarmic/backend/x64/block_of_code.h"
#include "dynarmic/backend/x64/emit_x64.h"
#include "dynarmic/common/assert.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::Backend::X64 {

using namespace Xbyak::util;

void EmitX64::EmitPack2x32To1x64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg64 lo = ctx.reg_alloc.UseScratchGpr(args[0]);
    const Xbyak::Reg64 hi = ctx.reg_alloc.UseScratchGpr(args[1]);

    code.shl(hi, 32);
    code.mov(lo.cvt32(), lo.cvt32());  // Zero extend to 64-bits
    code.or_(lo, hi);

    ctx.reg_alloc.DefineValue(inst, lo);
}

void EmitX64::EmitPack2x64To1x128(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg64 lo = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Reg64 hi = ctx.reg_alloc.UseGpr(args[1]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.movq(result, lo);
        code.pinsrq(result, hi, 1);
    } else {
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
        code.movq(result, lo);
        code.movq(tmp, hi);
        code.punpcklqdq(result, tmp);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitLeastSignificantWord(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    // TODO: DefineValue directly on Argument
    const Xbyak::Reg64 result = ctx.reg_alloc.ScratchGpr();
    const Xbyak::Reg64 source = ctx.reg_alloc.UseGpr(args[0]);
    code.mov(result.cvt32(), source.cvt32());
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitMostSignificantWord(EmitContext& ctx, IR::Inst* inst) {
    const auto carry_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetCarryFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);
    code.shr(result, 32);

    if (carry_inst) {
        const Xbyak::Reg64 carry = ctx.reg_alloc.ScratchGpr();
        code.setc(carry.cvt8());
        ctx.reg_alloc.DefineValue(carry_inst, carry);
        ctx.EraseInstruction(carry_inst);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitLeastSignificantHalf(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    // TODO: DefineValue directly on Argument
    const Xbyak::Reg64 result = ctx.reg_alloc.ScratchGpr();
    const Xbyak::Reg64 source = ctx.reg_alloc.UseGpr(args[0]);
    code.movzx(result.cvt32(), source.cvt16());
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitLeastSignificantByte(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    // TODO: DefineValue directly on Argument
    const Xbyak::Reg64 result = ctx.reg_alloc.ScratchGpr();
    const Xbyak::Reg64 source = ctx.reg_alloc.UseGpr(args[0]);
    code.movzx(result.cvt32(), source.cvt8());
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitMostSignificantBit(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
    // TODO: Flag optimization
    code.shr(result, 31);
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitIsZero32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
    // TODO: Flag optimization
    code.test(result, result);
    code.sete(result.cvt8());
    code.movzx(result, result.cvt8());
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitIsZero64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);
    // TODO: Flag optimization
    code.test(result, result);
    code.sete(result.cvt8());
    code.movzx(result, result.cvt8());
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitTestBit(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);
    ASSERT(args[1].IsImmediate());
    // TODO: Flag optimization
    code.bt(result, args[1].GetImmediateU8());
    code.setc(result.cvt8());
    ctx.reg_alloc.DefineValue(inst, result);
}

static void EmitConditionalSelect(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, int bitsize) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg32 nzcv = ctx.reg_alloc.ScratchGpr(HostLoc::RAX).cvt32();
    const Xbyak::Reg then_ = ctx.reg_alloc.UseGpr(args[1]).changeBit(bitsize);
    const Xbyak::Reg else_ = ctx.reg_alloc.UseScratchGpr(args[2]).changeBit(bitsize);

    code.mov(nzcv, dword[r15 + code.GetJitStateInfo().offsetof_cpsr_nzcv]);

    // sahf restores SF, ZF, CF
    // add al, 0x7F restores OF

    switch (args[0].GetImmediateCond()) {
    case IR::Cond::EQ:  //z
        code.sahf();
        code.cmovz(else_, then_);
        break;
    case IR::Cond::NE:  //!z
        code.sahf();
        code.cmovnz(else_, then_);
        break;
    case IR::Cond::CS:  //c
        code.sahf();
        code.cmovc(else_, then_);
        break;
    case IR::Cond::CC:  //!c
        code.sahf();
        code.cmovnc(else_, then_);
        break;
    case IR::Cond::MI:  //n
        code.sahf();
        code.cmovs(else_, then_);
        break;
    case IR::Cond::PL:  //!n
        code.sahf();
        code.cmovns(else_, then_);
        break;
    case IR::Cond::VS:  //v
        code.cmp(nzcv.cvt8(), 0x81);
        code.cmovo(else_, then_);
        break;
    case IR::Cond::VC:  //!v
        code.cmp(nzcv.cvt8(), 0x81);
        code.cmovno(else_, then_);
        break;
    case IR::Cond::HI:  //c & !z
        code.sahf();
        code.cmc();
        code.cmova(else_, then_);
        break;
    case IR::Cond::LS:  //!c | z
        code.sahf();
        code.cmc();
        code.cmovna(else_, then_);
        break;
    case IR::Cond::GE:  // n == v
        code.cmp(nzcv.cvt8(), 0x81);
        code.sahf();
        code.cmovge(else_, then_);
        break;
    case IR::Cond::LT:  // n != v
        code.cmp(nzcv.cvt8(), 0x81);
        code.sahf();
        code.cmovl(else_, then_);
        break;
    case IR::Cond::GT:  // !z & (n == v)
        code.cmp(nzcv.cvt8(), 0x81);
        code.sahf();
        code.cmovg(else_, then_);
        break;
    case IR::Cond::LE:  // z | (n != v)
        code.cmp(nzcv.cvt8(), 0x81);
        code.sahf();
        code.cmovle(else_, then_);
        break;
    case IR::Cond::AL:
    case IR::Cond::NV:
        code.mov(else_, then_);
        break;
    default:
        ASSERT_MSG(false, "Invalid cond {}", static_cast<size_t>(args[0].GetImmediateCond()));
    }

    ctx.reg_alloc.DefineValue(inst, else_);
}

void EmitX64::EmitConditionalSelect32(EmitContext& ctx, IR::Inst* inst) {
    EmitConditionalSelect(code, ctx, inst, 32);
}

void EmitX64::EmitConditionalSelect64(EmitContext& ctx, IR::Inst* inst) {
    EmitConditionalSelect(code, ctx, inst, 64);
}

void EmitX64::EmitConditionalSelectNZCV(EmitContext& ctx, IR::Inst* inst) {
    EmitConditionalSelect(code, ctx, inst, 32);
}

static void EmitExtractRegister(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, int bit_size) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg result = ctx.reg_alloc.UseScratchGpr(args[0]).changeBit(bit_size);
    const Xbyak::Reg operand = ctx.reg_alloc.UseGpr(args[1]).changeBit(bit_size);
    const u8 lsb = args[2].GetImmediateU8();

    code.shrd(result, operand, lsb);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitExtractRegister32(Dynarmic::Backend::X64::EmitContext& ctx, IR::Inst* inst) {
    EmitExtractRegister(code, ctx, inst, 32);
}

void EmitX64::EmitExtractRegister64(Dynarmic::Backend::X64::EmitContext& ctx, IR::Inst* inst) {
    EmitExtractRegister(code, ctx, inst, 64);
}

static void EmitReplicateBit(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, int bit_size) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const u8 bit = args[1].GetImmediateU8();

    if (bit == bit_size - 1) {
        const Xbyak::Reg result = ctx.reg_alloc.UseScratchGpr(args[0]).changeBit(bit_size);

        code.sar(result, bit_size - 1);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    const Xbyak::Reg value = ctx.reg_alloc.UseGpr(args[0]).changeBit(bit_size);
    const Xbyak::Reg result = ctx.reg_alloc.ScratchGpr().changeBit(bit_size);

    code.xor_(result, result);
    code.bt(value, bit);
    code.sbb(result, result);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitReplicateBit32(Dynarmic::Backend::X64::EmitContext& ctx, IR::Inst* inst) {
    EmitReplicateBit(code, ctx, inst, 32);
}

void EmitX64::EmitReplicateBit64(Dynarmic::Backend::X64::EmitContext& ctx, IR::Inst* inst) {
    EmitReplicateBit(code, ctx, inst, 64);
}

void EmitX64::EmitLogicalShiftLeft32(EmitContext& ctx, IR::Inst* inst) {
    const auto carry_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetCarryFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];
    auto& carry_arg = args[2];

    if (!carry_inst) {
        if (shift_arg.IsImmediate()) {
            const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();
            const u8 shift = shift_arg.GetImmediateU8();

            if (shift <= 31) {
                code.shl(result, shift);
            } else {
                code.xor_(result, result);
            }

            ctx.reg_alloc.DefineValue(inst, result);
        } else if (code.HasHostFeature(HostFeature::BMI2)) {
            const Xbyak::Reg32 shift = ctx.reg_alloc.UseGpr(shift_arg).cvt32();
            const Xbyak::Reg32 operand = ctx.reg_alloc.UseGpr(operand_arg).cvt32();
            const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();
            const Xbyak::Reg32 zero = ctx.reg_alloc.ScratchGpr().cvt32();

            code.shlx(result, operand, shift);
            code.xor_(zero, zero);
            code.cmp(shift.cvt8(), 32);
            code.cmovnb(result, zero);

            ctx.reg_alloc.DefineValue(inst, result);
        } else {
            ctx.reg_alloc.Use(shift_arg, HostLoc::RCX);
            const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();
            const Xbyak::Reg32 zero = ctx.reg_alloc.ScratchGpr().cvt32();

            // The 32-bit x64 SHL instruction masks the shift count by 0x1F before performing the shift.
            // ARM differs from the behaviour: It does not mask the count, so shifts above 31 result in zeros.

            code.shl(result, code.cl);
            code.xor_(zero, zero);
            code.cmp(code.cl, 32);
            code.cmovnb(result, zero);

            ctx.reg_alloc.DefineValue(inst, result);
        }
    } else {
        if (shift_arg.IsImmediate()) {
            const u8 shift = shift_arg.GetImmediateU8();
            const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();
            const Xbyak::Reg32 carry = ctx.reg_alloc.UseScratchGpr(carry_arg).cvt32();

            if (shift == 0) {
                // There is nothing more to do.
            } else if (shift < 32) {
                code.bt(carry.cvt32(), 0);
                code.shl(result, shift);
                code.setc(carry.cvt8());
            } else if (shift > 32) {
                code.xor_(result, result);
                code.xor_(carry, carry);
            } else {
                code.mov(carry, result);
                code.xor_(result, result);
                code.and_(carry, 1);
            }

            ctx.reg_alloc.DefineValue(carry_inst, carry);
            ctx.EraseInstruction(carry_inst);
            ctx.reg_alloc.DefineValue(inst, result);
        } else {
            ctx.reg_alloc.UseScratch(shift_arg, HostLoc::RCX);
            const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();
            const Xbyak::Reg32 tmp = ctx.reg_alloc.ScratchGpr().cvt32();
            const Xbyak::Reg32 carry = ctx.reg_alloc.UseScratchGpr(carry_arg).cvt32();

            code.mov(tmp, 63);
            code.cmp(code.cl, 63);
            code.cmova(code.ecx, tmp);
            code.shl(result.cvt64(), 32);
            code.bt(carry.cvt32(), 0);
            code.shl(result.cvt64(), code.cl);
            code.setc(carry.cvt8());
            code.shr(result.cvt64(), 32);

            ctx.reg_alloc.DefineValue(carry_inst, carry);
            ctx.EraseInstruction(carry_inst);
            ctx.reg_alloc.DefineValue(inst, result);
        }
    }
}

void EmitX64::EmitLogicalShiftLeft64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];

    if (shift_arg.IsImmediate()) {
        const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(operand_arg);
        const u8 shift = shift_arg.GetImmediateU8();

        if (shift < 64) {
            code.shl(result, shift);
        } else {
            code.xor_(result.cvt32(), result.cvt32());
        }

        ctx.reg_alloc.DefineValue(inst, result);
    } else if (code.HasHostFeature(HostFeature::BMI2)) {
        const Xbyak::Reg64 shift = ctx.reg_alloc.UseGpr(shift_arg);
        const Xbyak::Reg64 operand = ctx.reg_alloc.UseGpr(operand_arg);
        const Xbyak::Reg64 result = ctx.reg_alloc.ScratchGpr();
        const Xbyak::Reg64 zero = ctx.reg_alloc.ScratchGpr();

        code.shlx(result, operand, shift);
        code.xor_(zero.cvt32(), zero.cvt32());
        code.cmp(shift.cvt8(), 64);
        code.cmovnb(result, zero);

        ctx.reg_alloc.DefineValue(inst, result);
    } else {
        ctx.reg_alloc.Use(shift_arg, HostLoc::RCX);
        const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(operand_arg);
        const Xbyak::Reg64 zero = ctx.reg_alloc.ScratchGpr();

        // The x64 SHL instruction masks the shift count by 0x1F before performing the shift.
        // ARM differs from the behaviour: It does not mask the count, so shifts above 63 result in zeros.

        code.shl(result, code.cl);
        code.xor_(zero.cvt32(), zero.cvt32());
        code.cmp(code.cl, 64);
        code.cmovnb(result, zero);

        ctx.reg_alloc.DefineValue(inst, result);
    }
}

void EmitX64::EmitLogicalShiftRight32(EmitContext& ctx, IR::Inst* inst) {
    const auto carry_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetCarryFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];
    auto& carry_arg = args[2];

    if (!carry_inst) {
        if (shift_arg.IsImmediate()) {
            const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();
            const u8 shift = shift_arg.GetImmediateU8();

            if (shift <= 31) {
                code.shr(result, shift);
            } else {
                code.xor_(result, result);
            }

            ctx.reg_alloc.DefineValue(inst, result);
        } else if (code.HasHostFeature(HostFeature::BMI2)) {
            const Xbyak::Reg32 shift = ctx.reg_alloc.UseGpr(shift_arg).cvt32();
            const Xbyak::Reg32 operand = ctx.reg_alloc.UseGpr(operand_arg).cvt32();
            const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();
            const Xbyak::Reg32 zero = ctx.reg_alloc.ScratchGpr().cvt32();

            code.shrx(result, operand, shift);
            code.xor_(zero, zero);
            code.cmp(shift.cvt8(), 32);
            code.cmovnb(result, zero);

            ctx.reg_alloc.DefineValue(inst, result);
        } else {
            ctx.reg_alloc.Use(shift_arg, HostLoc::RCX);
            const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();
            const Xbyak::Reg32 zero = ctx.reg_alloc.ScratchGpr().cvt32();

            // The 32-bit x64 SHR instruction masks the shift count by 0x1F before performing the shift.
            // ARM differs from the behaviour: It does not mask the count, so shifts above 31 result in zeros.

            code.shr(result, code.cl);
            code.xor_(zero, zero);
            code.cmp(code.cl, 32);
            code.cmovnb(result, zero);

            ctx.reg_alloc.DefineValue(inst, result);
        }
    } else {
        if (shift_arg.IsImmediate()) {
            const u8 shift = shift_arg.GetImmediateU8();
            const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();
            const Xbyak::Reg32 carry = ctx.reg_alloc.UseScratchGpr(carry_arg).cvt32();

            if (shift == 0) {
                // There is nothing more to do.
            } else if (shift < 32) {
                code.shr(result, shift);
                code.setc(carry.cvt8());
            } else if (shift == 32) {
                code.bt(result, 31);
                code.setc(carry.cvt8());
                code.mov(result, 0);
            } else {
                code.xor_(result, result);
                code.xor_(carry, carry);
            }

            ctx.reg_alloc.DefineValue(carry_inst, carry);
            ctx.EraseInstruction(carry_inst);
            ctx.reg_alloc.DefineValue(inst, result);
        } else {
            ctx.reg_alloc.UseScratch(shift_arg, HostLoc::RCX);
            const Xbyak::Reg32 operand = ctx.reg_alloc.UseGpr(operand_arg).cvt32();
            const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();
            const Xbyak::Reg32 carry = ctx.reg_alloc.UseScratchGpr(carry_arg).cvt32();

            code.mov(result, 63);
            code.cmp(code.cl, 63);
            code.cmovnb(code.ecx, result);
            code.mov(result, operand);
            code.bt(carry.cvt32(), 0);
            code.shr(result.cvt64(), code.cl);
            code.setc(carry.cvt8());

            ctx.reg_alloc.DefineValue(carry_inst, carry);
            ctx.EraseInstruction(carry_inst);
            ctx.reg_alloc.DefineValue(inst, result);
        }
    }
}

void EmitX64::EmitLogicalShiftRight64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];

    if (shift_arg.IsImmediate()) {
        const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(operand_arg);
        const u8 shift = shift_arg.GetImmediateU8();

        if (shift < 64) {
            code.shr(result, shift);
        } else {
            code.xor_(result.cvt32(), result.cvt32());
        }

        ctx.reg_alloc.DefineValue(inst, result);
    } else if (code.HasHostFeature(HostFeature::BMI2)) {
        const Xbyak::Reg64 shift = ctx.reg_alloc.UseGpr(shift_arg);
        const Xbyak::Reg64 operand = ctx.reg_alloc.UseGpr(operand_arg);
        const Xbyak::Reg64 result = ctx.reg_alloc.ScratchGpr();
        const Xbyak::Reg64 zero = ctx.reg_alloc.ScratchGpr();

        code.shrx(result, operand, shift);
        code.xor_(zero.cvt32(), zero.cvt32());
        code.cmp(shift.cvt8(), 63);
        code.cmovnb(result, zero);

        ctx.reg_alloc.DefineValue(inst, result);
    } else {
        ctx.reg_alloc.Use(shift_arg, HostLoc::RCX);
        const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(operand_arg);
        const Xbyak::Reg64 zero = ctx.reg_alloc.ScratchGpr();

        // The x64 SHR instruction masks the shift count by 0x1F before performing the shift.
        // ARM differs from the behaviour: It does not mask the count, so shifts above 63 result in zeros.

        code.shr(result, code.cl);
        code.xor_(zero.cvt32(), zero.cvt32());
        code.cmp(code.cl, 64);
        code.cmovnb(result, zero);

        ctx.reg_alloc.DefineValue(inst, result);
    }
}

void EmitX64::EmitArithmeticShiftRight32(EmitContext& ctx, IR::Inst* inst) {
    const auto carry_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetCarryFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];
    auto& carry_arg = args[2];

    if (!carry_inst) {
        if (shift_arg.IsImmediate()) {
            const u8 shift = shift_arg.GetImmediateU8();
            const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();

            code.sar(result, u8(shift < 31 ? shift : 31));

            ctx.reg_alloc.DefineValue(inst, result);
        } else if (code.HasHostFeature(HostFeature::BMI2)) {
            const Xbyak::Reg32 shift = ctx.reg_alloc.UseScratchGpr(shift_arg).cvt32();
            const Xbyak::Reg32 operand = ctx.reg_alloc.UseGpr(operand_arg).cvt32();
            const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();
            const Xbyak::Reg32 const31 = ctx.reg_alloc.ScratchGpr().cvt32();

            // The 32-bit x64 SAR instruction masks the shift count by 0x1F before performing the shift.
            // ARM differs from the behaviour: It does not mask the count.

            // We note that all shift values above 31 have the same behaviour as 31 does, so we saturate `shift` to 31.
            code.mov(const31, 31);
            code.cmp(shift.cvt8(), 31);
            code.cmovnb(shift, const31);
            code.sarx(result, operand, shift);

            ctx.reg_alloc.DefineValue(inst, result);
        } else {
            ctx.reg_alloc.UseScratch(shift_arg, HostLoc::RCX);
            const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();
            const Xbyak::Reg32 const31 = ctx.reg_alloc.ScratchGpr().cvt32();

            // The 32-bit x64 SAR instruction masks the shift count by 0x1F before performing the shift.
            // ARM differs from the behaviour: It does not mask the count.

            // We note that all shift values above 31 have the same behaviour as 31 does, so we saturate `shift` to 31.
            code.mov(const31, 31);
            code.cmp(code.cl, u32(31));
            code.cmova(code.ecx, const31);
            code.sar(result, code.cl);

            ctx.reg_alloc.DefineValue(inst, result);
        }
    } else {
        if (shift_arg.IsImmediate()) {
            const u8 shift = shift_arg.GetImmediateU8();
            const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();
            const Xbyak::Reg8 carry = ctx.reg_alloc.UseScratchGpr(carry_arg).cvt8();

            if (shift == 0) {
                // There is nothing more to do.
            } else if (shift <= 31) {
                code.sar(result, shift);
                code.setc(carry);
            } else {
                code.sar(result, 31);
                code.bt(result, 31);
                code.setc(carry);
            }

            ctx.reg_alloc.DefineValue(carry_inst, carry);
            ctx.EraseInstruction(carry_inst);
            ctx.reg_alloc.DefineValue(inst, result);
        } else {
            ctx.reg_alloc.UseScratch(shift_arg, HostLoc::RCX);
            const Xbyak::Reg32 operand = ctx.reg_alloc.UseGpr(operand_arg).cvt32();
            const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();
            const Xbyak::Reg32 carry = ctx.reg_alloc.UseScratchGpr(carry_arg).cvt32();

            code.mov(result, 63);
            code.cmp(code.cl, 63);
            code.cmovnb(code.ecx, result);
            code.movsxd(result.cvt64(), operand);
            code.bt(carry.cvt32(), 0);
            code.sar(result.cvt64(), code.cl);
            code.setc(carry.cvt8());

            ctx.reg_alloc.DefineValue(carry_inst, carry);
            ctx.EraseInstruction(carry_inst);
            ctx.reg_alloc.DefineValue(inst, result);
        }
    }
}

void EmitX64::EmitArithmeticShiftRight64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];

    if (shift_arg.IsImmediate()) {
        const u8 shift = shift_arg.GetImmediateU8();
        const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(operand_arg);

        code.sar(result, u8(shift < 63 ? shift : 63));

        ctx.reg_alloc.DefineValue(inst, result);
    } else if (code.HasHostFeature(HostFeature::BMI2)) {
        const Xbyak::Reg64 shift = ctx.reg_alloc.UseScratchGpr(shift_arg);
        const Xbyak::Reg64 operand = ctx.reg_alloc.UseGpr(operand_arg);
        const Xbyak::Reg64 result = ctx.reg_alloc.ScratchGpr();
        const Xbyak::Reg64 const63 = ctx.reg_alloc.ScratchGpr();

        code.mov(const63.cvt32(), 63);
        code.cmp(shift.cvt8(), 63);
        code.cmovnb(shift, const63);
        code.sarx(result, operand, shift);

        ctx.reg_alloc.DefineValue(inst, result);
    } else {
        ctx.reg_alloc.UseScratch(shift_arg, HostLoc::RCX);
        const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(operand_arg);
        const Xbyak::Reg64 const63 = ctx.reg_alloc.ScratchGpr();

        // The 64-bit x64 SAR instruction masks the shift count by 0x3F before performing the shift.
        // ARM differs from the behaviour: It does not mask the count.

        // We note that all shift values above 63 have the same behaviour as 63 does, so we saturate `shift` to 63.
        code.mov(const63, 63);
        code.cmp(code.cl, u32(63));
        code.cmovnb(code.ecx, const63);
        code.sar(result, code.cl);

        ctx.reg_alloc.DefineValue(inst, result);
    }
}

void EmitX64::EmitRotateRight32(EmitContext& ctx, IR::Inst* inst) {
    const auto carry_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetCarryFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];
    auto& carry_arg = args[2];

    if (!carry_inst) {
        if (shift_arg.IsImmediate() && code.HasHostFeature(HostFeature::BMI2)) {
            const u8 shift = shift_arg.GetImmediateU8();
            const Xbyak::Reg32 operand = ctx.reg_alloc.UseGpr(operand_arg).cvt32();
            const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();

            code.rorx(result, operand, shift);

            ctx.reg_alloc.DefineValue(inst, result);
        } else if (shift_arg.IsImmediate()) {
            const u8 shift = shift_arg.GetImmediateU8();
            const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();

            code.ror(result, u8(shift & 0x1F));

            ctx.reg_alloc.DefineValue(inst, result);
        } else {
            ctx.reg_alloc.Use(shift_arg, HostLoc::RCX);
            const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();

            // x64 ROR instruction does (shift & 0x1F) for us.
            code.ror(result, code.cl);

            ctx.reg_alloc.DefineValue(inst, result);
        }
    } else {
        if (shift_arg.IsImmediate()) {
            const u8 shift = shift_arg.GetImmediateU8();
            const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();
            const Xbyak::Reg8 carry = ctx.reg_alloc.UseScratchGpr(carry_arg).cvt8();

            if (shift == 0) {
                // There is nothing more to do.
            } else if ((shift & 0x1F) == 0) {
                code.bt(result, u8(31));
                code.setc(carry);
            } else {
                code.ror(result, shift);
                code.setc(carry);
            }

            ctx.reg_alloc.DefineValue(carry_inst, carry);
            ctx.EraseInstruction(carry_inst);
            ctx.reg_alloc.DefineValue(inst, result);
        } else {
            ctx.reg_alloc.UseScratch(shift_arg, HostLoc::RCX);
            const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();
            const Xbyak::Reg8 carry = ctx.reg_alloc.UseScratchGpr(carry_arg).cvt8();

            Xbyak::Label end;

            code.test(code.cl, code.cl);
            code.jz(end);

            code.ror(result, code.cl);
            code.bt(result, u8(31));
            code.setc(carry);

            code.L(end);

            ctx.reg_alloc.DefineValue(carry_inst, carry);
            ctx.EraseInstruction(carry_inst);
            ctx.reg_alloc.DefineValue(inst, result);
        }
    }
}

void EmitX64::EmitRotateRight64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];

    if (shift_arg.IsImmediate() && code.HasHostFeature(HostFeature::BMI2)) {
        const u8 shift = shift_arg.GetImmediateU8();
        const Xbyak::Reg64 operand = ctx.reg_alloc.UseGpr(operand_arg);
        const Xbyak::Reg64 result = ctx.reg_alloc.ScratchGpr();

        code.rorx(result, operand, shift);

        ctx.reg_alloc.DefineValue(inst, result);
    } else if (shift_arg.IsImmediate()) {
        const u8 shift = shift_arg.GetImmediateU8();
        const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(operand_arg);

        code.ror(result, u8(shift & 0x3F));

        ctx.reg_alloc.DefineValue(inst, result);
    } else {
        ctx.reg_alloc.Use(shift_arg, HostLoc::RCX);
        const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(operand_arg);

        // x64 ROR instruction does (shift & 0x3F) for us.
        code.ror(result, code.cl);

        ctx.reg_alloc.DefineValue(inst, result);
    }
}

void EmitX64::EmitRotateRightExtended(EmitContext& ctx, IR::Inst* inst) {
    const auto carry_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetCarryFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
    const Xbyak::Reg8 carry = ctx.reg_alloc.UseScratchGpr(args[1]).cvt8();

    code.bt(carry.cvt32(), 0);
    code.rcr(result, 1);

    if (carry_inst) {
        code.setc(carry);

        ctx.reg_alloc.DefineValue(carry_inst, carry);
        ctx.EraseInstruction(carry_inst);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

template<typename ShfitFT, typename BMI2FT>
static void EmitMaskedShift32(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, ShfitFT shift_fn, [[maybe_unused]] BMI2FT bmi2_shift) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];

    if (shift_arg.IsImmediate()) {
        const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();
        const u32 shift = shift_arg.GetImmediateU32();

        shift_fn(result, static_cast<int>(shift & 0x1F));

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    if constexpr (!std::is_same_v<BMI2FT, std::nullptr_t>) {
        if (code.HasHostFeature(HostFeature::BMI2)) {
            const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();
            const Xbyak::Reg32 operand = ctx.reg_alloc.UseGpr(operand_arg).cvt32();
            const Xbyak::Reg32 shift = ctx.reg_alloc.UseGpr(shift_arg).cvt32();

            (code.*bmi2_shift)(result, operand, shift);

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }
    }

    ctx.reg_alloc.Use(shift_arg, HostLoc::RCX);
    const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(operand_arg).cvt32();

    shift_fn(result, code.cl);

    ctx.reg_alloc.DefineValue(inst, result);
}

template<typename ShfitFT, typename BMI2FT>
static void EmitMaskedShift64(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, ShfitFT shift_fn, [[maybe_unused]] BMI2FT bmi2_shift) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& operand_arg = args[0];
    auto& shift_arg = args[1];

    if (shift_arg.IsImmediate()) {
        const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(operand_arg);
        const u64 shift = shift_arg.GetImmediateU64();

        shift_fn(result, static_cast<int>(shift & 0x3F));

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    if constexpr (!std::is_same_v<BMI2FT, std::nullptr_t>) {
        if (code.HasHostFeature(HostFeature::BMI2)) {
            const Xbyak::Reg64 result = ctx.reg_alloc.ScratchGpr();
            const Xbyak::Reg64 operand = ctx.reg_alloc.UseGpr(operand_arg);
            const Xbyak::Reg64 shift = ctx.reg_alloc.UseGpr(shift_arg);

            (code.*bmi2_shift)(result, operand, shift);

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }
    }

    ctx.reg_alloc.Use(shift_arg, HostLoc::RCX);
    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(operand_arg);

    shift_fn(result, code.cl);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitLogicalShiftLeftMasked32(EmitContext& ctx, IR::Inst* inst) {
    EmitMaskedShift32(
        code, ctx, inst, [&](auto result, auto shift) { code.shl(result, shift); }, &Xbyak::CodeGenerator::shlx);
}

void EmitX64::EmitLogicalShiftLeftMasked64(EmitContext& ctx, IR::Inst* inst) {
    EmitMaskedShift64(
        code, ctx, inst, [&](auto result, auto shift) { code.shl(result, shift); }, &Xbyak::CodeGenerator::shlx);
}

void EmitX64::EmitLogicalShiftRightMasked32(EmitContext& ctx, IR::Inst* inst) {
    EmitMaskedShift32(
        code, ctx, inst, [&](auto result, auto shift) { code.shr(result, shift); }, &Xbyak::CodeGenerator::shrx);
}

void EmitX64::EmitLogicalShiftRightMasked64(EmitContext& ctx, IR::Inst* inst) {
    EmitMaskedShift64(
        code, ctx, inst, [&](auto result, auto shift) { code.shr(result, shift); }, &Xbyak::CodeGenerator::shrx);
}

void EmitX64::EmitArithmeticShiftRightMasked32(EmitContext& ctx, IR::Inst* inst) {
    EmitMaskedShift32(
        code, ctx, inst, [&](auto result, auto shift) { code.sar(result, shift); }, &Xbyak::CodeGenerator::sarx);
}

void EmitX64::EmitArithmeticShiftRightMasked64(EmitContext& ctx, IR::Inst* inst) {
    EmitMaskedShift64(
        code, ctx, inst, [&](auto result, auto shift) { code.sar(result, shift); }, &Xbyak::CodeGenerator::sarx);
}

void EmitX64::EmitRotateRightMasked32(EmitContext& ctx, IR::Inst* inst) {
    EmitMaskedShift32(
        code, ctx, inst, [&](auto result, auto shift) { code.ror(result, shift); }, nullptr);
}

void EmitX64::EmitRotateRightMasked64(EmitContext& ctx, IR::Inst* inst) {
    EmitMaskedShift64(
        code, ctx, inst, [&](auto result, auto shift) { code.ror(result, shift); }, nullptr);
}

static Xbyak::Reg8 DoCarry(RegAlloc& reg_alloc, Argument& carry_in, IR::Inst* carry_out) {
    if (carry_in.IsImmediate()) {
        return carry_out ? reg_alloc.ScratchGpr().cvt8() : Xbyak::Reg8{-1};
    } else {
        return carry_out ? reg_alloc.UseScratchGpr(carry_in).cvt8() : reg_alloc.UseGpr(carry_in).cvt8();
    }
}

static Xbyak::Reg64 DoNZCV(BlockOfCode& code, RegAlloc& reg_alloc, IR::Inst* nzcv_out) {
    if (!nzcv_out) {
        return Xbyak::Reg64{-1};
    }

    const Xbyak::Reg64 nzcv = reg_alloc.ScratchGpr(HostLoc::RAX);
    code.xor_(nzcv.cvt32(), nzcv.cvt32());
    return nzcv;
}

static void EmitAdd(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, int bitsize) {
    const auto carry_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetCarryFromOp);
    const auto overflow_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetOverflowFromOp);
    const auto nzcv_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetNZCVFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& carry_in = args[2];

    const Xbyak::Reg64 nzcv = DoNZCV(code, ctx.reg_alloc, nzcv_inst);
    const Xbyak::Reg result = ctx.reg_alloc.UseScratchGpr(args[0]).changeBit(bitsize);
    const Xbyak::Reg8 carry = DoCarry(ctx.reg_alloc, carry_in, carry_inst);
    const Xbyak::Reg8 overflow = overflow_inst ? ctx.reg_alloc.ScratchGpr().cvt8() : Xbyak::Reg8{-1};

    // TODO: Consider using LEA.

    if (args[1].IsImmediate() && args[1].GetType() == IR::Type::U32) {
        const u32 op_arg = args[1].GetImmediateU32();
        if (carry_in.IsImmediate()) {
            if (carry_in.GetImmediateU1()) {
                code.stc();
                code.adc(result, op_arg);
            } else {
                code.add(result, op_arg);
            }
        } else {
            code.bt(carry.cvt32(), 0);
            code.adc(result, op_arg);
        }
    } else {
        OpArg op_arg = ctx.reg_alloc.UseOpArg(args[1]);
        op_arg.setBit(bitsize);
        if (carry_in.IsImmediate()) {
            if (carry_in.GetImmediateU1()) {
                code.stc();
                code.adc(result, *op_arg);
            } else {
                code.add(result, *op_arg);
            }
        } else {
            code.bt(carry.cvt32(), 0);
            code.adc(result, *op_arg);
        }
    }

    if (nzcv_inst) {
        code.lahf();
        code.seto(code.al);
        ctx.reg_alloc.DefineValue(nzcv_inst, nzcv);
        ctx.EraseInstruction(nzcv_inst);
    }
    if (carry_inst) {
        code.setc(carry);
        ctx.reg_alloc.DefineValue(carry_inst, carry);
        ctx.EraseInstruction(carry_inst);
    }
    if (overflow_inst) {
        code.seto(overflow);
        ctx.reg_alloc.DefineValue(overflow_inst, overflow);
        ctx.EraseInstruction(overflow_inst);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitAdd32(EmitContext& ctx, IR::Inst* inst) {
    EmitAdd(code, ctx, inst, 32);
}

void EmitX64::EmitAdd64(EmitContext& ctx, IR::Inst* inst) {
    EmitAdd(code, ctx, inst, 64);
}

static void EmitSub(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, int bitsize) {
    const auto carry_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetCarryFromOp);
    const auto overflow_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetOverflowFromOp);
    const auto nzcv_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetNZCVFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto& carry_in = args[2];
    const bool is_cmp = inst->UseCount() == size_t(!!carry_inst + !!overflow_inst + !!nzcv_inst) && carry_in.IsImmediate() && carry_in.GetImmediateU1();

    const Xbyak::Reg64 nzcv = DoNZCV(code, ctx.reg_alloc, nzcv_inst);
    const Xbyak::Reg result = (is_cmp ? ctx.reg_alloc.UseGpr(args[0]) : ctx.reg_alloc.UseScratchGpr(args[0])).changeBit(bitsize);
    const Xbyak::Reg8 carry = DoCarry(ctx.reg_alloc, carry_in, carry_inst);
    const Xbyak::Reg8 overflow = overflow_inst ? ctx.reg_alloc.ScratchGpr().cvt8() : Xbyak::Reg8{-1};

    // TODO: Consider using LEA.
    // Note that x64 CF is inverse of what the ARM carry flag is here.

    bool invert_output_carry = true;

    if (is_cmp) {
        if (args[1].IsImmediate() && args[1].GetType() == IR::Type::U32) {
            const u32 op_arg = args[1].GetImmediateU32();
            code.cmp(result, op_arg);
        } else {
            OpArg op_arg = ctx.reg_alloc.UseOpArg(args[1]);
            op_arg.setBit(bitsize);
            code.cmp(result, *op_arg);
        }
    } else if (args[1].IsImmediate() && args[1].GetType() == IR::Type::U32) {
        const u32 op_arg = args[1].GetImmediateU32();
        if (carry_in.IsImmediate()) {
            if (carry_in.GetImmediateU1()) {
                code.sub(result, op_arg);
            } else {
                code.add(result, ~op_arg);
                invert_output_carry = false;
            }
        } else {
            code.bt(carry.cvt32(), 0);
            code.adc(result, ~op_arg);
            invert_output_carry = false;
        }
    } else {
        OpArg op_arg = ctx.reg_alloc.UseOpArg(args[1]);
        op_arg.setBit(bitsize);
        if (carry_in.IsImmediate()) {
            if (carry_in.GetImmediateU1()) {
                code.sub(result, *op_arg);
            } else {
                code.stc();
                code.sbb(result, *op_arg);
            }
        } else {
            code.bt(carry.cvt32(), 0);
            code.cmc();
            code.sbb(result, *op_arg);
        }
    }

    if (nzcv_inst) {
        if (invert_output_carry) {
            code.cmc();
        }
        code.lahf();
        code.seto(code.al);
        ctx.reg_alloc.DefineValue(nzcv_inst, nzcv);
        ctx.EraseInstruction(nzcv_inst);
    }
    if (carry_inst) {
        if (invert_output_carry) {
            code.setnc(carry);
        } else {
            code.setc(carry);
        }
        ctx.reg_alloc.DefineValue(carry_inst, carry);
        ctx.EraseInstruction(carry_inst);
    }
    if (overflow_inst) {
        code.seto(overflow);
        ctx.reg_alloc.DefineValue(overflow_inst, overflow);
        ctx.EraseInstruction(overflow_inst);
    }
    if (!is_cmp) {
        ctx.reg_alloc.DefineValue(inst, result);
    }
}

void EmitX64::EmitSub32(EmitContext& ctx, IR::Inst* inst) {
    EmitSub(code, ctx, inst, 32);
}

void EmitX64::EmitSub64(EmitContext& ctx, IR::Inst* inst) {
    EmitSub(code, ctx, inst, 64);
}

void EmitX64::EmitMul32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
    if (args[1].IsImmediate()) {
        code.imul(result, result, args[1].GetImmediateU32());
    } else {
        OpArg op_arg = ctx.reg_alloc.UseOpArg(args[1]);
        op_arg.setBit(32);

        code.imul(result, *op_arg);
    }
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitMul64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);
    OpArg op_arg = ctx.reg_alloc.UseOpArg(args[1]);

    code.imul(result, *op_arg);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitUnsignedMultiplyHigh64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    ctx.reg_alloc.ScratchGpr(HostLoc::RDX);
    ctx.reg_alloc.UseScratch(args[0], HostLoc::RAX);
    OpArg op_arg = ctx.reg_alloc.UseOpArg(args[1]);
    code.mul(*op_arg);

    ctx.reg_alloc.DefineValue(inst, rdx);
}

void EmitX64::EmitSignedMultiplyHigh64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    ctx.reg_alloc.ScratchGpr(HostLoc::RDX);
    ctx.reg_alloc.UseScratch(args[0], HostLoc::RAX);
    OpArg op_arg = ctx.reg_alloc.UseOpArg(args[1]);
    code.imul(*op_arg);

    ctx.reg_alloc.DefineValue(inst, rdx);
}

void EmitX64::EmitUnsignedDiv32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    ctx.reg_alloc.ScratchGpr(HostLoc::RAX);
    ctx.reg_alloc.ScratchGpr(HostLoc::RDX);
    const Xbyak::Reg32 dividend = ctx.reg_alloc.UseGpr(args[0]).cvt32();
    const Xbyak::Reg32 divisor = ctx.reg_alloc.UseGpr(args[1]).cvt32();

    Xbyak::Label end;

    code.xor_(eax, eax);
    code.test(divisor, divisor);
    code.jz(end);
    code.mov(eax, dividend);
    code.xor_(edx, edx);
    code.div(divisor);
    code.L(end);

    ctx.reg_alloc.DefineValue(inst, eax);
}

void EmitX64::EmitUnsignedDiv64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    ctx.reg_alloc.ScratchGpr(HostLoc::RAX);
    ctx.reg_alloc.ScratchGpr(HostLoc::RDX);
    const Xbyak::Reg64 dividend = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Reg64 divisor = ctx.reg_alloc.UseGpr(args[1]);

    Xbyak::Label end;

    code.xor_(eax, eax);
    code.test(divisor, divisor);
    code.jz(end);
    code.mov(rax, dividend);
    code.xor_(edx, edx);
    code.div(divisor);
    code.L(end);

    ctx.reg_alloc.DefineValue(inst, rax);
}

void EmitX64::EmitSignedDiv32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    ctx.reg_alloc.ScratchGpr(HostLoc::RAX);
    ctx.reg_alloc.ScratchGpr(HostLoc::RDX);
    const Xbyak::Reg32 dividend = ctx.reg_alloc.UseGpr(args[0]).cvt32();
    const Xbyak::Reg32 divisor = ctx.reg_alloc.UseGpr(args[1]).cvt32();

    Xbyak::Label end;

    code.xor_(eax, eax);
    code.test(divisor, divisor);
    code.jz(end);
    code.mov(eax, dividend);
    code.cdq();
    code.idiv(divisor);
    code.L(end);

    ctx.reg_alloc.DefineValue(inst, eax);
}

void EmitX64::EmitSignedDiv64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    ctx.reg_alloc.ScratchGpr(HostLoc::RAX);
    ctx.reg_alloc.ScratchGpr(HostLoc::RDX);
    const Xbyak::Reg64 dividend = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Reg64 divisor = ctx.reg_alloc.UseGpr(args[1]);

    Xbyak::Label end;

    code.xor_(eax, eax);
    code.test(divisor, divisor);
    code.jz(end);
    code.mov(rax, dividend);
    code.cqo();
    code.idiv(divisor);
    code.L(end);

    ctx.reg_alloc.DefineValue(inst, rax);
}

void EmitX64::EmitAnd32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();

    if (args[1].IsImmediate()) {
        const u32 op_arg = args[1].GetImmediateU32();

        code.and_(result, op_arg);
    } else {
        OpArg op_arg = ctx.reg_alloc.UseOpArg(args[1]);
        op_arg.setBit(32);

        code.and_(result, *op_arg);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitAnd64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);

    if (args[1].FitsInImmediateS32()) {
        const u32 op_arg = u32(args[1].GetImmediateS32());

        code.and_(result, op_arg);
    } else {
        OpArg op_arg = ctx.reg_alloc.UseOpArg(args[1]);
        op_arg.setBit(64);

        code.and_(result, *op_arg);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitAndNot32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (!args[0].IsImmediate() && !args[1].IsImmediate() && code.HasHostFeature(HostFeature::BMI1)) {
        Xbyak::Reg32 op_a = ctx.reg_alloc.UseGpr(args[0]).cvt32();
        Xbyak::Reg32 op_b = ctx.reg_alloc.UseGpr(args[1]).cvt32();
        Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();
        code.andn(result, op_b, op_a);
        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    Xbyak::Reg32 result;
    if (args[1].IsImmediate()) {
        result = ctx.reg_alloc.ScratchGpr().cvt32();
        code.mov(result, u32(~args[1].GetImmediateU32()));
    } else {
        result = ctx.reg_alloc.UseScratchGpr(args[1]).cvt32();
        code.not_(result);
    }

    if (args[0].IsImmediate()) {
        const u32 op_arg = args[0].GetImmediateU32();
        code.and_(result, op_arg);
    } else {
        OpArg op_arg = ctx.reg_alloc.UseOpArg(args[0]);
        op_arg.setBit(32);
        code.and_(result, *op_arg);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitAndNot64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (!args[0].IsImmediate() && !args[1].IsImmediate() && code.HasHostFeature(HostFeature::BMI1)) {
        Xbyak::Reg64 op_a = ctx.reg_alloc.UseGpr(args[0]);
        Xbyak::Reg64 op_b = ctx.reg_alloc.UseGpr(args[1]);
        Xbyak::Reg64 result = ctx.reg_alloc.ScratchGpr();
        code.andn(result, op_b, op_a);
        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    Xbyak::Reg64 result;
    if (args[1].IsImmediate()) {
        result = ctx.reg_alloc.ScratchGpr();
        code.mov(result, ~args[1].GetImmediateU64());
    } else {
        result = ctx.reg_alloc.UseScratchGpr(args[1]);
        code.not_(result);
    }

    if (args[0].FitsInImmediateS32()) {
        const u32 op_arg = u32(args[0].GetImmediateS32());
        code.and_(result, op_arg);
    } else {
        OpArg op_arg = ctx.reg_alloc.UseOpArg(args[0]);
        op_arg.setBit(64);
        code.and_(result, *op_arg);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitEor32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();

    if (args[1].IsImmediate()) {
        const u32 op_arg = args[1].GetImmediateU32();

        code.xor_(result, op_arg);
    } else {
        OpArg op_arg = ctx.reg_alloc.UseOpArg(args[1]);
        op_arg.setBit(32);

        code.xor_(result, *op_arg);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitEor64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);

    if (args[1].FitsInImmediateS32()) {
        const u32 op_arg = u32(args[1].GetImmediateS32());

        code.xor_(result, op_arg);
    } else {
        OpArg op_arg = ctx.reg_alloc.UseOpArg(args[1]);
        op_arg.setBit(64);

        code.xor_(result, *op_arg);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitOr32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();

    if (args[1].IsImmediate()) {
        const u32 op_arg = args[1].GetImmediateU32();

        code.or_(result, op_arg);
    } else {
        OpArg op_arg = ctx.reg_alloc.UseOpArg(args[1]);
        op_arg.setBit(32);

        code.or_(result, *op_arg);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitOr64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);

    if (args[1].FitsInImmediateS32()) {
        const u32 op_arg = u32(args[1].GetImmediateS32());

        code.or_(result, op_arg);
    } else {
        OpArg op_arg = ctx.reg_alloc.UseOpArg(args[1]);
        op_arg.setBit(64);

        code.or_(result, *op_arg);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitNot32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    Xbyak::Reg32 result;
    if (args[0].IsImmediate()) {
        result = ctx.reg_alloc.ScratchGpr().cvt32();
        code.mov(result, u32(~args[0].GetImmediateU32()));
    } else {
        result = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        code.not_(result);
    }
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitNot64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    Xbyak::Reg64 result;
    if (args[0].IsImmediate()) {
        result = ctx.reg_alloc.ScratchGpr();
        code.mov(result, ~args[0].GetImmediateU64());
    } else {
        result = ctx.reg_alloc.UseScratchGpr(args[0]);
        code.not_(result);
    }
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitSignExtendByteToWord(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);
    code.movsx(result.cvt32(), result.cvt8());
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitSignExtendHalfToWord(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);
    code.movsx(result.cvt32(), result.cvt16());
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitSignExtendByteToLong(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);
    code.movsx(result.cvt64(), result.cvt8());
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitSignExtendHalfToLong(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);
    code.movsx(result.cvt64(), result.cvt16());
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitSignExtendWordToLong(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);
    code.movsxd(result.cvt64(), result.cvt32());
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitZeroExtendByteToWord(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);
    code.movzx(result.cvt32(), result.cvt8());
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitZeroExtendHalfToWord(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);
    code.movzx(result.cvt32(), result.cvt16());
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitZeroExtendByteToLong(EmitContext& ctx, IR::Inst* inst) {
    // x64 zeros upper 32 bits on a 32-bit move
    EmitZeroExtendByteToWord(ctx, inst);
}

void EmitX64::EmitZeroExtendHalfToLong(EmitContext& ctx, IR::Inst* inst) {
    // x64 zeros upper 32 bits on a 32-bit move
    EmitZeroExtendHalfToWord(ctx, inst);
}

void EmitX64::EmitZeroExtendWordToLong(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);
    code.mov(result.cvt32(), result.cvt32());  // x64 zeros upper 32 bits on a 32-bit move
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitZeroExtendLongToQuad(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    if (args[0].IsInGpr()) {
        const Xbyak::Reg64 source = ctx.reg_alloc.UseGpr(args[0]);
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        code.movq(result, source);
        ctx.reg_alloc.DefineValue(inst, result);
    } else {
        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
        code.movq(result, result);
        ctx.reg_alloc.DefineValue(inst, result);
    }
}

void EmitX64::EmitByteReverseWord(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg32 result = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
    code.bswap(result);
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitByteReverseHalf(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg16 result = ctx.reg_alloc.UseScratchGpr(args[0]).cvt16();
    code.rol(result, 8);
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitByteReverseDual(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Reg64 result = ctx.reg_alloc.UseScratchGpr(args[0]);
    code.bswap(result);
    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitCountLeadingZeros32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    if (code.HasHostFeature(HostFeature::LZCNT)) {
        const Xbyak::Reg32 source = ctx.reg_alloc.UseGpr(args[0]).cvt32();
        const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();

        code.lzcnt(result, source);

        ctx.reg_alloc.DefineValue(inst, result);
    } else {
        const Xbyak::Reg32 source = ctx.reg_alloc.UseScratchGpr(args[0]).cvt32();
        const Xbyak::Reg32 result = ctx.reg_alloc.ScratchGpr().cvt32();

        // The result of a bsr of zero is undefined, but zf is set after it.
        code.bsr(result, source);
        code.mov(source, 0xFFFFFFFF);
        code.cmovz(result, source);
        code.neg(result);
        code.add(result, 31);

        ctx.reg_alloc.DefineValue(inst, result);
    }
}

void EmitX64::EmitCountLeadingZeros64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    if (code.HasHostFeature(HostFeature::LZCNT)) {
        const Xbyak::Reg64 source = ctx.reg_alloc.UseGpr(args[0]).cvt64();
        const Xbyak::Reg64 result = ctx.reg_alloc.ScratchGpr().cvt64();

        code.lzcnt(result, source);

        ctx.reg_alloc.DefineValue(inst, result);
    } else {
        const Xbyak::Reg64 source = ctx.reg_alloc.UseScratchGpr(args[0]).cvt64();
        const Xbyak::Reg64 result = ctx.reg_alloc.ScratchGpr().cvt64();

        // The result of a bsr of zero is undefined, but zf is set after it.
        code.bsr(result, source);
        code.mov(source.cvt32(), 0xFFFFFFFF);
        code.cmovz(result.cvt32(), source.cvt32());
        code.neg(result.cvt32());
        code.add(result.cvt32(), 63);

        ctx.reg_alloc.DefineValue(inst, result);
    }
}

void EmitX64::EmitMaxSigned32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 x = ctx.reg_alloc.UseGpr(args[0]).cvt32();
    const Xbyak::Reg32 y = ctx.reg_alloc.UseScratchGpr(args[1]).cvt32();

    code.cmp(x, y);
    code.cmovge(y, x);

    ctx.reg_alloc.DefineValue(inst, y);
}

void EmitX64::EmitMaxSigned64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg64 x = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Reg64 y = ctx.reg_alloc.UseScratchGpr(args[1]);

    code.cmp(x, y);
    code.cmovge(y, x);

    ctx.reg_alloc.DefineValue(inst, y);
}

void EmitX64::EmitMaxUnsigned32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 x = ctx.reg_alloc.UseGpr(args[0]).cvt32();
    const Xbyak::Reg32 y = ctx.reg_alloc.UseScratchGpr(args[1]).cvt32();

    code.cmp(x, y);
    code.cmova(y, x);

    ctx.reg_alloc.DefineValue(inst, y);
}

void EmitX64::EmitMaxUnsigned64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg64 x = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Reg64 y = ctx.reg_alloc.UseScratchGpr(args[1]);

    code.cmp(x, y);
    code.cmova(y, x);

    ctx.reg_alloc.DefineValue(inst, y);
}

void EmitX64::EmitMinSigned32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 x = ctx.reg_alloc.UseGpr(args[0]).cvt32();
    const Xbyak::Reg32 y = ctx.reg_alloc.UseScratchGpr(args[1]).cvt32();

    code.cmp(x, y);
    code.cmovle(y, x);

    ctx.reg_alloc.DefineValue(inst, y);
}

void EmitX64::EmitMinSigned64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg64 x = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Reg64 y = ctx.reg_alloc.UseScratchGpr(args[1]);

    code.cmp(x, y);
    code.cmovle(y, x);

    ctx.reg_alloc.DefineValue(inst, y);
}

void EmitX64::EmitMinUnsigned32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 x = ctx.reg_alloc.UseGpr(args[0]).cvt32();
    const Xbyak::Reg32 y = ctx.reg_alloc.UseScratchGpr(args[1]).cvt32();

    code.cmp(x, y);
    code.cmovb(y, x);

    ctx.reg_alloc.DefineValue(inst, y);
}

void EmitX64::EmitMinUnsigned64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg64 x = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Reg64 y = ctx.reg_alloc.UseScratchGpr(args[1]);

    code.cmp(x, y);
    code.cmovb(y, x);

    ctx.reg_alloc.DefineValue(inst, y);
}

}  // namespace Dynarmic::Backend::X64

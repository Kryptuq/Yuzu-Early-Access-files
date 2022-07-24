/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <algorithm>
#include <bitset>
#include <cstdlib>
#include <type_traits>

#include <mcl/assert.hpp>
#include <mcl/bit/bit_count.hpp>
#include <mcl/bit/bit_field.hpp>
#include <mcl/bitsizeof.hpp>
#include <mcl/stdint.hpp>
#include <mcl/type_traits/function_info.hpp>
#include <xbyak/xbyak.h>

#include "dynarmic/backend/x64/abi.h"
#include "dynarmic/backend/x64/block_of_code.h"
#include "dynarmic/backend/x64/constants.h"
#include "dynarmic/backend/x64/emit_x64.h"
#include "dynarmic/common/math_util.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::Backend::X64 {

using namespace Xbyak::util;

template<typename Function>
static void EmitVectorOperation(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, Function fn) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);

    (code.*fn)(xmm_a, xmm_b);

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

template<typename Function>
static void EmitAVXVectorOperation(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, Function fn) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);

    (code.*fn)(xmm_a, xmm_a, xmm_b);

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

template<typename Lambda>
static void EmitOneArgumentFallback(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, Lambda lambda) {
    const auto fn = static_cast<mcl::equivalent_function_type<Lambda>*>(lambda);
    constexpr u32 stack_space = 2 * 16;
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm arg1 = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    ctx.reg_alloc.EndOfAllocScope();

    ctx.reg_alloc.HostCall(nullptr);
    ctx.reg_alloc.AllocStackSpace(stack_space + ABI_SHADOW_SPACE);
    code.lea(code.ABI_PARAM1, ptr[rsp + ABI_SHADOW_SPACE + 0 * 16]);
    code.lea(code.ABI_PARAM2, ptr[rsp + ABI_SHADOW_SPACE + 1 * 16]);

    code.movaps(xword[code.ABI_PARAM2], arg1);
    code.CallFunction(fn);
    code.movaps(result, xword[rsp + ABI_SHADOW_SPACE + 0 * 16]);

    ctx.reg_alloc.ReleaseStackSpace(stack_space + ABI_SHADOW_SPACE);

    ctx.reg_alloc.DefineValue(inst, result);
}

template<typename Lambda>
static void EmitOneArgumentFallbackWithSaturation(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, Lambda lambda) {
    const auto fn = static_cast<mcl::equivalent_function_type<Lambda>*>(lambda);
    constexpr u32 stack_space = 2 * 16;
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm arg1 = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    ctx.reg_alloc.EndOfAllocScope();

    ctx.reg_alloc.HostCall(nullptr);
    ctx.reg_alloc.AllocStackSpace(stack_space + ABI_SHADOW_SPACE);
    code.lea(code.ABI_PARAM1, ptr[rsp + ABI_SHADOW_SPACE + 0 * 16]);
    code.lea(code.ABI_PARAM2, ptr[rsp + ABI_SHADOW_SPACE + 1 * 16]);

    code.movaps(xword[code.ABI_PARAM2], arg1);
    code.CallFunction(fn);
    code.movaps(result, xword[rsp + ABI_SHADOW_SPACE + 0 * 16]);

    ctx.reg_alloc.ReleaseStackSpace(stack_space + ABI_SHADOW_SPACE);

    code.or_(code.byte[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], code.ABI_RETURN.cvt8());

    ctx.reg_alloc.DefineValue(inst, result);
}

template<typename Lambda>
static void EmitTwoArgumentFallbackWithSaturation(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, Lambda lambda) {
    const auto fn = static_cast<mcl::equivalent_function_type<Lambda>*>(lambda);
    constexpr u32 stack_space = 3 * 16;
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm arg1 = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm arg2 = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    ctx.reg_alloc.EndOfAllocScope();

    ctx.reg_alloc.HostCall(nullptr);
    ctx.reg_alloc.AllocStackSpace(stack_space + ABI_SHADOW_SPACE);
    code.lea(code.ABI_PARAM1, ptr[rsp + ABI_SHADOW_SPACE + 0 * 16]);
    code.lea(code.ABI_PARAM2, ptr[rsp + ABI_SHADOW_SPACE + 1 * 16]);
    code.lea(code.ABI_PARAM3, ptr[rsp + ABI_SHADOW_SPACE + 2 * 16]);

    code.movaps(xword[code.ABI_PARAM2], arg1);
    code.movaps(xword[code.ABI_PARAM3], arg2);
    code.CallFunction(fn);
    code.movaps(result, xword[rsp + ABI_SHADOW_SPACE + 0 * 16]);

    ctx.reg_alloc.ReleaseStackSpace(stack_space + ABI_SHADOW_SPACE);

    code.or_(code.byte[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], code.ABI_RETURN.cvt8());

    ctx.reg_alloc.DefineValue(inst, result);
}

template<typename Lambda>
static void EmitTwoArgumentFallback(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, Lambda lambda) {
    const auto fn = static_cast<mcl::equivalent_function_type<Lambda>*>(lambda);
    constexpr u32 stack_space = 3 * 16;
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm arg1 = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm arg2 = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    ctx.reg_alloc.EndOfAllocScope();

    ctx.reg_alloc.HostCall(nullptr);
    ctx.reg_alloc.AllocStackSpace(stack_space + ABI_SHADOW_SPACE);
    code.lea(code.ABI_PARAM1, ptr[rsp + ABI_SHADOW_SPACE + 0 * 16]);
    code.lea(code.ABI_PARAM2, ptr[rsp + ABI_SHADOW_SPACE + 1 * 16]);
    code.lea(code.ABI_PARAM3, ptr[rsp + ABI_SHADOW_SPACE + 2 * 16]);

    code.movaps(xword[code.ABI_PARAM2], arg1);
    code.movaps(xword[code.ABI_PARAM3], arg2);
    code.CallFunction(fn);
    code.movaps(result, xword[rsp + ABI_SHADOW_SPACE + 0 * 16]);

    ctx.reg_alloc.ReleaseStackSpace(stack_space + ABI_SHADOW_SPACE);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorGetElement8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();

    // TODO: DefineValue directly on Argument for index == 0

    const Xbyak::Xmm source = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Reg32 dest = ctx.reg_alloc.ScratchGpr().cvt32();

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.pextrb(dest, source, index);
    } else {
        code.pextrw(dest, source, u8(index / 2));
        if (index % 2 == 1) {
            code.shr(dest, 8);
        } else {
            code.and_(dest, 0xFF);  // TODO: Remove when zext handling is corrected
        }
    }

    ctx.reg_alloc.DefineValue(inst, dest);
}

void EmitX64::EmitVectorGetElement16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();

    // TODO: DefineValue directly on Argument for index == 0

    const Xbyak::Xmm source = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Reg32 dest = ctx.reg_alloc.ScratchGpr().cvt32();
    code.pextrw(dest, source, index);
    ctx.reg_alloc.DefineValue(inst, dest);
}

void EmitX64::EmitVectorGetElement32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();

    // TODO: DefineValue directly on Argument for index == 0

    const Xbyak::Reg32 dest = ctx.reg_alloc.ScratchGpr().cvt32();

    if (code.HasHostFeature(HostFeature::SSE41)) {
        const Xbyak::Xmm source = ctx.reg_alloc.UseXmm(args[0]);
        code.pextrd(dest, source, index);
    } else {
        const Xbyak::Xmm source = ctx.reg_alloc.UseScratchXmm(args[0]);
        code.pshufd(source, source, index);
        code.movd(dest, source);
    }

    ctx.reg_alloc.DefineValue(inst, dest);
}

void EmitX64::EmitVectorGetElement64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();

    if (index == 0) {
        // TODO: DefineValue directly on Argument for index == 0
        const Xbyak::Reg64 dest = ctx.reg_alloc.ScratchGpr().cvt64();
        const Xbyak::Xmm source = ctx.reg_alloc.UseXmm(args[0]);
        code.movq(dest, source);
        ctx.reg_alloc.DefineValue(inst, dest);
        return;
    }

    const Xbyak::Reg64 dest = ctx.reg_alloc.ScratchGpr().cvt64();

    if (code.HasHostFeature(HostFeature::SSE41)) {
        const Xbyak::Xmm source = ctx.reg_alloc.UseXmm(args[0]);
        code.pextrq(dest, source, 1);
    } else {
        const Xbyak::Xmm source = ctx.reg_alloc.UseScratchXmm(args[0]);
        code.punpckhqdq(source, source);
        code.movq(dest, source);
    }

    ctx.reg_alloc.DefineValue(inst, dest);
}

void EmitX64::EmitVectorSetElement8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();
    const Xbyak::Xmm source_vector = ctx.reg_alloc.UseScratchXmm(args[0]);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        const Xbyak::Reg8 source_elem = ctx.reg_alloc.UseGpr(args[2]).cvt8();

        code.pinsrb(source_vector, source_elem.cvt32(), index);

        ctx.reg_alloc.DefineValue(inst, source_vector);
    } else {
        const Xbyak::Reg32 source_elem = ctx.reg_alloc.UseScratchGpr(args[2]).cvt32();
        const Xbyak::Reg32 tmp = ctx.reg_alloc.ScratchGpr().cvt32();

        code.pextrw(tmp, source_vector, index / 2);
        if (index % 2 == 0) {
            code.and_(tmp, 0xFF00);
            code.and_(source_elem, 0x00FF);
            code.or_(tmp, source_elem);
        } else {
            code.and_(tmp, 0x00FF);
            code.shl(source_elem, 8);
            code.or_(tmp, source_elem);
        }
        code.pinsrw(source_vector, tmp, index / 2);

        ctx.reg_alloc.DefineValue(inst, source_vector);
    }
}

void EmitX64::EmitVectorSetElement16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();

    const Xbyak::Xmm source_vector = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Reg16 source_elem = ctx.reg_alloc.UseGpr(args[2]).cvt16();

    code.pinsrw(source_vector, source_elem.cvt32(), index);

    ctx.reg_alloc.DefineValue(inst, source_vector);
}

void EmitX64::EmitVectorSetElement32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();
    const Xbyak::Xmm source_vector = ctx.reg_alloc.UseScratchXmm(args[0]);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        const Xbyak::Reg32 source_elem = ctx.reg_alloc.UseGpr(args[2]).cvt32();

        code.pinsrd(source_vector, source_elem, index);

        ctx.reg_alloc.DefineValue(inst, source_vector);
    } else {
        const Xbyak::Reg32 source_elem = ctx.reg_alloc.UseScratchGpr(args[2]).cvt32();

        code.pinsrw(source_vector, source_elem, index * 2);
        code.shr(source_elem, 16);
        code.pinsrw(source_vector, source_elem, index * 2 + 1);

        ctx.reg_alloc.DefineValue(inst, source_vector);
    }
}

void EmitX64::EmitVectorSetElement64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();
    const Xbyak::Xmm source_vector = ctx.reg_alloc.UseScratchXmm(args[0]);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        const Xbyak::Reg64 source_elem = ctx.reg_alloc.UseGpr(args[2]);

        code.pinsrq(source_vector, source_elem, index);

        ctx.reg_alloc.DefineValue(inst, source_vector);
    } else {
        const Xbyak::Reg64 source_elem = ctx.reg_alloc.UseGpr(args[2]);
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.movq(tmp, source_elem);

        if (index == 0) {
            code.movsd(source_vector, tmp);
        } else {
            code.punpcklqdq(source_vector, tmp);
        }

        ctx.reg_alloc.DefineValue(inst, source_vector);
    }
}

static void VectorAbs8(BlockOfCode& code, EmitContext& ctx, const Xbyak::Xmm& data) {
    if (code.HasHostFeature(HostFeature::SSSE3)) {
        code.pabsb(data, data);
    } else {
        const Xbyak::Xmm temp = ctx.reg_alloc.ScratchXmm();
        code.pxor(temp, temp);
        code.psubb(temp, data);
        code.pminub(data, temp);
    }
}

static void VectorAbs16(BlockOfCode& code, EmitContext& ctx, const Xbyak::Xmm& data) {
    if (code.HasHostFeature(HostFeature::SSSE3)) {
        code.pabsw(data, data);
    } else {
        const Xbyak::Xmm temp = ctx.reg_alloc.ScratchXmm();
        code.pxor(temp, temp);
        code.psubw(temp, data);
        code.pmaxsw(data, temp);
    }
}

static void VectorAbs32(BlockOfCode& code, EmitContext& ctx, const Xbyak::Xmm& data) {
    if (code.HasHostFeature(HostFeature::SSSE3)) {
        code.pabsd(data, data);
    } else {
        const Xbyak::Xmm temp = ctx.reg_alloc.ScratchXmm();
        code.movdqa(temp, data);
        code.psrad(temp, 31);
        code.pxor(data, temp);
        code.psubd(data, temp);
    }
}

static void VectorAbs64(BlockOfCode& code, EmitContext& ctx, const Xbyak::Xmm& data) {
    if (code.HasHostFeature(HostFeature::AVX512_Ortho)) {
        code.vpabsq(data, data);
    } else {
        const Xbyak::Xmm temp = ctx.reg_alloc.ScratchXmm();
        code.pshufd(temp, data, 0b11110101);
        code.psrad(temp, 31);
        code.pxor(data, temp);
        code.psubq(data, temp);
    }
}

static void EmitVectorAbs(size_t esize, EmitContext& ctx, IR::Inst* inst, BlockOfCode& code) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm data = ctx.reg_alloc.UseScratchXmm(args[0]);

    switch (esize) {
    case 8:
        VectorAbs8(code, ctx, data);
        break;
    case 16:
        VectorAbs16(code, ctx, data);
        break;
    case 32:
        VectorAbs32(code, ctx, data);
        break;
    case 64:
        VectorAbs64(code, ctx, data);
        break;
    }

    ctx.reg_alloc.DefineValue(inst, data);
}

void EmitX64::EmitVectorAbs8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorAbs(8, ctx, inst, code);
}

void EmitX64::EmitVectorAbs16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorAbs(16, ctx, inst, code);
}

void EmitX64::EmitVectorAbs32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorAbs(32, ctx, inst, code);
}

void EmitX64::EmitVectorAbs64(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorAbs(64, ctx, inst, code);
}

void EmitX64::EmitVectorAdd8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::paddb);
}

void EmitX64::EmitVectorAdd16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::paddw);
}

void EmitX64::EmitVectorAdd32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::paddd);
}

void EmitX64::EmitVectorAdd64(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::paddq);
}

void EmitX64::EmitVectorAnd(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pand);
}

void EmitX64::EmitVectorAndNot(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseScratchXmm(args[1]);

    code.pandn(xmm_b, xmm_a);

    ctx.reg_alloc.DefineValue(inst, xmm_b);
}

static void ArithmeticShiftRightByte(EmitContext& ctx, BlockOfCode& code, const Xbyak::Xmm& result, u8 shift_amount) {
    if (code.HasHostFeature(HostFeature::GFNI)) {
        const u64 shift_matrix = shift_amount < 8
                                   ? (0x0102040810204080 << (shift_amount * 8)) | (0x8080808080808080 >> (64 - shift_amount * 8))
                                   : 0x8080808080808080;
        code.gf2p8affineqb(result, code.XmmBConst<64>(xword, shift_matrix), 0);
        return;
    }

    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    code.punpckhbw(tmp, result);
    code.punpcklbw(result, result);
    code.psraw(tmp, 8 + shift_amount);
    code.psraw(result, 8 + shift_amount);
    code.packsswb(result, tmp);
}

void EmitX64::EmitVectorArithmeticShiftRight8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const u8 shift_amount = args[1].GetImmediateU8();

    ArithmeticShiftRightByte(ctx, code, result, shift_amount);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorArithmeticShiftRight16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const u8 shift_amount = args[1].GetImmediateU8();

    code.psraw(result, shift_amount);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorArithmeticShiftRight32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const u8 shift_amount = args[1].GetImmediateU8();

    code.psrad(result, shift_amount);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorArithmeticShiftRight64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const u8 shift_amount = std::min(args[1].GetImmediateU8(), u8(63));

    if (code.HasHostFeature(HostFeature::AVX512_Ortho)) {
        code.vpsraq(result, result, shift_amount);
    } else {
        const Xbyak::Xmm tmp1 = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm tmp2 = ctx.reg_alloc.ScratchXmm();

        const u64 sign_bit = 0x80000000'00000000u >> shift_amount;

        code.pxor(tmp2, tmp2);
        code.psrlq(result, shift_amount);
        code.movdqa(tmp1, code.XmmBConst<64>(xword, sign_bit));
        code.pand(tmp1, result);
        code.psubq(tmp2, tmp1);
        code.por(result, tmp2);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

template<typename T>
static constexpr T VShift(T x, T y) {
    const s8 shift_amount = static_cast<s8>(static_cast<u8>(y));
    const s64 bit_size = static_cast<s64>(mcl::bitsizeof<T>);

    if constexpr (std::is_signed_v<T>) {
        if (shift_amount >= bit_size) {
            return 0;
        }

        if (shift_amount <= -bit_size) {
            // Parentheses necessary, as MSVC doesn't appear to consider cast parentheses
            // as a grouping in terms of precedence, causing warning C4554 to fire. See:
            // https://developercommunity.visualstudio.com/content/problem/144783/msvc-2017-does-not-understand-that-static-cast-cou.html
            return x >> (T(bit_size - 1));
        }
    } else if (shift_amount <= -bit_size || shift_amount >= bit_size) {
        return 0;
    }

    if (shift_amount < 0) {
        return x >> T(-shift_amount);
    }

    using unsigned_type = std::make_unsigned_t<T>;
    return static_cast<T>(static_cast<unsigned_type>(x) << static_cast<unsigned_type>(shift_amount));
}

void EmitX64::EmitVectorArithmeticVShift8(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<s8>& result, const VectorArray<s8>& a, const VectorArray<s8>& b) {
        std::transform(a.begin(), a.end(), b.begin(), result.begin(), VShift<s8>);
    });
}

void EmitX64::EmitVectorArithmeticVShift16(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::AVX512_Ortho | HostFeature::AVX512BW)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm left_shift = ctx.reg_alloc.UseScratchXmm(args[1]);
        const Xbyak::Xmm right_shift = xmm16;
        const Xbyak::Xmm tmp = xmm17;

        code.vmovdqa32(tmp, code.XmmBConst<16>(xword, 0x00FF));
        code.vpxord(right_shift, right_shift, right_shift);
        code.vpsubw(right_shift, right_shift, left_shift);

        code.vpsllw(xmm0, left_shift, 8);
        code.vpsraw(xmm0, xmm0, 15);

        const Xbyak::Opmask mask = k1;
        code.vpmovb2m(mask, xmm0);

        code.vpandd(right_shift, right_shift, tmp);
        code.vpandd(left_shift, left_shift, tmp);

        code.vpsravw(tmp, result, right_shift);
        code.vpsllvw(result, result, left_shift);
        code.vpblendmb(result | mask, result, tmp);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<s16>& result, const VectorArray<s16>& a, const VectorArray<s16>& b) {
        std::transform(a.begin(), a.end(), b.begin(), result.begin(), VShift<s16>);
    });
}

void EmitX64::EmitVectorArithmeticVShift32(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::AVX2)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm left_shift = ctx.reg_alloc.UseScratchXmm(args[1]);
        const Xbyak::Xmm right_shift = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.vmovdqa(tmp, code.XmmBConst<32>(xword, 0x000000FF));
        code.vpxor(right_shift, right_shift, right_shift);
        code.vpsubd(right_shift, right_shift, left_shift);

        code.vpslld(xmm0, left_shift, 24);

        code.vpand(right_shift, right_shift, tmp);
        code.vpand(left_shift, left_shift, tmp);

        code.vpsravd(tmp, result, right_shift);
        code.vpsllvd(result, result, left_shift);
        code.blendvps(result, tmp);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<s32>& result, const VectorArray<s32>& a, const VectorArray<s32>& b) {
        std::transform(a.begin(), a.end(), b.begin(), result.begin(), VShift<s32>);
    });
}

void EmitX64::EmitVectorArithmeticVShift64(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::AVX512_Ortho)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm left_shift = ctx.reg_alloc.UseScratchXmm(args[1]);
        const Xbyak::Xmm right_shift = xmm16;
        const Xbyak::Xmm tmp = xmm17;

        code.vmovdqa32(tmp, code.XmmBConst<64>(xword, 0x00000000000000FF));
        code.vpxorq(right_shift, right_shift, right_shift);
        code.vpsubq(right_shift, right_shift, left_shift);

        code.vpsllq(xmm0, left_shift, 56);
        const Xbyak::Opmask mask = k1;
        code.vpmovq2m(mask, xmm0);

        code.vpandq(right_shift, right_shift, tmp);
        code.vpandq(left_shift, left_shift, tmp);

        code.vpsravq(tmp, result, right_shift);
        code.vpsllvq(result, result, left_shift);
        code.vpblendmq(result | mask, result, tmp);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<s64>& result, const VectorArray<s64>& a, const VectorArray<s64>& b) {
        std::transform(a.begin(), a.end(), b.begin(), result.begin(), VShift<s64>);
    });
}

void EmitX64::EmitVectorBroadcastLower8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);

    if (code.HasHostFeature(HostFeature::AVX2)) {
        code.vpbroadcastb(a, a);
        code.vmovq(a, a);
    } else if (code.HasHostFeature(HostFeature::SSSE3)) {
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.pxor(tmp, tmp);
        code.pshufb(a, tmp);
        code.movq(a, a);
    } else {
        code.punpcklbw(a, a);
        code.pshuflw(a, a, 0);
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorBroadcastLower16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);

    code.pshuflw(a, a, 0);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorBroadcastLower32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);

    code.pshuflw(a, a, 0b01000100);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorBroadcast8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);

    if (code.HasHostFeature(HostFeature::AVX2)) {
        code.vpbroadcastb(a, a);
    } else if (code.HasHostFeature(HostFeature::SSSE3)) {
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.pxor(tmp, tmp);
        code.pshufb(a, tmp);
    } else {
        code.punpcklbw(a, a);
        code.pshuflw(a, a, 0);
        code.punpcklqdq(a, a);
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorBroadcast16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);

    if (code.HasHostFeature(HostFeature::AVX2)) {
        code.vpbroadcastw(a, a);
    } else {
        code.pshuflw(a, a, 0);
        code.punpcklqdq(a, a);
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorBroadcast32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);

    if (code.HasHostFeature(HostFeature::AVX2)) {
        code.vpbroadcastd(a, a);
    } else {
        code.pshufd(a, a, 0);
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorBroadcast64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);

    if (code.HasHostFeature(HostFeature::AVX2)) {
        code.vpbroadcastq(a, a);
    } else {
        code.punpcklqdq(a, a);
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorBroadcastElementLower8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();
    ASSERT(index < 16);

    if (index > 0) {
        code.psrldq(a, index);
    }

    if (code.HasHostFeature(HostFeature::AVX2)) {
        code.vpbroadcastb(a, a);
        code.vmovq(a, a);
    } else if (code.HasHostFeature(HostFeature::SSSE3)) {
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.pxor(tmp, tmp);
        code.pshufb(a, tmp);
        code.movq(a, a);
    } else {
        code.punpcklbw(a, a);
        code.pshuflw(a, a, 0);
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorBroadcastElementLower16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();
    ASSERT(index < 8);

    if (index > 0) {
        code.psrldq(a, u8(index * 2));
    }

    code.pshuflw(a, a, 0);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorBroadcastElementLower32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();
    ASSERT(index < 4);

    if (index > 0) {
        code.psrldq(a, u8(index * 4));
    }

    code.pshuflw(a, a, 0b01'00'01'00);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorBroadcastElement8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();
    ASSERT(index < 16);

    if (index > 0) {
        code.psrldq(a, index);
    }

    if (code.HasHostFeature(HostFeature::AVX2)) {
        code.vpbroadcastb(a, a);
    } else if (code.HasHostFeature(HostFeature::SSSE3)) {
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.pxor(tmp, tmp);
        code.pshufb(a, tmp);
    } else {
        code.punpcklbw(a, a);
        code.pshuflw(a, a, 0);
        code.punpcklqdq(a, a);
    }
    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorBroadcastElement16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();
    ASSERT(index < 8);

    if (index == 0 && code.HasHostFeature(HostFeature::AVX2)) {
        code.vpbroadcastw(a, a);

        ctx.reg_alloc.DefineValue(inst, a);
        return;
    }

    if (index < 4) {
        code.pshuflw(a, a, mcl::bit::replicate_element<2, u8>(index));
        code.punpcklqdq(a, a);
    } else {
        code.pshufhw(a, a, mcl::bit::replicate_element<2, u8>(u8(index - 4)));
        code.punpckhqdq(a, a);
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorBroadcastElement32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();
    ASSERT(index < 4);

    code.pshufd(a, a, mcl::bit::replicate_element<2, u8>(index));

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorBroadcastElement64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    ASSERT(args[1].IsImmediate());
    const u8 index = args[1].GetImmediateU8();
    ASSERT(index < 2);

    if (code.HasHostFeature(HostFeature::AVX)) {
        code.vpermilpd(a, a, mcl::bit::replicate_element<1, u8>(index));
    } else {
        if (index == 0) {
            code.punpcklqdq(a, a);
        } else {
            code.punpckhqdq(a, a);
        }
    }
    ctx.reg_alloc.DefineValue(inst, a);
}

template<typename T>
static void EmitVectorCountLeadingZeros(VectorArray<T>& result, const VectorArray<T>& data) {
    for (size_t i = 0; i < result.size(); i++) {
        T element = data[i];

        size_t count = mcl::bitsizeof<T>;
        while (element != 0) {
            element >>= 1;
            --count;
        }

        result[i] = static_cast<T>(count);
    }
}

void EmitX64::EmitVectorCountLeadingZeros8(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSSE3)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm data = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm tmp1 = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm tmp2 = ctx.reg_alloc.ScratchXmm();

        code.movdqa(tmp1, code.XmmConst(xword, 0x0101010102020304, 0x0000000000000000));
        code.movdqa(tmp2, tmp1);

        code.pshufb(tmp2, data);
        code.psrlw(data, 4);
        code.pand(data, code.XmmBConst<8>(xword, 0x0F));
        code.pshufb(tmp1, data);

        code.movdqa(data, code.XmmBConst<8>(xword, 0x04));

        code.pcmpeqb(data, tmp1);
        code.pand(data, tmp2);
        code.paddb(data, tmp1);

        ctx.reg_alloc.DefineValue(inst, data);
        return;
    }

    EmitOneArgumentFallback(code, ctx, inst, EmitVectorCountLeadingZeros<u8>);
}

void EmitX64::EmitVectorCountLeadingZeros16(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::AVX)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm data = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm zeros = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.vpsrlw(tmp, data, 1);
        code.vpor(data, data, tmp);
        code.vpsrlw(tmp, data, 2);
        code.vpor(data, data, tmp);
        code.vpsrlw(tmp, data, 4);
        code.vpor(data, data, tmp);
        code.vpsrlw(tmp, data, 8);
        code.vpor(data, data, tmp);
        code.vpcmpeqw(zeros, zeros, zeros);
        code.vpcmpeqw(tmp, tmp, tmp);
        code.vpcmpeqw(zeros, zeros, data);
        code.vpmullw(data, data, code.XmmBConst<16>(xword, 0xf0d3));
        code.vpsllw(tmp, tmp, 15);
        code.vpsllw(zeros, zeros, 7);
        code.vpsrlw(data, data, 12);
        code.vmovdqa(result, code.XmmConst(xword, 0x0903060a040b0c10, 0x0f080e0207050d01));
        code.vpor(tmp, tmp, zeros);
        code.vpor(data, data, tmp);
        code.vpshufb(result, result, data);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    if (code.HasHostFeature(HostFeature::SSSE3)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm data = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm zeros = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.movdqa(tmp, data);
        code.psrlw(tmp, 1);
        code.por(data, tmp);
        code.movdqa(tmp, data);
        code.psrlw(tmp, 2);
        code.por(data, tmp);
        code.movdqa(tmp, data);
        code.psrlw(tmp, 4);
        code.por(data, tmp);
        code.movdqa(tmp, data);
        code.psrlw(tmp, 8);
        code.por(data, tmp);
        code.pcmpeqw(zeros, zeros);
        code.pcmpeqw(tmp, tmp);
        code.pcmpeqw(zeros, data);
        code.pmullw(data, code.XmmBConst<16>(xword, 0xf0d3));
        code.psllw(tmp, 15);
        code.psllw(zeros, 7);
        code.psrlw(data, 12);
        code.movdqa(result, code.XmmConst(xword, 0x0903060a040b0c10, 0x0f080e0207050d01));
        code.por(tmp, zeros);
        code.por(data, tmp);
        code.pshufb(result, data);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    EmitOneArgumentFallback(code, ctx, inst, EmitVectorCountLeadingZeros<u16>);
}

void EmitX64::EmitVectorCountLeadingZeros32(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::AVX512_Ortho | HostFeature::AVX512CD)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm data = ctx.reg_alloc.UseScratchXmm(args[0]);
        code.vplzcntd(data, data);

        ctx.reg_alloc.DefineValue(inst, data);
        return;
    }

    EmitOneArgumentFallback(code, ctx, inst, EmitVectorCountLeadingZeros<u32>);
}

void EmitX64::EmitVectorDeinterleaveEven8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    code.movdqa(tmp, code.XmmBConst<16>(xword, 0x00FF));
    code.pand(lhs, tmp);
    code.pand(rhs, tmp);
    code.packuswb(lhs, rhs);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveEven16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseScratchXmm(args[1]);

    code.pslld(lhs, 16);
    code.psrad(lhs, 16);

    code.pslld(rhs, 16);
    code.psrad(rhs, 16);

    code.packssdw(lhs, rhs);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveEven32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseXmm(args[1]);

    code.shufps(lhs, rhs, 0b10001000);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveEven64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseXmm(args[1]);

    code.shufpd(lhs, rhs, 0b00);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveEvenLower8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    code.movdqa(tmp, code.XmmBConst<16>(xword, 0x00FF));
    code.pand(lhs, tmp);
    code.pand(rhs, tmp);
    code.packuswb(lhs, rhs);
    code.pshufd(lhs, lhs, 0b11011000);
    code.movq(lhs, lhs);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveEvenLower16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseScratchXmm(args[1]);

    code.pslld(lhs, 16);
    code.psrad(lhs, 16);

    code.pslld(rhs, 16);
    code.psrad(rhs, 16);

    code.packssdw(lhs, rhs);
    code.pshufd(lhs, lhs, 0b11011000);
    code.movq(lhs, lhs);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveEvenLower32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseXmm(args[1]);

    code.unpcklps(lhs, rhs);
    code.movq(lhs, lhs);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveEvenLower64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    [[maybe_unused]] const Xbyak::Xmm rhs = ctx.reg_alloc.UseXmm(args[1]);

    code.movq(lhs, lhs);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveOdd8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseScratchXmm(args[1]);

    code.psraw(lhs, 8);
    code.psraw(rhs, 8);
    code.packsswb(lhs, rhs);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveOdd16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseScratchXmm(args[1]);

    code.psrad(lhs, 16);
    code.psrad(rhs, 16);
    code.packssdw(lhs, rhs);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveOdd32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseXmm(args[1]);

    code.shufps(lhs, rhs, 0b11011101);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveOdd64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseXmm(args[1]);

    code.shufpd(lhs, rhs, 0b11);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveOddLower8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseScratchXmm(args[1]);

    code.psraw(lhs, 8);
    code.psraw(rhs, 8);
    code.packsswb(lhs, rhs);
    code.pshufd(lhs, lhs, 0b11011000);
    code.movq(lhs, lhs);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveOddLower16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseScratchXmm(args[1]);

    code.psrad(lhs, 16);
    code.psrad(rhs, 16);
    code.packssdw(lhs, rhs);
    code.pshufd(lhs, lhs, 0b11011000);
    code.movq(lhs, lhs);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveOddLower32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm lhs = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm zero = ctx.reg_alloc.ScratchXmm();

    code.xorps(zero, zero);
    code.unpcklps(lhs, rhs);
    code.unpckhpd(lhs, zero);

    ctx.reg_alloc.DefineValue(inst, lhs);
}

void EmitX64::EmitVectorDeinterleaveOddLower64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    [[maybe_unused]] const Xbyak::Xmm lhs = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm rhs = ctx.reg_alloc.UseScratchXmm(args[1]);

    code.movq(rhs, rhs);

    ctx.reg_alloc.DefineValue(inst, rhs);
}

void EmitX64::EmitVectorEor(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pxor);
}

void EmitX64::EmitVectorEqual8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pcmpeqb);
}

void EmitX64::EmitVectorEqual16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pcmpeqw);
}

void EmitX64::EmitVectorEqual32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pcmpeqd);
}

void EmitX64::EmitVectorEqual64(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pcmpeqq);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    code.pcmpeqd(xmm_a, xmm_b);
    code.pshufd(tmp, xmm_a, 0b10110001);
    code.pand(xmm_a, tmp);

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitVectorEqual128(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.pcmpeqq(xmm_a, xmm_b);
        code.pshufd(tmp, xmm_a, 0b01001110);
        code.pand(xmm_a, tmp);

        ctx.reg_alloc.DefineValue(inst, xmm_a);
    } else {
        const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.pcmpeqd(xmm_a, xmm_b);
        code.pshufd(tmp, xmm_a, 0b10110001);
        code.pand(xmm_a, tmp);
        code.pshufd(tmp, xmm_a, 0b01001110);
        code.pand(xmm_a, tmp);

        ctx.reg_alloc.DefineValue(inst, xmm_a);
    }
}

void EmitX64::EmitVectorExtract(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const u8 position = args[2].GetImmediateU8();
    ASSERT(position % 8 == 0);

    if (position == 0) {
        ctx.reg_alloc.DefineValue(inst, args[0]);
        return;
    }

    if (code.HasHostFeature(HostFeature::SSSE3)) {
        const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseScratchXmm(args[1]);

        code.palignr(xmm_b, xmm_a, position / 8);
        ctx.reg_alloc.DefineValue(inst, xmm_b);
        return;
    }

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseScratchXmm(args[1]);

    code.psrldq(xmm_a, position / 8);
    code.pslldq(xmm_b, (128 - position) / 8);
    code.por(xmm_a, xmm_b);

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitVectorExtractLower(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);

    const u8 position = args[2].GetImmediateU8();
    ASSERT(position % 8 == 0);

    if (position != 0) {
        const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);

        code.punpcklqdq(xmm_a, xmm_b);
        code.psrldq(xmm_a, position / 8);
    }
    code.movq(xmm_a, xmm_a);

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitVectorGreaterS8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pcmpgtb);
}

void EmitX64::EmitVectorGreaterS16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pcmpgtw);
}

void EmitX64::EmitVectorGreaterS32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pcmpgtd);
}

void EmitX64::EmitVectorGreaterS64(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE42)) {
        EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pcmpgtq);
        return;
    }

    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u64>& result, const VectorArray<s64>& a, const VectorArray<s64>& b) {
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] = (a[i] > b[i]) ? ~u64(0) : 0;
        }
    });
}

static void EmitVectorHalvingAddSigned(size_t esize, EmitContext& ctx, IR::Inst* inst, BlockOfCode& code) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    code.movdqa(tmp, b);
    code.pand(tmp, a);
    code.pxor(a, b);

    switch (esize) {
    case 8:
        ArithmeticShiftRightByte(ctx, code, a, 1);
        code.paddb(a, tmp);
        break;
    case 16:
        code.psraw(a, 1);
        code.paddw(a, tmp);
        break;
    case 32:
        code.psrad(a, 1);
        code.paddd(a, tmp);
        break;
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorHalvingAddS8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorHalvingAddSigned(8, ctx, inst, code);
}

void EmitX64::EmitVectorHalvingAddS16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorHalvingAddSigned(16, ctx, inst, code);
}

void EmitX64::EmitVectorHalvingAddS32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorHalvingAddSigned(32, ctx, inst, code);
}

static void EmitVectorHalvingAddUnsigned(size_t esize, EmitContext& ctx, IR::Inst* inst, BlockOfCode& code) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    code.movdqa(tmp, b);

    switch (esize) {
    case 8:
        code.pavgb(tmp, a);
        code.pxor(a, b);
        code.pand(a, code.XmmBConst<8>(xword, 0x01));
        code.psubb(tmp, a);
        break;
    case 16:
        code.pavgw(tmp, a);
        code.pxor(a, b);
        code.pand(a, code.XmmBConst<16>(xword, 0x0001));
        code.psubw(tmp, a);
        break;
    case 32:
        code.pand(tmp, a);
        code.pxor(a, b);
        code.psrld(a, 1);
        code.paddd(tmp, a);
        break;
    }

    ctx.reg_alloc.DefineValue(inst, tmp);
}

void EmitX64::EmitVectorHalvingAddU8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorHalvingAddUnsigned(8, ctx, inst, code);
}

void EmitX64::EmitVectorHalvingAddU16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorHalvingAddUnsigned(16, ctx, inst, code);
}

void EmitX64::EmitVectorHalvingAddU32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorHalvingAddUnsigned(32, ctx, inst, code);
}

static void EmitVectorHalvingSubSigned(size_t esize, EmitContext& ctx, IR::Inst* inst, BlockOfCode& code) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);

    switch (esize) {
    case 8: {
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
        code.movdqa(tmp, code.XmmBConst<8>(xword, 0x80));
        code.pxor(a, tmp);
        code.pxor(b, tmp);
        code.pavgb(b, a);
        code.psubb(a, b);
        break;
    }
    case 16: {
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
        code.movdqa(tmp, code.XmmBConst<16>(xword, 0x8000));
        code.pxor(a, tmp);
        code.pxor(b, tmp);
        code.pavgw(b, a);
        code.psubw(a, b);
        break;
    }
    case 32:
        code.pxor(a, b);
        code.pand(b, a);
        code.psrad(a, 1);
        code.psubd(a, b);
        break;
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorHalvingSubS8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorHalvingSubSigned(8, ctx, inst, code);
}

void EmitX64::EmitVectorHalvingSubS16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorHalvingSubSigned(16, ctx, inst, code);
}

void EmitX64::EmitVectorHalvingSubS32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorHalvingSubSigned(32, ctx, inst, code);
}

static void EmitVectorHalvingSubUnsigned(size_t esize, EmitContext& ctx, IR::Inst* inst, BlockOfCode& code) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);

    switch (esize) {
    case 8:
        code.pavgb(b, a);
        code.psubb(a, b);
        break;
    case 16:
        code.pavgw(b, a);
        code.psubw(a, b);
        break;
    case 32:
        code.pxor(a, b);
        code.pand(b, a);
        code.psrld(a, 1);
        code.psubd(a, b);
        break;
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorHalvingSubU8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorHalvingSubUnsigned(8, ctx, inst, code);
}

void EmitX64::EmitVectorHalvingSubU16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorHalvingSubUnsigned(16, ctx, inst, code);
}

void EmitX64::EmitVectorHalvingSubU32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorHalvingSubUnsigned(32, ctx, inst, code);
}

static void EmitVectorInterleaveLower(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, int size) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseXmm(args[1]);

    switch (size) {
    case 8:
        code.punpcklbw(a, b);
        break;
    case 16:
        code.punpcklwd(a, b);
        break;
    case 32:
        code.punpckldq(a, b);
        break;
    case 64:
        code.punpcklqdq(a, b);
        break;
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorInterleaveLower8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorInterleaveLower(code, ctx, inst, 8);
}

void EmitX64::EmitVectorInterleaveLower16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorInterleaveLower(code, ctx, inst, 16);
}

void EmitX64::EmitVectorInterleaveLower32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorInterleaveLower(code, ctx, inst, 32);
}

void EmitX64::EmitVectorInterleaveLower64(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorInterleaveLower(code, ctx, inst, 64);
}

static void EmitVectorInterleaveUpper(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, int size) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseXmm(args[1]);

    switch (size) {
    case 8:
        code.punpckhbw(a, b);
        break;
    case 16:
        code.punpckhwd(a, b);
        break;
    case 32:
        code.punpckhdq(a, b);
        break;
    case 64:
        code.punpckhqdq(a, b);
        break;
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorInterleaveUpper8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorInterleaveUpper(code, ctx, inst, 8);
}

void EmitX64::EmitVectorInterleaveUpper16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorInterleaveUpper(code, ctx, inst, 16);
}

void EmitX64::EmitVectorInterleaveUpper32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorInterleaveUpper(code, ctx, inst, 32);
}

void EmitX64::EmitVectorInterleaveUpper64(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorInterleaveUpper(code, ctx, inst, 64);
}

void EmitX64::EmitVectorLogicalShiftLeft8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const u8 shift_amount = args[1].GetImmediateU8();

    if (shift_amount == 0) {
        // do nothing
    } else if (shift_amount >= 8) {
        code.pxor(result, result);
    } else if (shift_amount == 1) {
        code.paddb(result, result);
    } else if (code.HasHostFeature(HostFeature::GFNI)) {
        const u64 shift_matrix = 0x0102040810204080 >> (shift_amount * 8);
        code.gf2p8affineqb(result, code.XmmBConst<64>(xword, shift_matrix), 0);
    } else {
        const u64 replicand = (0xFFULL << shift_amount) & 0xFF;
        const u64 mask = mcl::bit::replicate_element<u8, u64>(replicand);

        code.psllw(result, shift_amount);
        code.pand(result, code.XmmBConst<64>(xword, mask));
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorLogicalShiftLeft16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const u8 shift_amount = args[1].GetImmediateU8();

    code.psllw(result, shift_amount);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorLogicalShiftLeft32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const u8 shift_amount = args[1].GetImmediateU8();

    code.pslld(result, shift_amount);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorLogicalShiftLeft64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const u8 shift_amount = args[1].GetImmediateU8();

    code.psllq(result, shift_amount);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorLogicalShiftRight8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const u8 shift_amount = args[1].GetImmediateU8();

    if (shift_amount == 0) {
        // Do nothing
    } else if (shift_amount >= 8) {
        code.pxor(result, result);
    } else if (code.HasHostFeature(HostFeature::GFNI)) {
        const u64 shift_matrix = 0x0102040810204080 << (shift_amount * 8);
        code.gf2p8affineqb(result, code.XmmBConst<64>(xword, shift_matrix), 0);
    } else {
        const u64 replicand = 0xFEULL >> shift_amount;
        const u64 mask = mcl::bit::replicate_element<u8, u64>(replicand);

        code.psrlw(result, shift_amount);
        code.pand(result, code.XmmConst(xword, mask, mask));
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorLogicalShiftRight16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const u8 shift_amount = args[1].GetImmediateU8();

    code.psrlw(result, shift_amount);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorLogicalShiftRight32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const u8 shift_amount = args[1].GetImmediateU8();

    code.psrld(result, shift_amount);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorLogicalShiftRight64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const u8 shift_amount = args[1].GetImmediateU8();

    code.psrlq(result, shift_amount);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorLogicalVShift8(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u8>& result, const VectorArray<u8>& a, const VectorArray<u8>& b) {
        std::transform(a.begin(), a.end(), b.begin(), result.begin(), VShift<u8>);
    });
}

void EmitX64::EmitVectorLogicalVShift16(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::AVX512_Ortho | HostFeature::AVX512BW)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm left_shift = ctx.reg_alloc.UseScratchXmm(args[1]);
        const Xbyak::Xmm right_shift = xmm16;
        const Xbyak::Xmm tmp = xmm17;

        code.vmovdqa32(tmp, code.XmmBConst<16>(xword, 0x00FF));
        code.vpxord(right_shift, right_shift, right_shift);
        code.vpsubw(right_shift, right_shift, left_shift);
        code.vpandd(left_shift, left_shift, tmp);
        code.vpandd(right_shift, right_shift, tmp);

        code.vpsllvw(tmp, result, left_shift);
        code.vpsrlvw(result, result, right_shift);
        code.vpord(result, result, tmp);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u16>& result, const VectorArray<u16>& a, const VectorArray<u16>& b) {
        std::transform(a.begin(), a.end(), b.begin(), result.begin(), VShift<u16>);
    });
}

void EmitX64::EmitVectorLogicalVShift32(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::AVX2)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm left_shift = ctx.reg_alloc.UseScratchXmm(args[1]);
        const Xbyak::Xmm right_shift = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.vmovdqa(tmp, code.XmmBConst<32>(xword, 0x000000FF));
        code.vpxor(right_shift, right_shift, right_shift);
        code.vpsubd(right_shift, right_shift, left_shift);
        code.vpand(left_shift, left_shift, tmp);
        code.vpand(right_shift, right_shift, tmp);

        code.vpsllvd(tmp, result, left_shift);
        code.vpsrlvd(result, result, right_shift);
        code.vpor(result, result, tmp);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u32>& result, const VectorArray<u32>& a, const VectorArray<u32>& b) {
        std::transform(a.begin(), a.end(), b.begin(), result.begin(), VShift<u32>);
    });
}

void EmitX64::EmitVectorLogicalVShift64(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::AVX2)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm left_shift = ctx.reg_alloc.UseScratchXmm(args[1]);
        const Xbyak::Xmm right_shift = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.vmovdqa(tmp, code.XmmBConst<64>(xword, 0x00000000000000FF));
        code.vpxor(right_shift, right_shift, right_shift);
        code.vpsubq(right_shift, right_shift, left_shift);
        code.vpand(left_shift, left_shift, tmp);
        code.vpand(right_shift, right_shift, tmp);

        code.vpsllvq(tmp, result, left_shift);
        code.vpsrlvq(result, result, right_shift);
        code.vpor(result, result, tmp);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u64>& result, const VectorArray<u64>& a, const VectorArray<u64>& b) {
        std::transform(a.begin(), a.end(), b.begin(), result.begin(), VShift<u64>);
    });
}

void EmitX64::EmitVectorMaxS8(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pmaxsb);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);

    const Xbyak::Xmm tmp_b = ctx.reg_alloc.ScratchXmm();
    code.movdqa(tmp_b, b);

    code.pcmpgtb(tmp_b, a);
    code.pand(b, tmp_b);
    code.pandn(tmp_b, a);
    code.por(tmp_b, b);

    ctx.reg_alloc.DefineValue(inst, tmp_b);
}

void EmitX64::EmitVectorMaxS16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pmaxsw);
}

void EmitX64::EmitVectorMaxS32(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pmaxsd);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);

    const Xbyak::Xmm tmp_b = ctx.reg_alloc.ScratchXmm();
    code.movdqa(tmp_b, b);

    code.pcmpgtd(tmp_b, a);
    code.pand(b, tmp_b);
    code.pandn(tmp_b, a);
    code.por(tmp_b, b);

    ctx.reg_alloc.DefineValue(inst, tmp_b);
}

void EmitX64::EmitVectorMaxS64(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::AVX512_Ortho)) {
        EmitAVXVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::vpmaxsq);
        return;
    }

    if (code.HasHostFeature(HostFeature::AVX)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);

        code.vpcmpgtq(xmm0, y, x);
        code.pblendvb(x, y);

        ctx.reg_alloc.DefineValue(inst, x);
        return;
    }

    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<s64>& result, const VectorArray<s64>& a, const VectorArray<s64>& b) {
        std::transform(a.begin(), a.end(), b.begin(), result.begin(), [](auto x, auto y) { return std::max(x, y); });
    });
}

void EmitX64::EmitVectorMaxU8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pmaxub);
}

void EmitX64::EmitVectorMaxU16(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pmaxuw);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseXmm(args[1]);

    code.psubusw(a, b);
    code.paddw(a, b);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorMaxU32(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pmaxud);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseXmm(args[1]);

    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
    code.movdqa(tmp, code.XmmBConst<32>(xword, 0x80000000));

    const Xbyak::Xmm tmp_b = ctx.reg_alloc.ScratchXmm();
    code.movdqa(tmp_b, b);

    code.pxor(tmp_b, tmp);
    code.pxor(tmp, a);

    code.pcmpgtd(tmp, tmp_b);
    code.pand(a, tmp);
    code.pandn(tmp, b);
    code.por(a, tmp);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorMaxU64(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::AVX512_Ortho)) {
        EmitAVXVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::vpmaxuq);
        return;
    }

    if (code.HasHostFeature(HostFeature::AVX)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.vmovdqa(xmm0, code.XmmBConst<64>(xword, 0x8000000000000000));
        code.vpsubq(tmp, y, xmm0);
        code.vpsubq(xmm0, x, xmm0);
        code.vpcmpgtq(xmm0, tmp, xmm0);
        code.pblendvb(x, y);

        ctx.reg_alloc.DefineValue(inst, x);
        return;
    }

    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u64>& result, const VectorArray<u64>& a, const VectorArray<u64>& b) {
        std::transform(a.begin(), a.end(), b.begin(), result.begin(), [](auto x, auto y) { return std::max(x, y); });
    });
}

void EmitX64::EmitVectorMinS8(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pminsb);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseXmm(args[1]);

    const Xbyak::Xmm tmp_b = ctx.reg_alloc.ScratchXmm();
    code.movdqa(tmp_b, b);

    code.pcmpgtb(tmp_b, a);
    code.pand(a, tmp_b);
    code.pandn(tmp_b, b);
    code.por(a, tmp_b);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorMinS16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pminsw);
}

void EmitX64::EmitVectorMinS32(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pminsd);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseXmm(args[1]);

    const Xbyak::Xmm tmp_b = ctx.reg_alloc.ScratchXmm();
    code.movdqa(tmp_b, b);

    code.pcmpgtd(tmp_b, a);
    code.pand(a, tmp_b);
    code.pandn(tmp_b, b);
    code.por(a, tmp_b);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorMinS64(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::AVX512_Ortho)) {
        EmitAVXVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::vpminsq);
        return;
    }

    if (code.HasHostFeature(HostFeature::AVX)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm x = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm y = ctx.reg_alloc.UseScratchXmm(args[1]);

        code.vpcmpgtq(xmm0, y, x);
        code.pblendvb(y, x);

        ctx.reg_alloc.DefineValue(inst, y);
        return;
    }

    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<s64>& result, const VectorArray<s64>& a, const VectorArray<s64>& b) {
        std::transform(a.begin(), a.end(), b.begin(), result.begin(), [](auto x, auto y) { return std::min(x, y); });
    });
}

void EmitX64::EmitVectorMinU8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pminub);
}

void EmitX64::EmitVectorMinU16(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pminuw);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);

    const Xbyak::Xmm tmp_b = ctx.reg_alloc.ScratchXmm();
    code.movdqa(tmp_b, b);

    code.psubusw(tmp_b, a);
    code.psubw(b, tmp_b);

    ctx.reg_alloc.DefineValue(inst, b);
}

void EmitX64::EmitVectorMinU32(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pminud);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseXmm(args[1]);

    const Xbyak::Xmm sint_max_plus_one = ctx.reg_alloc.ScratchXmm();
    code.movdqa(sint_max_plus_one, code.XmmBConst<32>(xword, 0x80000000));

    const Xbyak::Xmm tmp_a = ctx.reg_alloc.ScratchXmm();
    code.movdqa(tmp_a, a);
    code.psubd(tmp_a, sint_max_plus_one);

    const Xbyak::Xmm tmp_b = ctx.reg_alloc.ScratchXmm();
    code.movdqa(tmp_b, b);
    code.psubd(tmp_b, sint_max_plus_one);

    code.pcmpgtd(tmp_b, tmp_a);
    code.pand(a, tmp_b);
    code.pandn(tmp_b, b);
    code.por(a, tmp_b);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorMinU64(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::AVX512_Ortho)) {
        EmitAVXVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::vpminuq);
        return;
    }

    if (code.HasHostFeature(HostFeature::AVX)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm x = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm y = ctx.reg_alloc.UseScratchXmm(args[1]);
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.vmovdqa(xmm0, code.XmmBConst<64>(xword, 0x8000000000000000));
        code.vpsubq(tmp, y, xmm0);
        code.vpsubq(xmm0, x, xmm0);
        code.vpcmpgtq(xmm0, tmp, xmm0);
        code.pblendvb(y, x);

        ctx.reg_alloc.DefineValue(inst, y);
        return;
    }

    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u64>& result, const VectorArray<u64>& a, const VectorArray<u64>& b) {
        std::transform(a.begin(), a.end(), b.begin(), result.begin(), [](auto x, auto y) { return std::min(x, y); });
    });
}

void EmitX64::EmitVectorMultiply8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm tmp_a = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm tmp_b = ctx.reg_alloc.ScratchXmm();

    // TODO: Optimize
    code.movdqa(tmp_a, a);
    code.movdqa(tmp_b, b);
    code.pmullw(a, b);
    code.psrlw(tmp_a, 8);
    code.psrlw(tmp_b, 8);
    code.pmullw(tmp_a, tmp_b);
    code.pand(a, code.XmmBConst<16>(xword, 0x00FF));
    code.psllw(tmp_a, 8);
    code.por(a, tmp_a);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorMultiply16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pmullw);
}

void EmitX64::EmitVectorMultiply32(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pmulld);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    code.movdqa(tmp, a);
    code.psrlq(a, 32);
    code.pmuludq(tmp, b);
    code.psrlq(b, 32);
    code.pmuludq(a, b);
    code.pshufd(tmp, tmp, 0b00001000);
    code.pshufd(b, a, 0b00001000);
    code.punpckldq(tmp, b);

    ctx.reg_alloc.DefineValue(inst, tmp);
}

void EmitX64::EmitVectorMultiply64(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::AVX512_Ortho | HostFeature::AVX512DQ)) {
        EmitAVXVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::vpmullq);
        return;
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm b = ctx.reg_alloc.UseXmm(args[1]);
        const Xbyak::Reg64 tmp1 = ctx.reg_alloc.ScratchGpr();
        const Xbyak::Reg64 tmp2 = ctx.reg_alloc.ScratchGpr();

        code.movq(tmp1, a);
        code.movq(tmp2, b);
        code.imul(tmp2, tmp1);
        code.pextrq(tmp1, a, 1);
        code.movq(a, tmp2);
        code.pextrq(tmp2, b, 1);
        code.imul(tmp1, tmp2);
        code.pinsrq(a, tmp1, 1);

        ctx.reg_alloc.DefineValue(inst, a);
        return;
    }

    const Xbyak::Xmm a = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm tmp1 = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm tmp2 = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm tmp3 = ctx.reg_alloc.ScratchXmm();

    code.movdqa(tmp1, a);
    code.movdqa(tmp2, a);
    code.movdqa(tmp3, b);

    code.psrlq(tmp1, 32);
    code.psrlq(tmp3, 32);

    code.pmuludq(tmp2, b);
    code.pmuludq(tmp3, a);
    code.pmuludq(b, tmp1);

    code.paddq(b, tmp3);
    code.psllq(b, 32);
    code.paddq(tmp2, b);

    ctx.reg_alloc.DefineValue(inst, tmp2);
}

void EmitX64::EmitVectorNarrow16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (code.HasHostFeature(HostFeature::AVX512_Ortho | HostFeature::AVX512BW)) {
        const Xbyak::Xmm a = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

        code.vpmovwb(result, a);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm zeros = ctx.reg_alloc.ScratchXmm();

    code.pxor(zeros, zeros);
    code.pand(a, code.XmmBConst<16>(xword, 0x00FF));
    code.packuswb(a, zeros);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorNarrow32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (code.HasHostFeature(HostFeature::AVX512_Ortho)) {
        const Xbyak::Xmm a = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

        code.vpmovdw(result, a);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm zeros = ctx.reg_alloc.ScratchXmm();

    code.pxor(zeros, zeros);
    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.pblendw(a, zeros, 0b10101010);
        code.packusdw(a, zeros);
    } else {
        code.pslld(a, 16);
        code.psrad(a, 16);
        code.packssdw(a, zeros);
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorNarrow64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (code.HasHostFeature(HostFeature::AVX512_Ortho)) {
        const Xbyak::Xmm a = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

        code.vpmovqd(result, a);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm zeros = ctx.reg_alloc.ScratchXmm();

    code.pxor(zeros, zeros);
    code.shufps(a, zeros, 0b00001000);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorNot(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (code.HasHostFeature(HostFeature::AVX512_Ortho)) {
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm operand = ctx.reg_alloc.UseXmm(args[0]);
        code.vpternlogq(result, operand, operand, u8(~Tern::c));
        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.ScratchXmm();
    code.pcmpeqw(xmm_b, xmm_b);
    code.pxor(xmm_a, xmm_b);
    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitVectorOr(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::por);
}

void EmitX64::EmitVectorPairedAddLower8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    code.punpcklqdq(xmm_a, xmm_b);
    code.movdqa(tmp, xmm_a);
    code.psllw(xmm_a, 8);
    code.paddw(xmm_a, tmp);
    code.pxor(tmp, tmp);
    code.psrlw(xmm_a, 8);
    code.packuswb(xmm_a, tmp);

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitVectorPairedAddLower16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    code.punpcklqdq(xmm_a, xmm_b);
    if (code.HasHostFeature(HostFeature::SSSE3)) {
        code.pxor(tmp, tmp);
        code.phaddw(xmm_a, tmp);
    } else {
        code.movdqa(tmp, xmm_a);
        code.pslld(xmm_a, 16);
        code.paddd(xmm_a, tmp);
        code.pxor(tmp, tmp);
        code.psrad(xmm_a, 16);
        code.packssdw(xmm_a, tmp);  // Note: packusdw is SSE4.1, hence the arithmetic shift above.
    }

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitVectorPairedAddLower32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    code.punpcklqdq(xmm_a, xmm_b);
    if (code.HasHostFeature(HostFeature::SSSE3)) {
        code.pxor(tmp, tmp);
        code.phaddd(xmm_a, tmp);
    } else {
        code.movdqa(tmp, xmm_a);
        code.psllq(xmm_a, 32);
        code.paddq(xmm_a, tmp);
        code.psrlq(xmm_a, 32);
        code.pshufd(xmm_a, xmm_a, 0b11011000);
    }

    ctx.reg_alloc.DefineValue(inst, xmm_a);
}

void EmitX64::EmitVectorPairedAdd8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm c = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm d = ctx.reg_alloc.ScratchXmm();

    code.movdqa(c, a);
    code.movdqa(d, b);
    code.psllw(a, 8);
    code.psllw(b, 8);
    code.paddw(a, c);
    code.paddw(b, d);
    code.psrlw(a, 8);
    code.psrlw(b, 8);
    code.packuswb(a, b);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorPairedAdd16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (code.HasHostFeature(HostFeature::SSSE3)) {
        const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm b = ctx.reg_alloc.UseXmm(args[1]);

        code.phaddw(a, b);

        ctx.reg_alloc.DefineValue(inst, a);
    } else {
        const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);
        const Xbyak::Xmm c = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm d = ctx.reg_alloc.ScratchXmm();

        code.movdqa(c, a);
        code.movdqa(d, b);
        code.pslld(a, 16);
        code.pslld(b, 16);
        code.paddd(a, c);
        code.paddd(b, d);
        code.psrad(a, 16);
        code.psrad(b, 16);
        code.packssdw(a, b);

        ctx.reg_alloc.DefineValue(inst, a);
    }
}

void EmitX64::EmitVectorPairedAdd32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (code.HasHostFeature(HostFeature::SSSE3)) {
        const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm b = ctx.reg_alloc.UseXmm(args[1]);

        code.phaddd(a, b);

        ctx.reg_alloc.DefineValue(inst, a);
    } else {
        const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);
        const Xbyak::Xmm c = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm d = ctx.reg_alloc.ScratchXmm();

        code.movdqa(c, a);
        code.movdqa(d, b);
        code.psllq(a, 32);
        code.psllq(b, 32);
        code.paddq(a, c);
        code.paddq(b, d);
        code.shufps(a, b, 0b11011101);

        ctx.reg_alloc.DefineValue(inst, a);
    }
}

void EmitX64::EmitVectorPairedAdd64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm c = ctx.reg_alloc.ScratchXmm();

    code.movdqa(c, a);
    code.punpcklqdq(a, b);
    code.punpckhqdq(c, b);
    code.paddq(a, c);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorPairedAddSignedWiden8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm c = ctx.reg_alloc.ScratchXmm();

    code.movdqa(c, a);
    code.psllw(a, 8);
    code.psraw(c, 8);
    code.psraw(a, 8);
    code.paddw(a, c);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorPairedAddSignedWiden16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm c = ctx.reg_alloc.ScratchXmm();

    code.movdqa(c, a);
    code.pslld(a, 16);
    code.psrad(c, 16);
    code.psrad(a, 16);
    code.paddd(a, c);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorPairedAddSignedWiden32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);

    if (code.HasHostFeature(HostFeature::AVX512_Ortho)) {
        const Xbyak::Xmm c = xmm16;
        code.vpsraq(c, a, 32);
        code.vpsllq(a, a, 32);
        code.vpsraq(a, a, 32);
        code.vpaddq(a, a, c);
    } else {
        const Xbyak::Xmm tmp1 = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm tmp2 = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm c = ctx.reg_alloc.ScratchXmm();

        code.movdqa(c, a);
        code.psllq(a, 32);
        code.movdqa(tmp1, code.XmmBConst<64>(xword, 0x80000000'00000000));
        code.movdqa(tmp2, tmp1);
        code.pand(tmp1, a);
        code.pand(tmp2, c);
        code.psrlq(a, 32);
        code.psrlq(c, 32);
        code.psrad(tmp1, 31);
        code.psrad(tmp2, 31);
        code.por(a, tmp1);
        code.por(c, tmp2);
        code.paddq(a, c);
    }
    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorPairedAddUnsignedWiden8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm c = ctx.reg_alloc.ScratchXmm();

    code.movdqa(c, a);
    code.psllw(a, 8);
    code.psrlw(c, 8);
    code.psrlw(a, 8);
    code.paddw(a, c);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorPairedAddUnsignedWiden16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm c = ctx.reg_alloc.ScratchXmm();

    code.movdqa(c, a);
    code.pslld(a, 16);
    code.psrld(c, 16);
    code.psrld(a, 16);
    code.paddd(a, c);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorPairedAddUnsignedWiden32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm c = ctx.reg_alloc.ScratchXmm();

    code.movdqa(c, a);
    code.psllq(a, 32);
    code.psrlq(c, 32);
    code.psrlq(a, 32);
    code.paddq(a, c);

    ctx.reg_alloc.DefineValue(inst, a);
}

template<typename T, typename Function>
static void PairedOperation(VectorArray<T>& result, const VectorArray<T>& x, const VectorArray<T>& y, Function fn) {
    const size_t range = x.size() / 2;

    for (size_t i = 0; i < range; i++) {
        result[i] = fn(x[2 * i], x[2 * i + 1]);
    }

    for (size_t i = 0; i < range; i++) {
        result[range + i] = fn(y[2 * i], y[2 * i + 1]);
    }
}

template<typename T>
static void PairedMax(VectorArray<T>& result, const VectorArray<T>& x, const VectorArray<T>& y) {
    PairedOperation(result, x, y, [](auto a, auto b) { return std::max(a, b); });
}

template<typename T>
static void PairedMin(VectorArray<T>& result, const VectorArray<T>& x, const VectorArray<T>& y) {
    PairedOperation(result, x, y, [](auto a, auto b) { return std::min(a, b); });
}

void EmitX64::EmitVectorPairedMaxS8(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<s8>& result, const VectorArray<s8>& a, const VectorArray<s8>& b) {
        PairedMax(result, a, b);
    });
}

void EmitX64::EmitVectorPairedMaxS16(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<s16>& result, const VectorArray<s16>& a, const VectorArray<s16>& b) {
        PairedMax(result, a, b);
    });
}

void EmitX64::EmitVectorPairedMaxS32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    code.movdqa(tmp, x);
    code.shufps(tmp, y, 0b10001000);
    code.shufps(x, y, 0b11011101);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.pmaxsd(x, tmp);

        ctx.reg_alloc.DefineValue(inst, x);
    } else {
        const Xbyak::Xmm tmp2 = ctx.reg_alloc.ScratchXmm();

        code.movdqa(tmp2, tmp);
        code.pcmpgtd(tmp2, x);
        code.pand(tmp, tmp2);
        code.pandn(tmp2, x);
        code.por(tmp2, tmp);

        ctx.reg_alloc.DefineValue(inst, tmp2);
    }
}

void EmitX64::EmitVectorPairedMaxU8(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u8>& result, const VectorArray<u8>& a, const VectorArray<u8>& b) {
        PairedMax(result, a, b);
    });
}

void EmitX64::EmitVectorPairedMaxU16(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u16>& result, const VectorArray<u16>& a, const VectorArray<u16>& b) {
        PairedMax(result, a, b);
    });
}

void EmitX64::EmitVectorPairedMaxU32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm tmp1 = ctx.reg_alloc.ScratchXmm();

    code.movdqa(tmp1, x);
    code.shufps(tmp1, y, 0b10001000);
    code.shufps(x, y, 0b11011101);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.pmaxud(x, tmp1);

        ctx.reg_alloc.DefineValue(inst, x);
    } else {
        const Xbyak::Xmm tmp3 = ctx.reg_alloc.ScratchXmm();
        code.movdqa(tmp3, code.XmmBConst<32>(xword, 0x80000000));

        const Xbyak::Xmm tmp2 = ctx.reg_alloc.ScratchXmm();
        code.movdqa(tmp2, x);

        code.pxor(tmp2, tmp3);
        code.pxor(tmp3, tmp1);
        code.pcmpgtd(tmp3, tmp2);
        code.pand(tmp1, tmp3);
        code.pandn(tmp3, x);
        code.por(tmp1, tmp3);

        ctx.reg_alloc.DefineValue(inst, tmp1);
    }
}

void EmitX64::EmitVectorPairedMinS8(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<s8>& result, const VectorArray<s8>& a, const VectorArray<s8>& b) {
        PairedMin(result, a, b);
    });
}

void EmitX64::EmitVectorPairedMinS16(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<s16>& result, const VectorArray<s16>& a, const VectorArray<s16>& b) {
        PairedMin(result, a, b);
    });
}

void EmitX64::EmitVectorPairedMinS32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    code.movdqa(tmp, x);
    code.shufps(tmp, y, 0b10001000);
    code.shufps(x, y, 0b11011101);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.pminsd(x, tmp);

        ctx.reg_alloc.DefineValue(inst, x);
    } else {
        const Xbyak::Xmm tmp2 = ctx.reg_alloc.ScratchXmm();

        code.movaps(tmp2, x);
        code.pcmpgtd(tmp2, tmp);
        code.pand(tmp, tmp2);
        code.pandn(tmp2, x);
        code.por(tmp2, tmp);

        ctx.reg_alloc.DefineValue(inst, tmp2);
    }
}

void EmitX64::EmitVectorPairedMinU8(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u8>& result, const VectorArray<u8>& a, const VectorArray<u8>& b) {
        PairedMin(result, a, b);
    });
}

void EmitX64::EmitVectorPairedMinU16(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u16>& result, const VectorArray<u16>& a, const VectorArray<u16>& b) {
        PairedMin(result, a, b);
    });
}

void EmitX64::EmitVectorPairedMinU32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm tmp1 = ctx.reg_alloc.ScratchXmm();

    code.movdqa(tmp1, x);
    code.shufps(tmp1, y, 0b10001000);
    code.shufps(x, y, 0b11011101);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.pminud(x, tmp1);

        ctx.reg_alloc.DefineValue(inst, x);
    } else {
        const Xbyak::Xmm tmp3 = ctx.reg_alloc.ScratchXmm();
        code.movdqa(tmp3, code.XmmBConst<32>(xword, 0x80000000));

        const Xbyak::Xmm tmp2 = ctx.reg_alloc.ScratchXmm();
        code.movdqa(tmp2, tmp1);

        code.pxor(tmp2, tmp3);
        code.pxor(tmp3, x);
        code.pcmpgtd(tmp3, tmp2);
        code.pand(tmp1, tmp3);
        code.pandn(tmp3, x);
        code.por(tmp1, tmp3);

        ctx.reg_alloc.DefineValue(inst, tmp1);
    }
}

template<typename D, typename T>
static D PolynomialMultiply(T lhs, T rhs) {
    constexpr size_t bit_size = mcl::bitsizeof<T>;
    const std::bitset<bit_size> operand(lhs);

    D res = 0;
    for (size_t i = 0; i < bit_size; i++) {
        if (operand[i]) {
            res ^= rhs << i;
        }
    }

    return res;
}

void EmitX64::EmitVectorPolynomialMultiply8(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);
        const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm alternate = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm mask = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Reg32 counter = ctx.reg_alloc.ScratchGpr().cvt32();

        Xbyak::Label loop;

        code.pxor(result, result);
        code.movdqa(mask, code.XmmBConst<8>(xword, 0x01));
        code.mov(counter, 8);

        code.L(loop);
        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpand(xmm0, xmm_b, mask);
            code.vpxor(alternate, result, xmm_a);
        } else {
            code.movdqa(xmm0, xmm_b);
            code.movdqa(alternate, result);
            code.pand(xmm0, mask);
            code.pxor(alternate, xmm_a);
        }
        code.pcmpeqb(xmm0, mask);
        code.paddb(mask, mask);
        code.paddb(xmm_a, xmm_a);
        code.pblendvb(result, alternate);
        code.dec(counter);
        code.jnz(loop);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u8>& result, const VectorArray<u8>& a, const VectorArray<u8>& b) {
        std::transform(a.begin(), a.end(), b.begin(), result.begin(), PolynomialMultiply<u8, u8>);
    });
}

void EmitX64::EmitVectorPolynomialMultiplyLong8(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);
        const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseScratchXmm(args[1]);
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm alternate = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm mask = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Reg32 counter = ctx.reg_alloc.ScratchGpr().cvt32();

        Xbyak::Label loop;

        code.pmovzxbw(xmm_a, xmm_a);
        code.pmovzxbw(xmm_b, xmm_b);
        code.pxor(result, result);
        code.movdqa(mask, code.XmmBConst<16>(xword, 0x0001));
        code.mov(counter, 8);

        code.L(loop);
        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpand(xmm0, xmm_b, mask);
            code.vpxor(alternate, result, xmm_a);
        } else {
            code.movdqa(xmm0, xmm_b);
            code.movdqa(alternate, result);
            code.pand(xmm0, mask);
            code.pxor(alternate, xmm_a);
        }
        code.pcmpeqw(xmm0, mask);
        code.paddw(mask, mask);
        code.paddw(xmm_a, xmm_a);
        code.pblendvb(result, alternate);
        code.dec(counter);
        code.jnz(loop);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u16>& result, const VectorArray<u8>& a, const VectorArray<u8>& b) {
        for (size_t i = 0; i < result.size(); i++) {
            result[i] = PolynomialMultiply<u16, u8>(a[i], b[i]);
        }
    });
}

void EmitX64::EmitVectorPolynomialMultiplyLong64(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::PCLMULQDQ)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);
        const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);

        code.pclmulqdq(xmm_a, xmm_b, 0x00);

        ctx.reg_alloc.DefineValue(inst, xmm_a);
        return;
    }

    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u64>& result, const VectorArray<u64>& a, const VectorArray<u64>& b) {
        const auto handle_high_bits = [](u64 lhs, u64 rhs) {
            constexpr size_t bit_size = mcl::bitsizeof<u64>;
            u64 result = 0;

            for (size_t i = 1; i < bit_size; i++) {
                if (mcl::bit::get_bit(i, lhs)) {
                    result ^= rhs >> (bit_size - i);
                }
            }

            return result;
        };

        result[0] = PolynomialMultiply<u64, u64>(a[0], b[0]);
        result[1] = handle_high_bits(a[0], b[0]);
    });
}

void EmitX64::EmitVectorPopulationCount(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::AVX512VL | HostFeature::AVX512BITALG)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);
        const Xbyak::Xmm data = ctx.reg_alloc.UseScratchXmm(args[0]);

        code.vpopcntb(data, data);

        ctx.reg_alloc.DefineValue(inst, data);
        return;
    }

    if (code.HasHostFeature(HostFeature::SSSE3)) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm low_a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm high_a = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm tmp1 = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm tmp2 = ctx.reg_alloc.ScratchXmm();

        code.movdqa(high_a, low_a);
        code.psrlw(high_a, 4);
        code.movdqa(tmp1, code.XmmBConst<8>(xword, 0x0F));
        code.pand(high_a, tmp1);  // High nibbles
        code.pand(low_a, tmp1);   // Low nibbles

        code.movdqa(tmp1, code.XmmConst(xword, 0x0302020102010100, 0x0403030203020201));
        code.movdqa(tmp2, tmp1);
        code.pshufb(tmp1, low_a);
        code.pshufb(tmp2, high_a);

        code.paddb(tmp1, tmp2);

        ctx.reg_alloc.DefineValue(inst, tmp1);
        return;
    }

    EmitOneArgumentFallback(code, ctx, inst, [](VectorArray<u8>& result, const VectorArray<u8>& a) {
        std::transform(a.begin(), a.end(), result.begin(), [](u8 val) {
            return static_cast<u8>(mcl::bit::count_ones(val));
        });
    });
}

void EmitX64::EmitVectorReverseBits(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm data = ctx.reg_alloc.UseScratchXmm(args[0]);

    if (code.HasHostFeature(HostFeature::GFNI)) {
        code.gf2p8affineqb(data, code.XmmBConst<64>(xword, 0x8040201008040201), 0);
    } else {
        const Xbyak::Xmm high_nibble_reg = ctx.reg_alloc.ScratchXmm();
        code.movdqa(high_nibble_reg, code.XmmBConst<8>(xword, 0xF0));
        code.pand(high_nibble_reg, data);
        code.pxor(data, high_nibble_reg);
        code.psrld(high_nibble_reg, 4);

        if (code.HasHostFeature(HostFeature::SSSE3)) {
            // High lookup
            const Xbyak::Xmm high_reversed_reg = ctx.reg_alloc.ScratchXmm();
            code.movdqa(high_reversed_reg, code.XmmConst(xword, 0xE060A020C0408000, 0xF070B030D0509010));
            code.pshufb(high_reversed_reg, data);

            // Low lookup (low nibble equivalent of the above)
            code.movdqa(data, code.XmmConst(xword, 0x0E060A020C040800, 0x0F070B030D050901));
            code.pshufb(data, high_nibble_reg);
            code.por(data, high_reversed_reg);
        } else {
            code.pslld(data, 4);
            code.por(data, high_nibble_reg);

            code.movdqa(high_nibble_reg, code.XmmBConst<8>(xword, 0xCC));
            code.pand(high_nibble_reg, data);
            code.pxor(data, high_nibble_reg);
            code.psrld(high_nibble_reg, 2);
            code.pslld(data, 2);
            code.por(data, high_nibble_reg);

            code.movdqa(high_nibble_reg, code.XmmBConst<8>(xword, 0xAA));
            code.pand(high_nibble_reg, data);
            code.pxor(data, high_nibble_reg);
            code.psrld(high_nibble_reg, 1);
            code.paddd(data, data);
            code.por(data, high_nibble_reg);
        }
    }

    ctx.reg_alloc.DefineValue(inst, data);
}

void EmitX64::EmitVectorReduceAdd8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm data = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm temp = xmm0;

    // Add upper elements to lower elements
    code.pshufd(temp, data, 0b01'00'11'10);
    code.paddb(data, temp);

    // Add adjacent 8-bit values into 64-bit lanes
    code.pxor(temp, temp);
    code.psadbw(data, temp);

    // Zero-extend lower 8-bits
    code.pslldq(data, 15);
    code.psrldq(data, 15);

    ctx.reg_alloc.DefineValue(inst, data);
}

void EmitX64::EmitVectorReduceAdd16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm data = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm temp = xmm0;

    if (code.HasHostFeature(HostFeature::SSSE3)) {
        code.pxor(temp, temp);
        code.phaddw(data, xmm0);
        code.phaddw(data, xmm0);
        code.phaddw(data, xmm0);
    } else {
        // Add upper elements to lower elements
        code.pshufd(temp, data, 0b00'01'10'11);
        code.paddw(data, temp);

        // Add pairs of 16-bit values into 32-bit lanes
        code.movdqa(temp, code.XmmBConst<16>(xword, 0x0001));
        code.pmaddwd(data, temp);

        // Sum adjacent 32-bit lanes
        code.pshufd(temp, data, 0b10'11'00'01);
        code.paddd(data, temp);
        // Zero-extend lower 16-bits
        code.pslldq(data, 14);
        code.psrldq(data, 14);
    }

    ctx.reg_alloc.DefineValue(inst, data);
}

void EmitX64::EmitVectorReduceAdd32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm data = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm temp = xmm0;

    // Add upper elements to lower elements(reversed)
    code.pshufd(temp, data, 0b00'01'10'11);
    code.paddd(data, temp);

    // Sum adjacent 32-bit lanes
    if (code.HasHostFeature(HostFeature::SSSE3)) {
        code.phaddd(data, data);
    } else {
        code.pshufd(temp, data, 0b10'11'00'01);
        code.paddd(data, temp);
    }

    // shift upper-most result into lower-most lane
    code.psrldq(data, 12);

    ctx.reg_alloc.DefineValue(inst, data);
}

void EmitX64::EmitVectorReduceAdd64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm data = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm temp = xmm0;

    // Add upper elements to lower elements
    code.pshufd(temp, data, 0b01'00'11'10);
    code.paddq(data, temp);

    // Zero-extend lower 64-bits
    code.movq(data, data);

    ctx.reg_alloc.DefineValue(inst, data);
}

static void EmitVectorRoundingHalvingAddSigned(size_t esize, EmitContext& ctx, IR::Inst* inst, BlockOfCode& code) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);

    switch (esize) {
    case 8: {
        const Xbyak::Xmm vec_128 = ctx.reg_alloc.ScratchXmm();
        code.movdqa(vec_128, code.XmmBConst<8>(xword, 0x80));

        code.paddb(a, vec_128);
        code.paddb(b, vec_128);
        code.pavgb(a, b);
        code.paddb(a, vec_128);
        break;
    }
    case 16: {
        const Xbyak::Xmm vec_32768 = ctx.reg_alloc.ScratchXmm();
        code.movdqa(vec_32768, code.XmmBConst<16>(xword, 0x8000));

        code.paddw(a, vec_32768);
        code.paddw(b, vec_32768);
        code.pavgw(a, b);
        code.paddw(a, vec_32768);
        break;
    }
    case 32: {
        const Xbyak::Xmm tmp1 = ctx.reg_alloc.ScratchXmm();
        code.movdqa(tmp1, a);

        code.por(a, b);
        code.psrad(tmp1, 1);
        code.psrad(b, 1);
        code.pslld(a, 31);
        code.paddd(b, tmp1);
        code.psrld(a, 31);
        code.paddd(a, b);
        break;
    }
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorRoundingHalvingAddS8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorRoundingHalvingAddSigned(8, ctx, inst, code);
}

void EmitX64::EmitVectorRoundingHalvingAddS16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorRoundingHalvingAddSigned(16, ctx, inst, code);
}

void EmitX64::EmitVectorRoundingHalvingAddS32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorRoundingHalvingAddSigned(32, ctx, inst, code);
}

static void EmitVectorRoundingHalvingAddUnsigned(size_t esize, EmitContext& ctx, IR::Inst* inst, BlockOfCode& code) {
    switch (esize) {
    case 8:
        EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pavgb);
        return;
    case 16:
        EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::pavgw);
        return;
    case 32: {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);
        const Xbyak::Xmm tmp1 = ctx.reg_alloc.ScratchXmm();

        code.movdqa(tmp1, a);

        code.por(a, b);
        code.psrld(tmp1, 1);
        code.psrld(b, 1);
        code.pslld(a, 31);
        code.paddd(b, tmp1);
        code.psrld(a, 31);
        code.paddd(a, b);

        ctx.reg_alloc.DefineValue(inst, a);
        break;
    }
    }
}

void EmitX64::EmitVectorRoundingHalvingAddU8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorRoundingHalvingAddUnsigned(8, ctx, inst, code);
}

void EmitX64::EmitVectorRoundingHalvingAddU16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorRoundingHalvingAddUnsigned(16, ctx, inst, code);
}

void EmitX64::EmitVectorRoundingHalvingAddU32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorRoundingHalvingAddUnsigned(32, ctx, inst, code);
}

template<typename T, typename U>
static void RoundingShiftLeft(VectorArray<T>& out, const VectorArray<T>& lhs, const VectorArray<U>& rhs) {
    using signed_type = std::make_signed_t<T>;
    using unsigned_type = std::make_unsigned_t<T>;

    constexpr auto bit_size = static_cast<s64>(mcl::bitsizeof<T>);

    for (size_t i = 0; i < out.size(); i++) {
        const s64 extended_shift = static_cast<s64>(mcl::bit::sign_extend<8, u64>(rhs[i] & 0xFF));

        if (extended_shift >= 0) {
            if (extended_shift >= bit_size) {
                out[i] = 0;
            } else {
                out[i] = static_cast<T>(static_cast<unsigned_type>(lhs[i]) << extended_shift);
            }
        } else {
            if ((std::is_unsigned_v<T> && extended_shift < -bit_size) || (std::is_signed_v<T> && extended_shift <= -bit_size)) {
                out[i] = 0;
            } else {
                const s64 shift_value = -extended_shift - 1;
                const T shifted = (lhs[i] & (static_cast<signed_type>(1) << shift_value)) >> shift_value;

                if (extended_shift == -bit_size) {
                    out[i] = shifted;
                } else {
                    out[i] = (lhs[i] >> -extended_shift) + shifted;
                }
            }
        }
    }
}

void EmitX64::EmitVectorRoundingShiftLeftS8(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<s8>& result, const VectorArray<s8>& lhs, const VectorArray<s8>& rhs) {
        RoundingShiftLeft(result, lhs, rhs);
    });
}

void EmitX64::EmitVectorRoundingShiftLeftS16(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<s16>& result, const VectorArray<s16>& lhs, const VectorArray<s16>& rhs) {
        RoundingShiftLeft(result, lhs, rhs);
    });
}

void EmitX64::EmitVectorRoundingShiftLeftS32(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<s32>& result, const VectorArray<s32>& lhs, const VectorArray<s32>& rhs) {
        RoundingShiftLeft(result, lhs, rhs);
    });
}

void EmitX64::EmitVectorRoundingShiftLeftS64(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<s64>& result, const VectorArray<s64>& lhs, const VectorArray<s64>& rhs) {
        RoundingShiftLeft(result, lhs, rhs);
    });
}

void EmitX64::EmitVectorRoundingShiftLeftU8(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u8>& result, const VectorArray<u8>& lhs, const VectorArray<s8>& rhs) {
        RoundingShiftLeft(result, lhs, rhs);
    });
}

void EmitX64::EmitVectorRoundingShiftLeftU16(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u16>& result, const VectorArray<u16>& lhs, const VectorArray<s16>& rhs) {
        RoundingShiftLeft(result, lhs, rhs);
    });
}

void EmitX64::EmitVectorRoundingShiftLeftU32(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u32>& result, const VectorArray<u32>& lhs, const VectorArray<s32>& rhs) {
        RoundingShiftLeft(result, lhs, rhs);
    });
}

void EmitX64::EmitVectorRoundingShiftLeftU64(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallback(code, ctx, inst, [](VectorArray<u64>& result, const VectorArray<u64>& lhs, const VectorArray<s64>& rhs) {
        RoundingShiftLeft(result, lhs, rhs);
    });
}

static void VectorShuffleImpl(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, void (Xbyak::CodeGenerator::*fn)(const Xbyak::Mmx&, const Xbyak::Operand&, u8)) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm operand = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const u8 mask = args[1].GetImmediateU8();

    (code.*fn)(result, operand, mask);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorShuffleHighHalfwords(EmitContext& ctx, IR::Inst* inst) {
    VectorShuffleImpl(code, ctx, inst, &Xbyak::CodeGenerator::pshufhw);
}

void EmitX64::EmitVectorShuffleLowHalfwords(EmitContext& ctx, IR::Inst* inst) {
    VectorShuffleImpl(code, ctx, inst, &Xbyak::CodeGenerator::pshuflw);
}

void EmitX64::EmitVectorShuffleWords(EmitContext& ctx, IR::Inst* inst) {
    VectorShuffleImpl(code, ctx, inst, &Xbyak::CodeGenerator::pshufd);
}

void EmitX64::EmitVectorSignExtend8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    if (code.HasHostFeature(HostFeature::SSE41)) {
        const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
        code.pmovsxbw(a, a);
        ctx.reg_alloc.DefineValue(inst, a);
    } else {
        const Xbyak::Xmm a = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        code.pxor(result, result);
        code.punpcklbw(result, a);
        code.psraw(result, 8);
        ctx.reg_alloc.DefineValue(inst, result);
    }
}

void EmitX64::EmitVectorSignExtend16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    if (code.HasHostFeature(HostFeature::SSE41)) {
        const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
        code.pmovsxwd(a, a);
        ctx.reg_alloc.DefineValue(inst, a);
    } else {
        const Xbyak::Xmm a = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        code.pxor(result, result);
        code.punpcklwd(result, a);
        code.psrad(result, 16);
        ctx.reg_alloc.DefineValue(inst, result);
    }
}

void EmitX64::EmitVectorSignExtend32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.pmovsxdq(a, a);
    } else {
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.movaps(tmp, a);
        code.psrad(tmp, 31);
        code.punpckldq(a, tmp);
    }

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorSignExtend64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm data = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Reg64 gpr_tmp = ctx.reg_alloc.ScratchGpr();

    code.movq(gpr_tmp, data);
    code.sar(gpr_tmp, 63);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.pinsrq(data, gpr_tmp, 1);
    } else {
        const Xbyak::Xmm xmm_tmp = ctx.reg_alloc.ScratchXmm();

        code.movq(xmm_tmp, gpr_tmp);
        code.punpcklqdq(data, xmm_tmp);
    }

    ctx.reg_alloc.DefineValue(inst, data);
}

static void EmitVectorSignedAbsoluteDifference(size_t esize, EmitContext& ctx, IR::Inst* inst, BlockOfCode& code) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm mask = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm tmp1 = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm tmp2 = ctx.reg_alloc.ScratchXmm();

    code.movdqa(mask, x);
    code.movdqa(tmp1, y);

    switch (esize) {
    case 8:
        code.pcmpgtb(mask, y);
        code.psubb(tmp1, x);
        code.psubb(x, y);
        break;
    case 16:
        code.pcmpgtw(mask, y);
        code.psubw(tmp1, x);
        code.psubw(x, y);
        break;
    case 32:
        code.pcmpgtd(mask, y);
        code.psubd(tmp1, x);
        code.psubd(x, y);
        break;
    }

    code.movdqa(tmp2, mask);
    code.pand(x, mask);
    code.pandn(tmp2, tmp1);
    code.por(x, tmp2);

    ctx.reg_alloc.DefineValue(inst, x);
}

void EmitX64::EmitVectorSignedAbsoluteDifference8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedAbsoluteDifference(8, ctx, inst, code);
}

void EmitX64::EmitVectorSignedAbsoluteDifference16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedAbsoluteDifference(16, ctx, inst, code);
}

void EmitX64::EmitVectorSignedAbsoluteDifference32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedAbsoluteDifference(32, ctx, inst, code);
}

void EmitX64::EmitVectorSignedMultiply16(EmitContext& ctx, IR::Inst* inst) {
    const auto upper_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetUpperFromOp);
    const auto lower_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetLowerFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm x = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);

    if (upper_inst) {
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpmulhw(result, x, y);
        } else {
            code.movdqa(result, x);
            code.pmulhw(result, y);
        }

        ctx.reg_alloc.DefineValue(upper_inst, result);
        ctx.EraseInstruction(upper_inst);
    }

    if (lower_inst) {
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpmullw(result, x, y);
        } else {
            code.movdqa(result, x);
            code.pmullw(result, y);
        }
        ctx.reg_alloc.DefineValue(lower_inst, result);
        ctx.EraseInstruction(lower_inst);
    }
}

void EmitX64::EmitVectorSignedMultiply32(EmitContext& ctx, IR::Inst* inst) {
    const auto upper_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetUpperFromOp);
    const auto lower_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetLowerFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (lower_inst && !upper_inst && code.HasHostFeature(HostFeature::AVX)) {
        const Xbyak::Xmm x = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

        code.vpmulld(result, x, y);

        ctx.reg_alloc.DefineValue(lower_inst, result);
        ctx.EraseInstruction(lower_inst);
        return;
    }

    if (code.HasHostFeature(HostFeature::AVX)) {
        const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm y = ctx.reg_alloc.UseScratchXmm(args[1]);

        if (lower_inst) {
            const Xbyak::Xmm lower_result = ctx.reg_alloc.ScratchXmm();
            code.vpmulld(lower_result, x, y);
            ctx.reg_alloc.DefineValue(lower_inst, lower_result);
            ctx.EraseInstruction(lower_inst);
        }

        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

        code.vpmuldq(result, x, y);
        code.vpsrlq(x, x, 32);
        code.vpsrlq(y, y, 32);
        code.vpmuldq(x, x, y);
        code.shufps(result, x, 0b11011101);

        ctx.reg_alloc.DefineValue(upper_inst, result);
        ctx.EraseInstruction(upper_inst);
        return;
    }

    const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm sign_correction = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm upper_result = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm lower_result = ctx.reg_alloc.ScratchXmm();

    // calculate sign correction
    code.movdqa(tmp, x);
    code.movdqa(sign_correction, y);
    code.psrad(tmp, 31);
    code.psrad(sign_correction, 31);
    code.pand(tmp, y);
    code.pand(sign_correction, x);
    code.paddd(sign_correction, tmp);
    code.pand(sign_correction, code.XmmBConst<32>(xword, 0x7FFFFFFF));

    // calculate unsigned multiply
    code.movdqa(tmp, x);
    code.pmuludq(tmp, y);
    code.psrlq(x, 32);
    code.psrlq(y, 32);
    code.pmuludq(x, y);

    // put everything into place
    code.pcmpeqw(upper_result, upper_result);
    code.pcmpeqw(lower_result, lower_result);
    code.psllq(upper_result, 32);
    code.psrlq(lower_result, 32);
    code.pand(upper_result, x);
    code.pand(lower_result, tmp);
    code.psrlq(tmp, 32);
    code.psllq(x, 32);
    code.por(upper_result, tmp);
    code.por(lower_result, x);
    code.psubd(upper_result, sign_correction);

    if (upper_inst) {
        ctx.reg_alloc.DefineValue(upper_inst, upper_result);
        ctx.EraseInstruction(upper_inst);
    }
    if (lower_inst) {
        ctx.reg_alloc.DefineValue(lower_inst, lower_result);
        ctx.EraseInstruction(lower_inst);
    }
}

static void EmitVectorSignedSaturatedAbs(size_t esize, BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm data = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm data_test = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm sign = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Address mask = [esize, &code] {
        switch (esize) {
        case 8:
            return code.XmmBConst<8>(xword, 0x80);
        case 16:
            return code.XmmBConst<16>(xword, 0x8000);
        case 32:
            return code.XmmBConst<32>(xword, 0x80000000);
        case 64:
            return code.XmmBConst<64>(xword, 0x8000000000000000);
        default:
            UNREACHABLE();
        }
    }();

    const auto vector_equality = [esize, &code](const Xbyak::Xmm& x, const Xbyak::Xmm& y) {
        switch (esize) {
        case 8:
            code.pcmpeqb(x, y);
            break;
        case 16:
            code.pcmpeqw(x, y);
            break;
        case 32:
            code.pcmpeqd(x, y);
            break;
        case 64:
            code.pcmpeqq(x, y);
            break;
        }
    };

    // Keep a copy of the initial data for determining whether or not
    // to set the Q flag
    code.movdqa(data_test, data);

    switch (esize) {
    case 8:
        VectorAbs8(code, ctx, data);
        break;
    case 16:
        VectorAbs16(code, ctx, data);
        break;
    case 32:
        VectorAbs32(code, ctx, data);
        break;
    case 64:
        VectorAbs64(code, ctx, data);
        break;
    }

    code.movdqa(sign, mask);
    vector_equality(sign, data);
    code.pxor(data, sign);

    // Check if the initial data contained any elements with the value 0x80.
    // If any exist, then the Q flag needs to be set.
    const Xbyak::Reg32 bit = ctx.reg_alloc.ScratchGpr().cvt32();
    code.movdqa(sign, mask);
    vector_equality(data_test, sign);
    code.pmovmskb(bit, data_test);
    code.or_(code.dword[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], bit);

    ctx.reg_alloc.DefineValue(inst, data);
}

void EmitX64::EmitVectorSignedSaturatedAbs8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturatedAbs(8, code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedAbs16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturatedAbs(16, code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedAbs32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturatedAbs(32, code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedAbs64(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        EmitVectorSignedSaturatedAbs(64, code, ctx, inst);
        return;
    }

    EmitOneArgumentFallbackWithSaturation(code, ctx, inst, [](VectorArray<s64>& result, const VectorArray<s64>& data) {
        bool qc_flag = false;

        for (size_t i = 0; i < result.size(); i++) {
            if (static_cast<u64>(data[i]) == 0x8000000000000000) {
                result[i] = 0x7FFFFFFFFFFFFFFF;
                qc_flag = true;
            } else {
                result[i] = std::abs(data[i]);
            }
        }

        return qc_flag;
    });
}

template<size_t bit_width>
static void EmitVectorSignedSaturatedAccumulateUnsigned(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);
    code.movdqa(xmm0, y);
    ctx.reg_alloc.Release(y);

    const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

    switch (bit_width) {
    case 8:
        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpaddb(result, x, xmm0);
        } else {
            code.movdqa(result, x);
            code.paddb(result, xmm0);
        }
        break;
    case 16:
        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpaddw(result, x, xmm0);
        } else {
            code.movdqa(result, x);
            code.paddw(result, xmm0);
        }
        break;
    case 32:
        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpaddd(result, x, xmm0);
        } else {
            code.movdqa(result, x);
            code.paddd(result, xmm0);
        }
        break;
    case 64:
        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpaddq(result, x, xmm0);
        } else {
            code.movdqa(result, x);
            code.paddq(result, xmm0);
        }
        break;
    }

    if (code.HasHostFeature(HostFeature::AVX512_Ortho)) {
        // xmm0 = majority(~y, x, res)
        code.vpternlogd(xmm0, x, result, 0b10001110);
    } else if (code.HasHostFeature(HostFeature::AVX)) {
        code.vpor(tmp, x, result);
        code.pand(x, result);
        code.vpblendvb(xmm0, tmp, x, xmm0);
    } else {
        code.movdqa(tmp, x);
        code.pxor(x, result);
        code.pand(tmp, result);
        code.pandn(xmm0, x);
        code.por(xmm0, tmp);
    }

    ctx.reg_alloc.Release(x);

    switch (bit_width) {
    case 8:
        if (code.HasHostFeature(HostFeature::AVX)) {
            const Xbyak::Xmm tmp2 = ctx.reg_alloc.ScratchXmm();
            code.pcmpeqb(tmp2, tmp2);
            code.pxor(tmp, tmp);
            code.vpblendvb(xmm0, tmp, tmp2, xmm0);
            ctx.reg_alloc.Release(tmp2);
        } else {
            code.pand(xmm0, code.XmmBConst<8>(xword, 0x80));
            code.movdqa(tmp, xmm0);
            code.psrlw(tmp, 7);
            code.pxor(xmm0, xmm0);
            code.psubb(xmm0, tmp);
        }
        break;
    case 16:
        code.psraw(xmm0, 15);
        break;
    case 32:
        code.psrad(xmm0, 31);
        break;
    case 64:
        if (code.HasHostFeature(HostFeature::AVX512_Ortho)) {
            code.vpsraq(xmm0, xmm0, 63);
        } else {
            code.psrad(xmm0, 31);
            code.pshufd(xmm0, xmm0, 0b11110101);
        }
        break;
    }

    code.movdqa(tmp, xmm0);
    switch (bit_width) {
    case 8:
        code.paddb(tmp, tmp);
        code.psrlw(tmp, 1);
        break;
    case 16:
        code.psrlw(tmp, 1);
        break;
    case 32:
        code.psrld(tmp, 1);
        break;
    case 64:
        code.psrlq(tmp, 1);
        break;
    }

    const Xbyak::Reg32 mask = ctx.reg_alloc.ScratchGpr().cvt32();
    code.pmovmskb(mask, xmm0);
    code.or_(code.dword[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], mask);

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.pblendvb(result, tmp);
    } else {
        code.pandn(xmm0, result);
        code.por(xmm0, tmp);
        code.movdqa(result, xmm0);
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorSignedSaturatedAccumulateUnsigned8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturatedAccumulateUnsigned<8>(code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedAccumulateUnsigned16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturatedAccumulateUnsigned<16>(code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedAccumulateUnsigned32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturatedAccumulateUnsigned<32>(code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedAccumulateUnsigned64(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturatedAccumulateUnsigned<64>(code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedDoublingMultiply16(EmitContext& ctx, IR::Inst* inst) {
    const auto upper_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetUpperFromOp);
    const auto lower_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetLowerFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm x = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm upper_tmp = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm lower_tmp = ctx.reg_alloc.ScratchXmm();

    if (code.HasHostFeature(HostFeature::AVX)) {
        code.vpmulhw(upper_tmp, x, y);
    } else {
        code.movdqa(upper_tmp, x);
        code.pmulhw(upper_tmp, y);
    }

    if (code.HasHostFeature(HostFeature::AVX)) {
        code.vpmullw(lower_tmp, x, y);
    } else {
        code.movdqa(lower_tmp, x);
        code.pmullw(lower_tmp, y);
    }

    ctx.reg_alloc.Release(x);
    ctx.reg_alloc.Release(y);

    if (lower_inst) {
        const Xbyak::Xmm lower_result = ctx.reg_alloc.ScratchXmm();

        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpaddw(lower_result, lower_tmp, lower_tmp);
        } else {
            code.movdqa(lower_result, lower_tmp);
            code.paddw(lower_result, lower_result);
        }

        ctx.reg_alloc.DefineValue(lower_inst, lower_result);
        ctx.EraseInstruction(lower_inst);
    }

    if (upper_inst) {
        const Xbyak::Xmm upper_result = ctx.reg_alloc.ScratchXmm();

        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpsrlw(lower_tmp, lower_tmp, 15);
            code.vpaddw(upper_tmp, upper_tmp, upper_tmp);
            code.vpor(upper_result, upper_tmp, lower_tmp);
            code.vpcmpeqw(upper_tmp, upper_result, code.XmmBConst<16>(xword, 0x8000));
            code.vpxor(upper_result, upper_result, upper_tmp);
        } else {
            code.paddw(upper_tmp, upper_tmp);
            code.psrlw(lower_tmp, 15);
            code.movdqa(upper_result, upper_tmp);
            code.por(upper_result, lower_tmp);
            code.movdqa(upper_tmp, code.XmmBConst<16>(xword, 0x8000));
            code.pcmpeqw(upper_tmp, upper_result);
            code.pxor(upper_result, upper_tmp);
        }

        const Xbyak::Reg32 bit = ctx.reg_alloc.ScratchGpr().cvt32();
        code.pmovmskb(bit, upper_tmp);
        code.or_(code.dword[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], bit);

        ctx.reg_alloc.DefineValue(upper_inst, upper_result);
        ctx.EraseInstruction(upper_inst);
    }
}

void EmitX64::EmitVectorSignedSaturatedDoublingMultiply32(EmitContext& ctx, IR::Inst* inst) {
    const auto upper_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetUpperFromOp);
    const auto lower_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetLowerFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (code.HasHostFeature(HostFeature::AVX)) {
        const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm y = ctx.reg_alloc.UseScratchXmm(args[1]);
        const Xbyak::Xmm odds = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm even = ctx.reg_alloc.ScratchXmm();

        code.vpmuldq(odds, x, y);
        code.vpsrlq(x, x, 32);
        code.vpsrlq(y, y, 32);
        code.vpmuldq(even, x, y);

        ctx.reg_alloc.Release(x);
        ctx.reg_alloc.Release(y);

        code.vpaddq(odds, odds, odds);
        code.vpaddq(even, even, even);

        if (upper_inst) {
            const Xbyak::Xmm upper_result = ctx.reg_alloc.ScratchXmm();

            code.vpsrlq(upper_result, odds, 32);
            code.vblendps(upper_result, upper_result, even, 0b1010);

            const Xbyak::Xmm mask = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Reg32 bit = ctx.reg_alloc.ScratchGpr().cvt32();

            code.vpcmpeqd(mask, upper_result, code.XmmBConst<32>(xword, 0x80000000));
            code.vpxor(upper_result, upper_result, mask);
            code.pmovmskb(bit, mask);
            code.or_(code.dword[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], bit);

            ctx.reg_alloc.Release(mask);
            ctx.reg_alloc.Release(bit);

            ctx.reg_alloc.DefineValue(upper_inst, upper_result);
            ctx.EraseInstruction(upper_inst);
        }

        if (lower_inst) {
            const Xbyak::Xmm lower_result = ctx.reg_alloc.ScratchXmm();

            code.vpsllq(lower_result, even, 32);
            code.vblendps(lower_result, lower_result, odds, 0b0101);

            ctx.reg_alloc.DefineValue(lower_inst, lower_result);
            ctx.EraseInstruction(lower_inst);
        }

        return;
    }

    const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm sign_correction = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm upper_result = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm lower_result = ctx.reg_alloc.ScratchXmm();

    // calculate sign correction
    code.movdqa(tmp, x);
    code.movdqa(sign_correction, y);
    code.psrad(tmp, 31);
    code.psrad(sign_correction, 31);
    code.pand(tmp, y);
    code.pand(sign_correction, x);
    code.paddd(sign_correction, tmp);
    code.pslld(sign_correction, 1);

    // unsigned multiply
    code.movdqa(tmp, x);
    code.pmuludq(tmp, y);
    code.psrlq(x, 32);
    code.psrlq(y, 32);
    code.pmuludq(x, y);

    // double
    code.paddq(tmp, tmp);
    code.paddq(x, x);

    // put everything into place
    code.pcmpeqw(upper_result, upper_result);
    code.pcmpeqw(lower_result, lower_result);
    code.psllq(upper_result, 32);
    code.psrlq(lower_result, 32);
    code.pand(upper_result, x);
    code.pand(lower_result, tmp);
    code.psrlq(tmp, 32);
    code.psllq(x, 32);
    code.por(upper_result, tmp);
    code.por(lower_result, x);
    code.psubd(upper_result, sign_correction);

    if (upper_inst) {
        const Xbyak::Reg32 bit = ctx.reg_alloc.ScratchGpr().cvt32();

        code.movdqa(tmp, code.XmmBConst<32>(xword, 0x80000000));
        code.pcmpeqd(tmp, upper_result);
        code.pxor(upper_result, tmp);
        code.pmovmskb(bit, tmp);
        code.or_(code.dword[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], bit);

        ctx.reg_alloc.DefineValue(upper_inst, upper_result);
        ctx.EraseInstruction(upper_inst);
    }
    if (lower_inst) {
        ctx.reg_alloc.DefineValue(lower_inst, lower_result);
        ctx.EraseInstruction(lower_inst);
    }
}

void EmitX64::EmitVectorSignedSaturatedDoublingMultiplyLong16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseScratchXmm(args[1]);

    code.punpcklwd(x, x);
    code.punpcklwd(y, y);
    code.pmaddwd(x, y);

    if (code.HasHostFeature(HostFeature::AVX)) {
        code.vpcmpeqd(y, x, code.XmmBConst<32>(xword, 0x80000000));
        code.vpxor(x, x, y);
    } else {
        code.movdqa(y, code.XmmBConst<32>(xword, 0x80000000));
        code.pcmpeqd(y, x);
        code.pxor(x, y);
    }

    const Xbyak::Reg32 bit = ctx.reg_alloc.ScratchGpr().cvt32();
    code.pmovmskb(bit, y);
    code.or_(code.dword[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], bit);

    ctx.reg_alloc.DefineValue(inst, x);
}

void EmitX64::EmitVectorSignedSaturatedDoublingMultiplyLong32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseScratchXmm(args[1]);

    if (code.HasHostFeature(HostFeature::AVX)) {
        code.vpmovsxdq(x, x);
        code.vpmovsxdq(y, y);
        code.vpmuldq(x, x, y);
        code.vpaddq(x, x, x);
    } else {
        const Xbyak::Reg64 a = ctx.reg_alloc.ScratchGpr();
        const Xbyak::Reg64 b = ctx.reg_alloc.ScratchGpr();
        const Xbyak::Reg64 c = ctx.reg_alloc.ScratchGpr();
        const Xbyak::Reg64 d = ctx.reg_alloc.ScratchGpr();

        code.movq(c, x);
        code.movq(d, y);
        code.movsxd(a, c.cvt32());
        code.movsxd(b, d.cvt32());
        code.sar(c, 32);
        code.sar(d, 32);
        code.imul(a, b);
        code.imul(c, d);
        code.movq(x, a);
        code.movq(y, c);
        code.punpcklqdq(x, y);
        code.paddq(x, x);

        ctx.reg_alloc.Release(a);
        ctx.reg_alloc.Release(b);
        ctx.reg_alloc.Release(c);
        ctx.reg_alloc.Release(d);
    }

    const Xbyak::Reg32 bit = ctx.reg_alloc.ScratchGpr().cvt32();
    if (code.HasHostFeature(HostFeature::AVX)) {
        code.vpcmpeqq(y, x, code.XmmBConst<64>(xword, 0x8000000000000000));
        code.vpxor(x, x, y);
        code.vpmovmskb(bit, y);
    } else {
        code.movdqa(y, code.XmmBConst<64>(xword, 0x8000000000000000));
        code.pcmpeqd(y, x);
        code.shufps(y, y, 0b11110101);
        code.pxor(x, y);
        code.pmovmskb(bit, y);
    }
    code.or_(code.dword[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], bit);

    ctx.reg_alloc.DefineValue(inst, x);
}

static void EmitVectorSignedSaturatedNarrowToSigned(size_t original_esize, BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm src = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm dest = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm reconstructed = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm sign = ctx.reg_alloc.ScratchXmm();

    code.movdqa(dest, src);

    switch (original_esize) {
    case 16:
        code.packsswb(dest, dest);
        code.movdqa(sign, src);
        code.psraw(sign, 15);
        code.packsswb(sign, sign);
        code.movdqa(reconstructed, dest);
        code.punpcklbw(reconstructed, sign);
        break;
    case 32:
        code.packssdw(dest, dest);
        code.movdqa(reconstructed, dest);
        code.movdqa(sign, dest);
        code.psraw(sign, 15);
        code.punpcklwd(reconstructed, sign);
        break;
    default:
        UNREACHABLE();
    }

    const Xbyak::Reg32 bit = ctx.reg_alloc.ScratchGpr().cvt32();
    code.pcmpeqd(reconstructed, src);
    code.movmskps(bit, reconstructed);
    code.xor_(bit, 0b1111);
    code.or_(code.dword[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], bit);

    ctx.reg_alloc.DefineValue(inst, dest);
}

void EmitX64::EmitVectorSignedSaturatedNarrowToSigned16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturatedNarrowToSigned(16, code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedNarrowToSigned32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturatedNarrowToSigned(32, code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedNarrowToSigned64(EmitContext& ctx, IR::Inst* inst) {
    EmitOneArgumentFallbackWithSaturation(code, ctx, inst, [](VectorArray<s32>& result, const VectorArray<s64>& a) {
        bool qc_flag = false;
        for (size_t i = 0; i < a.size(); ++i) {
            const s64 saturated = std::clamp<s64>(a[i], -s64(0x80000000), s64(0x7FFFFFFF));
            result[i] = static_cast<s32>(saturated);
            qc_flag |= saturated != a[i];
        }
        return qc_flag;
    });
}

static void EmitVectorSignedSaturatedNarrowToUnsigned(size_t original_esize, BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm src = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm dest = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm reconstructed = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm zero = ctx.reg_alloc.ScratchXmm();

    code.movdqa(dest, src);
    code.pxor(zero, zero);

    switch (original_esize) {
    case 16:
        code.packuswb(dest, dest);
        code.movdqa(reconstructed, dest);
        code.punpcklbw(reconstructed, zero);
        break;
    case 32:
        ASSERT(code.HasHostFeature(HostFeature::SSE41));
        code.packusdw(dest, dest);  // SSE4.1
        code.movdqa(reconstructed, dest);
        code.punpcklwd(reconstructed, zero);
        break;
    default:
        UNREACHABLE();
    }

    const Xbyak::Reg32 bit = ctx.reg_alloc.ScratchGpr().cvt32();
    code.pcmpeqd(reconstructed, src);
    code.movmskps(bit, reconstructed);
    code.xor_(bit, 0b1111);
    code.or_(code.dword[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], bit);

    ctx.reg_alloc.DefineValue(inst, dest);
}

void EmitX64::EmitVectorSignedSaturatedNarrowToUnsigned16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturatedNarrowToUnsigned(16, code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedNarrowToUnsigned32(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        EmitVectorSignedSaturatedNarrowToUnsigned(32, code, ctx, inst);
        return;
    }

    EmitOneArgumentFallbackWithSaturation(code, ctx, inst, [](VectorArray<u16>& result, const VectorArray<s32>& a) {
        bool qc_flag = false;
        for (size_t i = 0; i < a.size(); ++i) {
            const s32 saturated = std::clamp<s32>(a[i], 0, 0xFFFF);
            result[i] = static_cast<u16>(saturated);
            qc_flag |= saturated != a[i];
        }
        return qc_flag;
    });
}

void EmitX64::EmitVectorSignedSaturatedNarrowToUnsigned64(EmitContext& ctx, IR::Inst* inst) {
    EmitOneArgumentFallbackWithSaturation(code, ctx, inst, [](VectorArray<u32>& result, const VectorArray<s64>& a) {
        bool qc_flag = false;
        for (size_t i = 0; i < a.size(); ++i) {
            const s64 saturated = std::clamp<s64>(a[i], 0, 0xFFFFFFFF);
            result[i] = static_cast<u32>(saturated);
            qc_flag |= saturated != a[i];
        }
        return qc_flag;
    });
}

static void EmitVectorSignedSaturatedNeg(size_t esize, BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm data = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm zero = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Address mask = [esize, &code] {
        switch (esize) {
        case 8:
            return code.XmmBConst<8>(xword, 0x80);
        case 16:
            return code.XmmBConst<16>(xword, 0x8000);
        case 32:
            return code.XmmBConst<32>(xword, 0x80000000);
        case 64:
            return code.XmmBConst<64>(xword, 0x8000000000000000);
        default:
            UNREACHABLE();
        }
    }();

    const auto vector_equality = [esize, &code](const Xbyak::Xmm& x, const auto& y) {
        switch (esize) {
        case 8:
            code.pcmpeqb(x, y);
            break;
        case 16:
            code.pcmpeqw(x, y);
            break;
        case 32:
            code.pcmpeqd(x, y);
            break;
        case 64:
            code.pcmpeqq(x, y);
            break;
        }
    };

    code.movdqa(tmp, data);
    vector_equality(tmp, mask);

    // Perform negation
    code.pxor(zero, zero);
    switch (esize) {
    case 8:
        code.psubsb(zero, data);
        break;
    case 16:
        code.psubsw(zero, data);
        break;
    case 32:
        code.psubd(zero, data);
        code.pxor(zero, tmp);
        break;
    case 64:
        code.psubq(zero, data);
        code.pxor(zero, tmp);
        break;
    }

    // Check if any elements matched the mask prior to performing saturation. If so, set the Q bit.
    const Xbyak::Reg32 bit = ctx.reg_alloc.ScratchGpr().cvt32();
    code.pmovmskb(bit, tmp);
    code.or_(code.dword[code.r15 + code.GetJitStateInfo().offsetof_fpsr_qc], bit);

    ctx.reg_alloc.DefineValue(inst, zero);
}

void EmitX64::EmitVectorSignedSaturatedNeg8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturatedNeg(8, code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedNeg16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturatedNeg(16, code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedNeg32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorSignedSaturatedNeg(32, code, ctx, inst);
}

void EmitX64::EmitVectorSignedSaturatedNeg64(EmitContext& ctx, IR::Inst* inst) {
    if (code.HasHostFeature(HostFeature::SSE41)) {
        EmitVectorSignedSaturatedNeg(64, code, ctx, inst);
        return;
    }

    EmitOneArgumentFallbackWithSaturation(code, ctx, inst, [](VectorArray<s64>& result, const VectorArray<s64>& data) {
        bool qc_flag = false;

        for (size_t i = 0; i < result.size(); i++) {
            if (static_cast<u64>(data[i]) == 0x8000000000000000) {
                result[i] = 0x7FFFFFFFFFFFFFFF;
                qc_flag = true;
            } else {
                result[i] = -data[i];
            }
        }

        return qc_flag;
    });
}

// MSVC requires the capture within the saturate lambda, but it's
// determined to be unnecessary via clang and GCC.
#ifdef __clang__
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunused-lambda-capture"
#endif
template<typename T, typename U = std::make_unsigned_t<T>>
static bool VectorSignedSaturatedShiftLeft(VectorArray<T>& dst, const VectorArray<T>& data, const VectorArray<T>& shift_values) {
    static_assert(std::is_signed_v<T>, "T must be signed.");

    bool qc_flag = false;

    constexpr size_t bit_size_minus_one = mcl::bitsizeof<T> - 1;

    const auto saturate = [bit_size_minus_one](T value) {
        return static_cast<T>((static_cast<U>(value) >> bit_size_minus_one) + (U{1} << bit_size_minus_one) - 1);
    };

    for (size_t i = 0; i < dst.size(); i++) {
        const T element = data[i];
        const T shift = std::clamp<T>(static_cast<T>(mcl::bit::sign_extend<8>(static_cast<U>(shift_values[i] & 0xFF))),
                                      -static_cast<T>(bit_size_minus_one), std::numeric_limits<T>::max());

        if (element == 0) {
            dst[i] = 0;
        } else if (shift < 0) {
            dst[i] = static_cast<T>(element >> -shift);
        } else if (static_cast<U>(shift) > bit_size_minus_one) {
            dst[i] = saturate(element);
            qc_flag = true;
        } else {
            const T shifted = T(U(element) << shift);

            if ((shifted >> shift) != element) {
                dst[i] = saturate(element);
                qc_flag = true;
            } else {
                dst[i] = shifted;
            }
        }
    }

    return qc_flag;
}
#ifdef __clang__
#    pragma clang diagnostic pop
#endif

void EmitX64::EmitVectorSignedSaturatedShiftLeft8(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, VectorSignedSaturatedShiftLeft<s8>);
}

void EmitX64::EmitVectorSignedSaturatedShiftLeft16(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, VectorSignedSaturatedShiftLeft<s16>);
}

void EmitX64::EmitVectorSignedSaturatedShiftLeft32(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, VectorSignedSaturatedShiftLeft<s32>);
}

void EmitX64::EmitVectorSignedSaturatedShiftLeft64(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, VectorSignedSaturatedShiftLeft<s64>);
}

template<typename T, typename U = std::make_unsigned_t<T>>
static bool VectorSignedSaturatedShiftLeftUnsigned(VectorArray<T>& dst, const VectorArray<T>& data, const VectorArray<T>& shift_values) {
    static_assert(std::is_signed_v<T>, "T must be signed.");

    constexpr size_t bit_size_minus_one = mcl::bitsizeof<T> - 1;

    bool qc_flag = false;
    for (size_t i = 0; i < dst.size(); i++) {
        const T element = data[i];
        const T shift = std::clamp<T>(static_cast<T>(mcl::bit::sign_extend<8>(static_cast<U>(shift_values[i] & 0xFF))),
                                      -static_cast<T>(bit_size_minus_one), std::numeric_limits<T>::max());

        if (element == 0) {
            dst[i] = 0;
        } else if (element < 0) {
            dst[i] = 0;
            qc_flag = true;
        } else if (shift < 0) {
            dst[i] = static_cast<T>(element >> -shift);
        } else if (static_cast<U>(shift) > bit_size_minus_one) {
            dst[i] = static_cast<T>(std::numeric_limits<U>::max());
            qc_flag = true;
        } else {
            const U shifted = static_cast<U>(element) << static_cast<U>(shift);
            const U shifted_test = shifted >> static_cast<U>(shift);

            if (shifted_test != static_cast<U>(element)) {
                dst[i] = static_cast<T>(std::numeric_limits<U>::max());
                qc_flag = true;
            } else {
                dst[i] = shifted;
            }
        }
    }

    return qc_flag;
}

void EmitX64::EmitVectorSignedSaturatedShiftLeftUnsigned8(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, VectorSignedSaturatedShiftLeftUnsigned<s8>);
}

void EmitX64::EmitVectorSignedSaturatedShiftLeftUnsigned16(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, VectorSignedSaturatedShiftLeftUnsigned<s16>);
}

void EmitX64::EmitVectorSignedSaturatedShiftLeftUnsigned32(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, VectorSignedSaturatedShiftLeftUnsigned<s32>);
}

void EmitX64::EmitVectorSignedSaturatedShiftLeftUnsigned64(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, VectorSignedSaturatedShiftLeftUnsigned<s64>);
}

void EmitX64::EmitVectorSub8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::psubb);
}

void EmitX64::EmitVectorSub16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::psubw);
}

void EmitX64::EmitVectorSub32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::psubd);
}

void EmitX64::EmitVectorSub64(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorOperation(code, ctx, inst, &Xbyak::CodeGenerator::psubq);
}

void EmitX64::EmitVectorTable(EmitContext&, IR::Inst* inst) {
    // Do nothing. We *want* to hold on to the refcount for our arguments, so VectorTableLookup can use our arguments.
    ASSERT_MSG(inst->UseCount() == 1, "Table cannot be used multiple times");
}

void EmitX64::EmitVectorTableLookup64(EmitContext& ctx, IR::Inst* inst) {
    ASSERT(inst->GetArg(1).GetInst()->GetOpcode() == IR::Opcode::VectorTable);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto table = ctx.reg_alloc.GetArgumentInfo(inst->GetArg(1).GetInst());

    const size_t table_size = std::count_if(table.begin(), table.end(), [](const auto& elem) { return !elem.IsVoid(); });
    const bool is_defaults_zero = inst->GetArg(0).IsZero();

    // TODO: AVX512VL implementation when available (VPERMB / VPERMI2B / VPERMT2B)

    const std::array<u64, 5> sat_const{
        0,
        0x7878787878787878,
        0x7070707070707070,
        0x6868686868686868,
        0x6060606060606060,
    };

    if (code.HasHostFeature(HostFeature::SSSE3) && is_defaults_zero && table_size <= 2) {
        const Xbyak::Xmm indicies = ctx.reg_alloc.UseScratchXmm(args[2]);
        const Xbyak::Xmm xmm_table0 = ctx.reg_alloc.UseScratchXmm(table[0]);

        if (table_size == 2) {
            const Xbyak::Xmm xmm_table0_upper = ctx.reg_alloc.UseXmm(table[1]);
            code.punpcklqdq(xmm_table0, xmm_table0_upper);
            ctx.reg_alloc.Release(xmm_table0_upper);
        }

        code.paddusb(indicies, code.XmmConst(xword, 0x7070707070707070, 0xFFFFFFFFFFFFFFFF));
        code.pshufb(xmm_table0, indicies);

        ctx.reg_alloc.DefineValue(inst, xmm_table0);
        return;
    }

    if (code.HasHostFeature(HostFeature::SSE41) && table_size <= 2) {
        const Xbyak::Xmm indicies = ctx.reg_alloc.UseXmm(args[2]);
        const Xbyak::Xmm defaults = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm xmm_table0 = ctx.reg_alloc.UseScratchXmm(table[0]);

        if (table_size == 2) {
            const Xbyak::Xmm xmm_table0_upper = ctx.reg_alloc.UseXmm(table[1]);
            code.punpcklqdq(xmm_table0, xmm_table0_upper);
            ctx.reg_alloc.Release(xmm_table0_upper);
        }

        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpaddusb(xmm0, indicies, code.XmmConst(xword, sat_const[table_size], 0xFFFFFFFFFFFFFFFF));
        } else {
            code.movaps(xmm0, indicies);
            code.paddusb(xmm0, code.XmmConst(xword, sat_const[table_size], 0xFFFFFFFFFFFFFFFF));
        }
        code.pshufb(xmm_table0, indicies);
        code.pblendvb(xmm_table0, defaults);

        ctx.reg_alloc.DefineValue(inst, xmm_table0);
        return;
    }

    if (code.HasHostFeature(HostFeature::SSE41) && is_defaults_zero) {
        const Xbyak::Xmm indicies = ctx.reg_alloc.UseScratchXmm(args[2]);
        const Xbyak::Xmm xmm_table0 = ctx.reg_alloc.UseScratchXmm(table[0]);
        const Xbyak::Xmm xmm_table1 = ctx.reg_alloc.UseScratchXmm(table[2]);

        {
            const Xbyak::Xmm xmm_table0_upper = ctx.reg_alloc.UseXmm(table[1]);
            code.punpcklqdq(xmm_table0, xmm_table0_upper);
            ctx.reg_alloc.Release(xmm_table0_upper);
        }
        if (table_size == 4) {
            const Xbyak::Xmm xmm_table1_upper = ctx.reg_alloc.UseXmm(table[3]);
            code.punpcklqdq(xmm_table1, xmm_table1_upper);
            ctx.reg_alloc.Release(xmm_table1_upper);
        }

        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpaddusb(xmm0, indicies, code.XmmConst(xword, 0x7070707070707070, 0xFFFFFFFFFFFFFFFF));
        } else {
            code.movaps(xmm0, indicies);
            code.paddusb(xmm0, code.XmmConst(xword, 0x7070707070707070, 0xFFFFFFFFFFFFFFFF));
        }
        code.paddusb(indicies, code.XmmConst(xword, 0x6060606060606060, 0xFFFFFFFFFFFFFFFF));
        code.pshufb(xmm_table0, xmm0);
        code.pshufb(xmm_table1, indicies);
        code.pblendvb(xmm_table0, xmm_table1);

        ctx.reg_alloc.DefineValue(inst, xmm_table0);
        return;
    }

    if (code.HasHostFeature(HostFeature::SSE41)) {
        const Xbyak::Xmm indicies = ctx.reg_alloc.UseScratchXmm(args[2]);
        const Xbyak::Xmm defaults = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm xmm_table0 = ctx.reg_alloc.UseScratchXmm(table[0]);
        const Xbyak::Xmm xmm_table1 = ctx.reg_alloc.UseScratchXmm(table[2]);

        {
            const Xbyak::Xmm xmm_table0_upper = ctx.reg_alloc.UseXmm(table[1]);
            code.punpcklqdq(xmm_table0, xmm_table0_upper);
            ctx.reg_alloc.Release(xmm_table0_upper);
        }
        if (table_size == 4) {
            const Xbyak::Xmm xmm_table1_upper = ctx.reg_alloc.UseXmm(table[3]);
            code.punpcklqdq(xmm_table1, xmm_table1_upper);
            ctx.reg_alloc.Release(xmm_table1_upper);
        }

        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpaddusb(xmm0, indicies, code.XmmConst(xword, 0x7070707070707070, 0xFFFFFFFFFFFFFFFF));
        } else {
            code.movaps(xmm0, indicies);
            code.paddusb(xmm0, code.XmmConst(xword, 0x7070707070707070, 0xFFFFFFFFFFFFFFFF));
        }
        code.pshufb(xmm_table0, indicies);
        code.pshufb(xmm_table1, indicies);
        code.pblendvb(xmm_table0, xmm_table1);
        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpaddusb(xmm0, indicies, code.XmmConst(xword, sat_const[table_size], 0xFFFFFFFFFFFFFFFF));
        } else {
            code.movaps(xmm0, indicies);
            code.paddusb(xmm0, code.XmmConst(xword, sat_const[table_size], 0xFFFFFFFFFFFFFFFF));
        }
        code.pblendvb(xmm_table0, defaults);

        ctx.reg_alloc.DefineValue(inst, xmm_table0);
        return;
    }

    const u32 stack_space = static_cast<u32>(6 * 8);
    ctx.reg_alloc.AllocStackSpace(stack_space + ABI_SHADOW_SPACE);
    for (size_t i = 0; i < table_size; ++i) {
        const Xbyak::Xmm table_value = ctx.reg_alloc.UseXmm(table[i]);
        code.movq(qword[rsp + ABI_SHADOW_SPACE + i * 8], table_value);
        ctx.reg_alloc.Release(table_value);
    }
    const Xbyak::Xmm defaults = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm indicies = ctx.reg_alloc.UseXmm(args[2]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    ctx.reg_alloc.EndOfAllocScope();
    ctx.reg_alloc.HostCall(nullptr);

    code.lea(code.ABI_PARAM1, ptr[rsp + ABI_SHADOW_SPACE]);
    code.lea(code.ABI_PARAM2, ptr[rsp + ABI_SHADOW_SPACE + 4 * 8]);
    code.lea(code.ABI_PARAM3, ptr[rsp + ABI_SHADOW_SPACE + 5 * 8]);
    code.mov(code.ABI_PARAM4.cvt32(), table_size);
    code.movq(qword[code.ABI_PARAM2], defaults);
    code.movq(qword[code.ABI_PARAM3], indicies);

    code.CallLambda(
        [](const HalfVectorArray<u8>* table, HalfVectorArray<u8>& result, const HalfVectorArray<u8>& indicies, size_t table_size) {
            for (size_t i = 0; i < result.size(); ++i) {
                const size_t index = indicies[i] / table[0].size();
                const size_t elem = indicies[i] % table[0].size();
                if (index < table_size) {
                    result[i] = table[index][elem];
                }
            }
        });

    code.movq(result, qword[rsp + ABI_SHADOW_SPACE + 4 * 8]);
    ctx.reg_alloc.ReleaseStackSpace(stack_space + ABI_SHADOW_SPACE);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorTableLookup128(EmitContext& ctx, IR::Inst* inst) {
    ASSERT(inst->GetArg(1).GetInst()->GetOpcode() == IR::Opcode::VectorTable);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    auto table = ctx.reg_alloc.GetArgumentInfo(inst->GetArg(1).GetInst());

    const size_t table_size = std::count_if(table.begin(), table.end(), [](const auto& elem) { return !elem.IsVoid(); });
    const bool is_defaults_zero = !inst->GetArg(0).IsImmediate() && inst->GetArg(0).GetInst()->GetOpcode() == IR::Opcode::ZeroVector;

    if (code.HasHostFeature(HostFeature::AVX512_Ortho | HostFeature::AVX512BW | HostFeature::AVX512VBMI) && table_size == 4) {
        const Xbyak::Xmm indicies = ctx.reg_alloc.UseScratchXmm(args[2]);
        const Xbyak::Xmm defaults = ctx.reg_alloc.UseScratchXmm(args[0]);

        const Xbyak::Opmask write_mask = k1;
        const Xbyak::Opmask upper_mask = k2;

        // Handle vector-table 0,1
        const Xbyak::Xmm xmm_table0 = ctx.reg_alloc.UseScratchXmm(table[0]);
        const Xbyak::Xmm xmm_table1 = ctx.reg_alloc.UseScratchXmm(table[1]);

        code.vptestnmb(write_mask, indicies, code.XmmBConst<8>(xword, 0xE0));
        code.vpermi2b(indicies | write_mask, xmm_table0, xmm_table1);

        ctx.reg_alloc.Release(xmm_table0);
        ctx.reg_alloc.Release(xmm_table1);

        if (is_defaults_zero) {
            code.vmovdqu8(defaults | write_mask | T_z, indicies);
        } else {
            code.vmovdqu8(defaults | write_mask, indicies);
        }

        // Handle vector-table 2,3
        // vpcmpuble
        code.vpcmpub(upper_mask, indicies, code.XmmBConst<8>(xword, 0x3F), CmpInt::LessEqual);
        code.kandnw(write_mask, write_mask, upper_mask);

        const Xbyak::Xmm xmm_table2 = ctx.reg_alloc.UseScratchXmm(table[2]);
        const Xbyak::Xmm xmm_table3 = ctx.reg_alloc.UseScratchXmm(table[3]);

        code.vpermi2b(indicies, xmm_table2, xmm_table3);
        code.vmovdqu8(defaults | write_mask, indicies);

        ctx.reg_alloc.DefineValue(inst, defaults);
        return;
    }

    if (code.HasHostFeature(HostFeature::AVX512_Ortho | HostFeature::AVX512BW | HostFeature::AVX512VBMI) && table_size == 2) {
        const Xbyak::Xmm indicies = ctx.reg_alloc.UseScratchXmm(args[2]);
        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm xmm_table0 = ctx.reg_alloc.UseScratchXmm(table[0]);
        const Xbyak::Xmm xmm_table1 = ctx.reg_alloc.UseScratchXmm(table[1]);
        const Xbyak::Opmask write_mask = k1;

        code.vptestnmb(write_mask, indicies, code.XmmBConst<8>(xword, 0xE0));
        code.vpermi2b(indicies, xmm_table0, xmm_table1);

        if (is_defaults_zero) {
            code.vmovdqu8(result | write_mask | T_z, indicies);
        } else {
            code.vmovdqu8(result | write_mask, indicies);
        }

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    if (code.HasHostFeature(HostFeature::SSSE3) && is_defaults_zero && table_size == 1) {
        const Xbyak::Xmm indicies = ctx.reg_alloc.UseScratchXmm(args[2]);
        const Xbyak::Xmm xmm_table0 = ctx.reg_alloc.UseScratchXmm(table[0]);

        code.paddusb(indicies, code.XmmBConst<8>(xword, 0x70));
        code.pshufb(xmm_table0, indicies);

        ctx.reg_alloc.DefineValue(inst, xmm_table0);
        return;
    }

    if (code.HasHostFeature(HostFeature::SSE41) && table_size == 1) {
        const Xbyak::Xmm indicies = ctx.reg_alloc.UseXmm(args[2]);
        const Xbyak::Xmm defaults = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm xmm_table0 = ctx.reg_alloc.UseScratchXmm(table[0]);

        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpaddusb(xmm0, indicies, code.XmmBConst<8>(xword, 0x70));
        } else {
            code.movaps(xmm0, indicies);
            code.paddusb(xmm0, code.XmmBConst<8>(xword, 0x70));
        }
        code.pshufb(xmm_table0, indicies);
        code.pblendvb(xmm_table0, defaults);

        ctx.reg_alloc.DefineValue(inst, xmm_table0);
        return;
    }

    if (code.HasHostFeature(HostFeature::SSE41) && is_defaults_zero && table_size == 2) {
        const Xbyak::Xmm indicies = ctx.reg_alloc.UseScratchXmm(args[2]);
        const Xbyak::Xmm xmm_table0 = ctx.reg_alloc.UseScratchXmm(table[0]);
        const Xbyak::Xmm xmm_table1 = ctx.reg_alloc.UseScratchXmm(table[1]);

        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpaddusb(xmm0, indicies, code.XmmBConst<8>(xword, 0x70));
        } else {
            code.movaps(xmm0, indicies);
            code.paddusb(xmm0, code.XmmBConst<8>(xword, 0x70));
        }
        code.paddusb(indicies, code.XmmBConst<8>(xword, 0x60));
        code.pshufb(xmm_table0, xmm0);
        code.pshufb(xmm_table1, indicies);
        code.pblendvb(xmm_table0, xmm_table1);

        ctx.reg_alloc.DefineValue(inst, xmm_table0);
        return;
    }

    if (code.HasHostFeature(HostFeature::AVX512_Ortho | HostFeature::AVX512BW)) {
        const Xbyak::Xmm indicies = ctx.reg_alloc.UseXmm(args[2]);
        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm masked = xmm16;

        code.vpandd(masked, indicies, code.XmmBConst<8>(xword_b, 0xF0));

        for (size_t i = 0; i < table_size; ++i) {
            const Xbyak::Xmm xmm_table = ctx.reg_alloc.UseScratchXmm(table[i]);
            const Xbyak::Opmask table_mask = k1;
            const u8 table_index = u8(i * 16);

            code.vpcmpeqb(table_mask, masked, code.XmmBConst<8>(xword, i * 16));

            if (table_index == 0 && is_defaults_zero) {
                code.vpshufb(result | table_mask | T_z, xmm_table, indicies);
            } else {
                code.vpshufb(result | table_mask, xmm_table, indicies);
            }

            ctx.reg_alloc.Release(xmm_table);
        }

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    if (code.HasHostFeature(HostFeature::SSE41)) {
        const Xbyak::Xmm indicies = ctx.reg_alloc.UseXmm(args[2]);
        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm masked = ctx.reg_alloc.ScratchXmm();

        code.movaps(masked, code.XmmBConst<8>(xword, 0xF0));
        code.pand(masked, indicies);

        for (size_t i = 0; i < table_size; ++i) {
            const Xbyak::Xmm xmm_table = ctx.reg_alloc.UseScratchXmm(table[i]);

            const u8 table_index = u8(i * 16);

            if (table_index == 0) {
                code.pxor(xmm0, xmm0);
                code.pcmpeqb(xmm0, masked);
            } else if (code.HasHostFeature(HostFeature::AVX)) {
                code.vpcmpeqb(xmm0, masked, code.XmmBConst<8>(xword, table_index));
            } else {
                code.movaps(xmm0, code.XmmBConst<8>(xword, table_index));
                code.pcmpeqb(xmm0, masked);
            }
            code.pshufb(xmm_table, indicies);
            code.pblendvb(result, xmm_table);

            ctx.reg_alloc.Release(xmm_table);
        }

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    const u32 stack_space = static_cast<u32>((table_size + 2) * 16);
    ctx.reg_alloc.AllocStackSpace(stack_space + ABI_SHADOW_SPACE);
    for (size_t i = 0; i < table_size; ++i) {
        const Xbyak::Xmm table_value = ctx.reg_alloc.UseXmm(table[i]);
        code.movaps(xword[rsp + ABI_SHADOW_SPACE + i * 16], table_value);
        ctx.reg_alloc.Release(table_value);
    }
    const Xbyak::Xmm defaults = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm indicies = ctx.reg_alloc.UseXmm(args[2]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    ctx.reg_alloc.EndOfAllocScope();
    ctx.reg_alloc.HostCall(nullptr);

    code.lea(code.ABI_PARAM1, ptr[rsp + ABI_SHADOW_SPACE]);
    code.lea(code.ABI_PARAM2, ptr[rsp + ABI_SHADOW_SPACE + (table_size + 0) * 16]);
    code.lea(code.ABI_PARAM3, ptr[rsp + ABI_SHADOW_SPACE + (table_size + 1) * 16]);
    code.mov(code.ABI_PARAM4.cvt32(), table_size);
    code.movaps(xword[code.ABI_PARAM2], defaults);
    code.movaps(xword[code.ABI_PARAM3], indicies);

    code.CallLambda(
        [](const VectorArray<u8>* table, VectorArray<u8>& result, const VectorArray<u8>& indicies, size_t table_size) {
            for (size_t i = 0; i < result.size(); ++i) {
                const size_t index = indicies[i] / table[0].size();
                const size_t elem = indicies[i] % table[0].size();
                if (index < table_size) {
                    result[i] = table[index][elem];
                }
            }
        });

    code.movaps(result, xword[rsp + ABI_SHADOW_SPACE + (table_size + 0) * 16]);
    ctx.reg_alloc.ReleaseStackSpace(stack_space + ABI_SHADOW_SPACE);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitVectorTranspose8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm lower = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm upper = ctx.reg_alloc.UseScratchXmm(args[1]);
    const bool part = args[2].GetImmediateU1();

    if (!part) {
        code.pand(lower, code.XmmBConst<16>(xword, 0x00FF));
        code.psllw(upper, 8);
    } else {
        code.psrlw(lower, 8);
        code.pand(upper, code.XmmBConst<16>(xword, 0xFF00));
    }
    code.por(lower, upper);

    ctx.reg_alloc.DefineValue(inst, lower);
}

void EmitX64::EmitVectorTranspose16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm lower = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm upper = ctx.reg_alloc.UseScratchXmm(args[1]);
    const bool part = args[2].GetImmediateU1();

    if (!part) {
        code.pand(lower, code.XmmBConst<32>(xword, 0x0000FFFF));
        code.pslld(upper, 16);
    } else {
        code.psrld(lower, 16);
        code.pand(upper, code.XmmBConst<32>(xword, 0xFFFF0000));
    }
    code.por(lower, upper);

    ctx.reg_alloc.DefineValue(inst, lower);
}

void EmitX64::EmitVectorTranspose32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm lower = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm upper = ctx.reg_alloc.UseXmm(args[1]);
    const bool part = args[2].GetImmediateU1();

    code.shufps(lower, upper, !part ? 0b10001000 : 0b11011101);
    code.pshufd(lower, lower, 0b11011000);

    ctx.reg_alloc.DefineValue(inst, lower);
}

void EmitX64::EmitVectorTranspose64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm lower = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm upper = ctx.reg_alloc.UseXmm(args[1]);
    const bool part = args[2].GetImmediateU1();

    code.shufpd(lower, upper, !part ? 0b00 : 0b11);

    ctx.reg_alloc.DefineValue(inst, lower);
}

static void EmitVectorUnsignedAbsoluteDifference(size_t esize, EmitContext& ctx, IR::Inst* inst, BlockOfCode& code) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm temp = ctx.reg_alloc.ScratchXmm();

    switch (esize) {
    case 8: {
        const Xbyak::Xmm x = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm y = ctx.reg_alloc.UseScratchXmm(args[1]);

        code.movdqa(temp, x);
        code.psubusb(temp, y);
        code.psubusb(y, x);
        code.por(temp, y);
        break;
    }
    case 16: {
        const Xbyak::Xmm x = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm y = ctx.reg_alloc.UseScratchXmm(args[1]);

        code.movdqa(temp, x);
        code.psubusw(temp, y);
        code.psubusw(y, x);
        code.por(temp, y);
        break;
    }
    case 32:
        if (code.HasHostFeature(HostFeature::SSE41)) {
            const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
            const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);

            code.movdqa(temp, x);
            code.pminud(x, y);
            code.pmaxud(temp, y);
            code.psubd(temp, x);
        } else {
            const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
            const Xbyak::Xmm y = ctx.reg_alloc.UseScratchXmm(args[1]);

            code.movdqa(temp, code.XmmBConst<32>(xword, 0x80000000));
            code.pxor(x, temp);
            code.pxor(y, temp);
            code.movdqa(temp, x);
            code.psubd(temp, y);
            code.pcmpgtd(y, x);
            code.psrld(y, 1);
            code.pxor(temp, y);
            code.psubd(temp, y);
        }
        break;
    }

    ctx.reg_alloc.DefineValue(inst, temp);
}

void EmitX64::EmitVectorUnsignedAbsoluteDifference8(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorUnsignedAbsoluteDifference(8, ctx, inst, code);
}

void EmitX64::EmitVectorUnsignedAbsoluteDifference16(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorUnsignedAbsoluteDifference(16, ctx, inst, code);
}

void EmitX64::EmitVectorUnsignedAbsoluteDifference32(EmitContext& ctx, IR::Inst* inst) {
    EmitVectorUnsignedAbsoluteDifference(32, ctx, inst, code);
}

void EmitX64::EmitVectorUnsignedMultiply16(EmitContext& ctx, IR::Inst* inst) {
    const auto upper_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetUpperFromOp);
    const auto lower_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetLowerFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm x = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);

    if (upper_inst) {
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpmulhuw(result, x, y);
        } else {
            code.movdqa(result, x);
            code.pmulhuw(result, y);
        }

        ctx.reg_alloc.DefineValue(upper_inst, result);
        ctx.EraseInstruction(upper_inst);
    }

    if (lower_inst) {
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        if (code.HasHostFeature(HostFeature::AVX)) {
            code.vpmullw(result, x, y);
        } else {
            code.movdqa(result, x);
            code.pmullw(result, y);
        }
        ctx.reg_alloc.DefineValue(lower_inst, result);
        ctx.EraseInstruction(lower_inst);
    }
}

void EmitX64::EmitVectorUnsignedMultiply32(EmitContext& ctx, IR::Inst* inst) {
    const auto upper_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetUpperFromOp);
    const auto lower_inst = inst->GetAssociatedPseudoOperation(IR::Opcode::GetLowerFromOp);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (lower_inst && !upper_inst && code.HasHostFeature(HostFeature::AVX)) {
        const Xbyak::Xmm x = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm y = ctx.reg_alloc.UseXmm(args[1]);
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

        code.vpmulld(result, x, y);

        ctx.reg_alloc.DefineValue(lower_inst, result);
        ctx.EraseInstruction(lower_inst);
        return;
    }

    if (code.HasHostFeature(HostFeature::AVX)) {
        const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm y = ctx.reg_alloc.UseScratchXmm(args[1]);

        if (lower_inst) {
            const Xbyak::Xmm lower_result = ctx.reg_alloc.ScratchXmm();
            code.vpmulld(lower_result, x, y);
            ctx.reg_alloc.DefineValue(lower_inst, lower_result);
            ctx.EraseInstruction(lower_inst);
        }

        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

        code.vpmuludq(result, x, y);
        code.vpsrlq(x, x, 32);
        code.vpsrlq(y, y, 32);
        code.vpmuludq(x, x, y);
        code.shufps(result, x, 0b11011101);

        ctx.reg_alloc.DefineValue(upper_inst, result);
        ctx.EraseInstruction(upper_inst);
        return;
    }

    const Xbyak::Xmm x = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm y = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm upper_result = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm lower_result = ctx.reg_alloc.ScratchXmm();

    // calculate unsigned multiply
    code.movdqa(tmp, x);
    code.pmuludq(tmp, y);
    code.psrlq(x, 32);
    code.psrlq(y, 32);
    code.pmuludq(x, y);

    // put everything into place
    code.pcmpeqw(upper_result, upper_result);
    code.pcmpeqw(lower_result, lower_result);
    code.psllq(upper_result, 32);
    code.psrlq(lower_result, 32);
    code.pand(upper_result, x);
    code.pand(lower_result, tmp);
    code.psrlq(tmp, 32);
    code.psllq(x, 32);
    code.por(upper_result, tmp);
    code.por(lower_result, x);

    if (upper_inst) {
        ctx.reg_alloc.DefineValue(upper_inst, upper_result);
        ctx.EraseInstruction(upper_inst);
    }
    if (lower_inst) {
        ctx.reg_alloc.DefineValue(lower_inst, lower_result);
        ctx.EraseInstruction(lower_inst);
    }
}

void EmitX64::EmitVectorUnsignedRecipEstimate(EmitContext& ctx, IR::Inst* inst) {
    EmitOneArgumentFallback(code, ctx, inst, [](VectorArray<u32>& result, const VectorArray<u32>& a) {
        for (size_t i = 0; i < result.size(); i++) {
            if ((a[i] & 0x80000000) == 0) {
                result[i] = 0xFFFFFFFF;
                continue;
            }

            const u32 input = mcl::bit::get_bits<23, 31>(a[i]);
            const u32 estimate = Common::RecipEstimate(input);

            result[i] = (0b100000000 | estimate) << 23;
        }
    });
}

void EmitX64::EmitVectorUnsignedRecipSqrtEstimate(EmitContext& ctx, IR::Inst* inst) {
    EmitOneArgumentFallback(code, ctx, inst, [](VectorArray<u32>& result, const VectorArray<u32>& a) {
        for (size_t i = 0; i < result.size(); i++) {
            if ((a[i] & 0xC0000000) == 0) {
                result[i] = 0xFFFFFFFF;
                continue;
            }

            const u32 input = mcl::bit::get_bits<23, 31>(a[i]);
            const u32 estimate = Common::RecipSqrtEstimate(input);

            result[i] = (0b100000000 | estimate) << 23;
        }
    });
}

// Simple generic case for 8, 16, and 32-bit values. 64-bit values
// will need to be special-cased as we can't simply use a larger integral size.
template<typename T, typename U = std::make_unsigned_t<T>>
static bool EmitVectorUnsignedSaturatedAccumulateSigned(VectorArray<U>& result, const VectorArray<T>& lhs, const VectorArray<T>& rhs) {
    static_assert(std::is_signed_v<T>, "T must be signed.");
    static_assert(mcl::bitsizeof<T> < 64, "T must be less than 64 bits in size.");

    bool qc_flag = false;

    for (size_t i = 0; i < result.size(); i++) {
        // We treat rhs' members as unsigned, so cast to unsigned before signed to inhibit sign-extension.
        // We use the unsigned equivalent of T, as we want zero-extension to occur, rather than a plain move.
        const s64 x = s64{lhs[i]};
        const s64 y = static_cast<s64>(static_cast<std::make_unsigned_t<U>>(rhs[i]));
        const s64 sum = x + y;

        if (sum > std::numeric_limits<U>::max()) {
            result[i] = std::numeric_limits<U>::max();
            qc_flag = true;
        } else if (sum < 0) {
            result[i] = std::numeric_limits<U>::min();
            qc_flag = true;
        } else {
            result[i] = static_cast<U>(sum);
        }
    }

    return qc_flag;
}

void EmitX64::EmitVectorUnsignedSaturatedAccumulateSigned8(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, EmitVectorUnsignedSaturatedAccumulateSigned<s8>);
}

void EmitX64::EmitVectorUnsignedSaturatedAccumulateSigned16(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, EmitVectorUnsignedSaturatedAccumulateSigned<s16>);
}

void EmitX64::EmitVectorUnsignedSaturatedAccumulateSigned32(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, EmitVectorUnsignedSaturatedAccumulateSigned<s32>);
}

void EmitX64::EmitVectorUnsignedSaturatedAccumulateSigned64(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, [](VectorArray<u64>& result, const VectorArray<u64>& lhs, const VectorArray<u64>& rhs) {
        bool qc_flag = false;

        for (size_t i = 0; i < result.size(); i++) {
            const u64 x = lhs[i];
            const u64 y = rhs[i];
            const u64 res = x + y;

            // Check sign bits to determine if an overflow occurred.
            if ((~x & y & ~res) & 0x8000000000000000) {
                result[i] = UINT64_MAX;
                qc_flag = true;
            } else if ((x & ~y & res) & 0x8000000000000000) {
                result[i] = 0;
                qc_flag = true;
            } else {
                result[i] = res;
            }
        }

        return qc_flag;
    });
}

void EmitX64::EmitVectorUnsignedSaturatedNarrow16(EmitContext& ctx, IR::Inst* inst) {
    EmitOneArgumentFallbackWithSaturation(code, ctx, inst, [](VectorArray<u8>& result, const VectorArray<u16>& a) {
        bool qc_flag = false;
        for (size_t i = 0; i < a.size(); ++i) {
            const u16 saturated = std::clamp<u16>(a[i], 0, 0xFF);
            result[i] = static_cast<u8>(saturated);
            qc_flag |= saturated != a[i];
        }
        return qc_flag;
    });
}

void EmitX64::EmitVectorUnsignedSaturatedNarrow32(EmitContext& ctx, IR::Inst* inst) {
    EmitOneArgumentFallbackWithSaturation(code, ctx, inst, [](VectorArray<u16>& result, const VectorArray<u32>& a) {
        bool qc_flag = false;
        for (size_t i = 0; i < a.size(); ++i) {
            const u32 saturated = std::clamp<u32>(a[i], 0, 0xFFFF);
            result[i] = static_cast<u16>(saturated);
            qc_flag |= saturated != a[i];
        }
        return qc_flag;
    });
}

void EmitX64::EmitVectorUnsignedSaturatedNarrow64(EmitContext& ctx, IR::Inst* inst) {
    EmitOneArgumentFallbackWithSaturation(code, ctx, inst, [](VectorArray<u32>& result, const VectorArray<u64>& a) {
        bool qc_flag = false;
        for (size_t i = 0; i < a.size(); ++i) {
            const u64 saturated = std::clamp<u64>(a[i], 0, 0xFFFFFFFF);
            result[i] = static_cast<u32>(saturated);
            qc_flag |= saturated != a[i];
        }
        return qc_flag;
    });
}

template<typename T, typename S = std::make_signed_t<T>>
static bool VectorUnsignedSaturatedShiftLeft(VectorArray<T>& dst, const VectorArray<T>& data, const VectorArray<T>& shift_values) {
    static_assert(std::is_unsigned_v<T>, "T must be an unsigned type.");

    bool qc_flag = false;

    constexpr size_t bit_size = mcl::bitsizeof<T>;
    constexpr S negative_bit_size = -static_cast<S>(bit_size);

    for (size_t i = 0; i < dst.size(); i++) {
        const T element = data[i];
        const S shift = std::clamp(static_cast<S>(mcl::bit::sign_extend<8>(static_cast<T>(shift_values[i] & 0xFF))),
                                   negative_bit_size, std::numeric_limits<S>::max());

        if (element == 0 || shift <= negative_bit_size) {
            dst[i] = 0;
        } else if (shift < 0) {
            dst[i] = static_cast<T>(element >> -shift);
        } else if (shift >= static_cast<S>(bit_size)) {
            dst[i] = std::numeric_limits<T>::max();
            qc_flag = true;
        } else {
            const T shifted = element << shift;

            if ((shifted >> shift) != element) {
                dst[i] = std::numeric_limits<T>::max();
                qc_flag = true;
            } else {
                dst[i] = shifted;
            }
        }
    }

    return qc_flag;
}

void EmitX64::EmitVectorUnsignedSaturatedShiftLeft8(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, VectorUnsignedSaturatedShiftLeft<u8>);
}

void EmitX64::EmitVectorUnsignedSaturatedShiftLeft16(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, VectorUnsignedSaturatedShiftLeft<u16>);
}

void EmitX64::EmitVectorUnsignedSaturatedShiftLeft32(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, VectorUnsignedSaturatedShiftLeft<u32>);
}

void EmitX64::EmitVectorUnsignedSaturatedShiftLeft64(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoArgumentFallbackWithSaturation(code, ctx, inst, VectorUnsignedSaturatedShiftLeft<u64>);
}

void EmitX64::EmitVectorZeroExtend8(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.pmovzxbw(a, a);
    } else {
        const Xbyak::Xmm zeros = ctx.reg_alloc.ScratchXmm();
        code.pxor(zeros, zeros);
        code.punpcklbw(a, zeros);
    }
    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorZeroExtend16(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.pmovzxwd(a, a);
    } else {
        const Xbyak::Xmm zeros = ctx.reg_alloc.ScratchXmm();
        code.pxor(zeros, zeros);
        code.punpcklwd(a, zeros);
    }
    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorZeroExtend32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.pmovzxdq(a, a);
    } else {
        const Xbyak::Xmm zeros = ctx.reg_alloc.ScratchXmm();
        code.pxor(zeros, zeros);
        code.punpckldq(a, zeros);
    }
    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorZeroExtend64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm zeros = ctx.reg_alloc.ScratchXmm();
    code.pxor(zeros, zeros);
    code.punpcklqdq(a, zeros);
    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitVectorZeroUpper(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);

    code.movq(a, a);  // TODO: !IsLastUse

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitZeroVector(EmitContext& ctx, IR::Inst* inst) {
    const Xbyak::Xmm a = ctx.reg_alloc.ScratchXmm();
    code.pxor(a, a);
    ctx.reg_alloc.DefineValue(inst, a);
}

}  // namespace Dynarmic::Backend::X64

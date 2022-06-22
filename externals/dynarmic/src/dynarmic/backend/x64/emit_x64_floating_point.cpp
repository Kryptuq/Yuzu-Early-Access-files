/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <optional>
#include <type_traits>
#include <utility>

#include <mcl/assert.hpp>
#include <mcl/mp/metavalue/lift_value.hpp>
#include <mcl/mp/typelist/cartesian_product.hpp>
#include <mcl/mp/typelist/get.hpp>
#include <mcl/mp/typelist/lift_sequence.hpp>
#include <mcl/mp/typelist/list.hpp>
#include <mcl/mp/typelist/lower_to_tuple.hpp>
#include <mcl/stdint.hpp>
#include <mcl/type_traits/integer_of_size.hpp>

#include "dynarmic/backend/x64/abi.h"
#include "dynarmic/backend/x64/block_of_code.h"
#include "dynarmic/backend/x64/constants.h"
#include "dynarmic/backend/x64/emit_x64.h"
#include "dynarmic/common/cast_util.h"
#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/fpsr.h"
#include "dynarmic/common/fp/info.h"
#include "dynarmic/common/fp/op.h"
#include "dynarmic/common/fp/rounding_mode.h"
#include "dynarmic/common/lut_from_list.h"
#include "dynarmic/interface/optimization_flags.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"

namespace Dynarmic::Backend::X64 {

using namespace Xbyak::util;
namespace mp = mcl::mp;

namespace {

const Xbyak::Reg64 INVALID_REG = Xbyak::Reg64(-1);

constexpr u64 f32_negative_zero = 0x80000000u;
constexpr u64 f32_nan = 0x7fc00000u;
constexpr u64 f32_non_sign_mask = 0x7fffffffu;
constexpr u64 f32_smallest_normal = 0x00800000u;

constexpr u64 f64_negative_zero = 0x8000000000000000u;
constexpr u64 f64_nan = 0x7ff8000000000000u;
constexpr u64 f64_non_sign_mask = 0x7fffffffffffffffu;
constexpr u64 f64_smallest_normal = 0x0010000000000000u;

constexpr u64 f64_min_s16 = 0xc0e0000000000000u;      // -32768 as a double
constexpr u64 f64_max_s16 = 0x40dfffc000000000u;      // 32767 as a double
constexpr u64 f64_min_u16 = 0x0000000000000000u;      // 0 as a double
constexpr u64 f64_max_u16 = 0x40efffe000000000u;      // 65535 as a double
constexpr u64 f64_max_s32 = 0x41dfffffffc00000u;      // 2147483647 as a double
constexpr u64 f64_max_u32 = 0x41efffffffe00000u;      // 4294967295 as a double
constexpr u64 f64_max_s64_lim = 0x43e0000000000000u;  // 2^63 as a double (actual maximum unrepresentable)

#define FCODE(NAME)                  \
    [&code](auto... args) {          \
        if constexpr (fsize == 32) { \
            code.NAME##s(args...);   \
        } else {                     \
            code.NAME##d(args...);   \
        }                            \
    }
#define ICODE(NAME)                  \
    [&code](auto... args) {          \
        if constexpr (fsize == 32) { \
            code.NAME##d(args...);   \
        } else {                     \
            code.NAME##q(args...);   \
        }                            \
    }

template<size_t fsize>
void DenormalsAreZero(BlockOfCode& code, EmitContext& ctx, std::initializer_list<Xbyak::Xmm> to_daz) {
    if (ctx.FPCR().FZ()) {
        for (const Xbyak::Xmm& xmm : to_daz) {
            code.movaps(xmm0, code.MConst(xword, fsize == 32 ? f32_non_sign_mask : f64_non_sign_mask));
            code.andps(xmm0, xmm);
            if constexpr (fsize == 32) {
                code.pcmpgtd(xmm0, code.MConst(xword, f32_smallest_normal - 1));
            } else if (code.HasHostFeature(HostFeature::SSE42)) {
                code.pcmpgtq(xmm0, code.MConst(xword, f64_smallest_normal - 1));
            } else {
                code.pcmpgtd(xmm0, code.MConst(xword, f64_smallest_normal - 1));
                code.pshufd(xmm0, xmm0, 0b11100101);
            }
            code.orps(xmm0, code.MConst(xword, fsize == 32 ? f32_negative_zero : f64_negative_zero));
            code.andps(xmm, xmm0);
        }
    }
}

template<size_t fsize>
void ZeroIfNaN(BlockOfCode& code, Xbyak::Xmm xmm_value, Xbyak::Xmm xmm_scratch) {
    if (code.HasHostFeature(HostFeature::AVX512_OrthoFloat)) {
        constexpr u32 nan_to_zero = FixupLUT(FpFixup::PosZero,
                                             FpFixup::PosZero);
        FCODE(vfixupimms)(xmm_value, xmm_value, code.MConst(ptr, u64(nan_to_zero)), u8(0));
    } else if (code.HasHostFeature(HostFeature::AVX)) {
        FCODE(vcmpords)(xmm_scratch, xmm_value, xmm_value);
        FCODE(vandp)(xmm_value, xmm_value, xmm_scratch);
    } else {
        code.xorps(xmm_scratch, xmm_scratch);
        FCODE(cmpords)(xmm_scratch, xmm_value);  // true mask when ordered (i.e.: when not an NaN)
        code.pand(xmm_value, xmm_scratch);
    }
}

template<size_t fsize>
void ForceToDefaultNaN(BlockOfCode& code, Xbyak::Xmm result) {
    if (code.HasHostFeature(HostFeature::AVX)) {
        FCODE(vcmpunords)(xmm0, result, result);
        FCODE(blendvp)(result, code.MConst(xword, fsize == 32 ? f32_nan : f64_nan));
    } else {
        Xbyak::Label end;
        FCODE(ucomis)(result, result);
        code.jnp(end);
        code.movaps(result, code.MConst(xword, fsize == 32 ? f32_nan : f64_nan));
        code.L(end);
    }
}

template<size_t fsize>
Xbyak::Label ProcessNaN(BlockOfCode& code, Xbyak::Xmm a) {
    Xbyak::Label nan, end;

    FCODE(ucomis)(a, a);
    code.jp(nan, code.T_NEAR);
    code.SwitchToFarCode();
    code.L(nan);

    code.orps(a, code.MConst(xword, fsize == 32 ? 0x00400000 : 0x0008'0000'0000'0000));

    code.jmp(end, code.T_NEAR);
    code.SwitchToNearCode();
    return end;
}

template<size_t fsize>
void PostProcessNaN(BlockOfCode& code, Xbyak::Xmm result, Xbyak::Xmm tmp) {
    code.movaps(tmp, result);
    FCODE(cmpunordp)(tmp, tmp);
    ICODE(psll)(tmp, int(fsize - 1));
    code.xorps(result, tmp);
}

// This is necessary because x86 and ARM differ in they way they return NaNs from floating point operations
//
// ARM behaviour:
// op1         op2          result
// SNaN        SNaN/QNaN    op1
// QNaN        SNaN         op2
// QNaN        QNaN         op1
// SNaN/QNaN   other        op1
// other       SNaN/QNaN    op2
//
// x86 behaviour:
// op1         op2          result
// SNaN/QNaN   SNaN/QNaN    op1
// SNaN/QNaN   other        op1
// other       SNaN/QNaN    op2
//
// With ARM: SNaNs take priority. With x86: it doesn't matter.
//
// From the above we can see what differs between the architectures is
// the case when op1 == QNaN and op2 == SNaN.
//
// We assume that registers op1 and op2 are read-only. This function also trashes xmm0.
// We allow for the case where op1 and result are the same register. We do not read from op1 once result is written to.
template<size_t fsize>
void EmitPostProcessNaNs(BlockOfCode& code, Xbyak::Xmm result, Xbyak::Xmm op1, Xbyak::Xmm op2, Xbyak::Reg64 tmp, Xbyak::Label end) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;
    constexpr FPT exponent_mask = FP::FPInfo<FPT>::exponent_mask;
    constexpr FPT mantissa_msb = FP::FPInfo<FPT>::mantissa_msb;
    constexpr u8 mantissa_msb_bit = static_cast<u8>(FP::FPInfo<FPT>::explicit_mantissa_width - 1);

    // At this point we know that at least one of op1 and op2 is a NaN.
    // Thus in op1 ^ op2 at least one of the two would have all 1 bits in the exponent.
    // Keeping in mind xor is commutative, there are only four cases:
    // SNaN      ^ SNaN/Inf  -> exponent == 0, mantissa_msb == 0
    // QNaN      ^ QNaN      -> exponent == 0, mantissa_msb == 0
    // QNaN      ^ SNaN/Inf  -> exponent == 0, mantissa_msb == 1
    // SNaN/QNaN ^ Otherwise -> exponent != 0, mantissa_msb == ?
    //
    // We're only really interested in op1 == QNaN and op2 == SNaN,
    // so we filter out everything else.
    //
    // We do it this way instead of checking that op1 is QNaN because
    // op1 == QNaN && op2 == QNaN is the most common case. With this method
    // that case would only require one branch.

    if (code.HasHostFeature(HostFeature::AVX)) {
        code.vxorps(xmm0, op1, op2);
    } else {
        code.movaps(xmm0, op1);
        code.xorps(xmm0, op2);
    }

    constexpr size_t shift = fsize == 32 ? 0 : 48;
    if constexpr (fsize == 32) {
        code.movd(tmp.cvt32(), xmm0);
    } else {
        // We do this to avoid requiring 64-bit immediates
        code.pextrw(tmp.cvt32(), xmm0, shift / 16);
    }
    code.and_(tmp.cvt32(), static_cast<u32>((exponent_mask | mantissa_msb) >> shift));
    code.cmp(tmp.cvt32(), static_cast<u32>(mantissa_msb >> shift));
    code.jne(end, code.T_NEAR);

    // If we're here there are four cases left:
    // op1 == SNaN && op2 == QNaN
    // op1 == Inf  && op2 == QNaN
    // op1 == QNaN && op2 == SNaN <<< The problematic case
    // op1 == QNaN && op2 == Inf

    if constexpr (fsize == 32) {
        code.movd(tmp.cvt32(), op2);
        code.shl(tmp.cvt32(), 32 - mantissa_msb_bit);
    } else {
        code.movq(tmp, op2);
        code.shl(tmp, 64 - mantissa_msb_bit);
    }
    // If op2 is a SNaN, CF = 0 and ZF = 0.
    code.jna(end, code.T_NEAR);

    // Silence the SNaN as required by spec.
    if (code.HasHostFeature(HostFeature::AVX)) {
        code.vorps(result, op2, code.MConst(xword, mantissa_msb));
    } else {
        code.movaps(result, op2);
        code.orps(result, code.MConst(xword, mantissa_msb));
    }
    code.jmp(end, code.T_NEAR);
}

template<size_t fsize, typename Function>
void FPTwoOp(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, Function fn) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    Xbyak::Label end;

    Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);

    if (!ctx.FPCR().DN() && !ctx.HasOptimization(OptimizationFlag::Unsafe_InaccurateNaN)) {
        end = ProcessNaN<fsize>(code, result);
    }
    if constexpr (std::is_member_function_pointer_v<Function>) {
        (code.*fn)(result, result);
    } else {
        fn(result);
    }
    if (ctx.HasOptimization(OptimizationFlag::Unsafe_InaccurateNaN)) {
        // Do nothing
    } else if (ctx.FPCR().DN()) {
        ForceToDefaultNaN<fsize>(code, result);
    } else {
        PostProcessNaN<fsize>(code, result, ctx.reg_alloc.ScratchXmm());
    }
    code.L(end);

    ctx.reg_alloc.DefineValue(inst, result);
}

template<size_t fsize, typename Function>
void FPThreeOp(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, Function fn) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if (ctx.FPCR().DN() || ctx.HasOptimization(OptimizationFlag::Unsafe_InaccurateNaN)) {
        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm operand = ctx.reg_alloc.UseScratchXmm(args[1]);

        if constexpr (std::is_member_function_pointer_v<Function>) {
            (code.*fn)(result, operand);
        } else {
            fn(result, operand);
        }

        if (!ctx.HasOptimization(OptimizationFlag::Unsafe_InaccurateNaN)) {
            ForceToDefaultNaN<fsize>(code, result);
        }

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    const Xbyak::Xmm op1 = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm op2 = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Reg64 tmp = ctx.reg_alloc.ScratchGpr();

    Xbyak::Label end, nan, op_are_nans;

    code.movaps(result, op1);
    if constexpr (std::is_member_function_pointer_v<Function>) {
        (code.*fn)(result, op2);
    } else {
        fn(result, op2);
    }
    FCODE(ucomis)(result, result);
    code.jp(nan, code.T_NEAR);
    code.L(end);

    code.SwitchToFarCode();
    code.L(nan);
    FCODE(ucomis)(op1, op2);
    code.jp(op_are_nans);
    // Here we must return a positive NaN, because the indefinite value on x86 is a negative NaN!
    code.movaps(result, code.MConst(xword, FP::FPInfo<FPT>::DefaultNaN()));
    code.jmp(end, code.T_NEAR);
    code.L(op_are_nans);
    EmitPostProcessNaNs<fsize>(code, result, op1, op2, tmp, end);
    code.SwitchToNearCode();

    ctx.reg_alloc.DefineValue(inst, result);
}

}  // anonymous namespace

template<size_t fsize>
void FPAbs(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;
    constexpr FPT non_sign_mask = FP::FPInfo<FPT>::sign_mask - FPT(1u);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Address mask = code.MConst(xword, non_sign_mask);

    code.andps(result, mask);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitFPAbs16(EmitContext& ctx, IR::Inst* inst) {
    FPAbs<16>(code, ctx, inst);
}

void EmitX64::EmitFPAbs32(EmitContext& ctx, IR::Inst* inst) {
    FPAbs<32>(code, ctx, inst);
}

void EmitX64::EmitFPAbs64(EmitContext& ctx, IR::Inst* inst) {
    FPAbs<64>(code, ctx, inst);
}

template<size_t fsize>
void FPNeg(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;
    constexpr FPT sign_mask = FP::FPInfo<FPT>::sign_mask;

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Address mask = code.MConst(xword, u64(sign_mask));

    code.xorps(result, mask);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitFPNeg16(EmitContext& ctx, IR::Inst* inst) {
    FPNeg<16>(code, ctx, inst);
}

void EmitX64::EmitFPNeg32(EmitContext& ctx, IR::Inst* inst) {
    FPNeg<32>(code, ctx, inst);
}

void EmitX64::EmitFPNeg64(EmitContext& ctx, IR::Inst* inst) {
    FPNeg<64>(code, ctx, inst);
}

void EmitX64::EmitFPAdd32(EmitContext& ctx, IR::Inst* inst) {
    FPThreeOp<32>(code, ctx, inst, &Xbyak::CodeGenerator::addss);
}

void EmitX64::EmitFPAdd64(EmitContext& ctx, IR::Inst* inst) {
    FPThreeOp<64>(code, ctx, inst, &Xbyak::CodeGenerator::addsd);
}

void EmitX64::EmitFPDiv32(EmitContext& ctx, IR::Inst* inst) {
    FPThreeOp<32>(code, ctx, inst, &Xbyak::CodeGenerator::divss);
}

void EmitX64::EmitFPDiv64(EmitContext& ctx, IR::Inst* inst) {
    FPThreeOp<64>(code, ctx, inst, &Xbyak::CodeGenerator::divsd);
}

template<size_t fsize, bool is_max>
static void EmitFPMinMax(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm operand = ctx.reg_alloc.UseScratchXmm(args[1]);
    const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Reg64 gpr_scratch = ctx.reg_alloc.ScratchGpr();

    DenormalsAreZero<fsize>(code, ctx, {result, operand});

    Xbyak::Label equal, end, nan;

    FCODE(ucomis)(result, operand);
    code.jz(equal, code.T_NEAR);
    if constexpr (is_max) {
        FCODE(maxs)(result, operand);
    } else {
        FCODE(mins)(result, operand);
    }
    code.L(end);

    code.SwitchToFarCode();

    code.L(equal);
    code.jp(nan);
    if constexpr (is_max) {
        code.andps(result, operand);
    } else {
        code.orps(result, operand);
    }
    code.jmp(end);

    code.L(nan);
    if (ctx.FPCR().DN()) {
        code.movaps(result, code.MConst(xword, fsize == 32 ? f32_nan : f64_nan));
        code.jmp(end);
    } else {
        code.movaps(tmp, result);
        FCODE(adds)(result, operand);
        EmitPostProcessNaNs<fsize>(code, result, tmp, operand, gpr_scratch, end);
    }

    code.SwitchToNearCode();

    ctx.reg_alloc.DefineValue(inst, result);
}

template<size_t fsize, bool is_max>
static void EmitFPMinMaxNumeric(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;
    constexpr FPT default_nan = FP::FPInfo<FPT>::DefaultNaN();
    constexpr u8 mantissa_msb_bit = static_cast<u8>(FP::FPInfo<FPT>::explicit_mantissa_width - 1);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm op1 = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm op2 = ctx.reg_alloc.UseScratchXmm(args[1]);  // Result stored here!

    DenormalsAreZero<fsize>(code, ctx, {op1, op2});

    if (code.HasHostFeature(HostFeature::AVX512_OrthoFloat)) {
        // vrangep{s,d} will already correctly handle comparing
        // signed zeros and propagating NaNs similar to ARM
        constexpr FpRangeSelect range_select = is_max ? FpRangeSelect::Max : FpRangeSelect::Min;
        FCODE(vranges)(op2, op1, op2, FpRangeLUT(range_select, FpRangeSign::Preserve));

        if (ctx.FPCR().DN()) {
            FCODE(vcmps)(k1, op2, op2, Cmp::Unordered_Q);
            FCODE(vmovs)(op2 | k1, code.MConst(xword, default_nan));
        }
    } else {
        Xbyak::Reg tmp = ctx.reg_alloc.ScratchGpr();
        tmp.setBit(fsize);

        const auto move_to_tmp = [&](const Xbyak::Xmm& xmm) {
            if constexpr (fsize == 32) {
                code.movd(tmp.cvt32(), xmm);
            } else {
                code.movq(tmp.cvt64(), xmm);
            }
        };

        Xbyak::Label end, z, nan, op2_is_nan, snan, maybe_both_nan, normal;

        FCODE(ucomis)(op1, op2);
        code.jz(z, code.T_NEAR);
        code.L(normal);
        if constexpr (is_max) {
            FCODE(maxs)(op2, op1);
        } else {
            FCODE(mins)(op2, op1);
        }
        code.L(end);

        code.SwitchToFarCode();

        code.L(z);
        code.jp(nan);
        if constexpr (is_max) {
            code.andps(op2, op1);
        } else {
            code.orps(op2, op1);
        }
        code.jmp(end);

        // NaN requirements:
        // op1     op2      result
        // SNaN    anything op1
        // !SNaN   SNaN     op2
        // QNaN    !NaN     op2
        // !NaN    QNaN     op1
        // QNaN    QNaN     op1

        code.L(nan);
        FCODE(ucomis)(op1, op1);
        code.jnp(op2_is_nan);

        // op1 is NaN
        move_to_tmp(op1);
        code.bt(tmp, mantissa_msb_bit);
        code.jc(maybe_both_nan);
        if (ctx.FPCR().DN()) {
            code.L(snan);
            code.movaps(op2, code.MConst(xword, default_nan));
            code.jmp(end);
        } else {
            code.movaps(op2, op1);
            code.L(snan);
            code.orps(op2, code.MConst(xword, FP::FPInfo<FPT>::mantissa_msb));
            code.jmp(end);
        }

        code.L(maybe_both_nan);
        FCODE(ucomis)(op2, op2);
        code.jnp(end, code.T_NEAR);
        if (ctx.FPCR().DN()) {
            code.jmp(snan);
        } else {
            move_to_tmp(op2);
            code.bt(tmp.cvt64(), mantissa_msb_bit);
            code.jnc(snan);
            code.movaps(op2, op1);
            code.jmp(end);
        }

        // op2 is NaN
        code.L(op2_is_nan);
        move_to_tmp(op2);
        code.bt(tmp, mantissa_msb_bit);
        code.jnc(snan);
        code.movaps(op2, op1);
        code.jmp(end);

        code.SwitchToNearCode();
    }

    ctx.reg_alloc.DefineValue(inst, op2);
}

void EmitX64::EmitFPMax32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPMinMax<32, true>(code, ctx, inst);
}

void EmitX64::EmitFPMax64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPMinMax<64, true>(code, ctx, inst);
}

void EmitX64::EmitFPMaxNumeric32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPMinMaxNumeric<32, true>(code, ctx, inst);
}

void EmitX64::EmitFPMaxNumeric64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPMinMaxNumeric<64, true>(code, ctx, inst);
}

void EmitX64::EmitFPMin32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPMinMax<32, false>(code, ctx, inst);
}

void EmitX64::EmitFPMin64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPMinMax<64, false>(code, ctx, inst);
}

void EmitX64::EmitFPMinNumeric32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPMinMaxNumeric<32, false>(code, ctx, inst);
}

void EmitX64::EmitFPMinNumeric64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPMinMaxNumeric<64, false>(code, ctx, inst);
}

void EmitX64::EmitFPMul32(EmitContext& ctx, IR::Inst* inst) {
    FPThreeOp<32>(code, ctx, inst, &Xbyak::CodeGenerator::mulss);
}

void EmitX64::EmitFPMul64(EmitContext& ctx, IR::Inst* inst) {
    FPThreeOp<64>(code, ctx, inst, &Xbyak::CodeGenerator::mulsd);
}

template<size_t fsize>
static void EmitFPMulAdd(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if constexpr (fsize != 16) {
        if (code.HasHostFeature(HostFeature::FMA) && ctx.HasOptimization(OptimizationFlag::Unsafe_InaccurateNaN)) {
            const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseXmm(args[1]);
            const Xbyak::Xmm operand3 = ctx.reg_alloc.UseXmm(args[2]);

            FCODE(vfmadd231s)(result, operand2, operand3);

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }

        if (code.HasHostFeature(HostFeature::FMA)) {
            Xbyak::Label end, fallback;

            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseXmm(args[1]);
            const Xbyak::Xmm operand3 = ctx.reg_alloc.UseXmm(args[2]);
            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

            code.movaps(result, operand1);
            FCODE(vfmadd231s)(result, operand2, operand3);

            code.movaps(tmp, code.MConst(xword, fsize == 32 ? f32_non_sign_mask : f64_non_sign_mask));
            code.andps(tmp, result);
            FCODE(ucomis)(tmp, code.MConst(xword, fsize == 32 ? f32_smallest_normal : f64_smallest_normal));
            code.jz(fallback, code.T_NEAR);
            code.L(end);

            code.SwitchToFarCode();
            code.L(fallback);

            code.sub(rsp, 8);
            ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
            code.movq(code.ABI_PARAM1, operand1);
            code.movq(code.ABI_PARAM2, operand2);
            code.movq(code.ABI_PARAM3, operand3);
            code.mov(code.ABI_PARAM4.cvt32(), ctx.FPCR().Value());
#ifdef _WIN32
            code.sub(rsp, 16 + ABI_SHADOW_SPACE);
            code.lea(rax, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
            code.mov(qword[rsp + ABI_SHADOW_SPACE], rax);
            code.CallFunction(&FP::FPMulAdd<FPT>);
            code.add(rsp, 16 + ABI_SHADOW_SPACE);
#else
            code.lea(code.ABI_PARAM5, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
            code.CallFunction(&FP::FPMulAdd<FPT>);
#endif
            code.movq(result, code.ABI_RETURN);
            ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
            code.add(rsp, 8);

            code.jmp(end, code.T_NEAR);
            code.SwitchToNearCode();

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }

        if (ctx.HasOptimization(OptimizationFlag::Unsafe_UnfuseFMA)) {
            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseScratchXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseScratchXmm(args[1]);
            const Xbyak::Xmm operand3 = ctx.reg_alloc.UseXmm(args[2]);

            FCODE(muls)(operand2, operand3);
            FCODE(adds)(operand1, operand2);

            ctx.reg_alloc.DefineValue(inst, operand1);
            return;
        }
    }

    ctx.reg_alloc.HostCall(inst, args[0], args[1], args[2]);
    code.mov(code.ABI_PARAM4.cvt32(), ctx.FPCR().Value());
#ifdef _WIN32
    ctx.reg_alloc.AllocStackSpace(16 + ABI_SHADOW_SPACE);
    code.lea(rax, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
    code.mov(qword[rsp + ABI_SHADOW_SPACE], rax);
    code.CallFunction(&FP::FPMulAdd<FPT>);
    ctx.reg_alloc.ReleaseStackSpace(16 + ABI_SHADOW_SPACE);
#else
    code.lea(code.ABI_PARAM5, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
    code.CallFunction(&FP::FPMulAdd<FPT>);
#endif
}

void EmitX64::EmitFPMulAdd16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPMulAdd<16>(code, ctx, inst);
}

void EmitX64::EmitFPMulAdd32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPMulAdd<32>(code, ctx, inst);
}

void EmitX64::EmitFPMulAdd64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPMulAdd<64>(code, ctx, inst);
}

template<size_t fsize>
static void EmitFPMulX(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const bool do_default_nan = ctx.FPCR().DN();

    const Xbyak::Xmm op1 = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm op2 = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Reg64 tmp = do_default_nan ? INVALID_REG : ctx.reg_alloc.ScratchGpr();

    Xbyak::Label end, nan, op_are_nans;

    if (code.HasHostFeature(HostFeature::AVX)) {
        FCODE(vmuls)(result, op1, op2);
    } else {
        code.movaps(result, op1);
        FCODE(muls)(result, op2);
    }
    FCODE(ucomis)(result, result);
    code.jp(nan, code.T_NEAR);
    code.L(end);

    code.SwitchToFarCode();
    code.L(nan);
    FCODE(ucomis)(op1, op2);
    code.jp(op_are_nans);
    if (code.HasHostFeature(HostFeature::AVX)) {
        code.vxorps(result, op1, op2);
    } else {
        code.movaps(result, op1);
        code.xorps(result, op2);
    }
    code.andps(result, code.MConst(xword, FP::FPInfo<FPT>::sign_mask));
    code.orps(result, code.MConst(xword, FP::FPValue<FPT, false, 0, 2>()));
    code.jmp(end, code.T_NEAR);
    code.L(op_are_nans);
    if (do_default_nan) {
        code.movaps(result, code.MConst(xword, FP::FPInfo<FPT>::DefaultNaN()));
        code.jmp(end, code.T_NEAR);
    } else {
        EmitPostProcessNaNs<fsize>(code, result, op1, op2, tmp, end);
    }
    code.SwitchToNearCode();

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitFPMulX32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPMulX<32>(code, ctx, inst);
}

void EmitX64::EmitFPMulX64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPMulX<64>(code, ctx, inst);
}

template<size_t fsize>
static void EmitFPRecipEstimate(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    if constexpr (fsize != 16) {
        if (ctx.HasOptimization(OptimizationFlag::Unsafe_ReducedErrorFP)) {
            auto args = ctx.reg_alloc.GetArgumentInfo(inst);
            const Xbyak::Xmm operand = ctx.reg_alloc.UseXmm(args[0]);
            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

            if (code.HasHostFeature(HostFeature::AVX512_OrthoFloat)) {
                FCODE(vrcp14s)(result, operand, operand);
            } else {
                if constexpr (fsize == 32) {
                    code.rcpss(result, operand);
                } else {
                    code.cvtsd2ss(result, operand);
                    code.rcpss(result, result);
                    code.cvtss2sd(result, result);
                }
            }

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }
    }

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.HostCall(inst, args[0]);
    code.mov(code.ABI_PARAM2.cvt32(), ctx.FPCR().Value());
    code.lea(code.ABI_PARAM3, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
    code.CallFunction(&FP::FPRecipEstimate<FPT>);
}

void EmitX64::EmitFPRecipEstimate16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRecipEstimate<16>(code, ctx, inst);
}

void EmitX64::EmitFPRecipEstimate32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRecipEstimate<32>(code, ctx, inst);
}

void EmitX64::EmitFPRecipEstimate64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRecipEstimate<64>(code, ctx, inst);
}

template<size_t fsize>
static void EmitFPRecipExponent(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.HostCall(inst, args[0]);
    code.mov(code.ABI_PARAM2.cvt32(), ctx.FPCR().Value());
    code.lea(code.ABI_PARAM3, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
    code.CallFunction(&FP::FPRecipExponent<FPT>);
}

void EmitX64::EmitFPRecipExponent16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRecipExponent<16>(code, ctx, inst);
}

void EmitX64::EmitFPRecipExponent32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRecipExponent<32>(code, ctx, inst);
}

void EmitX64::EmitFPRecipExponent64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRecipExponent<64>(code, ctx, inst);
}

template<size_t fsize>
static void EmitFPRecipStepFused(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if constexpr (fsize != 16) {
        if (code.HasHostFeature(HostFeature::FMA) && ctx.HasOptimization(OptimizationFlag::Unsafe_InaccurateNaN)) {
            Xbyak::Label end, fallback;

            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseXmm(args[1]);
            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

            code.movaps(result, code.MConst(xword, FP::FPValue<FPT, false, 0, 2>()));
            FCODE(vfnmadd231s)(result, operand1, operand2);

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }

        if (code.HasHostFeature(HostFeature::FMA)) {
            Xbyak::Label end, fallback;

            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseXmm(args[1]);
            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

            code.movaps(result, code.MConst(xword, FP::FPValue<FPT, false, 0, 2>()));
            FCODE(vfnmadd231s)(result, operand1, operand2);
            FCODE(ucomis)(result, result);
            code.jp(fallback, code.T_NEAR);
            code.L(end);

            code.SwitchToFarCode();
            code.L(fallback);

            code.sub(rsp, 8);
            ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
            code.movq(code.ABI_PARAM1, operand1);
            code.movq(code.ABI_PARAM2, operand2);
            code.mov(code.ABI_PARAM3.cvt32(), ctx.FPCR().Value());
            code.lea(code.ABI_PARAM4, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
            code.CallFunction(&FP::FPRecipStepFused<FPT>);
            code.movq(result, code.ABI_RETURN);
            ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
            code.add(rsp, 8);

            code.jmp(end, code.T_NEAR);
            code.SwitchToNearCode();

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }

        if (ctx.HasOptimization(OptimizationFlag::Unsafe_UnfuseFMA)) {
            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseScratchXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseXmm(args[1]);
            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

            code.movaps(result, code.MConst(xword, FP::FPValue<FPT, false, 0, 2>()));
            FCODE(muls)(operand1, operand2);
            FCODE(subs)(result, operand1);

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }
    }

    ctx.reg_alloc.HostCall(inst, args[0], args[1]);
    code.mov(code.ABI_PARAM3.cvt32(), ctx.FPCR().Value());
    code.lea(code.ABI_PARAM4, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
    code.CallFunction(&FP::FPRecipStepFused<FPT>);
}

void EmitX64::EmitFPRecipStepFused16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRecipStepFused<16>(code, ctx, inst);
}

void EmitX64::EmitFPRecipStepFused32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRecipStepFused<32>(code, ctx, inst);
}

void EmitX64::EmitFPRecipStepFused64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRecipStepFused<64>(code, ctx, inst);
}

static void EmitFPRound(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, size_t fsize) {
    const auto rounding_mode = static_cast<FP::RoundingMode>(inst->GetArg(1).GetU8());
    const bool exact = inst->GetArg(2).GetU1();
    const auto round_imm = ConvertRoundingModeToX64Immediate(rounding_mode);

    if (fsize != 16 && code.HasHostFeature(HostFeature::SSE41) && round_imm && !exact) {
        if (fsize == 64) {
            FPTwoOp<64>(code, ctx, inst, [&](Xbyak::Xmm result) {
                code.roundsd(result, result, *round_imm);
            });
        } else {
            FPTwoOp<32>(code, ctx, inst, [&](Xbyak::Xmm result) {
                code.roundss(result, result, *round_imm);
            });
        }

        return;
    }

    using fsize_list = mp::list<mp::lift_value<size_t(16)>,
                                mp::lift_value<size_t(32)>,
                                mp::lift_value<size_t(64)>>;
    using rounding_list = mp::list<
        mp::lift_value<FP::RoundingMode::ToNearest_TieEven>,
        mp::lift_value<FP::RoundingMode::TowardsPlusInfinity>,
        mp::lift_value<FP::RoundingMode::TowardsMinusInfinity>,
        mp::lift_value<FP::RoundingMode::TowardsZero>,
        mp::lift_value<FP::RoundingMode::ToNearest_TieAwayFromZero>>;
    using exact_list = mp::list<std::true_type, std::false_type>;

    static const auto lut = Common::GenerateLookupTableFromList(
        []<typename I>(I) {
            return std::pair{
                mp::lower_to_tuple_v<I>,
                Common::FptrCast(
                    [](u64 input, FP::FPSR& fpsr, FP::FPCR fpcr) {
                        constexpr size_t fsize = mp::get<0, I>::value;
                        constexpr FP::RoundingMode rounding_mode = mp::get<1, I>::value;
                        constexpr bool exact = mp::get<2, I>::value;
                        using InputSize = mcl::unsigned_integer_of_size<fsize>;

                        return FP::FPRoundInt<InputSize>(static_cast<InputSize>(input), fpcr, rounding_mode, exact, fpsr);
                    })};
        },
        mp::cartesian_product<fsize_list, rounding_list, exact_list>{});

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    ctx.reg_alloc.HostCall(inst, args[0]);
    code.lea(code.ABI_PARAM2, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
    code.mov(code.ABI_PARAM3.cvt32(), ctx.FPCR().Value());
    code.CallFunction(lut.at(std::make_tuple(fsize, rounding_mode, exact)));
}

void EmitX64::EmitFPRoundInt16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRound(code, ctx, inst, 16);
}

void EmitX64::EmitFPRoundInt32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRound(code, ctx, inst, 32);
}

void EmitX64::EmitFPRoundInt64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRound(code, ctx, inst, 64);
}

template<size_t fsize>
static void EmitFPRSqrtEstimate(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    if constexpr (fsize != 16) {
        if (ctx.HasOptimization(OptimizationFlag::Unsafe_ReducedErrorFP)) {
            auto args = ctx.reg_alloc.GetArgumentInfo(inst);
            const Xbyak::Xmm operand = ctx.reg_alloc.UseXmm(args[0]);
            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

            if (code.HasHostFeature(HostFeature::AVX512_OrthoFloat)) {
                FCODE(vrsqrt14s)(result, operand, operand);
            } else {
                if constexpr (fsize == 32) {
                    code.rsqrtss(result, operand);
                } else {
                    code.cvtsd2ss(result, operand);
                    code.rsqrtss(result, result);
                    code.cvtss2sd(result, result);
                }
            }

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }

        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm operand = ctx.reg_alloc.UseXmm(args[0]);
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm value = ctx.reg_alloc.ScratchXmm();
        [[maybe_unused]] const Xbyak::Reg32 tmp = ctx.reg_alloc.ScratchGpr().cvt32();

        Xbyak::Label fallback, bad_values, end, default_nan;
        bool needs_fallback = false;

        code.movaps(value, operand);

        code.movaps(xmm0, code.MConst(xword, fsize == 32 ? 0xFFFF8000 : 0xFFFF'F000'0000'0000));
        code.pand(value, xmm0);
        code.por(value, code.MConst(xword, fsize == 32 ? 0x00008000 : 0x0000'1000'0000'0000));

        // Detect NaNs, negatives, zeros, denormals and infinities
        FCODE(ucomis)(value, code.MConst(xword, FPT(1) << FP::FPInfo<FPT>::explicit_mantissa_width));
        code.jna(bad_values, code.T_NEAR);

        FCODE(sqrts)(value, value);
        ICODE(mov)(result, code.MConst(xword, FP::FPValue<FPT, false, 0, 1>()));
        FCODE(divs)(result, value);

        ICODE(padd)(result, code.MConst(xword, fsize == 32 ? 0x00004000 : 0x0000'0800'0000'0000));
        code.pand(result, xmm0);

        code.L(end);

        code.SwitchToFarCode();

        code.L(bad_values);
        if constexpr (fsize == 32) {
            code.movd(tmp, operand);

            if (!ctx.FPCR().FZ()) {
                if (ctx.FPCR().DN()) {
                    // a > 0x80000000
                    code.cmp(tmp, 0x80000000);
                    code.ja(default_nan, code.T_NEAR);
                }

                // a > 0 && a < 0x00800000;
                code.sub(tmp, 1);
                code.cmp(tmp, 0x007FFFFF);
                code.jb(fallback);
                needs_fallback = true;
            }

            code.rsqrtss(result, operand);

            if (ctx.FPCR().DN()) {
                code.ucomiss(result, result);
                code.jnp(end, code.T_NEAR);
            } else {
                // FZ ? (a >= 0x80800000 && a <= 0xFF800000) : (a >= 0x80000001 && a <= 0xFF800000)
                // !FZ path takes into account the subtraction by one from the earlier block
                code.add(tmp, ctx.FPCR().FZ() ? 0x7F800000 : 0x80000000);
                code.cmp(tmp, ctx.FPCR().FZ() ? 0x7F000001 : 0x7F800000);
                code.jnb(end, code.T_NEAR);
            }

            code.L(default_nan);
            code.movd(result, code.MConst(xword, 0x7FC00000));
            code.jmp(end, code.T_NEAR);
        } else {
            Xbyak::Label nan, zero;

            code.movaps(value, operand);
            DenormalsAreZero<fsize>(code, ctx, {value});
            code.pxor(result, result);

            code.ucomisd(value, result);
            if (ctx.FPCR().DN()) {
                code.jc(default_nan);
                code.je(zero);
            } else {
                code.jp(nan);
                code.je(zero);
                code.jc(default_nan);
            }

            if (!ctx.FPCR().FZ()) {
                needs_fallback = true;
                code.jmp(fallback);
            } else {
                // result = 0
                code.jmp(end, code.T_NEAR);
            }

            code.L(zero);
            if (code.HasHostFeature(HostFeature::AVX)) {
                code.vpor(result, value, code.MConst(xword, 0x7FF0'0000'0000'0000));
            } else {
                code.movaps(result, value);
                code.por(result, code.MConst(xword, 0x7FF0'0000'0000'0000));
            }
            code.jmp(end, code.T_NEAR);

            code.L(nan);
            if (!ctx.FPCR().DN()) {
                if (code.HasHostFeature(HostFeature::AVX)) {
                    code.vpor(result, operand, code.MConst(xword, 0x0008'0000'0000'0000));
                } else {
                    code.movaps(result, operand);
                    code.por(result, code.MConst(xword, 0x0008'0000'0000'0000));
                }
                code.jmp(end, code.T_NEAR);
            }

            code.L(default_nan);
            code.movq(result, code.MConst(xword, 0x7FF8'0000'0000'0000));
            code.jmp(end, code.T_NEAR);
        }

        code.L(fallback);
        if (needs_fallback) {
            code.sub(rsp, 8);
            ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
            code.movq(code.ABI_PARAM1, operand);
            code.mov(code.ABI_PARAM2.cvt32(), ctx.FPCR().Value());
            code.lea(code.ABI_PARAM3, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
            code.CallFunction(&FP::FPRSqrtEstimate<FPT>);
            code.movq(result, rax);
            ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
            code.add(rsp, 8);
            code.jmp(end, code.T_NEAR);
        }

        code.SwitchToNearCode();

        ctx.reg_alloc.DefineValue(inst, result);
    } else {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);
        ctx.reg_alloc.HostCall(inst, args[0]);
        code.mov(code.ABI_PARAM2.cvt32(), ctx.FPCR().Value());
        code.lea(code.ABI_PARAM3, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
        code.CallFunction(&FP::FPRSqrtEstimate<FPT>);
    }
}

void EmitX64::EmitFPRSqrtEstimate16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRSqrtEstimate<16>(code, ctx, inst);
}

void EmitX64::EmitFPRSqrtEstimate32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRSqrtEstimate<32>(code, ctx, inst);
}

void EmitX64::EmitFPRSqrtEstimate64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRSqrtEstimate<64>(code, ctx, inst);
}

template<size_t fsize>
static void EmitFPRSqrtStepFused(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    if constexpr (fsize != 16) {
        if (code.HasHostFeature(HostFeature::FMA | HostFeature::AVX) && ctx.HasOptimization(OptimizationFlag::Unsafe_InaccurateNaN)) {
            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseXmm(args[1]);
            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

            code.vmovaps(result, code.MConst(xword, FP::FPValue<FPT, false, 0, 3>()));
            FCODE(vfnmadd231s)(result, operand1, operand2);
            FCODE(vmuls)(result, result, code.MConst(xword, FP::FPValue<FPT, false, -1, 1>()));

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }

        if (code.HasHostFeature(HostFeature::FMA | HostFeature::AVX)) {
            Xbyak::Label end, fallback;

            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseXmm(args[1]);
            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

            code.vmovaps(result, code.MConst(xword, FP::FPValue<FPT, false, 0, 3>()));
            FCODE(vfnmadd231s)(result, operand1, operand2);

            // Detect if the intermediate result is infinity or NaN or nearly an infinity.
            // Why do we need to care about infinities? This is because x86 doesn't allow us
            // to fuse the divide-by-two with the rest of the FMA operation. Therefore the
            // intermediate value may overflow and we would like to handle this case.
            const Xbyak::Reg32 tmp = ctx.reg_alloc.ScratchGpr().cvt32();
            code.vpextrw(tmp, result, fsize == 32 ? 1 : 3);
            code.and_(tmp.cvt16(), fsize == 32 ? 0x7f80 : 0x7ff0);
            code.cmp(tmp.cvt16(), fsize == 32 ? 0x7f00 : 0x7fe0);
            ctx.reg_alloc.Release(tmp);

            code.jae(fallback, code.T_NEAR);

            FCODE(vmuls)(result, result, code.MConst(xword, FP::FPValue<FPT, false, -1, 1>()));
            code.L(end);

            code.SwitchToFarCode();
            code.L(fallback);

            code.sub(rsp, 8);
            ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
            code.movq(code.ABI_PARAM1, operand1);
            code.movq(code.ABI_PARAM2, operand2);
            code.mov(code.ABI_PARAM3.cvt32(), ctx.FPCR().Value());
            code.lea(code.ABI_PARAM4, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
            code.CallFunction(&FP::FPRSqrtStepFused<FPT>);
            code.movq(result, code.ABI_RETURN);
            ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
            code.add(rsp, 8);

            code.jmp(end, code.T_NEAR);
            code.SwitchToNearCode();

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }

        if (ctx.HasOptimization(OptimizationFlag::Unsafe_UnfuseFMA)) {
            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseScratchXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseXmm(args[1]);
            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

            code.movaps(result, code.MConst(xword, FP::FPValue<FPT, false, 0, 3>()));
            FCODE(muls)(operand1, operand2);
            FCODE(subs)(result, operand1);
            FCODE(muls)(result, code.MConst(xword, FP::FPValue<FPT, false, -1, 1>()));

            ctx.reg_alloc.DefineValue(inst, operand1);
            return;
        }
    }

    ctx.reg_alloc.HostCall(inst, args[0], args[1]);
    code.mov(code.ABI_PARAM3.cvt32(), ctx.FPCR().Value());
    code.lea(code.ABI_PARAM4, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
    code.CallFunction(&FP::FPRSqrtStepFused<FPT>);
}

void EmitX64::EmitFPRSqrtStepFused16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRSqrtStepFused<16>(code, ctx, inst);
}

void EmitX64::EmitFPRSqrtStepFused32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRSqrtStepFused<32>(code, ctx, inst);
}

void EmitX64::EmitFPRSqrtStepFused64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPRSqrtStepFused<64>(code, ctx, inst);
}

void EmitX64::EmitFPSqrt32(EmitContext& ctx, IR::Inst* inst) {
    FPTwoOp<32>(code, ctx, inst, &Xbyak::CodeGenerator::sqrtss);
}

void EmitX64::EmitFPSqrt64(EmitContext& ctx, IR::Inst* inst) {
    FPTwoOp<64>(code, ctx, inst, &Xbyak::CodeGenerator::sqrtsd);
}

void EmitX64::EmitFPSub32(EmitContext& ctx, IR::Inst* inst) {
    FPThreeOp<32>(code, ctx, inst, &Xbyak::CodeGenerator::subss);
}

void EmitX64::EmitFPSub64(EmitContext& ctx, IR::Inst* inst) {
    FPThreeOp<64>(code, ctx, inst, &Xbyak::CodeGenerator::subsd);
}

static Xbyak::Reg64 SetFpscrNzcvFromFlags(BlockOfCode& code, EmitContext& ctx) {
    ctx.reg_alloc.ScratchGpr(HostLoc::RCX);  // shifting requires use of cl
    const Xbyak::Reg64 nzcv = ctx.reg_alloc.ScratchGpr();

    //               x64 flags    ARM flags
    //               ZF  PF  CF     NZCV
    // Unordered      1   1   1     0011
    // Greater than   0   0   0     0010
    // Less than      0   0   1     1000
    // Equal          1   0   0     0110
    //
    // Thus we can take use ZF:CF as an index into an array like so:
    //  x64      ARM      ARM as x64
    // ZF:CF     NZCV     NZ-----C-------V
    //   0       0010     0000000100000000 = 0x0100
    //   1       1000     1000000000000000 = 0x8000
    //   2       0110     0100000100000000 = 0x4100
    //   3       0011     0000000100000001 = 0x0101

    code.mov(nzcv, 0x0101'4100'8000'0100);
    code.sete(cl);
    code.rcl(cl, 5);  // cl = ZF:CF:0000
    code.shr(nzcv, cl);

    return nzcv;
}

void EmitX64::EmitFPCompare32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm reg_a = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm reg_b = ctx.reg_alloc.UseXmm(args[1]);
    const bool exc_on_qnan = args[2].GetImmediateU1();

    if (exc_on_qnan) {
        code.comiss(reg_a, reg_b);
    } else {
        code.ucomiss(reg_a, reg_b);
    }

    const Xbyak::Reg64 nzcv = SetFpscrNzcvFromFlags(code, ctx);
    ctx.reg_alloc.DefineValue(inst, nzcv);
}

void EmitX64::EmitFPCompare64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm reg_a = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm reg_b = ctx.reg_alloc.UseXmm(args[1]);
    const bool exc_on_qnan = args[2].GetImmediateU1();

    if (exc_on_qnan) {
        code.comisd(reg_a, reg_b);
    } else {
        code.ucomisd(reg_a, reg_b);
    }

    const Xbyak::Reg64 nzcv = SetFpscrNzcvFromFlags(code, ctx);
    ctx.reg_alloc.DefineValue(inst, nzcv);
}

void EmitX64::EmitFPHalfToDouble(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto rounding_mode = static_cast<FP::RoundingMode>(args[1].GetImmediateU8());

    if (code.HasHostFeature(HostFeature::F16C) && !ctx.FPCR().AHP() && !ctx.FPCR().FZ16()) {
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm value = ctx.reg_alloc.UseXmm(args[0]);

        // Double-conversion here is acceptable as this is expanding precision.
        code.vcvtph2ps(result, value);
        code.vcvtps2pd(result, result);
        if (ctx.FPCR().DN()) {
            ForceToDefaultNaN<64>(code, result);
        }

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    ctx.reg_alloc.HostCall(inst, args[0]);
    code.mov(code.ABI_PARAM2.cvt32(), ctx.FPCR().Value());
    code.mov(code.ABI_PARAM3.cvt32(), static_cast<u32>(rounding_mode));
    code.lea(code.ABI_PARAM4, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
    code.CallFunction(&FP::FPConvert<u64, u16>);
}

void EmitX64::EmitFPHalfToSingle(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto rounding_mode = static_cast<FP::RoundingMode>(args[1].GetImmediateU8());

    if (code.HasHostFeature(HostFeature::F16C) && !ctx.FPCR().AHP() && !ctx.FPCR().FZ16()) {
        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm value = ctx.reg_alloc.UseXmm(args[0]);

        code.vcvtph2ps(result, value);
        if (ctx.FPCR().DN()) {
            ForceToDefaultNaN<32>(code, result);
        }

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    ctx.reg_alloc.HostCall(inst, args[0]);
    code.mov(code.ABI_PARAM2.cvt32(), ctx.FPCR().Value());
    code.mov(code.ABI_PARAM3.cvt32(), static_cast<u32>(rounding_mode));
    code.lea(code.ABI_PARAM4, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
    code.CallFunction(&FP::FPConvert<u32, u16>);
}

void EmitX64::EmitFPSingleToDouble(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto rounding_mode = static_cast<FP::RoundingMode>(args[1].GetImmediateU8());

    // We special-case the non-IEEE-defined ToOdd rounding mode.
    if (rounding_mode == ctx.FPCR().RMode() && rounding_mode != FP::RoundingMode::ToOdd) {
        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);

        code.cvtss2sd(result, result);
        if (ctx.FPCR().DN()) {
            ForceToDefaultNaN<64>(code, result);
        }
        ctx.reg_alloc.DefineValue(inst, result);
    } else {
        ctx.reg_alloc.HostCall(inst, args[0]);
        code.mov(code.ABI_PARAM2.cvt32(), ctx.FPCR().Value());
        code.mov(code.ABI_PARAM3.cvt32(), static_cast<u32>(rounding_mode));
        code.lea(code.ABI_PARAM4, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
        code.CallFunction(&FP::FPConvert<u64, u32>);
    }
}

void EmitX64::EmitFPSingleToHalf(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto rounding_mode = static_cast<FP::RoundingMode>(args[1].GetImmediateU8());
    const auto round_imm = ConvertRoundingModeToX64Immediate(rounding_mode);

    if (code.HasHostFeature(HostFeature::F16C) && !ctx.FPCR().AHP() && !ctx.FPCR().FZ16()) {
        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);

        if (ctx.FPCR().DN()) {
            ForceToDefaultNaN<32>(code, result);
        }
        code.vcvtps2ph(result, result, static_cast<u8>(*round_imm));

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    ctx.reg_alloc.HostCall(inst, args[0]);
    code.mov(code.ABI_PARAM2.cvt32(), ctx.FPCR().Value());
    code.mov(code.ABI_PARAM3.cvt32(), static_cast<u32>(rounding_mode));
    code.lea(code.ABI_PARAM4, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
    code.CallFunction(&FP::FPConvert<u16, u32>);
}

void EmitX64::EmitFPDoubleToHalf(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto rounding_mode = static_cast<FP::RoundingMode>(args[1].GetImmediateU8());

    // NOTE: Do not double-convert here as that is inaccurate.
    //       To be accurate, the first conversion would need to be "round-to-odd", which x64 doesn't support.

    ctx.reg_alloc.HostCall(inst, args[0]);
    code.mov(code.ABI_PARAM2.cvt32(), ctx.FPCR().Value());
    code.mov(code.ABI_PARAM3.cvt32(), static_cast<u32>(rounding_mode));
    code.lea(code.ABI_PARAM4, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
    code.CallFunction(&FP::FPConvert<u16, u64>);
}

void EmitX64::EmitFPDoubleToSingle(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const auto rounding_mode = static_cast<FP::RoundingMode>(args[1].GetImmediateU8());

    // We special-case the non-IEEE-defined ToOdd rounding mode.
    if (rounding_mode == ctx.FPCR().RMode() && rounding_mode != FP::RoundingMode::ToOdd) {
        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);

        code.cvtsd2ss(result, result);
        if (ctx.FPCR().DN()) {
            ForceToDefaultNaN<32>(code, result);
        }
        ctx.reg_alloc.DefineValue(inst, result);
    } else {
        ctx.reg_alloc.HostCall(inst, args[0]);
        code.mov(code.ABI_PARAM2.cvt32(), ctx.FPCR().Value());
        code.mov(code.ABI_PARAM3.cvt32(), static_cast<u32>(rounding_mode));
        code.lea(code.ABI_PARAM4, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
        code.CallFunction(&FP::FPConvert<u32, u64>);
    }
}

template<size_t fsize, bool unsigned_, size_t isize>
static void EmitFPToFixed(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const size_t fbits = args[1].GetImmediateU8();
    const auto rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());

    if constexpr (fsize != 16) {
        const auto round_imm = ConvertRoundingModeToX64Immediate(rounding_mode);

        // cvttsd2si truncates during operation so rounding (and thus SSE4.1) not required
        const bool truncating = rounding_mode == FP::RoundingMode::TowardsZero;

        if (round_imm && (truncating || code.HasHostFeature(HostFeature::SSE41))) {
            const Xbyak::Xmm src = ctx.reg_alloc.UseScratchXmm(args[0]);
            const Xbyak::Reg64 result = ctx.reg_alloc.ScratchGpr().cvt64();

            if constexpr (fsize == 64) {
                if (fbits != 0) {
                    const u64 scale_factor = static_cast<u64>((fbits + 1023) << 52);
                    code.mulsd(src, code.MConst(xword, scale_factor));
                }

                if (!truncating) {
                    code.roundsd(src, src, *round_imm);
                }
            } else {
                if (fbits != 0) {
                    const u32 scale_factor = static_cast<u32>((fbits + 127) << 23);
                    code.mulss(src, code.MConst(xword, scale_factor));
                }

                if (!truncating) {
                    code.roundss(src, src, *round_imm);
                }

                code.cvtss2sd(src, src);
            }

            if constexpr (isize == 64) {
                const Xbyak::Xmm scratch = ctx.reg_alloc.ScratchXmm();

                Xbyak::Label saturate_max, end;

                if (!unsigned_) {
                    ZeroIfNaN<64>(code, src, scratch);

                    code.movsd(scratch, code.MConst(xword, f64_max_s64_lim));
                    code.comisd(scratch, src);
                    code.jna(saturate_max, code.T_NEAR);
                    code.cvttsd2si(result, src);  // 64 bit gpr
                    code.L(end);

                    code.SwitchToFarCode();
                    code.L(saturate_max);
                    code.mov(result, 0x7FFF'FFFF'FFFF'FFFF);
                    code.jmp(end, code.T_NEAR);
                    code.SwitchToNearCode();
                } else {
                    Xbyak::Label below_max;

                    const Xbyak::Reg64 result2 = ctx.reg_alloc.ScratchGpr().cvt64();

                    code.pxor(xmm0, xmm0);

                    code.movaps(scratch, src);
                    code.subsd(scratch, code.MConst(xword, f64_max_s64_lim));

                    // these both result in zero if src/scratch are NaN
                    code.maxsd(src, xmm0);
                    code.maxsd(scratch, xmm0);

                    code.cvttsd2si(result, src);
                    code.cvttsd2si(result2, scratch);
                    code.or_(result, result2);

                    // when src < 2^63, result2 == 0, and result contains the final result
                    // when src >= 2^63, result contains 0x800.... and result2 contains the non-MSB bits
                    // MSB if result2 is 1 when src >= 2^64

                    code.sar(result2, 63);
                    code.or_(result, result2);
                }
            } else if constexpr (isize == 32) {
                if (!unsigned_) {
                    const Xbyak::Xmm scratch = ctx.reg_alloc.ScratchXmm();

                    ZeroIfNaN<64>(code, src, scratch);
                    code.minsd(src, code.MConst(xword, f64_max_s32));
                    // maxsd not required as cvttsd2si results in 0x8000'0000 when out of range
                    code.cvttsd2si(result.cvt32(), src);  // 32 bit gpr
                } else {
                    code.pxor(xmm0, xmm0);
                    code.maxsd(src, xmm0);  // results in a zero if src is NaN
                    code.minsd(src, code.MConst(xword, f64_max_u32));
                    code.cvttsd2si(result, src);  // 64 bit gpr
                }
            } else {
                const Xbyak::Xmm scratch = ctx.reg_alloc.ScratchXmm();

                ZeroIfNaN<64>(code, src, scratch);
                code.maxsd(src, code.MConst(xword, unsigned_ ? f64_min_u16 : f64_min_s16));
                code.minsd(src, code.MConst(xword, unsigned_ ? f64_max_u16 : f64_max_s16));
                code.cvttsd2si(result, src);  // 64 bit gpr
            }

            ctx.reg_alloc.DefineValue(inst, result);

            return;
        }
    }

    using fbits_list = mp::lift_sequence<std::make_index_sequence<isize + 1>>;
    using rounding_list = mp::list<
        mp::lift_value<FP::RoundingMode::ToNearest_TieEven>,
        mp::lift_value<FP::RoundingMode::TowardsPlusInfinity>,
        mp::lift_value<FP::RoundingMode::TowardsMinusInfinity>,
        mp::lift_value<FP::RoundingMode::TowardsZero>,
        mp::lift_value<FP::RoundingMode::ToNearest_TieAwayFromZero>>;

    static const auto lut = Common::GenerateLookupTableFromList(
        []<typename I>(I) {
            return std::pair{
                mp::lower_to_tuple_v<I>,
                Common::FptrCast(
                    [](u64 input, FP::FPSR& fpsr, FP::FPCR fpcr) {
                        constexpr size_t fbits = mp::get<0, I>::value;
                        constexpr FP::RoundingMode rounding_mode = mp::get<1, I>::value;
                        using FPT = mcl::unsigned_integer_of_size<fsize>;

                        return FP::FPToFixed<FPT>(isize, static_cast<FPT>(input), fbits, unsigned_, fpcr, rounding_mode, fpsr);
                    })};
        },
        mp::cartesian_product<fbits_list, rounding_list>{});

    ctx.reg_alloc.HostCall(inst, args[0]);
    code.lea(code.ABI_PARAM2, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
    code.mov(code.ABI_PARAM3.cvt32(), ctx.FPCR().Value());
    code.CallFunction(lut.at(std::make_tuple(fbits, rounding_mode)));
}

void EmitX64::EmitFPDoubleToFixedS16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<64, false, 16>(code, ctx, inst);
}

void EmitX64::EmitFPDoubleToFixedS32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<64, false, 32>(code, ctx, inst);
}

void EmitX64::EmitFPDoubleToFixedS64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<64, false, 64>(code, ctx, inst);
}

void EmitX64::EmitFPDoubleToFixedU16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<64, true, 16>(code, ctx, inst);
}

void EmitX64::EmitFPDoubleToFixedU32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<64, true, 32>(code, ctx, inst);
}

void EmitX64::EmitFPDoubleToFixedU64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<64, true, 64>(code, ctx, inst);
}

void EmitX64::EmitFPHalfToFixedS16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<16, false, 16>(code, ctx, inst);
}

void EmitX64::EmitFPHalfToFixedS32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<16, false, 32>(code, ctx, inst);
}

void EmitX64::EmitFPHalfToFixedS64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<16, false, 64>(code, ctx, inst);
}

void EmitX64::EmitFPHalfToFixedU16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<16, true, 16>(code, ctx, inst);
}

void EmitX64::EmitFPHalfToFixedU32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<16, true, 32>(code, ctx, inst);
}

void EmitX64::EmitFPHalfToFixedU64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<16, true, 64>(code, ctx, inst);
}

void EmitX64::EmitFPSingleToFixedS16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<32, false, 16>(code, ctx, inst);
}

void EmitX64::EmitFPSingleToFixedS32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<32, false, 32>(code, ctx, inst);
}

void EmitX64::EmitFPSingleToFixedS64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<32, false, 64>(code, ctx, inst);
}

void EmitX64::EmitFPSingleToFixedU16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<32, true, 16>(code, ctx, inst);
}

void EmitX64::EmitFPSingleToFixedU32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<32, true, 32>(code, ctx, inst);
}

void EmitX64::EmitFPSingleToFixedU64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPToFixed<32, true, 64>(code, ctx, inst);
}

void EmitX64::EmitFPFixedS16ToSingle(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg16 from = ctx.reg_alloc.UseGpr(args[0]).cvt16();
    const Xbyak::Reg32 tmp = ctx.reg_alloc.ScratchGpr().cvt32();
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const size_t fbits = args[1].GetImmediateU8();
    [[maybe_unused]] const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());  // Not required

    code.movsx(tmp, from);
    code.cvtsi2ss(result, tmp);

    if (fbits != 0) {
        const u32 scale_factor = static_cast<u32>((127 - fbits) << 23);
        code.mulss(result, code.MConst(xword, scale_factor));
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitFPFixedU16ToSingle(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg16 from = ctx.reg_alloc.UseGpr(args[0]).cvt16();
    const Xbyak::Reg32 tmp = ctx.reg_alloc.ScratchGpr().cvt32();
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const size_t fbits = args[1].GetImmediateU8();
    [[maybe_unused]] const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());  // Not required

    code.movzx(tmp, from);
    code.cvtsi2ss(result, tmp);

    if (fbits != 0) {
        const u32 scale_factor = static_cast<u32>((127 - fbits) << 23);
        code.mulss(result, code.MConst(xword, scale_factor));
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitFPFixedS32ToSingle(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 from = ctx.reg_alloc.UseGpr(args[0]).cvt32();
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const size_t fbits = args[1].GetImmediateU8();
    const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());

    if (rounding_mode == ctx.FPCR().RMode() || ctx.HasOptimization(OptimizationFlag::Unsafe_IgnoreStandardFPCRValue)) {
        code.cvtsi2ss(result, from);
    } else {
        ASSERT(rounding_mode == FP::RoundingMode::ToNearest_TieEven);
        code.EnterStandardASIMD();
        code.cvtsi2ss(result, from);
        code.LeaveStandardASIMD();
    }

    if (fbits != 0) {
        const u32 scale_factor = static_cast<u32>((127 - fbits) << 23);
        code.mulss(result, code.MConst(xword, scale_factor));
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitFPFixedU32ToSingle(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const size_t fbits = args[1].GetImmediateU8();
    const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());

    const auto op = [&] {
        if (code.HasHostFeature(HostFeature::AVX512F)) {
            const Xbyak::Reg64 from = ctx.reg_alloc.UseGpr(args[0]);
            code.vcvtusi2ss(result, result, from.cvt32());
        } else {
            // We are using a 64-bit GPR register to ensure we don't end up treating the input as signed
            const Xbyak::Reg64 from = ctx.reg_alloc.UseScratchGpr(args[0]);
            code.mov(from.cvt32(), from.cvt32());  // TODO: Verify if this is necessary
            code.cvtsi2ss(result, from);
        }
    };

    if (rounding_mode == ctx.FPCR().RMode() || ctx.HasOptimization(OptimizationFlag::Unsafe_IgnoreStandardFPCRValue)) {
        op();
    } else {
        ASSERT(rounding_mode == FP::RoundingMode::ToNearest_TieEven);
        code.EnterStandardASIMD();
        op();
        code.LeaveStandardASIMD();
    }

    if (fbits != 0) {
        const u32 scale_factor = static_cast<u32>((127 - fbits) << 23);
        code.mulss(result, code.MConst(xword, scale_factor));
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitFPFixedS16ToDouble(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg16 from = ctx.reg_alloc.UseGpr(args[0]).cvt16();
    const Xbyak::Reg32 tmp = ctx.reg_alloc.ScratchGpr().cvt32();
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const size_t fbits = args[1].GetImmediateU8();
    [[maybe_unused]] const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());  // Not required

    code.movsx(tmp, from);
    code.cvtsi2sd(result, tmp);

    if (fbits != 0) {
        const u64 scale_factor = static_cast<u64>((1023 - fbits) << 52);
        code.mulsd(result, code.MConst(xword, scale_factor));
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitFPFixedU16ToDouble(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg16 from = ctx.reg_alloc.UseGpr(args[0]).cvt16();
    const Xbyak::Reg32 tmp = ctx.reg_alloc.ScratchGpr().cvt32();
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const size_t fbits = args[1].GetImmediateU8();
    [[maybe_unused]] const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());  // Not required

    code.movzx(tmp, from);
    code.cvtsi2sd(result, tmp);

    if (fbits != 0) {
        const u64 scale_factor = static_cast<u64>((1023 - fbits) << 52);
        code.mulsd(result, code.MConst(xword, scale_factor));
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitFPFixedS32ToDouble(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg32 from = ctx.reg_alloc.UseGpr(args[0]).cvt32();
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const size_t fbits = args[1].GetImmediateU8();
    [[maybe_unused]] const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());  // Not required

    code.cvtsi2sd(result, from);

    if (fbits != 0) {
        const u64 scale_factor = static_cast<u64>((1023 - fbits) << 52);
        code.mulsd(result, code.MConst(xword, scale_factor));
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitFPFixedU32ToDouble(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm to = ctx.reg_alloc.ScratchXmm();
    const size_t fbits = args[1].GetImmediateU8();
    [[maybe_unused]] const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());  // Not required

    if (code.HasHostFeature(HostFeature::AVX512F)) {
        const Xbyak::Reg64 from = ctx.reg_alloc.UseGpr(args[0]);
        code.vcvtusi2sd(to, to, from.cvt32());
    } else {
        // We are using a 64-bit GPR register to ensure we don't end up treating the input as signed
        const Xbyak::Reg64 from = ctx.reg_alloc.UseScratchGpr(args[0]);
        code.mov(from.cvt32(), from.cvt32());  // TODO: Verify if this is necessary
        code.cvtsi2sd(to, from);
    }

    if (fbits != 0) {
        const u64 scale_factor = static_cast<u64>((1023 - fbits) << 52);
        code.mulsd(to, code.MConst(xword, scale_factor));
    }

    ctx.reg_alloc.DefineValue(inst, to);
}

void EmitX64::EmitFPFixedS64ToDouble(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg64 from = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const size_t fbits = args[1].GetImmediateU8();
    const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());
    ASSERT(rounding_mode == ctx.FPCR().RMode());

    code.cvtsi2sd(result, from);

    if (fbits != 0) {
        const u64 scale_factor = static_cast<u64>((1023 - fbits) << 52);
        code.mulsd(result, code.MConst(xword, scale_factor));
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitFPFixedS64ToSingle(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg64 from = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const size_t fbits = args[1].GetImmediateU8();
    const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());
    ASSERT(rounding_mode == ctx.FPCR().RMode());

    code.cvtsi2ss(result, from);

    if (fbits != 0) {
        const u32 scale_factor = static_cast<u32>((127 - fbits) << 23);
        code.mulss(result, code.MConst(xword, scale_factor));
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitFPFixedU64ToDouble(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Reg64 from = ctx.reg_alloc.UseGpr(args[0]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const size_t fbits = args[1].GetImmediateU8();
    const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());
    ASSERT(rounding_mode == ctx.FPCR().RMode());

    if (code.HasHostFeature(HostFeature::AVX512F)) {
        code.vcvtusi2sd(result, result, from);
    } else {
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

        code.movq(tmp, from);
        code.punpckldq(tmp, code.MConst(xword, 0x4530000043300000, 0));
        code.subpd(tmp, code.MConst(xword, 0x4330000000000000, 0x4530000000000000));
        code.pshufd(result, tmp, 0b01001110);
        code.addpd(result, tmp);
        if (ctx.FPCR().RMode() == FP::RoundingMode::TowardsMinusInfinity) {
            code.pand(result, code.MConst(xword, f64_non_sign_mask));
        }
    }

    if (fbits != 0) {
        const u64 scale_factor = static_cast<u64>((1023 - fbits) << 52);
        code.mulsd(result, code.MConst(xword, scale_factor));
    }

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitFPFixedU64ToSingle(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const size_t fbits = args[1].GetImmediateU8();
    const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());
    ASSERT(rounding_mode == ctx.FPCR().RMode());

    if (code.HasHostFeature(HostFeature::AVX512F)) {
        const Xbyak::Reg64 from = ctx.reg_alloc.UseGpr(args[0]);
        code.vcvtusi2ss(result, result, from);
    } else {
        const Xbyak::Reg64 from = ctx.reg_alloc.UseScratchGpr(args[0]);
        code.pxor(result, result);

        Xbyak::Label negative;
        Xbyak::Label end;

        code.test(from, from);
        code.js(negative);

        code.cvtsi2ss(result, from);
        code.jmp(end);

        code.L(negative);
        const Xbyak::Reg64 tmp = ctx.reg_alloc.ScratchGpr();
        code.mov(tmp, from);
        code.shr(tmp, 1);
        code.and_(from.cvt32(), 1);
        code.or_(from, tmp);
        code.cvtsi2ss(result, from);
        code.addss(result, result);

        code.L(end);
    }

    if (fbits != 0) {
        const u32 scale_factor = static_cast<u32>((127 - fbits) << 23);
        code.mulss(result, code.MConst(xword, scale_factor));
    }

    ctx.reg_alloc.DefineValue(inst, result);
}
}  // namespace Dynarmic::Backend::X64

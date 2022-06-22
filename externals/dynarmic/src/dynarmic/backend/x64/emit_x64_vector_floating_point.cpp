/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <array>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>

#include <mcl/assert.hpp>
#include <mcl/mp/metavalue/lift_value.hpp>
#include <mcl/mp/typelist/cartesian_product.hpp>
#include <mcl/mp/typelist/get.hpp>
#include <mcl/mp/typelist/lift_sequence.hpp>
#include <mcl/mp/typelist/list.hpp>
#include <mcl/mp/typelist/lower_to_tuple.hpp>
#include <mcl/type_traits/function_info.hpp>
#include <mcl/type_traits/integer_of_size.hpp>

#include "dynarmic/backend/x64/abi.h"
#include "dynarmic/backend/x64/block_of_code.h"
#include "dynarmic/backend/x64/constants.h"
#include "dynarmic/backend/x64/emit_x64.h"
#include "dynarmic/common/fp/fpcr.h"
#include "dynarmic/common/fp/info.h"
#include "dynarmic/common/fp/op.h"
#include "dynarmic/common/fp/util.h"
#include "dynarmic/common/lut_from_list.h"
#include "dynarmic/interface/optimization_flags.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/microinstruction.h"

namespace Dynarmic::Backend::X64 {

using namespace Xbyak::util;
namespace mp = mcl::mp;

namespace {

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

template<typename Lambda>
void MaybeStandardFPSCRValue(BlockOfCode& code, EmitContext& ctx, bool fpcr_controlled, Lambda lambda) {
    const bool switch_mxcsr = ctx.FPCR(fpcr_controlled) != ctx.FPCR();

    if (switch_mxcsr && !ctx.HasOptimization(OptimizationFlag::Unsafe_IgnoreStandardFPCRValue)) {
        code.EnterStandardASIMD();
        lambda();
        code.LeaveStandardASIMD();
    } else {
        lambda();
    }
}

template<size_t fsize, template<typename> class Indexer, size_t narg>
struct NaNHandler {
public:
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    using function_type = void (*)(std::array<VectorArray<FPT>, narg>&, FP::FPCR);

    static function_type GetDefault() {
        return GetDefaultImpl(std::make_index_sequence<narg - 1>{});
    }

private:
    template<size_t... argi>
    static function_type GetDefaultImpl(std::index_sequence<argi...>) {
        const auto result = [](std::array<VectorArray<FPT>, narg>& values, FP::FPCR) {
            VectorArray<FPT>& result = values[0];
            for (size_t elementi = 0; elementi < result.size(); ++elementi) {
                const auto current_values = Indexer<FPT>{}(elementi, values[argi + 1]...);
                if (auto r = FP::ProcessNaNs(std::get<argi>(current_values)...)) {
                    result[elementi] = *r;
                } else if (FP::IsNaN(result[elementi])) {
                    result[elementi] = FP::FPInfo<FPT>::DefaultNaN();
                }
            }
        };

        return static_cast<function_type>(result);
    }
};

template<size_t fsize, size_t nargs, typename NaNHandler>
void HandleNaNs(BlockOfCode& code, EmitContext& ctx, bool fpcr_controlled, std::array<Xbyak::Xmm, nargs + 1> xmms, const Xbyak::Xmm& nan_mask, NaNHandler nan_handler) {
    static_assert(fsize == 32 || fsize == 64, "fsize must be either 32 or 64");

    if (code.HasHostFeature(HostFeature::SSE41)) {
        code.ptest(nan_mask, nan_mask);
    } else {
        const Xbyak::Reg32 bitmask = ctx.reg_alloc.ScratchGpr().cvt32();
        code.movmskps(bitmask, nan_mask);
        code.cmp(bitmask, 0);
    }

    Xbyak::Label end;
    Xbyak::Label nan;

    code.jnz(nan, code.T_NEAR);
    code.L(end);

    code.SwitchToFarCode();
    code.L(nan);

    const Xbyak::Xmm result = xmms[0];

    code.sub(rsp, 8);
    ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));

    const size_t stack_space = xmms.size() * 16;
    code.sub(rsp, stack_space + ABI_SHADOW_SPACE);
    for (size_t i = 0; i < xmms.size(); ++i) {
        code.movaps(xword[rsp + ABI_SHADOW_SPACE + i * 16], xmms[i]);
    }
    code.lea(code.ABI_PARAM1, ptr[rsp + ABI_SHADOW_SPACE + 0 * 16]);
    code.mov(code.ABI_PARAM2, ctx.FPCR(fpcr_controlled).Value());

    code.CallFunction(nan_handler);

    code.movaps(result, xword[rsp + ABI_SHADOW_SPACE + 0 * 16]);
    code.add(rsp, stack_space + ABI_SHADOW_SPACE);
    ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
    code.add(rsp, 8);
    code.jmp(end, code.T_NEAR);
    code.SwitchToNearCode();
}

template<size_t fsize>
Xbyak::Address GetVectorOf(BlockOfCode& code, u64 value) {
    if constexpr (fsize == 32) {
        return code.MConst(xword, (value << 32) | value, (value << 32) | value);
    } else {
        return code.MConst(xword, value, value);
    }
}

template<size_t fsize, u64 value>
Xbyak::Address GetVectorOf(BlockOfCode& code) {
    if constexpr (fsize == 32) {
        return code.MConst(xword, (value << 32) | value, (value << 32) | value);
    } else {
        return code.MConst(xword, value, value);
    }
}

template<size_t fsize>
Xbyak::Address GetNaNVector(BlockOfCode& code) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;
    return GetVectorOf<fsize, FP::FPInfo<FPT>::DefaultNaN()>(code);
}

template<size_t fsize>
Xbyak::Address GetNegativeZeroVector(BlockOfCode& code) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;
    return GetVectorOf<fsize, FP::FPInfo<FPT>::Zero(true)>(code);
}

template<size_t fsize>
Xbyak::Address GetSmallestNormalVector(BlockOfCode& code) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;
    constexpr FPT smallest_normal_number = FP::FPValue<FPT, false, FP::FPInfo<FPT>::exponent_min, 1>();
    return GetVectorOf<fsize, smallest_normal_number>(code);
}

template<size_t fsize, bool sign, int exponent, mcl::unsigned_integer_of_size<fsize> value>
Xbyak::Address GetVectorOf(BlockOfCode& code) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;
    return GetVectorOf<fsize, FP::FPValue<FPT, sign, exponent, value>()>(code);
}

template<size_t fsize>
void ForceToDefaultNaN(BlockOfCode& code, FP::FPCR fpcr, Xbyak::Xmm result) {
    if (fpcr.DN()) {
        const Xbyak::Xmm nan_mask = xmm0;
        if (code.HasHostFeature(HostFeature::AVX)) {
            FCODE(vcmpunordp)(nan_mask, result, result);
            FCODE(blendvp)(result, GetNaNVector<fsize>(code));
        } else {
            code.movaps(nan_mask, result);
            FCODE(cmpordp)(nan_mask, nan_mask);
            code.andps(result, nan_mask);
            code.andnps(nan_mask, GetNaNVector<fsize>(code));
            code.orps(result, nan_mask);
        }
    }
}

template<size_t fsize>
void ZeroIfNaN(BlockOfCode& code, Xbyak::Xmm result) {
    const Xbyak::Xmm nan_mask = xmm0;
    if (code.HasHostFeature(HostFeature::AVX512_OrthoFloat)) {
        constexpr u32 nan_to_zero = FixupLUT(FpFixup::PosZero,
                                             FpFixup::PosZero);
        FCODE(vfixupimmp)(result, result, code.MConst(ptr_b, u64(nan_to_zero)), u8(0));
    } else if (code.HasHostFeature(HostFeature::AVX)) {
        FCODE(vcmpordp)(nan_mask, result, result);
        FCODE(vandp)(result, result, nan_mask);
    } else {
        code.movaps(nan_mask, result);
        FCODE(cmpordp)(nan_mask, nan_mask);
        code.andps(result, nan_mask);
    }
}

template<size_t fsize>
void DenormalsAreZero(BlockOfCode& code, FP::FPCR fpcr, std::initializer_list<Xbyak::Xmm> to_daz, Xbyak::Xmm tmp) {
    if (fpcr.FZ()) {
        if (fpcr.RMode() != FP::RoundingMode::TowardsMinusInfinity) {
            code.movaps(tmp, GetNegativeZeroVector<fsize>(code));
        } else {
            code.xorps(tmp, tmp);
        }
        for (const Xbyak::Xmm& xmm : to_daz) {
            FCODE(addp)(xmm, tmp);
        }
    }
}

template<typename T>
struct DefaultIndexer {
    std::tuple<T> operator()(size_t i, const VectorArray<T>& a) {
        return std::make_tuple(a[i]);
    }

    std::tuple<T, T> operator()(size_t i, const VectorArray<T>& a, const VectorArray<T>& b) {
        return std::make_tuple(a[i], b[i]);
    }

    std::tuple<T, T, T> operator()(size_t i, const VectorArray<T>& a, const VectorArray<T>& b, const VectorArray<T>& c) {
        return std::make_tuple(a[i], b[i], c[i]);
    }
};

template<typename T>
struct PairedIndexer {
    std::tuple<T, T> operator()(size_t i, const VectorArray<T>& a, const VectorArray<T>& b) {
        constexpr size_t halfway = std::tuple_size_v<VectorArray<T>> / 2;
        const size_t which_array = i / halfway;
        i %= halfway;
        switch (which_array) {
        case 0:
            return std::make_tuple(a[2 * i], a[2 * i + 1]);
        case 1:
            return std::make_tuple(b[2 * i], b[2 * i + 1]);
        }
        UNREACHABLE();
    }
};

template<typename T>
struct PairedLowerIndexer {
    std::tuple<T, T> operator()(size_t i, const VectorArray<T>& a, const VectorArray<T>& b) {
        constexpr size_t array_size = std::tuple_size_v<VectorArray<T>>;
        if constexpr (array_size == 4) {
            switch (i) {
            case 0:
                return std::make_tuple(a[0], a[1]);
            case 1:
                return std::make_tuple(b[0], b[1]);
            default:
                return std::make_tuple(0, 0);
            }
        } else if constexpr (array_size == 2) {
            if (i == 0) {
                return std::make_tuple(a[0], b[0]);
            }
            return std::make_tuple(0, 0);
        } else {
            UNREACHABLE();
        }
    }
};

template<size_t fsize, template<typename> class Indexer, size_t fpcr_controlled_arg_index = 1, typename Function>
void EmitTwoOpVectorOperation(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, Function fn, typename NaNHandler<fsize, Indexer, 2>::function_type nan_handler = NaNHandler<fsize, Indexer, 2>::GetDefault()) {
    static_assert(fsize == 32 || fsize == 64, "fsize must be either 32 or 64");

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const bool fpcr_controlled = args[fpcr_controlled_arg_index].GetImmediateU1();

    if (ctx.FPCR(fpcr_controlled).DN() || ctx.HasOptimization(OptimizationFlag::Unsafe_InaccurateNaN)) {
        Xbyak::Xmm result;

        if constexpr (std::is_member_function_pointer_v<Function>) {
            result = ctx.reg_alloc.UseScratchXmm(args[0]);
            MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
                (code.*fn)(result);
            });
        } else {
            const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseXmm(args[0]);
            result = ctx.reg_alloc.ScratchXmm();
            MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
                fn(result, xmm_a);
            });
        }

        if (!ctx.HasOptimization(OptimizationFlag::Unsafe_InaccurateNaN)) {
            ForceToDefaultNaN<fsize>(code, ctx.FPCR(fpcr_controlled), result);
        }

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm nan_mask = ctx.reg_alloc.ScratchXmm();

    if constexpr (std::is_member_function_pointer_v<Function>) {
        code.movaps(result, xmm_a);
        (code.*fn)(result);
    } else {
        fn(result, xmm_a);
    }

    if (code.HasHostFeature(HostFeature::AVX)) {
        FCODE(vcmpunordp)(nan_mask, result, result);
    } else {
        code.movaps(nan_mask, result);
        FCODE(cmpunordp)(nan_mask, nan_mask);
    }

    HandleNaNs<fsize, 1>(code, ctx, fpcr_controlled, {result, xmm_a}, nan_mask, nan_handler);

    ctx.reg_alloc.DefineValue(inst, result);
}

enum CheckInputNaN {
    Yes,
    No,
};

template<size_t fsize, template<typename> class Indexer, typename Function>
void EmitThreeOpVectorOperation(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, Function fn, CheckInputNaN check_input_nan = CheckInputNaN::No, typename NaNHandler<fsize, Indexer, 3>::function_type nan_handler = NaNHandler<fsize, Indexer, 3>::GetDefault()) {
    static_assert(fsize == 32 || fsize == 64, "fsize must be either 32 or 64");

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const bool fpcr_controlled = args[2].GetImmediateU1();

    if (ctx.FPCR(fpcr_controlled).DN() || ctx.HasOptimization(OptimizationFlag::Unsafe_InaccurateNaN)) {
        const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);

        if constexpr (std::is_member_function_pointer_v<Function>) {
            MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
                (code.*fn)(xmm_a, xmm_b);
            });
        } else {
            MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
                fn(xmm_a, xmm_b);
            });
        }

        if (!ctx.HasOptimization(OptimizationFlag::Unsafe_InaccurateNaN)) {
            ForceToDefaultNaN<fsize>(code, ctx.FPCR(fpcr_controlled), xmm_a);
        }

        ctx.reg_alloc.DefineValue(inst, xmm_a);
        return;
    }

    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm nan_mask = ctx.reg_alloc.ScratchXmm();

    code.movaps(result, xmm_a);

    if (check_input_nan == CheckInputNaN::Yes) {
        if (code.HasHostFeature(HostFeature::AVX)) {
            FCODE(vcmpunordp)(nan_mask, xmm_a, xmm_b);
        } else {
            code.movaps(nan_mask, xmm_b);
            FCODE(cmpunordp)(nan_mask, xmm_a);
        }
    }

    if constexpr (std::is_member_function_pointer_v<Function>) {
        (code.*fn)(result, xmm_b);
    } else {
        fn(result, xmm_b);
    }

    if (check_input_nan == CheckInputNaN::Yes) {
        FCODE(cmpunordp)(nan_mask, result);
    } else if (code.HasHostFeature(HostFeature::AVX)) {
        FCODE(vcmpunordp)(nan_mask, result, result);
    } else {
        code.movaps(nan_mask, result);
        FCODE(cmpunordp)(nan_mask, nan_mask);
    }

    HandleNaNs<fsize, 2>(code, ctx, fpcr_controlled, {result, xmm_a, xmm_b}, nan_mask, nan_handler);

    ctx.reg_alloc.DefineValue(inst, result);
}

template<typename Lambda>
void EmitTwoOpFallbackWithoutRegAlloc(BlockOfCode& code, EmitContext& ctx, Xbyak::Xmm result, Xbyak::Xmm arg1, Lambda lambda, bool fpcr_controlled) {
    const auto fn = static_cast<mcl::equivalent_function_type<Lambda>*>(lambda);

    const u32 fpcr = ctx.FPCR(fpcr_controlled).Value();

    constexpr u32 stack_space = 2 * 16;
    code.sub(rsp, stack_space + ABI_SHADOW_SPACE);
    code.lea(code.ABI_PARAM1, ptr[rsp + ABI_SHADOW_SPACE + 0 * 16]);
    code.lea(code.ABI_PARAM2, ptr[rsp + ABI_SHADOW_SPACE + 1 * 16]);
    code.mov(code.ABI_PARAM3.cvt32(), fpcr);
    code.lea(code.ABI_PARAM4, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);

    code.movaps(xword[code.ABI_PARAM2], arg1);
    code.CallFunction(fn);
    code.movaps(result, xword[rsp + ABI_SHADOW_SPACE + 0 * 16]);

    code.add(rsp, stack_space + ABI_SHADOW_SPACE);
}

template<size_t fpcr_controlled_arg_index = 1, typename Lambda>
void EmitTwoOpFallback(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, Lambda lambda) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm arg1 = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    ctx.reg_alloc.EndOfAllocScope();
    ctx.reg_alloc.HostCall(nullptr);

    const bool fpcr_controlled = args[fpcr_controlled_arg_index].GetImmediateU1();

    EmitTwoOpFallbackWithoutRegAlloc(code, ctx, result, arg1, lambda, fpcr_controlled);

    ctx.reg_alloc.DefineValue(inst, result);
}

template<typename Lambda>
void EmitThreeOpFallbackWithoutRegAlloc(BlockOfCode& code, EmitContext& ctx, Xbyak::Xmm result, Xbyak::Xmm arg1, Xbyak::Xmm arg2, Lambda lambda, bool fpcr_controlled) {
    const auto fn = static_cast<mcl::equivalent_function_type<Lambda>*>(lambda);

    const u32 fpcr = ctx.FPCR(fpcr_controlled).Value();

#ifdef _WIN32
    constexpr u32 stack_space = 4 * 16;
    code.sub(rsp, stack_space + ABI_SHADOW_SPACE);
    code.lea(code.ABI_PARAM1, ptr[rsp + ABI_SHADOW_SPACE + 1 * 16]);
    code.lea(code.ABI_PARAM2, ptr[rsp + ABI_SHADOW_SPACE + 2 * 16]);
    code.lea(code.ABI_PARAM3, ptr[rsp + ABI_SHADOW_SPACE + 3 * 16]);
    code.mov(code.ABI_PARAM4.cvt32(), fpcr);
    code.lea(rax, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
    code.mov(qword[rsp + ABI_SHADOW_SPACE + 0], rax);
#else
    constexpr u32 stack_space = 3 * 16;
    code.sub(rsp, stack_space + ABI_SHADOW_SPACE);
    code.lea(code.ABI_PARAM1, ptr[rsp + ABI_SHADOW_SPACE + 0 * 16]);
    code.lea(code.ABI_PARAM2, ptr[rsp + ABI_SHADOW_SPACE + 1 * 16]);
    code.lea(code.ABI_PARAM3, ptr[rsp + ABI_SHADOW_SPACE + 2 * 16]);
    code.mov(code.ABI_PARAM4.cvt32(), fpcr);
    code.lea(code.ABI_PARAM5, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
#endif

    code.movaps(xword[code.ABI_PARAM2], arg1);
    code.movaps(xword[code.ABI_PARAM3], arg2);
    code.CallFunction(fn);

#ifdef _WIN32
    code.movaps(result, xword[rsp + ABI_SHADOW_SPACE + 1 * 16]);
#else
    code.movaps(result, xword[rsp + ABI_SHADOW_SPACE + 0 * 16]);
#endif

    code.add(rsp, stack_space + ABI_SHADOW_SPACE);
}

template<typename Lambda>
void EmitThreeOpFallback(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, Lambda lambda) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm arg1 = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm arg2 = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    ctx.reg_alloc.EndOfAllocScope();
    ctx.reg_alloc.HostCall(nullptr);

    const bool fpcr_controlled = args[2].GetImmediateU1();

    EmitThreeOpFallbackWithoutRegAlloc(code, ctx, result, arg1, arg2, lambda, fpcr_controlled);

    ctx.reg_alloc.DefineValue(inst, result);
}

template<typename Lambda>
void EmitFourOpFallbackWithoutRegAlloc(BlockOfCode& code, EmitContext& ctx, Xbyak::Xmm result, Xbyak::Xmm arg1, Xbyak::Xmm arg2, Xbyak::Xmm arg3, Lambda lambda, bool fpcr_controlled) {
    const auto fn = static_cast<mcl::equivalent_function_type<Lambda>*>(lambda);

#ifdef _WIN32
    constexpr u32 stack_space = 5 * 16;
    code.sub(rsp, stack_space + ABI_SHADOW_SPACE);
    code.lea(code.ABI_PARAM1, ptr[rsp + ABI_SHADOW_SPACE + 1 * 16]);
    code.lea(code.ABI_PARAM2, ptr[rsp + ABI_SHADOW_SPACE + 2 * 16]);
    code.lea(code.ABI_PARAM3, ptr[rsp + ABI_SHADOW_SPACE + 3 * 16]);
    code.lea(code.ABI_PARAM4, ptr[rsp + ABI_SHADOW_SPACE + 4 * 16]);
    code.mov(qword[rsp + ABI_SHADOW_SPACE + 0], ctx.FPCR(fpcr_controlled).Value());
    code.lea(rax, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
    code.mov(qword[rsp + ABI_SHADOW_SPACE + 8], rax);
#else
    constexpr u32 stack_space = 4 * 16;
    code.sub(rsp, stack_space + ABI_SHADOW_SPACE);
    code.lea(code.ABI_PARAM1, ptr[rsp + ABI_SHADOW_SPACE + 0 * 16]);
    code.lea(code.ABI_PARAM2, ptr[rsp + ABI_SHADOW_SPACE + 1 * 16]);
    code.lea(code.ABI_PARAM3, ptr[rsp + ABI_SHADOW_SPACE + 2 * 16]);
    code.lea(code.ABI_PARAM4, ptr[rsp + ABI_SHADOW_SPACE + 3 * 16]);
    code.mov(code.ABI_PARAM5.cvt32(), ctx.FPCR(fpcr_controlled).Value());
    code.lea(code.ABI_PARAM6, code.ptr[code.r15 + code.GetJitStateInfo().offsetof_fpsr_exc]);
#endif

    code.movaps(xword[code.ABI_PARAM2], arg1);
    code.movaps(xword[code.ABI_PARAM3], arg2);
    code.movaps(xword[code.ABI_PARAM4], arg3);
    code.CallFunction(fn);

#ifdef _WIN32
    code.movaps(result, xword[rsp + ABI_SHADOW_SPACE + 1 * 16]);
#else
    code.movaps(result, xword[rsp + ABI_SHADOW_SPACE + 0 * 16]);
#endif

    code.add(rsp, stack_space + ABI_SHADOW_SPACE);
}

template<typename Lambda>
void EmitFourOpFallback(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst, Lambda lambda) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const bool fpcr_controlled = args[3].GetImmediateU1();
    const Xbyak::Xmm arg1 = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm arg2 = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm arg3 = ctx.reg_alloc.UseXmm(args[2]);
    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    ctx.reg_alloc.EndOfAllocScope();
    ctx.reg_alloc.HostCall(nullptr);

    EmitFourOpFallbackWithoutRegAlloc(code, ctx, result, arg1, arg2, arg3, lambda, fpcr_controlled);

    ctx.reg_alloc.DefineValue(inst, result);
}

}  // anonymous namespace

template<size_t fsize>
void FPVectorAbs(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;
    constexpr FPT non_sign_mask = FP::FPInfo<FPT>::sign_mask - FPT(1u);
    constexpr u64 non_sign_mask64 = mcl::bit::replicate_element<fsize, u64>(non_sign_mask);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Address mask = code.MConst(xword, non_sign_mask64, non_sign_mask64);

    code.andps(a, mask);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitFPVectorAbs16(EmitContext& ctx, IR::Inst* inst) {
    FPVectorAbs<16>(code, ctx, inst);
}

void EmitX64::EmitFPVectorAbs32(EmitContext& ctx, IR::Inst* inst) {
    FPVectorAbs<32>(code, ctx, inst);
}

void EmitX64::EmitFPVectorAbs64(EmitContext& ctx, IR::Inst* inst) {
    FPVectorAbs<64>(code, ctx, inst);
}

void EmitX64::EmitFPVectorAdd32(EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpVectorOperation<32, DefaultIndexer>(code, ctx, inst, &Xbyak::CodeGenerator::addps);
}

void EmitX64::EmitFPVectorAdd64(EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpVectorOperation<64, DefaultIndexer>(code, ctx, inst, &Xbyak::CodeGenerator::addpd);
}

void EmitX64::EmitFPVectorDiv32(EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpVectorOperation<32, DefaultIndexer>(code, ctx, inst, &Xbyak::CodeGenerator::divps);
}

void EmitX64::EmitFPVectorDiv64(EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpVectorOperation<64, DefaultIndexer>(code, ctx, inst, &Xbyak::CodeGenerator::divpd);
}

void EmitX64::EmitFPVectorEqual16(EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpFallback(code, ctx, inst, [](VectorArray<u16>& result, const VectorArray<u16>& op1, const VectorArray<u16>& op2, FP::FPCR fpcr, FP::FPSR& fpsr) {
        for (size_t i = 0; i < result.size(); i++) {
            result[i] = FP::FPCompareEQ(op1[i], op2[i], fpcr, fpsr) ? 0xFFFF : 0;
        }
    });
}

void EmitX64::EmitFPVectorEqual32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const bool fpcr_controlled = args[2].GetImmediateU1();
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.FPCR(fpcr_controlled).FZ() ? ctx.reg_alloc.UseScratchXmm(args[1]) : ctx.reg_alloc.UseXmm(args[1]);

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        DenormalsAreZero<32>(code, ctx.FPCR(fpcr_controlled), {a, b}, xmm0);
        code.cmpeqps(a, b);
    });

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitFPVectorEqual64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const bool fpcr_controlled = args[2].GetImmediateU1();
    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Xmm b = ctx.FPCR(fpcr_controlled).FZ() ? ctx.reg_alloc.UseScratchXmm(args[1]) : ctx.reg_alloc.UseXmm(args[1]);

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        DenormalsAreZero<64>(code, ctx.FPCR(fpcr_controlled), {a, b}, xmm0);
        code.cmpeqpd(a, b);
    });

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitFPVectorFromHalf32(EmitContext& ctx, IR::Inst* inst) {
    const auto rounding_mode = static_cast<FP::RoundingMode>(inst->GetArg(1).GetU8());
    const bool fpcr_controlled = inst->GetArg(2).GetU1();

    if (code.HasHostFeature(HostFeature::F16C) && !ctx.FPCR().AHP() && !ctx.FPCR().FZ16()) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);

        const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm value = ctx.reg_alloc.UseXmm(args[0]);

        code.vcvtph2ps(result, value);
        ForceToDefaultNaN<32>(code, ctx.FPCR(fpcr_controlled), result);

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

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
                    [](VectorArray<u32>& output, const VectorArray<u16>& input, FP::FPCR fpcr, FP::FPSR& fpsr) {
                        constexpr FP::RoundingMode rounding_mode = mp::get<0, I>::value;

                        for (size_t i = 0; i < output.size(); ++i) {
                            output[i] = FP::FPConvert<u32, u16>(input[i], fpcr, rounding_mode, fpsr);
                        }
                    })};
        },
        mp::cartesian_product<rounding_list>{});

    EmitTwoOpFallback<2>(code, ctx, inst, lut.at(std::make_tuple(rounding_mode)));
}

void EmitX64::EmitFPVectorFromSignedFixed32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm xmm = ctx.reg_alloc.UseScratchXmm(args[0]);
    const int fbits = args[1].GetImmediateU8();
    const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());
    const bool fpcr_controlled = args[3].GetImmediateU1();
    ASSERT(rounding_mode == ctx.FPCR(fpcr_controlled).RMode());

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        code.cvtdq2ps(xmm, xmm);
        if (fbits != 0) {
            code.mulps(xmm, GetVectorOf<32>(code, static_cast<u32>(127 - fbits) << 23));
        }
    });

    ctx.reg_alloc.DefineValue(inst, xmm);
}

void EmitX64::EmitFPVectorFromSignedFixed64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm xmm = ctx.reg_alloc.UseScratchXmm(args[0]);
    const int fbits = args[1].GetImmediateU8();
    const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());
    const bool fpcr_controlled = args[3].GetImmediateU1();
    ASSERT(rounding_mode == ctx.FPCR(fpcr_controlled).RMode());

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        if (code.HasHostFeature(HostFeature::AVX512_OrthoFloat)) {
            code.vcvtqq2pd(xmm, xmm);
        } else if (code.HasHostFeature(HostFeature::SSE41)) {
            const Xbyak::Xmm xmm_tmp = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Reg64 tmp = ctx.reg_alloc.ScratchGpr();

            // First quadword
            code.movq(tmp, xmm);
            code.cvtsi2sd(xmm, tmp);

            // Second quadword
            code.pextrq(tmp, xmm, 1);
            code.cvtsi2sd(xmm_tmp, tmp);

            // Combine
            code.unpcklpd(xmm, xmm_tmp);
        } else {
            const Xbyak::Xmm high_xmm = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Xmm xmm_tmp = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Reg64 tmp = ctx.reg_alloc.ScratchGpr();

            // First quadword
            code.movhlps(high_xmm, xmm);
            code.movq(tmp, xmm);
            code.cvtsi2sd(xmm, tmp);

            // Second quadword
            code.movq(tmp, high_xmm);
            code.cvtsi2sd(xmm_tmp, tmp);

            // Combine
            code.unpcklpd(xmm, xmm_tmp);
        }

        if (fbits != 0) {
            code.mulpd(xmm, GetVectorOf<64>(code, static_cast<u64>(1023 - fbits) << 52));
        }
    });

    ctx.reg_alloc.DefineValue(inst, xmm);
}

void EmitX64::EmitFPVectorFromUnsignedFixed32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm xmm = ctx.reg_alloc.UseScratchXmm(args[0]);
    const int fbits = args[1].GetImmediateU8();
    const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());
    const bool fpcr_controlled = args[3].GetImmediateU1();
    ASSERT(rounding_mode == ctx.FPCR(fpcr_controlled).RMode());

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        if (code.HasHostFeature(HostFeature::AVX512_Ortho)) {
            code.vcvtudq2ps(xmm, xmm);
        } else {
            const Xbyak::Address mem_4B000000 = code.MConst(xword, 0x4B0000004B000000, 0x4B0000004B000000);
            const Xbyak::Address mem_53000000 = code.MConst(xword, 0x5300000053000000, 0x5300000053000000);
            const Xbyak::Address mem_D3000080 = code.MConst(xword, 0xD3000080D3000080, 0xD3000080D3000080);

            const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

            if (code.HasHostFeature(HostFeature::AVX)) {
                code.vpblendw(tmp, xmm, mem_4B000000, 0b10101010);
                code.vpsrld(xmm, xmm, 16);
                code.vpblendw(xmm, xmm, mem_53000000, 0b10101010);
                code.vaddps(xmm, xmm, mem_D3000080);
                code.vaddps(xmm, tmp, xmm);
            } else {
                const Xbyak::Address mem_0xFFFF = code.MConst(xword, 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF);

                code.movdqa(tmp, mem_0xFFFF);

                code.pand(tmp, xmm);
                code.por(tmp, mem_4B000000);
                code.psrld(xmm, 16);
                code.por(xmm, mem_53000000);
                code.addps(xmm, mem_D3000080);
                code.addps(xmm, tmp);
            }
        }

        if (fbits != 0) {
            code.mulps(xmm, GetVectorOf<32>(code, static_cast<u32>(127 - fbits) << 23));
        }

        if (ctx.FPCR(fpcr_controlled).RMode() == FP::RoundingMode::TowardsMinusInfinity) {
            code.pand(xmm, code.MConst(xword, 0x7FFFFFFF7FFFFFFF, 0x7FFFFFFF7FFFFFFF));
        }
    });

    ctx.reg_alloc.DefineValue(inst, xmm);
}

void EmitX64::EmitFPVectorFromUnsignedFixed64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const Xbyak::Xmm xmm = ctx.reg_alloc.UseScratchXmm(args[0]);
    const int fbits = args[1].GetImmediateU8();
    const FP::RoundingMode rounding_mode = static_cast<FP::RoundingMode>(args[2].GetImmediateU8());
    const bool fpcr_controlled = args[3].GetImmediateU1();
    ASSERT(rounding_mode == ctx.FPCR(fpcr_controlled).RMode());

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        if (code.HasHostFeature(HostFeature::AVX512_OrthoFloat)) {
            code.vcvtuqq2pd(xmm, xmm);
        } else {
            const Xbyak::Address unpack = code.MConst(xword, 0x4530000043300000, 0);
            const Xbyak::Address subtrahend = code.MConst(xword, 0x4330000000000000, 0x4530000000000000);

            const Xbyak::Xmm unpack_reg = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Xmm subtrahend_reg = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Xmm tmp1 = ctx.reg_alloc.ScratchXmm();

            if (code.HasHostFeature(HostFeature::AVX)) {
                code.vmovapd(unpack_reg, unpack);
                code.vmovapd(subtrahend_reg, subtrahend);

                code.vunpcklps(tmp1, xmm, unpack_reg);
                code.vsubpd(tmp1, tmp1, subtrahend_reg);

                code.vpermilps(xmm, xmm, 0b01001110);

                code.vunpcklps(xmm, xmm, unpack_reg);
                code.vsubpd(xmm, xmm, subtrahend_reg);

                code.vhaddpd(xmm, tmp1, xmm);
            } else {
                const Xbyak::Xmm tmp2 = ctx.reg_alloc.ScratchXmm();

                code.movapd(unpack_reg, unpack);
                code.movapd(subtrahend_reg, subtrahend);

                code.pshufd(tmp1, xmm, 0b01001110);

                code.punpckldq(xmm, unpack_reg);
                code.subpd(xmm, subtrahend_reg);
                code.pshufd(tmp2, xmm, 0b01001110);
                code.addpd(xmm, tmp2);

                code.punpckldq(tmp1, unpack_reg);
                code.subpd(tmp1, subtrahend_reg);

                code.pshufd(unpack_reg, tmp1, 0b01001110);
                code.addpd(unpack_reg, tmp1);

                code.unpcklpd(xmm, unpack_reg);
            }
        }

        if (fbits != 0) {
            code.mulpd(xmm, GetVectorOf<64>(code, static_cast<u64>(1023 - fbits) << 52));
        }

        if (ctx.FPCR(fpcr_controlled).RMode() == FP::RoundingMode::TowardsMinusInfinity) {
            code.pand(xmm, code.MConst(xword, 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF));
        }
    });

    ctx.reg_alloc.DefineValue(inst, xmm);
}

void EmitX64::EmitFPVectorGreater32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const bool fpcr_controlled = args[2].GetImmediateU1();
    const Xbyak::Xmm a = ctx.FPCR(fpcr_controlled).FZ() ? ctx.reg_alloc.UseScratchXmm(args[0]) : ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        DenormalsAreZero<32>(code, ctx.FPCR(fpcr_controlled), {a, b}, xmm0);
        code.cmpltps(b, a);
    });

    ctx.reg_alloc.DefineValue(inst, b);
}

void EmitX64::EmitFPVectorGreater64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const bool fpcr_controlled = args[2].GetImmediateU1();
    const Xbyak::Xmm a = ctx.FPCR(fpcr_controlled).FZ() ? ctx.reg_alloc.UseScratchXmm(args[0]) : ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        DenormalsAreZero<64>(code, ctx.FPCR(fpcr_controlled), {a, b}, xmm0);
        code.cmpltpd(b, a);
    });

    ctx.reg_alloc.DefineValue(inst, b);
}

void EmitX64::EmitFPVectorGreaterEqual32(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const bool fpcr_controlled = args[2].GetImmediateU1();
    const Xbyak::Xmm a = ctx.FPCR(fpcr_controlled).FZ() ? ctx.reg_alloc.UseScratchXmm(args[0]) : ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        DenormalsAreZero<32>(code, ctx.FPCR(fpcr_controlled), {a, b}, xmm0);
        code.cmpleps(b, a);
    });

    ctx.reg_alloc.DefineValue(inst, b);
}

void EmitX64::EmitFPVectorGreaterEqual64(EmitContext& ctx, IR::Inst* inst) {
    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const bool fpcr_controlled = args[2].GetImmediateU1();
    const Xbyak::Xmm a = ctx.FPCR(fpcr_controlled).FZ() ? ctx.reg_alloc.UseScratchXmm(args[0]) : ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm b = ctx.reg_alloc.UseScratchXmm(args[1]);

    MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
        DenormalsAreZero<64>(code, ctx.FPCR(fpcr_controlled), {a, b}, xmm0);
        code.cmplepd(b, a);
    });

    ctx.reg_alloc.DefineValue(inst, b);
}

template<size_t fsize, bool is_max>
static void EmitFPVectorMinMax(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    const bool fpcr_controlled = inst->GetArg(2).GetU1();

    if (ctx.FPCR(fpcr_controlled).DN()) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);
        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm xmm_b = ctx.FPCR(fpcr_controlled).FZ() ? ctx.reg_alloc.UseScratchXmm(args[1]) : ctx.reg_alloc.UseXmm(args[1]);

        const Xbyak::Xmm mask = xmm0;
        const Xbyak::Xmm eq = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm nan_mask = ctx.reg_alloc.ScratchXmm();

        MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
            DenormalsAreZero<fsize>(code, ctx.FPCR(fpcr_controlled), {result, xmm_b}, mask);

            if (code.HasHostFeature(HostFeature::AVX512_OrthoFloat)) {
                constexpr FpRangeSelect range_select = is_max ? FpRangeSelect::Max : FpRangeSelect::Min;
                FCODE(vcmpp)(k1, result, xmm_b, Cmp::Unordered_Q);
                FCODE(vrangep)(result, result, xmm_b, FpRangeLUT(range_select, FpRangeSign::Preserve));
                FCODE(vblendmp)(result | k1, result, GetNaNVector<fsize>(code));
            } else if (code.HasHostFeature(HostFeature::AVX)) {
                FCODE(vcmpeqp)(mask, result, xmm_b);
                FCODE(vcmpunordp)(nan_mask, result, xmm_b);
                if constexpr (is_max) {
                    FCODE(vandp)(eq, result, xmm_b);
                    FCODE(vmaxp)(result, result, xmm_b);
                } else {
                    FCODE(vorp)(eq, result, xmm_b);
                    FCODE(vminp)(result, result, xmm_b);
                }
                FCODE(blendvp)(result, eq);
                FCODE(vblendvp)(result, result, GetNaNVector<fsize>(code), nan_mask);
            } else {
                code.movaps(mask, result);
                code.movaps(eq, result);
                code.movaps(nan_mask, result);
                FCODE(cmpneqp)(mask, xmm_b);
                FCODE(cmpordp)(nan_mask, xmm_b);

                if constexpr (is_max) {
                    code.andps(eq, xmm_b);
                    FCODE(maxp)(result, xmm_b);
                } else {
                    code.orps(eq, xmm_b);
                    FCODE(minp)(result, xmm_b);
                }

                code.andps(result, mask);
                code.andnps(mask, eq);
                code.orps(result, mask);

                code.andps(result, nan_mask);
                code.andnps(nan_mask, GetNaNVector<fsize>(code));
                code.orps(result, nan_mask);
            }
        });

        ctx.reg_alloc.DefineValue(inst, result);

        return;
    }

    EmitThreeOpVectorOperation<fsize, DefaultIndexer>(
        code, ctx, inst, [&](const Xbyak::Xmm& result, Xbyak::Xmm xmm_b) {
            const Xbyak::Xmm mask = xmm0;
            const Xbyak::Xmm eq = ctx.reg_alloc.ScratchXmm();

            if (ctx.FPCR(fpcr_controlled).FZ()) {
                const Xbyak::Xmm prev_xmm_b = xmm_b;
                xmm_b = ctx.reg_alloc.ScratchXmm();
                code.movaps(xmm_b, prev_xmm_b);
                DenormalsAreZero<fsize>(code, ctx.FPCR(fpcr_controlled), {result, xmm_b}, mask);
            }

            // What we are doing here is handling the case when the inputs are differently signed zeros.
            // x86-64 treats differently signed zeros as equal while ARM does not.
            // Thus if we AND together things that x86-64 thinks are equal we'll get the positive zero.

            if (code.HasHostFeature(HostFeature::AVX512_OrthoFloat)) {
                // vrangep{s,d} will already correctly handle comparing
                // signed zeros similar to ARM
                // max(+0.0, -0.0) = +0.0.
                // min(+0.0, -0.0) = -0.0
                constexpr FpRangeSelect range_select = is_max ? FpRangeSelect::Max : FpRangeSelect::Min;
                FCODE(vrangep)(result, result, xmm_b, FpRangeLUT(range_select, FpRangeSign::Preserve));
            } else if (code.HasHostFeature(HostFeature::AVX)) {
                FCODE(vcmpeqp)(mask, result, xmm_b);
                if constexpr (is_max) {
                    FCODE(vandp)(eq, result, xmm_b);
                    FCODE(vmaxp)(result, result, xmm_b);
                } else {
                    FCODE(vorp)(eq, result, xmm_b);
                    FCODE(vminp)(result, result, xmm_b);
                }
                FCODE(blendvp)(result, eq);
            } else {
                code.movaps(mask, result);
                code.movaps(eq, result);
                FCODE(cmpneqp)(mask, xmm_b);

                if constexpr (is_max) {
                    code.andps(eq, xmm_b);
                    FCODE(maxp)(result, xmm_b);
                } else {
                    code.orps(eq, xmm_b);
                    FCODE(minp)(result, xmm_b);
                }

                code.andps(result, mask);
                code.andnps(mask, eq);
                code.orps(result, mask);
            }
        },
        CheckInputNaN::Yes);
}

void EmitX64::EmitFPVectorMax32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorMinMax<32, true>(code, ctx, inst);
}

void EmitX64::EmitFPVectorMax64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorMinMax<64, true>(code, ctx, inst);
}

void EmitX64::EmitFPVectorMin32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorMinMax<32, false>(code, ctx, inst);
}

void EmitX64::EmitFPVectorMin64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorMinMax<64, false>(code, ctx, inst);
}

void EmitX64::EmitFPVectorMul32(EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpVectorOperation<32, DefaultIndexer>(code, ctx, inst, &Xbyak::CodeGenerator::mulps);
}

void EmitX64::EmitFPVectorMul64(EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpVectorOperation<64, DefaultIndexer>(code, ctx, inst, &Xbyak::CodeGenerator::mulpd);
}

template<size_t fsize>
void EmitFPVectorMulAdd(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    const auto fallback_fn = [](VectorArray<FPT>& result, const VectorArray<FPT>& addend, const VectorArray<FPT>& op1, const VectorArray<FPT>& op2, FP::FPCR fpcr, FP::FPSR& fpsr) {
        for (size_t i = 0; i < result.size(); i++) {
            result[i] = FP::FPMulAdd<FPT>(addend[i], op1[i], op2[i], fpcr, fpsr);
        }
    };

    if constexpr (fsize != 16) {
        if (code.HasHostFeature(HostFeature::FMA | HostFeature::AVX) && ctx.HasOptimization(OptimizationFlag::Unsafe_InaccurateNaN)) {
            auto args = ctx.reg_alloc.GetArgumentInfo(inst);
            const bool fpcr_controlled = args[3].GetImmediateU1();

            const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
            const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);
            const Xbyak::Xmm xmm_c = ctx.reg_alloc.UseXmm(args[2]);

            MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
                FCODE(vfmadd231p)(result, xmm_b, xmm_c);
            });

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }

        if (code.HasHostFeature(HostFeature::FMA | HostFeature::AVX)) {
            auto args = ctx.reg_alloc.GetArgumentInfo(inst);
            const bool fpcr_controlled = args[3].GetImmediateU1();

            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseXmm(args[0]);
            const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);
            const Xbyak::Xmm xmm_c = ctx.reg_alloc.UseXmm(args[2]);
            const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

            Xbyak::Label end, fallback;

            MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
                code.movaps(result, xmm_a);
                FCODE(vfmadd231p)(result, xmm_b, xmm_c);

                code.movaps(tmp, GetNegativeZeroVector<fsize>(code));
                code.andnps(tmp, result);
                FCODE(vcmpeq_uqp)(tmp, tmp, GetSmallestNormalVector<fsize>(code));
                code.vptest(tmp, tmp);
                code.jnz(fallback, code.T_NEAR);
                code.L(end);
            });

            code.SwitchToFarCode();
            code.L(fallback);
            code.sub(rsp, 8);
            ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
            EmitFourOpFallbackWithoutRegAlloc(code, ctx, result, xmm_a, xmm_b, xmm_c, fallback_fn, fpcr_controlled);
            ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
            code.add(rsp, 8);
            code.jmp(end, code.T_NEAR);
            code.SwitchToNearCode();

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }

        if (ctx.HasOptimization(OptimizationFlag::Unsafe_UnfuseFMA)) {
            auto args = ctx.reg_alloc.GetArgumentInfo(inst);

            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseScratchXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseScratchXmm(args[1]);
            const Xbyak::Xmm operand3 = ctx.reg_alloc.UseXmm(args[2]);

            FCODE(mulp)(operand2, operand3);
            FCODE(addp)(operand1, operand2);

            ctx.reg_alloc.DefineValue(inst, operand1);
            return;
        }
    }

    EmitFourOpFallback(code, ctx, inst, fallback_fn);
}

void EmitX64::EmitFPVectorMulAdd16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorMulAdd<16>(code, ctx, inst);
}

void EmitX64::EmitFPVectorMulAdd32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorMulAdd<32>(code, ctx, inst);
}

void EmitX64::EmitFPVectorMulAdd64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorMulAdd<64>(code, ctx, inst);
}

template<size_t fsize>
static void EmitFPVectorMulX(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);
    const bool fpcr_controlled = args[2].GetImmediateU1();

    if (ctx.FPCR(fpcr_controlled).DN() && code.HasHostFeature(HostFeature::AVX)) {
        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);
        const Xbyak::Xmm operand = ctx.reg_alloc.UseXmm(args[1]);
        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
        const Xbyak::Xmm twos = ctx.reg_alloc.ScratchXmm();

        MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
            FCODE(vcmpunordp)(xmm0, result, operand);
            FCODE(vxorp)(twos, result, operand);
            FCODE(mulp)(result, operand);
            FCODE(andp)(twos, GetNegativeZeroVector<fsize>(code));
            FCODE(vcmpunordp)(tmp, result, result);
            FCODE(blendvp)(result, GetNaNVector<fsize>(code));
            FCODE(orp)(twos, GetVectorOf<fsize, false, 0, 2>(code));
            FCODE(andnp)(xmm0, tmp);
            FCODE(blendvp)(result, twos);
        });

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

    const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
    const Xbyak::Xmm xmm_a = ctx.reg_alloc.UseXmm(args[0]);
    const Xbyak::Xmm xmm_b = ctx.reg_alloc.UseXmm(args[1]);
    const Xbyak::Xmm nan_mask = ctx.reg_alloc.ScratchXmm();

    code.movaps(nan_mask, xmm_b);
    code.movaps(result, xmm_a);
    FCODE(cmpunordp)(nan_mask, xmm_a);
    FCODE(mulp)(result, xmm_b);
    FCODE(cmpunordp)(nan_mask, result);

    const auto nan_handler = Common::FptrCast(
        [](std::array<VectorArray<FPT>, 3>& values, FP::FPCR fpcr) {
            VectorArray<FPT>& result = values[0];
            for (size_t elementi = 0; elementi < result.size(); ++elementi) {
                if (auto r = FP::ProcessNaNs(values[1][elementi], values[2][elementi])) {
                    result[elementi] = fpcr.DN() ? FP::FPInfo<FPT>::DefaultNaN() : *r;
                } else if (FP::IsNaN(result[elementi])) {
                    const FPT sign = (values[1][elementi] ^ values[2][elementi]) & FP::FPInfo<FPT>::sign_mask;
                    result[elementi] = sign | FP::FPValue<FPT, false, 0, 2>();
                }
            }
        });

    HandleNaNs<fsize, 2>(code, ctx, fpcr_controlled, {result, xmm_a, xmm_b}, nan_mask, nan_handler);

    ctx.reg_alloc.DefineValue(inst, result);
}

void EmitX64::EmitFPVectorMulX32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorMulX<32>(code, ctx, inst);
}

void EmitX64::EmitFPVectorMulX64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorMulX<64>(code, ctx, inst);
}

template<size_t fsize>
void FPVectorNeg(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;
    constexpr FPT sign_mask = FP::FPInfo<FPT>::sign_mask;
    constexpr u64 sign_mask64 = mcl::bit::replicate_element<fsize, u64>(sign_mask);

    auto args = ctx.reg_alloc.GetArgumentInfo(inst);

    const Xbyak::Xmm a = ctx.reg_alloc.UseScratchXmm(args[0]);
    const Xbyak::Address mask = code.MConst(xword, sign_mask64, sign_mask64);

    code.xorps(a, mask);

    ctx.reg_alloc.DefineValue(inst, a);
}

void EmitX64::EmitFPVectorNeg16(EmitContext& ctx, IR::Inst* inst) {
    FPVectorNeg<16>(code, ctx, inst);
}

void EmitX64::EmitFPVectorNeg32(EmitContext& ctx, IR::Inst* inst) {
    FPVectorNeg<32>(code, ctx, inst);
}

void EmitX64::EmitFPVectorNeg64(EmitContext& ctx, IR::Inst* inst) {
    FPVectorNeg<64>(code, ctx, inst);
}

void EmitX64::EmitFPVectorPairedAdd32(EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpVectorOperation<32, PairedIndexer>(code, ctx, inst, &Xbyak::CodeGenerator::haddps);
}

void EmitX64::EmitFPVectorPairedAdd64(EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpVectorOperation<64, PairedIndexer>(code, ctx, inst, &Xbyak::CodeGenerator::haddpd);
}

void EmitX64::EmitFPVectorPairedAddLower32(EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpVectorOperation<32, PairedLowerIndexer>(code, ctx, inst, [&](Xbyak::Xmm result, Xbyak::Xmm xmm_b) {
        const Xbyak::Xmm zero = ctx.reg_alloc.ScratchXmm();
        code.xorps(zero, zero);
        code.punpcklqdq(result, xmm_b);
        code.haddps(result, zero);
    });
}

void EmitX64::EmitFPVectorPairedAddLower64(EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpVectorOperation<64, PairedLowerIndexer>(code, ctx, inst, [&](Xbyak::Xmm result, Xbyak::Xmm xmm_b) {
        const Xbyak::Xmm zero = ctx.reg_alloc.ScratchXmm();
        code.xorps(zero, zero);
        code.punpcklqdq(result, xmm_b);
        code.haddpd(result, zero);
    });
}

template<size_t fsize>
static void EmitRecipEstimate(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    if constexpr (fsize != 16) {
        if (ctx.HasOptimization(OptimizationFlag::Unsafe_ReducedErrorFP)) {
            auto args = ctx.reg_alloc.GetArgumentInfo(inst);
            const Xbyak::Xmm operand = ctx.reg_alloc.UseXmm(args[0]);
            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

            if (code.HasHostFeature(HostFeature::AVX512_OrthoFloat)) {
                FCODE(vrcp14p)(result, operand);
            } else {
                if constexpr (fsize == 32) {
                    code.rcpps(result, operand);
                } else {
                    code.cvtpd2ps(result, operand);
                    code.rcpps(result, result);
                    code.cvtps2pd(result, result);
                }
            }

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }
    }

    EmitTwoOpFallback(code, ctx, inst, [](VectorArray<FPT>& result, const VectorArray<FPT>& operand, FP::FPCR fpcr, FP::FPSR& fpsr) {
        for (size_t i = 0; i < result.size(); i++) {
            result[i] = FP::FPRecipEstimate<FPT>(operand[i], fpcr, fpsr);
        }
    });
}

void EmitX64::EmitFPVectorRecipEstimate16(EmitContext& ctx, IR::Inst* inst) {
    EmitRecipEstimate<16>(code, ctx, inst);
}

void EmitX64::EmitFPVectorRecipEstimate32(EmitContext& ctx, IR::Inst* inst) {
    EmitRecipEstimate<32>(code, ctx, inst);
}

void EmitX64::EmitFPVectorRecipEstimate64(EmitContext& ctx, IR::Inst* inst) {
    EmitRecipEstimate<64>(code, ctx, inst);
}

template<size_t fsize>
static void EmitRecipStepFused(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    const auto fallback_fn = [](VectorArray<FPT>& result, const VectorArray<FPT>& op1, const VectorArray<FPT>& op2, FP::FPCR fpcr, FP::FPSR& fpsr) {
        for (size_t i = 0; i < result.size(); i++) {
            result[i] = FP::FPRecipStepFused<FPT>(op1[i], op2[i], fpcr, fpsr);
        }
    };

    if constexpr (fsize != 16) {
        if (code.HasHostFeature(HostFeature::FMA | HostFeature::AVX) && ctx.HasOptimization(OptimizationFlag::Unsafe_InaccurateNaN)) {
            auto args = ctx.reg_alloc.GetArgumentInfo(inst);
            const bool fpcr_controlled = args[2].GetImmediateU1();

            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseXmm(args[1]);

            MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
                code.movaps(result, GetVectorOf<fsize, false, 0, 2>(code));
                FCODE(vfnmadd231p)(result, operand1, operand2);
            });

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }

        if (code.HasHostFeature(HostFeature::FMA | HostFeature::AVX)) {
            auto args = ctx.reg_alloc.GetArgumentInfo(inst);
            const bool fpcr_controlled = args[2].GetImmediateU1();

            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseXmm(args[1]);
            const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();

            Xbyak::Label end, fallback;

            MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
                code.movaps(result, GetVectorOf<fsize, false, 0, 2>(code));
                FCODE(vfnmadd231p)(result, operand1, operand2);

                FCODE(vcmpunordp)(tmp, result, result);
                code.vptest(tmp, tmp);
                code.jnz(fallback, code.T_NEAR);
                code.L(end);
            });

            code.SwitchToFarCode();
            code.L(fallback);
            code.sub(rsp, 8);
            ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
            EmitThreeOpFallbackWithoutRegAlloc(code, ctx, result, operand1, operand2, fallback_fn, fpcr_controlled);
            ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
            code.add(rsp, 8);
            code.jmp(end, code.T_NEAR);
            code.SwitchToNearCode();

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }

        if (ctx.HasOptimization(OptimizationFlag::Unsafe_UnfuseFMA)) {
            auto args = ctx.reg_alloc.GetArgumentInfo(inst);

            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseScratchXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseXmm(args[1]);
            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

            code.movaps(result, GetVectorOf<fsize, false, 0, 2>(code));
            FCODE(mulp)(operand1, operand2);
            FCODE(subp)(result, operand1);

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }
    }

    EmitThreeOpFallback(code, ctx, inst, fallback_fn);
}

void EmitX64::EmitFPVectorRecipStepFused16(EmitContext& ctx, IR::Inst* inst) {
    EmitRecipStepFused<16>(code, ctx, inst);
}

void EmitX64::EmitFPVectorRecipStepFused32(EmitContext& ctx, IR::Inst* inst) {
    EmitRecipStepFused<32>(code, ctx, inst);
}

void EmitX64::EmitFPVectorRecipStepFused64(EmitContext& ctx, IR::Inst* inst) {
    EmitRecipStepFused<64>(code, ctx, inst);
}

template<size_t fsize>
void EmitFPVectorRoundInt(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    const auto rounding = static_cast<FP::RoundingMode>(inst->GetArg(1).GetU8());
    const bool exact = inst->GetArg(2).GetU1();

    if constexpr (fsize != 16) {
        if (code.HasHostFeature(HostFeature::SSE41) && rounding != FP::RoundingMode::ToNearest_TieAwayFromZero && !exact) {
            const u8 round_imm = [&]() -> u8 {
                switch (rounding) {
                case FP::RoundingMode::ToNearest_TieEven:
                    return 0b00;
                case FP::RoundingMode::TowardsPlusInfinity:
                    return 0b10;
                case FP::RoundingMode::TowardsMinusInfinity:
                    return 0b01;
                case FP::RoundingMode::TowardsZero:
                    return 0b11;
                default:
                    UNREACHABLE();
                }
            }();

            EmitTwoOpVectorOperation<fsize, DefaultIndexer, 3>(code, ctx, inst, [&](const Xbyak::Xmm& result, const Xbyak::Xmm& xmm_a) {
                FCODE(roundp)(result, xmm_a, round_imm);
            });

            return;
        }
    }

    using rounding_list = mp::list<
        mp::lift_value<FP::RoundingMode::ToNearest_TieEven>,
        mp::lift_value<FP::RoundingMode::TowardsPlusInfinity>,
        mp::lift_value<FP::RoundingMode::TowardsMinusInfinity>,
        mp::lift_value<FP::RoundingMode::TowardsZero>,
        mp::lift_value<FP::RoundingMode::ToNearest_TieAwayFromZero>>;
    using exact_list = mp::list<std::true_type, std::false_type>;

    static const auto lut = Common::GenerateLookupTableFromList(
        []<typename I>(I) {
            using FPT = mcl::unsigned_integer_of_size<fsize>;  // WORKAROUND: For issue 678 on MSVC
            return std::pair{
                mp::lower_to_tuple_v<I>,
                Common::FptrCast(
                    [](VectorArray<FPT>& output, const VectorArray<FPT>& input, FP::FPCR fpcr, FP::FPSR& fpsr) {
                        constexpr FP::RoundingMode rounding_mode = mp::get<0, I>::value;
                        constexpr bool exact = mp::get<1, I>::value;

                        for (size_t i = 0; i < output.size(); ++i) {
                            output[i] = static_cast<FPT>(FP::FPRoundInt<FPT>(input[i], fpcr, rounding_mode, exact, fpsr));
                        }
                    })};
        },
        mp::cartesian_product<rounding_list, exact_list>{});

    EmitTwoOpFallback<3>(code, ctx, inst, lut.at(std::make_tuple(rounding, exact)));
}

void EmitX64::EmitFPVectorRoundInt16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorRoundInt<16>(code, ctx, inst);
}

void EmitX64::EmitFPVectorRoundInt32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorRoundInt<32>(code, ctx, inst);
}

void EmitX64::EmitFPVectorRoundInt64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorRoundInt<64>(code, ctx, inst);
}

template<size_t fsize>
static void EmitRSqrtEstimate(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    const auto fallback_fn = [](VectorArray<FPT>& result, const VectorArray<FPT>& operand, FP::FPCR fpcr, FP::FPSR& fpsr) {
        for (size_t i = 0; i < result.size(); i++) {
            result[i] = FP::FPRSqrtEstimate<FPT>(operand[i], fpcr, fpsr);
        }
    };

    if constexpr (fsize != 16) {
        if (ctx.HasOptimization(OptimizationFlag::Unsafe_ReducedErrorFP)) {
            auto args = ctx.reg_alloc.GetArgumentInfo(inst);
            const Xbyak::Xmm operand = ctx.reg_alloc.UseXmm(args[0]);
            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

            if (code.HasHostFeature(HostFeature::AVX512_OrthoFloat)) {
                FCODE(vrsqrt14p)(result, operand);
            } else {
                if constexpr (fsize == 32) {
                    code.rsqrtps(result, operand);
                } else {
                    code.cvtpd2ps(result, operand);
                    code.rsqrtps(result, result);
                    code.cvtps2pd(result, result);
                }
            }

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }
    }

    EmitTwoOpFallback(code, ctx, inst, fallback_fn);
}

void EmitX64::EmitFPVectorRSqrtEstimate16(EmitContext& ctx, IR::Inst* inst) {
    EmitRSqrtEstimate<16>(code, ctx, inst);
}

void EmitX64::EmitFPVectorRSqrtEstimate32(EmitContext& ctx, IR::Inst* inst) {
    EmitRSqrtEstimate<32>(code, ctx, inst);
}

void EmitX64::EmitFPVectorRSqrtEstimate64(EmitContext& ctx, IR::Inst* inst) {
    EmitRSqrtEstimate<64>(code, ctx, inst);
}

template<size_t fsize>
static void EmitRSqrtStepFused(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    using FPT = mcl::unsigned_integer_of_size<fsize>;

    const auto fallback_fn = [](VectorArray<FPT>& result, const VectorArray<FPT>& op1, const VectorArray<FPT>& op2, FP::FPCR fpcr, FP::FPSR& fpsr) {
        for (size_t i = 0; i < result.size(); i++) {
            result[i] = FP::FPRSqrtStepFused<FPT>(op1[i], op2[i], fpcr, fpsr);
        }
    };

    if constexpr (fsize != 16) {
        if (code.HasHostFeature(HostFeature::FMA | HostFeature::AVX) && ctx.HasOptimization(OptimizationFlag::Unsafe_InaccurateNaN)) {
            auto args = ctx.reg_alloc.GetArgumentInfo(inst);
            const bool fpcr_controlled = args[2].GetImmediateU1();

            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseXmm(args[1]);

            MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
                code.vmovaps(result, GetVectorOf<fsize, false, 0, 3>(code));
                FCODE(vfnmadd231p)(result, operand1, operand2);
                FCODE(vmulp)(result, result, GetVectorOf<fsize, false, -1, 1>(code));
            });

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }

        if (code.HasHostFeature(HostFeature::FMA | HostFeature::AVX)) {
            auto args = ctx.reg_alloc.GetArgumentInfo(inst);
            const bool fpcr_controlled = args[2].GetImmediateU1();

            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseXmm(args[1]);
            const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
            const Xbyak::Xmm mask = ctx.reg_alloc.ScratchXmm();

            Xbyak::Label end, fallback;

            MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
                code.vmovaps(result, GetVectorOf<fsize, false, 0, 3>(code));
                FCODE(vfnmadd231p)(result, operand1, operand2);

                // An explanation for this is given in EmitFPRSqrtStepFused.
                code.vmovaps(mask, GetVectorOf<fsize, (fsize == 32 ? 0x7f000000 : 0x7fe0000000000000)>(code));
                FCODE(vandp)(tmp, result, mask);
                ICODE(vpcmpeq)(tmp, tmp, mask);
                code.ptest(tmp, tmp);
                code.jnz(fallback, code.T_NEAR);

                FCODE(vmulp)(result, result, GetVectorOf<fsize, false, -1, 1>(code));
                code.L(end);
            });

            code.SwitchToFarCode();
            code.L(fallback);
            code.sub(rsp, 8);
            ABI_PushCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
            EmitThreeOpFallbackWithoutRegAlloc(code, ctx, result, operand1, operand2, fallback_fn, fpcr_controlled);
            ABI_PopCallerSaveRegistersAndAdjustStackExcept(code, HostLocXmmIdx(result.getIdx()));
            code.add(rsp, 8);
            code.jmp(end, code.T_NEAR);
            code.SwitchToNearCode();

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }

        if (ctx.HasOptimization(OptimizationFlag::Unsafe_UnfuseFMA)) {
            auto args = ctx.reg_alloc.GetArgumentInfo(inst);

            const Xbyak::Xmm operand1 = ctx.reg_alloc.UseScratchXmm(args[0]);
            const Xbyak::Xmm operand2 = ctx.reg_alloc.UseXmm(args[1]);
            const Xbyak::Xmm result = ctx.reg_alloc.ScratchXmm();

            code.movaps(result, GetVectorOf<fsize, false, 0, 3>(code));
            FCODE(mulp)(operand1, operand2);
            FCODE(subp)(result, operand1);
            FCODE(mulp)(result, GetVectorOf<fsize, false, -1, 1>(code));

            ctx.reg_alloc.DefineValue(inst, result);
            return;
        }
    }

    EmitThreeOpFallback(code, ctx, inst, fallback_fn);
}

void EmitX64::EmitFPVectorRSqrtStepFused16(EmitContext& ctx, IR::Inst* inst) {
    EmitRSqrtStepFused<16>(code, ctx, inst);
}

void EmitX64::EmitFPVectorRSqrtStepFused32(EmitContext& ctx, IR::Inst* inst) {
    EmitRSqrtStepFused<32>(code, ctx, inst);
}

void EmitX64::EmitFPVectorRSqrtStepFused64(EmitContext& ctx, IR::Inst* inst) {
    EmitRSqrtStepFused<64>(code, ctx, inst);
}

void EmitX64::EmitFPVectorSqrt32(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOpVectorOperation<32, DefaultIndexer>(code, ctx, inst, [this](const Xbyak::Xmm& result, const Xbyak::Xmm& operand) {
        code.sqrtps(result, operand);
    });
}

void EmitX64::EmitFPVectorSqrt64(EmitContext& ctx, IR::Inst* inst) {
    EmitTwoOpVectorOperation<64, DefaultIndexer>(code, ctx, inst, [this](const Xbyak::Xmm& result, const Xbyak::Xmm& operand) {
        code.sqrtpd(result, operand);
    });
}

void EmitX64::EmitFPVectorSub32(EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpVectorOperation<32, DefaultIndexer>(code, ctx, inst, &Xbyak::CodeGenerator::subps);
}

void EmitX64::EmitFPVectorSub64(EmitContext& ctx, IR::Inst* inst) {
    EmitThreeOpVectorOperation<64, DefaultIndexer>(code, ctx, inst, &Xbyak::CodeGenerator::subpd);
}

void EmitX64::EmitFPVectorToHalf32(EmitContext& ctx, IR::Inst* inst) {
    const auto rounding_mode = static_cast<FP::RoundingMode>(inst->GetArg(1).GetU8());
    const bool fpcr_controlled = inst->GetArg(2).GetU1();

    if (code.HasHostFeature(HostFeature::F16C) && !ctx.FPCR().AHP() && !ctx.FPCR().FZ16()) {
        auto args = ctx.reg_alloc.GetArgumentInfo(inst);
        const auto round_imm = ConvertRoundingModeToX64Immediate(rounding_mode);

        const Xbyak::Xmm result = ctx.reg_alloc.UseScratchXmm(args[0]);

        ForceToDefaultNaN<32>(code, ctx.FPCR(fpcr_controlled), result);
        code.vcvtps2ph(result, result, static_cast<u8>(*round_imm));

        ctx.reg_alloc.DefineValue(inst, result);
        return;
    }

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
                    [](VectorArray<u16>& output, const VectorArray<u32>& input, FP::FPCR fpcr, FP::FPSR& fpsr) {
                        constexpr FP::RoundingMode rounding_mode = mp::get<0, I>::value;

                        for (size_t i = 0; i < output.size(); ++i) {
                            if (i < input.size()) {
                                output[i] = FP::FPConvert<u16, u32>(input[i], fpcr, rounding_mode, fpsr);
                            } else {
                                output[i] = 0;
                            }
                        }
                    })};
        },
        mp::cartesian_product<rounding_list>{});

    EmitTwoOpFallback<2>(code, ctx, inst, lut.at(std::make_tuple(rounding_mode)));
}

template<size_t fsize, bool unsigned_>
void EmitFPVectorToFixed(BlockOfCode& code, EmitContext& ctx, IR::Inst* inst) {
    const size_t fbits = inst->GetArg(1).GetU8();
    const auto rounding = static_cast<FP::RoundingMode>(inst->GetArg(2).GetU8());
    [[maybe_unused]] const bool fpcr_controlled = inst->GetArg(3).GetU1();

    if constexpr (fsize != 16) {
        if (code.HasHostFeature(HostFeature::SSE41) && rounding != FP::RoundingMode::ToNearest_TieAwayFromZero) {
            auto args = ctx.reg_alloc.GetArgumentInfo(inst);

            const Xbyak::Xmm src = ctx.reg_alloc.UseScratchXmm(args[0]);

            MaybeStandardFPSCRValue(code, ctx, fpcr_controlled, [&] {
                const int round_imm = [&] {
                    switch (rounding) {
                    case FP::RoundingMode::ToNearest_TieEven:
                    default:
                        return 0b00;
                    case FP::RoundingMode::TowardsPlusInfinity:
                        return 0b10;
                    case FP::RoundingMode::TowardsMinusInfinity:
                        return 0b01;
                    case FP::RoundingMode::TowardsZero:
                        return 0b11;
                    }
                }();

                const auto perform_conversion = [&code, &ctx](const Xbyak::Xmm& src) {
                    // MSVC doesn't allow us to use a [&] capture, so we have to do this instead.
                    (void)ctx;

                    if constexpr (fsize == 32) {
                        code.cvttps2dq(src, src);
                    } else {
                        if (code.HasHostFeature(HostFeature::AVX512_OrthoFloat)) {
                            code.vcvttpd2qq(src, src);
                        } else {
                            const Xbyak::Reg64 hi = ctx.reg_alloc.ScratchGpr();
                            const Xbyak::Reg64 lo = ctx.reg_alloc.ScratchGpr();

                            code.cvttsd2si(lo, src);
                            code.punpckhqdq(src, src);
                            code.cvttsd2si(hi, src);
                            code.movq(src, lo);
                            code.pinsrq(src, hi, 1);

                            ctx.reg_alloc.Release(hi);
                            ctx.reg_alloc.Release(lo);
                        }
                    }
                };

                if (fbits != 0) {
                    const u64 scale_factor = fsize == 32
                                               ? static_cast<u64>(fbits + 127) << 23
                                               : static_cast<u64>(fbits + 1023) << 52;
                    FCODE(mulp)(src, GetVectorOf<fsize>(code, scale_factor));
                }

                FCODE(roundp)(src, src, static_cast<u8>(round_imm));
                ZeroIfNaN<fsize>(code, src);

                constexpr u64 float_upper_limit_signed = fsize == 32 ? 0x4f000000 : 0x43e0000000000000;
                [[maybe_unused]] constexpr u64 float_upper_limit_unsigned = fsize == 32 ? 0x4f800000 : 0x43f0000000000000;

                if constexpr (unsigned_) {
                    if (code.HasHostFeature(HostFeature::AVX512_OrthoFloat)) {
                        // Mask positive values
                        code.xorps(xmm0, xmm0);
                        FCODE(vcmpp)(k1, src, xmm0, Cmp::GreaterEqual_OQ);

                        // Convert positive values to unsigned integers, write 0 anywhere else
                        // vcvttp*2u*q already saturates out-of-range values to (0xFFFF...)
                        if constexpr (fsize == 32) {
                            code.vcvttps2udq(src | k1 | T_z, src);
                        } else {
                            code.vcvttpd2uqq(src | k1 | T_z, src);
                        }
                    } else {
                        // Zero is minimum
                        code.xorps(xmm0, xmm0);
                        FCODE(cmplep)(xmm0, src);
                        FCODE(andp)(src, xmm0);

                        // Will we exceed unsigned range?
                        const Xbyak::Xmm exceed_unsigned = ctx.reg_alloc.ScratchXmm();
                        code.movaps(exceed_unsigned, GetVectorOf<fsize, float_upper_limit_unsigned>(code));
                        FCODE(cmplep)(exceed_unsigned, src);

                        // Will be exceed signed range?
                        const Xbyak::Xmm tmp = ctx.reg_alloc.ScratchXmm();
                        code.movaps(tmp, GetVectorOf<fsize, float_upper_limit_signed>(code));
                        code.movaps(xmm0, tmp);
                        FCODE(cmplep)(xmm0, src);
                        FCODE(andp)(tmp, xmm0);
                        FCODE(subp)(src, tmp);
                        perform_conversion(src);
                        ICODE(psll)(xmm0, static_cast<u8>(fsize - 1));
                        FCODE(orp)(src, xmm0);

                        // Saturate to max
                        FCODE(orp)(src, exceed_unsigned);
                    }
                } else {
                    using FPT = mcl::unsigned_integer_of_size<fsize>;  // WORKAROUND: For issue 678 on MSVC
                    constexpr u64 integer_max = static_cast<FPT>(std::numeric_limits<std::conditional_t<unsigned_, FPT, std::make_signed_t<FPT>>>::max());

                    code.movaps(xmm0, GetVectorOf<fsize, float_upper_limit_signed>(code));
                    FCODE(cmplep)(xmm0, src);
                    perform_conversion(src);
                    FCODE(blendvp)(src, GetVectorOf<fsize, integer_max>(code));
                }
            });

            ctx.reg_alloc.DefineValue(inst, src);
            return;
        }
    }

    using fbits_list = mp::lift_sequence<std::make_index_sequence<fsize + 1>>;
    using rounding_list = mp::list<
        mp::lift_value<FP::RoundingMode::ToNearest_TieEven>,
        mp::lift_value<FP::RoundingMode::TowardsPlusInfinity>,
        mp::lift_value<FP::RoundingMode::TowardsMinusInfinity>,
        mp::lift_value<FP::RoundingMode::TowardsZero>,
        mp::lift_value<FP::RoundingMode::ToNearest_TieAwayFromZero>>;

    static const auto lut = Common::GenerateLookupTableFromList(
        []<typename I>(I) {
            using FPT = mcl::unsigned_integer_of_size<fsize>;  // WORKAROUND: For issue 678 on MSVC
            return std::pair{
                mp::lower_to_tuple_v<I>,
                Common::FptrCast(
                    [](VectorArray<FPT>& output, const VectorArray<FPT>& input, FP::FPCR fpcr, FP::FPSR& fpsr) {
                        constexpr size_t fbits = mp::get<0, I>::value;
                        constexpr FP::RoundingMode rounding_mode = mp::get<1, I>::value;

                        for (size_t i = 0; i < output.size(); ++i) {
                            output[i] = static_cast<FPT>(FP::FPToFixed<FPT>(fsize, input[i], fbits, unsigned_, fpcr, rounding_mode, fpsr));
                        }
                    })};
        },
        mp::cartesian_product<fbits_list, rounding_list>{});

    EmitTwoOpFallback<3>(code, ctx, inst, lut.at(std::make_tuple(fbits, rounding)));
}

void EmitX64::EmitFPVectorToSignedFixed16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorToFixed<16, false>(code, ctx, inst);
}

void EmitX64::EmitFPVectorToSignedFixed32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorToFixed<32, false>(code, ctx, inst);
}

void EmitX64::EmitFPVectorToSignedFixed64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorToFixed<64, false>(code, ctx, inst);
}

void EmitX64::EmitFPVectorToUnsignedFixed16(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorToFixed<16, true>(code, ctx, inst);
}

void EmitX64::EmitFPVectorToUnsignedFixed32(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorToFixed<32, true>(code, ctx, inst);
}

void EmitX64::EmitFPVectorToUnsignedFixed64(EmitContext& ctx, IR::Inst* inst) {
    EmitFPVectorToFixed<64, true>(code, ctx, inst);
}

}  // namespace Dynarmic::Backend::X64

/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <utility>

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
std::pair<size_t, Vec> Combine(Imm<2> size, Imm<1> H, Imm<1> L, Imm<1> M, Imm<4> Vmlo) {
    if (size == 0b01) {
        return {concatenate(H, L, M).ZeroExtend(), Vmlo.ZeroExtend<Vec>()};
    }

    return {concatenate(H, L).ZeroExtend(), concatenate(M, Vmlo).ZeroExtend<Vec>()};
}

enum class ExtraBehavior {
    None,
    Accumulate,
    Subtract,
    MultiplyExtended,
};

bool MultiplyByElement(TranslatorVisitor& v, bool sz, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd, ExtraBehavior extra_behavior) {
    if (sz && L == 1) {
        return v.ReservedValue();
    }

    const size_t idxdsize = H == 1 ? 128 : 64;
    const size_t index = sz ? H.ZeroExtend() : concatenate(H, L).ZeroExtend();
    const Vec Vm = concatenate(M, Vmlo).ZeroExtend<Vec>();
    const size_t esize = sz ? 64 : 32;

    const IR::U32U64 element = v.ir.VectorGetElement(esize, v.V(idxdsize, Vm), index);
    const IR::U32U64 result = [&]() -> IR::U32U64 {
        IR::U32U64 operand1 = v.V_scalar(esize, Vn);

        if (extra_behavior == ExtraBehavior::None) {
            return v.ir.FPMul(operand1, element);
        }

        if (extra_behavior == ExtraBehavior::MultiplyExtended) {
            return v.ir.FPMulX(operand1, element);
        }

        if (extra_behavior == ExtraBehavior::Subtract) {
            operand1 = v.ir.FPNeg(operand1);
        }

        const IR::U32U64 operand2 = v.V_scalar(esize, Vd);
        return v.ir.FPMulAdd(operand2, operand1, element);
    }();

    v.V_scalar(esize, Vd, result);
    return true;
}

bool MultiplyByElementHalfPrecision(TranslatorVisitor& v, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd, ExtraBehavior extra_behavior) {
    const size_t esize = 16;
    const size_t idxsize = H == 1 ? 128 : 64;
    const size_t index = concatenate(H, L, M).ZeroExtend();

    const auto Vm = Vmlo.ZeroExtend<Vec>();
    const IR::U16 element = v.ir.VectorGetElement(esize, v.V(idxsize, Vm), index);
    const IR::U16 result = [&]() -> IR::U16 {
        IR::U16 operand1 = v.V_scalar(esize, Vn);

        // TODO: Currently we don't implement half-precision paths
        //       for regular multiplication and extended multiplication.

        if (extra_behavior == ExtraBehavior::None) {
            ASSERT_FALSE("half-precision option unimplemented");
        }

        if (extra_behavior == ExtraBehavior::MultiplyExtended) {
            ASSERT_FALSE("half-precision option unimplemented");
        }

        if (extra_behavior == ExtraBehavior::Subtract) {
            operand1 = v.ir.FPNeg(operand1);
        }

        const IR::U16 operand2 = v.V_scalar(esize, Vd);
        return v.ir.FPMulAdd(operand2, operand1, element);
    }();

    v.V_scalar(esize, Vd, result);
    return true;
}
}  // Anonymous namespace

bool TranslatorVisitor::FMLA_elt_1(Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyByElementHalfPrecision(*this, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Accumulate);
}

bool TranslatorVisitor::FMLA_elt_2(bool sz, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyByElement(*this, sz, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Accumulate);
}

bool TranslatorVisitor::FMLS_elt_1(Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyByElementHalfPrecision(*this, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Subtract);
}

bool TranslatorVisitor::FMLS_elt_2(bool sz, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyByElement(*this, sz, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Subtract);
}

bool TranslatorVisitor::FMUL_elt_2(bool sz, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyByElement(*this, sz, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::None);
}

bool TranslatorVisitor::FMULX_elt_2(bool sz, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyByElement(*this, sz, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::MultiplyExtended);
}

bool TranslatorVisitor::SQDMULH_elt_1(Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    if (size == 0b00 || size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const auto [index, Vm] = Combine(size, H, L, M, Vmlo);

    const IR::UAny operand1 = V_scalar(esize, Vn);
    const IR::UAny operand2 = ir.VectorGetElement(esize, V(128, Vm), index);
    const auto result = ir.SignedSaturatedDoublingMultiplyReturnHigh(operand1, operand2);

    ir.OrQC(result.overflow);

    V_scalar(esize, Vd, result.result);
    return true;
}

bool TranslatorVisitor::SQRDMULH_elt_1(Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    if (size == 0b00 || size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const auto [index, Vm] = Combine(size, H, L, M, Vmlo);

    const IR::U128 operand1 = ir.ZeroExtendToQuad(ir.VectorGetElement(esize, V(128, Vn), 0));
    const IR::U128 operand2 = V(128, Vm);
    const IR::U128 broadcast = ir.VectorBroadcastElement(esize, operand2, index);
    const IR::UpperAndLower multiply = ir.VectorSignedSaturatedDoublingMultiply(esize, operand1, broadcast);
    const IR::U128 result = ir.VectorAdd(esize, multiply.upper, ir.VectorLogicalShiftRight(esize, multiply.lower, static_cast<u8>(esize - 1)));

    V(128, Vd, result);
    return true;
}

bool TranslatorVisitor::SQDMULL_elt_1(Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    if (size == 0b00 || size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const auto [index, Vm] = Combine(size, H, L, M, Vmlo);

    const IR::U128 operand1 = ir.ZeroExtendToQuad(ir.VectorGetElement(esize, V(128, Vn), 0));
    const IR::U128 operand2 = V(128, Vm);
    const IR::U128 broadcast = ir.VectorBroadcastElement(esize, operand2, index);
    const IR::U128 result = ir.VectorSignedSaturatedDoublingMultiplyLong(esize, operand1, broadcast);

    V(128, Vd, result);
    return true;
}

}  // namespace Dynarmic::A64

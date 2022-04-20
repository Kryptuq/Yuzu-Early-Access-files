/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <utility>

#include <mcl/assert.hpp>
#include <mcl/bit/bit_field.hpp>

#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"

namespace Dynarmic::A32 {
namespace {
std::pair<ExtReg, size_t> GetScalarLocation(size_t esize, bool M, size_t Vm) {
    const ExtReg m = ExtReg::Q0 + ((Vm >> 1) & (esize == 16 ? 0b11 : 0b111));
    const size_t index = concatenate(Imm<1>{mcl::bit::get_bit<0>(Vm)}, Imm<1>{M}, Imm<1>{mcl::bit::get_bit<3>(Vm)}).ZeroExtend() >> (esize == 16 ? 0 : 1);
    return std::make_pair(m, index);
}

enum class MultiplyBehavior {
    Multiply,
    MultiplyAccumulate,
    MultiplySubtract,
};

enum class Rounding {
    None,
    Round,
};

bool ScalarMultiply(TranslatorVisitor& v, bool Q, bool D, size_t sz, size_t Vn, size_t Vd, bool F, bool N, bool M, size_t Vm, MultiplyBehavior multiply) {
    if (sz == 0b11) {
        return v.DecodeError();
    }

    if (sz == 0b00 || (F && sz == 0b01)) {
        return v.UndefinedInstruction();
    }

    if (Q && (mcl::bit::get_bit<0>(Vd) || mcl::bit::get_bit<0>(Vn))) {
        return v.UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto n = ToVector(Q, Vn, N);
    const auto [m, index] = GetScalarLocation(esize, M, Vm);

    const auto reg_n = v.ir.GetVector(n);
    const auto reg_m = v.ir.VectorBroadcastElement(esize, v.ir.GetVector(m), index);
    const auto addend = F ? v.ir.FPVectorMul(esize, reg_n, reg_m, false)
                          : v.ir.VectorMultiply(esize, reg_n, reg_m);
    const auto result = [&] {
        switch (multiply) {
        case MultiplyBehavior::Multiply:
            return addend;
        case MultiplyBehavior::MultiplyAccumulate:
            return F ? v.ir.FPVectorAdd(esize, v.ir.GetVector(d), addend, false)
                     : v.ir.VectorAdd(esize, v.ir.GetVector(d), addend);
        case MultiplyBehavior::MultiplySubtract:
            return F ? v.ir.FPVectorSub(esize, v.ir.GetVector(d), addend, false)
                     : v.ir.VectorSub(esize, v.ir.GetVector(d), addend);
        default:
            return IR::U128{};
        }
    }();

    v.ir.SetVector(d, result);
    return true;
}

bool ScalarMultiplyLong(TranslatorVisitor& v, bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm, MultiplyBehavior multiply) {
    if (sz == 0b11) {
        return v.DecodeError();
    }

    if (sz == 0b00 || mcl::bit::get_bit<0>(Vd)) {
        return v.UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(true, Vd, D);
    const auto n = ToVector(false, Vn, N);
    const auto [m, index] = GetScalarLocation(esize, M, Vm);

    const auto scalar = v.ir.VectorGetElement(esize, v.ir.GetVector(m), index);
    const auto ext_scalar = U ? (esize == 16 ? IR::U32U64{v.ir.ZeroExtendToWord(scalar)} : IR::U32U64{v.ir.ZeroExtendToLong(scalar)})
                              : (esize == 16 ? IR::U32U64{v.ir.SignExtendToWord(scalar)} : IR::U32U64{v.ir.SignExtendToLong(scalar)});
    const auto reg_n = U ? v.ir.VectorZeroExtend(esize, v.ir.GetVector(n)) : v.ir.VectorSignExtend(esize, v.ir.GetVector(n));
    const auto reg_m = v.ir.VectorBroadcast(esize * 2, ext_scalar);
    const auto addend = v.ir.VectorMultiply(esize * 2, reg_n, reg_m);
    const auto result = [&] {
        switch (multiply) {
        case MultiplyBehavior::Multiply:
            return addend;
        case MultiplyBehavior::MultiplyAccumulate:
            return v.ir.VectorAdd(esize * 2, v.ir.GetVector(d), addend);
        case MultiplyBehavior::MultiplySubtract:
            return v.ir.VectorSub(esize * 2, v.ir.GetVector(d), addend);
        default:
            return IR::U128{};
        }
    }();

    v.ir.SetVector(d, result);
    return true;
}

bool ScalarMultiplyReturnHigh(TranslatorVisitor& v, bool Q, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm, Rounding round) {
    if (sz == 0b11) {
        return v.DecodeError();
    }

    if (sz == 0b00) {
        return v.UndefinedInstruction();
    }

    if (Q && (mcl::bit::get_bit<0>(Vd) || mcl::bit::get_bit<0>(Vn))) {
        return v.UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto n = ToVector(Q, Vn, N);
    const auto [m, index] = GetScalarLocation(esize, M, Vm);

    const auto reg_n = v.ir.GetVector(n);
    const auto reg_m = v.ir.VectorBroadcastElement(esize, v.ir.GetVector(m), index);
    const auto result = [&] {
        const auto tmp = v.ir.VectorSignedSaturatedDoublingMultiply(esize, reg_n, reg_m);

        if (round == Rounding::Round) {
            return v.ir.VectorAdd(esize, tmp.upper, v.ir.VectorLogicalShiftRight(esize, tmp.lower, static_cast<u8>(esize - 1)));
        }

        return tmp.upper;
    }();

    v.ir.SetVector(d, result);
    return true;
}
}  // Anonymous namespace

bool TranslatorVisitor::asimd_VMLA_scalar(bool Q, bool D, size_t sz, size_t Vn, size_t Vd, bool op, bool F, bool N, bool M, size_t Vm) {
    const auto behavior = op ? MultiplyBehavior::MultiplySubtract
                             : MultiplyBehavior::MultiplyAccumulate;
    return ScalarMultiply(*this, Q, D, sz, Vn, Vd, F, N, M, Vm, behavior);
}

bool TranslatorVisitor::asimd_VMLAL_scalar(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool op, bool N, bool M, size_t Vm) {
    const auto behavior = op ? MultiplyBehavior::MultiplySubtract
                             : MultiplyBehavior::MultiplyAccumulate;
    return ScalarMultiplyLong(*this, U, D, sz, Vn, Vd, N, M, Vm, behavior);
}

bool TranslatorVisitor::asimd_VMUL_scalar(bool Q, bool D, size_t sz, size_t Vn, size_t Vd, bool F, bool N, bool M, size_t Vm) {
    return ScalarMultiply(*this, Q, D, sz, Vn, Vd, F, N, M, Vm, MultiplyBehavior::Multiply);
}

bool TranslatorVisitor::asimd_VMULL_scalar(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm) {
    return ScalarMultiplyLong(*this, U, D, sz, Vn, Vd, N, M, Vm, MultiplyBehavior::Multiply);
}

bool TranslatorVisitor::asimd_VQDMULL_scalar(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm) {
    if (sz == 0b11) {
        return DecodeError();
    }

    if (sz == 0b00 || mcl::bit::get_bit<0>(Vd)) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(true, Vd, D);
    const auto n = ToVector(false, Vn, N);
    const auto [m, index] = GetScalarLocation(esize, M, Vm);

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.VectorBroadcastElement(esize, ir.GetVector(m), index);
    const auto result = ir.VectorSignedSaturatedDoublingMultiplyLong(esize, reg_n, reg_m);

    ir.SetVector(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VQDMULH_scalar(bool Q, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm) {
    return ScalarMultiplyReturnHigh(*this, Q, D, sz, Vn, Vd, N, M, Vm, Rounding::None);
}

bool TranslatorVisitor::asimd_VQRDMULH_scalar(bool Q, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm) {
    return ScalarMultiplyReturnHigh(*this, Q, D, sz, Vn, Vd, N, M, Vm, Rounding::Round);
}

}  // namespace Dynarmic::A32

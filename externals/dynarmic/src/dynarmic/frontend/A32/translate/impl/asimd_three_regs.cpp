/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/bit_util.h"
#include "dynarmic/frontend/A32/translate/impl/translate.h"

namespace Dynarmic::A32 {
namespace {
enum class Comparison {
    GE,
    GT,
    EQ,
    AbsoluteGE,
    AbsoluteGT,
};

enum class AccumulateBehavior {
    None,
    Accumulate,
};

enum class WidenBehaviour {
    Second,
    Both,
};

template<bool WithDst, typename Callable>
bool BitwiseInstruction(TranslatorVisitor& v, bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm, Callable fn) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return v.UndefinedInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    if constexpr (WithDst) {
        const IR::U128 reg_d = v.ir.GetVector(d);
        const IR::U128 reg_m = v.ir.GetVector(m);
        const IR::U128 reg_n = v.ir.GetVector(n);
        const IR::U128 result = fn(reg_d, reg_n, reg_m);
        v.ir.SetVector(d, result);
    } else {
        const IR::U128 reg_m = v.ir.GetVector(m);
        const IR::U128 reg_n = v.ir.GetVector(n);
        const IR::U128 result = fn(reg_n, reg_m);
        v.ir.SetVector(d, result);
    }

    return true;
}

template<typename Callable>
bool FloatingPointInstruction(TranslatorVisitor& v, bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm, Callable fn) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return v.UndefinedInstruction();
    }

    if (sz == 0b1) {
        return v.UndefinedInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_d = v.ir.GetVector(d);
    const auto reg_n = v.ir.GetVector(n);
    const auto reg_m = v.ir.GetVector(m);
    const auto result = fn(reg_d, reg_n, reg_m);

    v.ir.SetVector(d, result);
    return true;
}

bool IntegerComparison(TranslatorVisitor& v, bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm, Comparison comparison) {
    if (sz == 0b11) {
        return v.UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return v.UndefinedInstruction();
    }

    const size_t esize = 8 << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = v.ir.GetVector(n);
    const auto reg_m = v.ir.GetVector(m);
    const auto result = [&] {
        switch (comparison) {
        case Comparison::GT:
            return U ? v.ir.VectorGreaterUnsigned(esize, reg_n, reg_m)
                     : v.ir.VectorGreaterSigned(esize, reg_n, reg_m);
        case Comparison::GE:
            return U ? v.ir.VectorGreaterEqualUnsigned(esize, reg_n, reg_m)
                     : v.ir.VectorGreaterEqualSigned(esize, reg_n, reg_m);
        case Comparison::EQ:
            return v.ir.VectorEqual(esize, reg_n, reg_m);
        default:
            return IR::U128{};
        }
    }();

    v.ir.SetVector(d, result);
    return true;
}

bool FloatComparison(TranslatorVisitor& v, bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm, Comparison comparison) {
    if (sz) {
        return v.UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return v.UndefinedInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = v.ir.GetVector(n);
    const auto reg_m = v.ir.GetVector(m);
    const auto result = [&] {
        switch (comparison) {
        case Comparison::GE:
            return v.ir.FPVectorGreaterEqual(32, reg_n, reg_m, false);
        case Comparison::GT:
            return v.ir.FPVectorGreater(32, reg_n, reg_m, false);
        case Comparison::EQ:
            return v.ir.FPVectorEqual(32, reg_n, reg_m, false);
        case Comparison::AbsoluteGE:
            return v.ir.FPVectorGreaterEqual(32, v.ir.FPVectorAbs(32, reg_n), v.ir.FPVectorAbs(32, reg_m), false);
        case Comparison::AbsoluteGT:
            return v.ir.FPVectorGreater(32, v.ir.FPVectorAbs(32, reg_n), v.ir.FPVectorAbs(32, reg_m), false);
        default:
            return IR::U128{};
        }
    }();

    v.ir.SetVector(d, result);
    return true;
}

bool AbsoluteDifference(TranslatorVisitor& v, bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm, AccumulateBehavior accumulate) {
    if (sz == 0b11) {
        return v.UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return v.UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_m = v.ir.GetVector(m);
    const auto reg_n = v.ir.GetVector(n);
    const auto result = [&] {
        const auto absdiff = U ? v.ir.VectorUnsignedAbsoluteDifference(esize, reg_n, reg_m)
                               : v.ir.VectorSignedAbsoluteDifference(esize, reg_n, reg_m);

        if (accumulate == AccumulateBehavior::Accumulate) {
            const auto reg_d = v.ir.GetVector(d);
            return v.ir.VectorAdd(esize, reg_d, absdiff);
        }

        return absdiff;
    }();

    v.ir.SetVector(d, result);
    return true;
}

bool AbsoluteDifferenceLong(TranslatorVisitor& v, bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm, AccumulateBehavior accumulate) {
    if (sz == 0b11) {
        return v.DecodeError();
    }

    if (Common::Bit<0>(Vd)) {
        return v.UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(true, Vd, D);
    const auto m = ToVector(false, Vm, M);
    const auto n = ToVector(false, Vn, N);

    const auto reg_m = v.ir.GetVector(m);
    const auto reg_n = v.ir.GetVector(n);
    const auto operand_m = v.ir.VectorZeroExtend(esize, v.ir.ZeroExtendToQuad(v.ir.VectorGetElement(64, reg_m, 0)));
    const auto operand_n = v.ir.VectorZeroExtend(esize, v.ir.ZeroExtendToQuad(v.ir.VectorGetElement(64, reg_n, 0)));
    const auto result = [&] {
        const auto absdiff = U ? v.ir.VectorUnsignedAbsoluteDifference(esize, operand_m, operand_n)
                               : v.ir.VectorSignedAbsoluteDifference(esize, operand_m, operand_n);

        if (accumulate == AccumulateBehavior::Accumulate) {
            const auto reg_d = v.ir.GetVector(d);
            return v.ir.VectorAdd(2 * esize, reg_d, absdiff);
        }

        return absdiff;
    }();

    v.ir.SetVector(d, result);
    return true;
}

template<typename Callable>
bool WideInstruction(TranslatorVisitor& v, bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm, WidenBehaviour widen_behaviour, Callable fn) {
    const size_t esize = 8U << sz;
    const bool widen_first = widen_behaviour == WidenBehaviour::Both;

    if (sz == 0b11) {
        return v.DecodeError();
    }

    if (Common::Bit<0>(Vd) || (!widen_first && Common::Bit<0>(Vn))) {
        return v.UndefinedInstruction();
    }

    const auto d = ToVector(true, Vd, D);
    const auto m = ToVector(false, Vm, M);
    const auto n = ToVector(!widen_first, Vn, N);

    const auto reg_d = v.ir.GetVector(d);
    const auto reg_m = v.ir.GetVector(m);
    const auto reg_n = v.ir.GetVector(n);
    const auto wide_n = U ? v.ir.VectorZeroExtend(esize, reg_n) : v.ir.VectorSignExtend(esize, reg_n);
    const auto wide_m = U ? v.ir.VectorZeroExtend(esize, reg_m) : v.ir.VectorSignExtend(esize, reg_m);
    const auto result = fn(esize * 2, reg_d, widen_first ? wide_n : reg_n, wide_m);

    v.ir.SetVector(d, result);
    return true;
}

}  // Anonymous namespace

// ASIMD Three registers of the same length

bool TranslatorVisitor::asimd_VHADD(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8 << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const IR::U128 reg_n = ir.GetVector(n);
    const IR::U128 reg_m = ir.GetVector(m);
    const IR::U128 result = U ? ir.VectorHalvingAddUnsigned(esize, reg_n, reg_m) : ir.VectorHalvingAddSigned(esize, reg_n, reg_m);
    ir.SetVector(d, result);

    return true;
}

bool TranslatorVisitor::asimd_VQADD(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8 << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const IR::U128 reg_n = ir.GetVector(n);
    const IR::U128 reg_m = ir.GetVector(m);
    const IR::U128 result = U ? ir.VectorUnsignedSaturatedAdd(esize, reg_n, reg_m) : ir.VectorSignedSaturatedAdd(esize, reg_n, reg_m);
    ir.SetVector(d, result);

    return true;
}

bool TranslatorVisitor::asimd_VRHADD(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8 << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const IR::U128 reg_n = ir.GetVector(n);
    const IR::U128 reg_m = ir.GetVector(m);
    const IR::U128 result = U ? ir.VectorRoundingHalvingAddUnsigned(esize, reg_n, reg_m) : ir.VectorRoundingHalvingAddSigned(esize, reg_n, reg_m);
    ir.SetVector(d, result);

    return true;
}

bool TranslatorVisitor::asimd_VAND_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<false>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.VectorAnd(reg_n, reg_m);
    });
}

bool TranslatorVisitor::asimd_VBIC_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<false>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.VectorAnd(reg_n, ir.VectorNot(reg_m));
    });
}

bool TranslatorVisitor::asimd_VORR_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<false>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.VectorOr(reg_n, reg_m);
    });
}

bool TranslatorVisitor::asimd_VORN_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<false>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.VectorOr(reg_n, ir.VectorNot(reg_m));
    });
}

bool TranslatorVisitor::asimd_VEOR_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<false>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.VectorEor(reg_n, reg_m);
    });
}

bool TranslatorVisitor::asimd_VBSL(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<true>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        return ir.VectorOr(ir.VectorAnd(reg_n, reg_d), ir.VectorAnd(reg_m, ir.VectorNot(reg_d)));
    });
}

bool TranslatorVisitor::asimd_VBIT(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<true>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        return ir.VectorOr(ir.VectorAnd(reg_n, reg_m), ir.VectorAnd(reg_d, ir.VectorNot(reg_m)));
    });
}

bool TranslatorVisitor::asimd_VBIF(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<true>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        return ir.VectorOr(ir.VectorAnd(reg_d, reg_m), ir.VectorAnd(reg_n, ir.VectorNot(reg_m)));
    });
}

bool TranslatorVisitor::asimd_VHSUB(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8 << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const IR::U128 reg_n = ir.GetVector(n);
    const IR::U128 reg_m = ir.GetVector(m);
    const IR::U128 result = U ? ir.VectorHalvingSubUnsigned(esize, reg_n, reg_m) : ir.VectorHalvingSubSigned(esize, reg_n, reg_m);
    ir.SetVector(d, result);

    return true;
}

bool TranslatorVisitor::asimd_VQSUB(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8 << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const IR::U128 reg_n = ir.GetVector(n);
    const IR::U128 reg_m = ir.GetVector(m);
    const IR::U128 result = U ? ir.VectorUnsignedSaturatedSub(esize, reg_n, reg_m) : ir.VectorSignedSaturatedSub(esize, reg_n, reg_m);
    ir.SetVector(d, result);

    return true;
}

bool TranslatorVisitor::asimd_VCGT_reg(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return IntegerComparison(*this, U, D, sz, Vn, Vd, N, Q, M, Vm, Comparison::GT);
}

bool TranslatorVisitor::asimd_VCGE_reg(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return IntegerComparison(*this, U, D, sz, Vn, Vd, N, Q, M, Vm, Comparison::GE);
}

bool TranslatorVisitor::asimd_VABD(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return AbsoluteDifference(*this, U, D, sz, Vn, Vd, N, Q, M, Vm, AccumulateBehavior::None);
}

bool TranslatorVisitor::asimd_VABA(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return AbsoluteDifference(*this, U, D, sz, Vn, Vd, N, Q, M, Vm, AccumulateBehavior::Accumulate);
}

bool TranslatorVisitor::asimd_VADD_int(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_m = ir.GetVector(m);
    const auto reg_n = ir.GetVector(n);
    const auto result = ir.VectorAdd(esize, reg_n, reg_m);

    ir.SetVector(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VSUB_int(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_m = ir.GetVector(m);
    const auto reg_n = ir.GetVector(n);
    const auto result = ir.VectorSub(esize, reg_n, reg_m);

    ir.SetVector(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VSHL_reg(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_m = ir.GetVector(m);
    const auto reg_n = ir.GetVector(n);
    const auto result = U ? ir.VectorLogicalVShift(esize, reg_m, reg_n)
                          : ir.VectorArithmeticVShift(esize, reg_m, reg_n);

    ir.SetVector(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VQSHL_reg(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_m = ir.GetVector(m);
    const auto reg_n = ir.GetVector(n);
    const auto result = U ? ir.VectorUnsignedSaturatedShiftLeft(esize, reg_m, reg_n)
                          : ir.VectorSignedSaturatedShiftLeft(esize, reg_m, reg_n);

    ir.SetVector(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VRSHL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_m = ir.GetVector(m);
    const auto reg_n = ir.GetVector(n);
    const auto result = U ? ir.VectorRoundingShiftLeftUnsigned(esize, reg_m, reg_n)
                          : ir.VectorRoundingShiftLeftSigned(esize, reg_m, reg_n);

    ir.SetVector(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VMAX(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, bool op, size_t Vm) {
    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_m = ir.GetVector(m);
    const auto reg_n = ir.GetVector(n);
    const auto result = [&] {
        if (op) {
            return U ? ir.VectorMinUnsigned(esize, reg_n, reg_m)
                     : ir.VectorMinSigned(esize, reg_n, reg_m);
        } else {
            return U ? ir.VectorMaxUnsigned(esize, reg_n, reg_m)
                     : ir.VectorMaxSigned(esize, reg_n, reg_m);
        }
    }();

    ir.SetVector(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VTST(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8 << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.GetVector(m);
    const auto anded = ir.VectorAnd(reg_n, reg_m);
    const auto result = ir.VectorNot(ir.VectorEqual(esize, anded, ir.ZeroVector()));

    ir.SetVector(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VCEQ_reg(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return IntegerComparison(*this, false, D, sz, Vn, Vd, N, Q, M, Vm, Comparison::EQ);
}

bool TranslatorVisitor::asimd_VMLA(bool op, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.GetVector(m);
    const auto reg_d = ir.GetVector(d);
    const auto multiply = ir.VectorMultiply(esize, reg_n, reg_m);
    const auto result = op ? ir.VectorSub(esize, reg_d, multiply)
                           : ir.VectorAdd(esize, reg_d, multiply);

    ir.SetVector(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VMUL(bool P, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (sz == 0b11 || (P && sz != 0b00)) {
        return UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.GetVector(m);
    const auto result = P ? ir.VectorPolynomialMultiply(reg_n, reg_m)
                          : ir.VectorMultiply(esize, reg_n, reg_m);

    ir.SetVector(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VPMAX_int(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, bool op, size_t Vm) {
    if (sz == 0b11 || Q) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(false, Vd, D);
    const auto m = ToVector(false, Vm, M);
    const auto n = ToVector(false, Vn, N);

    const auto reg_m = ir.GetVector(m);
    const auto reg_n = ir.GetVector(n);

    const auto bottom = ir.VectorDeinterleaveEvenLower(esize, reg_n, reg_m);
    const auto top = ir.VectorDeinterleaveOddLower(esize, reg_n, reg_m);

    const auto result = [&] {
        if (op) {
            return U ? ir.VectorMinUnsigned(esize, bottom, top)
                     : ir.VectorMinSigned(esize, bottom, top);
        } else {
            return U ? ir.VectorMaxUnsigned(esize, bottom, top)
                     : ir.VectorMaxSigned(esize, bottom, top);
        }
    }();

    ir.SetVector(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VQDMULH(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (sz == 0b00 || sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.GetVector(m);
    const auto result = ir.VectorSignedSaturatedDoublingMultiply(esize, reg_n, reg_m);

    ir.SetVector(d, result.upper);
    return true;
}

bool TranslatorVisitor::asimd_VQRDMULH(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (sz == 0b00 || sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.GetVector(m);
    const auto multiply = ir.VectorSignedSaturatedDoublingMultiply(esize, reg_n, reg_m);
    const auto result = ir.VectorAdd(esize, multiply.upper, ir.VectorLogicalShiftRight(esize, multiply.lower, static_cast<u8>(esize - 1)));

    ir.SetVector(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VPADD(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q || sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.GetVector(m);
    const auto result = ir.VectorPairedAddLower(esize, reg_n, reg_m);

    ir.SetVector(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VFMA(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorMulAdd(32, reg_d, reg_n, reg_m, false);
    });
}

bool TranslatorVisitor::asimd_VFMS(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorMulAdd(32, reg_d, ir.FPVectorNeg(32, reg_n), reg_m, false);
    });
}

bool TranslatorVisitor::asimd_VADD_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorAdd(32, reg_n, reg_m, false);
    });
}

bool TranslatorVisitor::asimd_VSUB_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorSub(32, reg_n, reg_m, false);
    });
}

bool TranslatorVisitor::asimd_VPADD_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q) {
        return UndefinedInstruction();
    }
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorPairedAddLower(32, reg_n, reg_m, false);
    });
}

bool TranslatorVisitor::asimd_VABD_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorAbs(32, ir.FPVectorSub(32, reg_n, reg_m, false));
    });
}

bool TranslatorVisitor::asimd_VMLA_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        const auto product = ir.FPVectorMul(32, reg_n, reg_m, false);
        return ir.FPVectorAdd(32, reg_d, product, false);
    });
}

bool TranslatorVisitor::asimd_VMLS_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        const auto product = ir.FPVectorMul(32, reg_n, reg_m, false);
        return ir.FPVectorAdd(32, reg_d, ir.FPVectorNeg(32, product), false);
    });
}

bool TranslatorVisitor::asimd_VMUL_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorMul(32, reg_n, reg_m, false);
    });
}

bool TranslatorVisitor::asimd_VCEQ_reg_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatComparison(*this, D, sz, Vn, Vd, N, Q, M, Vm, Comparison::EQ);
}

bool TranslatorVisitor::asimd_VCGE_reg_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatComparison(*this, D, sz, Vn, Vd, N, Q, M, Vm, Comparison::GE);
}

bool TranslatorVisitor::asimd_VCGT_reg_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatComparison(*this, D, sz, Vn, Vd, N, Q, M, Vm, Comparison::GT);
}

bool TranslatorVisitor::asimd_VACGE(bool D, bool op, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    const auto comparison = op ? Comparison::AbsoluteGT : Comparison::AbsoluteGE;
    return FloatComparison(*this, D, sz, Vn, Vd, N, Q, M, Vm, comparison);
}

bool TranslatorVisitor::asimd_VMAX_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorMax(32, reg_n, reg_m, false);
    });
}

bool TranslatorVisitor::asimd_VMIN_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorMin(32, reg_n, reg_m, false);
    });
}

bool TranslatorVisitor::asimd_VPMAX_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q) {
        return UndefinedInstruction();
    }
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        const auto bottom = ir.VectorDeinterleaveEvenLower(32, reg_n, reg_m);
        const auto top = ir.VectorDeinterleaveOddLower(32, reg_n, reg_m);
        return ir.FPVectorMax(32, bottom, top, false);
    });
}

bool TranslatorVisitor::asimd_VPMIN_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q) {
        return UndefinedInstruction();
    }
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        const auto bottom = ir.VectorDeinterleaveEvenLower(32, reg_n, reg_m);
        const auto top = ir.VectorDeinterleaveOddLower(32, reg_n, reg_m);
        return ir.FPVectorMin(32, bottom, top, false);
    });
}

bool TranslatorVisitor::asimd_VRECPS(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorRecipStepFused(32, reg_n, reg_m, false);
    });
}

bool TranslatorVisitor::asimd_VRSQRTS(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorRSqrtStepFused(32, reg_n, reg_m, false);
    });
}

// ASIMD Three registers of different length

bool TranslatorVisitor::asimd_VADDL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool op, bool N, bool M, size_t Vm) {
    return WideInstruction(*this, U, D, sz, Vn, Vd, N, M, Vm, op ? WidenBehaviour::Second : WidenBehaviour::Both, [this](size_t esize, const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.VectorAdd(esize, reg_n, reg_m);
    });
}

bool TranslatorVisitor::asimd_VSUBL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool op, bool N, bool M, size_t Vm) {
    return WideInstruction(*this, U, D, sz, Vn, Vd, N, M, Vm, op ? WidenBehaviour::Second : WidenBehaviour::Both, [this](size_t esize, const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.VectorSub(esize, reg_n, reg_m);
    });
}

bool TranslatorVisitor::asimd_VABAL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm) {
    return AbsoluteDifferenceLong(*this, U, D, sz, Vn, Vd, N, M, Vm, AccumulateBehavior::Accumulate);
}

bool TranslatorVisitor::asimd_VABDL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool M, size_t Vm) {
    return AbsoluteDifferenceLong(*this, U, D, sz, Vn, Vd, N, M, Vm, AccumulateBehavior::None);
}

bool TranslatorVisitor::asimd_VMLAL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool op, bool N, bool M, size_t Vm) {
    return WideInstruction(*this, U, D, sz, Vn, Vd, N, M, Vm, WidenBehaviour::Both, [this, op](size_t esize, const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        const auto multiply = ir.VectorMultiply(esize, reg_n, reg_m);
        return op ? ir.VectorSub(esize, reg_d, multiply)
                  : ir.VectorAdd(esize, reg_d, multiply);
    });
}

bool TranslatorVisitor::asimd_VMULL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool P, bool N, bool M, size_t Vm) {
    if (sz == 0b11) {
        return DecodeError();
    }

    if ((P & (U || sz == 0b10)) || Common::Bit<0>(Vd)) {
        return UndefinedInstruction();
    }

    const size_t esize = P ? (sz == 0b00 ? 8 : 64) : 8U << sz;
    const auto d = ToVector(true, Vd, D);
    const auto m = ToVector(false, Vm, M);
    const auto n = ToVector(false, Vn, N);

    const auto extend_reg = [&](const auto& reg) {
        return U ? ir.VectorZeroExtend(esize, reg) : ir.VectorSignExtend(esize, reg);
    };

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.GetVector(m);
    const auto result = P ? ir.VectorPolynomialMultiplyLong(esize, reg_n, reg_m)
                          : ir.VectorMultiply(2 * esize, extend_reg(reg_n), extend_reg(reg_m));

    ir.SetVector(d, result);
    return true;
}

}  // namespace Dynarmic::A32

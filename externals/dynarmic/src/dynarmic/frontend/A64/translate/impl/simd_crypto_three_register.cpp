/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
enum class SM3TTVariant {
    A,
    B,
};

bool SM3TT1(TranslatorVisitor& v, Vec Vm, Imm<2> imm2, Vec Vn, Vec Vd, SM3TTVariant behavior) {
    const IR::U128 d = v.ir.GetQ(Vd);
    const IR::U128 m = v.ir.GetQ(Vm);
    const IR::U128 n = v.ir.GetQ(Vn);
    const u32 index = imm2.ZeroExtend();

    const IR::U32 top_d = v.ir.VectorGetElement(32, d, 3);
    const IR::U32 before_top_d = v.ir.VectorGetElement(32, d, 2);
    const IR::U32 after_low_d = v.ir.VectorGetElement(32, d, 1);
    const IR::U32 low_d = v.ir.VectorGetElement(32, d, 0);
    const IR::U32 top_n = v.ir.VectorGetElement(32, n, 3);

    const IR::U32 wj_prime = v.ir.VectorGetElement(32, m, index);
    const IR::U32 ss2 = v.ir.Eor(top_n, v.ir.RotateRight(top_d, v.ir.Imm8(20)));
    const IR::U32 tt1 = [&] {
        if (behavior == SM3TTVariant::A) {
            return v.ir.Eor(after_low_d, v.ir.Eor(top_d, before_top_d));
        }
        const IR::U32 tmp1 = v.ir.And(top_d, after_low_d);
        const IR::U32 tmp2 = v.ir.And(top_d, before_top_d);
        const IR::U32 tmp3 = v.ir.And(after_low_d, before_top_d);
        return v.ir.Or(v.ir.Or(tmp1, tmp2), tmp3);
    }();
    const IR::U32 final_tt1 = v.ir.Add(tt1, v.ir.Add(low_d, v.ir.Add(ss2, wj_prime)));

    const IR::U128 zero_vector = v.ir.ZeroVector();
    const IR::U128 tmp1 = v.ir.VectorSetElement(32, zero_vector, 0, after_low_d);
    const IR::U128 tmp2 = v.ir.VectorSetElement(32, tmp1, 1, v.ir.RotateRight(before_top_d, v.ir.Imm8(23)));
    const IR::U128 tmp3 = v.ir.VectorSetElement(32, tmp2, 2, top_d);
    const IR::U128 result = v.ir.VectorSetElement(32, tmp3, 3, final_tt1);

    v.ir.SetQ(Vd, result);
    return true;
}

bool SM3TT2(TranslatorVisitor& v, Vec Vm, Imm<2> imm2, Vec Vn, Vec Vd, SM3TTVariant behavior) {
    const IR::U128 d = v.ir.GetQ(Vd);
    const IR::U128 m = v.ir.GetQ(Vm);
    const IR::U128 n = v.ir.GetQ(Vn);
    const u32 index = imm2.ZeroExtend();

    const IR::U32 top_d = v.ir.VectorGetElement(32, d, 3);
    const IR::U32 before_top_d = v.ir.VectorGetElement(32, d, 2);
    const IR::U32 after_low_d = v.ir.VectorGetElement(32, d, 1);
    const IR::U32 low_d = v.ir.VectorGetElement(32, d, 0);
    const IR::U32 top_n = v.ir.VectorGetElement(32, n, 3);

    const IR::U32 wj = v.ir.VectorGetElement(32, m, index);
    const IR::U32 tt2 = [&] {
        if (behavior == SM3TTVariant::A) {
            return v.ir.Eor(after_low_d, v.ir.Eor(top_d, before_top_d));
        }
        const IR::U32 tmp1 = v.ir.And(top_d, before_top_d);
        const IR::U32 tmp2 = v.ir.And(v.ir.Not(top_d), after_low_d);
        return v.ir.Or(tmp1, tmp2);
    }();
    const IR::U32 final_tt2 = v.ir.Add(tt2, v.ir.Add(low_d, v.ir.Add(top_n, wj)));
    const IR::U32 top_result = v.ir.Eor(final_tt2, v.ir.Eor(v.ir.RotateRight(final_tt2, v.ir.Imm8(23)),
                                                            v.ir.RotateRight(final_tt2, v.ir.Imm8(15))));

    const IR::U128 zero_vector = v.ir.ZeroVector();
    const IR::U128 tmp1 = v.ir.VectorSetElement(32, zero_vector, 0, after_low_d);
    const IR::U128 tmp2 = v.ir.VectorSetElement(32, tmp1, 1, v.ir.RotateRight(before_top_d, v.ir.Imm8(13)));
    const IR::U128 tmp3 = v.ir.VectorSetElement(32, tmp2, 2, top_d);
    const IR::U128 result = v.ir.VectorSetElement(32, tmp3, 3, top_result);

    v.ir.SetQ(Vd, result);
    return true;
}
}  // Anonymous namespace

bool TranslatorVisitor::SM3TT1A(Vec Vm, Imm<2> imm2, Vec Vn, Vec Vd) {
    return SM3TT1(*this, Vm, imm2, Vn, Vd, SM3TTVariant::A);
}

bool TranslatorVisitor::SM3TT1B(Vec Vm, Imm<2> imm2, Vec Vn, Vec Vd) {
    return SM3TT1(*this, Vm, imm2, Vn, Vd, SM3TTVariant::B);
}

bool TranslatorVisitor::SM3TT2A(Vec Vm, Imm<2> imm2, Vec Vn, Vec Vd) {
    return SM3TT2(*this, Vm, imm2, Vn, Vd, SM3TTVariant::A);
}

bool TranslatorVisitor::SM3TT2B(Vec Vm, Imm<2> imm2, Vec Vn, Vec Vd) {
    return SM3TT2(*this, Vm, imm2, Vn, Vd, SM3TTVariant::B);
}

}  // namespace Dynarmic::A64

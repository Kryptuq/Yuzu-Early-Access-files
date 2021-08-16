/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::EOR3(Vec Vm, Vec Va, Vec Vn, Vec Vd) {
    const IR::U128 a = ir.GetQ(Va);
    const IR::U128 m = ir.GetQ(Vm);
    const IR::U128 n = ir.GetQ(Vn);

    const IR::U128 result = ir.VectorEor(ir.VectorEor(n, m), a);

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::BCAX(Vec Vm, Vec Va, Vec Vn, Vec Vd) {
    const IR::U128 a = ir.GetQ(Va);
    const IR::U128 m = ir.GetQ(Vm);
    const IR::U128 n = ir.GetQ(Vn);

    const IR::U128 result = ir.VectorEor(n, ir.VectorAndNot(m, a));

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SM3SS1(Vec Vm, Vec Va, Vec Vn, Vec Vd) {
    const IR::U128 a = ir.GetQ(Va);
    const IR::U128 m = ir.GetQ(Vm);
    const IR::U128 n = ir.GetQ(Vn);

    const IR::U32 top_a = ir.VectorGetElement(32, a, 3);
    const IR::U32 top_m = ir.VectorGetElement(32, m, 3);
    const IR::U32 top_n = ir.VectorGetElement(32, n, 3);

    const IR::U32 rotated_n = ir.RotateRight(top_n, ir.Imm8(20));
    const IR::U32 sum = ir.Add(ir.Add(rotated_n, top_m), top_a);
    const IR::U32 result = ir.RotateRight(sum, ir.Imm8(25));

    const IR::U128 zero_vector = ir.ZeroVector();
    const IR::U128 vector_result = ir.VectorSetElement(32, zero_vector, 3, result);

    ir.SetQ(Vd, vector_result);
    return true;
}

}  // namespace Dynarmic::A64

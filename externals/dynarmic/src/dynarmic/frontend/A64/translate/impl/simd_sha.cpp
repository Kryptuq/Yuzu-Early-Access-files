/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
IR::U32 SHAchoose(IREmitter& ir, IR::U32 x, IR::U32 y, IR::U32 z) {
    return ir.Eor(ir.And(ir.Eor(y, z), x), z);
}

IR::U32 SHAmajority(IREmitter& ir, IR::U32 x, IR::U32 y, IR::U32 z) {
    return ir.Or(ir.And(x, y), ir.And(ir.Or(x, y), z));
}

IR::U32 SHAparity(IREmitter& ir, IR::U32 x, IR::U32 y, IR::U32 z) {
    return ir.Eor(ir.Eor(y, z), x);
}

using SHA1HashUpdateFunction = IR::U32(IREmitter&, IR::U32, IR::U32, IR::U32);

IR::U128 SHA1HashUpdate(IREmitter& ir, Vec Vm, Vec Vn, Vec Vd, SHA1HashUpdateFunction fn) {
    IR::U128 x = ir.GetQ(Vd);
    IR::U32 y = ir.VectorGetElement(32, ir.GetQ(Vn), 0);
    const IR::U128 w = ir.GetQ(Vm);

    for (size_t i = 0; i < 4; i++) {
        const IR::U32 low_x = ir.VectorGetElement(32, x, 0);
        const IR::U32 after_low_x = ir.VectorGetElement(32, x, 1);
        const IR::U32 before_high_x = ir.VectorGetElement(32, x, 2);
        const IR::U32 high_x = ir.VectorGetElement(32, x, 3);
        const IR::U32 t = fn(ir, after_low_x, before_high_x, high_x);
        const IR::U32 w_segment = ir.VectorGetElement(32, w, i);

        y = ir.Add(ir.Add(ir.Add(y, ir.RotateRight(low_x, ir.Imm8(27))), t), w_segment);
        x = ir.VectorSetElement(32, x, 1, ir.RotateRight(after_low_x, ir.Imm8(2)));

        // Move each 32-bit element to the left once
        // e.g. [3, 2, 1, 0], becomes [2, 1, 0, 3]
        const IR::U128 shuffled_x = ir.VectorShuffleWords(x, 0b10010011);
        x = ir.VectorSetElement(32, shuffled_x, 0, y);
        y = high_x;
    }

    return x;
}
}  // Anonymous namespace

bool TranslatorVisitor::SHA1C(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 result = SHA1HashUpdate(ir, Vm, Vn, Vd, SHAchoose);
    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA1M(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 result = SHA1HashUpdate(ir, Vm, Vn, Vd, SHAmajority);
    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA1P(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 result = SHA1HashUpdate(ir, Vm, Vn, Vd, SHAparity);
    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA1SU0(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 d = ir.GetQ(Vd);
    const IR::U128 m = ir.GetQ(Vm);
    const IR::U128 n = ir.GetQ(Vn);

    IR::U128 result = [&] {
        const IR::U64 d_high = ir.VectorGetElement(64, d, 1);
        const IR::U64 n_low = ir.VectorGetElement(64, n, 0);
        const IR::U128 zero = ir.ZeroVector();

        const IR::U128 tmp1 = ir.VectorSetElement(64, zero, 0, d_high);
        return ir.VectorSetElement(64, tmp1, 1, n_low);
    }();

    result = ir.VectorEor(ir.VectorEor(result, d), m);

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA1SU1(Vec Vn, Vec Vd) {
    const IR::U128 d = ir.GetQ(Vd);
    const IR::U128 n = ir.GetQ(Vn);

    // Shuffle down the whole vector and zero out the top 32 bits
    const IR::U128 shuffled_n = ir.VectorSetElement(32, ir.VectorShuffleWords(n, 0b00111001), 3, ir.Imm32(0));
    const IR::U128 t = ir.VectorEor(d, shuffled_n);
    const IR::U128 rotated_t = ir.VectorRotateLeft(32, t, 1);

    const IR::U32 low_rotated_t = ir.RotateRight(ir.VectorGetElement(32, rotated_t, 0), ir.Imm8(31));
    const IR::U32 high_t = ir.VectorGetElement(32, rotated_t, 3);
    const IR::U128 result = ir.VectorSetElement(32, rotated_t, 3, ir.Eor(low_rotated_t, high_t));

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA1H(Vec Vn, Vec Vd) {
    const IR::U128 data = ir.GetS(Vn);

    const IR::U128 result = ir.VectorOr(ir.VectorLogicalShiftLeft(32, data, 30),
                                        ir.VectorLogicalShiftRight(32, data, 2));

    ir.SetS(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA256SU0(Vec Vn, Vec Vd) {
    const IR::U128 x = ir.GetQ(Vd);
    const IR::U128 y = ir.GetQ(Vn);

    const IR::U128 result = ir.SHA256MessageSchedule0(x, y);

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA256SU1(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 x = ir.GetQ(Vd);
    const IR::U128 y = ir.GetQ(Vn);
    const IR::U128 z = ir.GetQ(Vm);

    const IR::U128 result = ir.SHA256MessageSchedule1(x, y, z);

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA256H(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 result = ir.SHA256Hash(ir.GetQ(Vd), ir.GetQ(Vn), ir.GetQ(Vm), true);
    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA256H2(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 result = ir.SHA256Hash(ir.GetQ(Vn), ir.GetQ(Vd), ir.GetQ(Vm), false);
    ir.SetQ(Vd, result);
    return true;
}

}  // namespace Dynarmic::A64

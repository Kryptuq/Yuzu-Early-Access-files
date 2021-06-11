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

IR::U32 SHAhashSIGMA0(IREmitter& ir, IR::U32 x) {
    const IR::U32 tmp1 = ir.RotateRight(x, ir.Imm8(2));
    const IR::U32 tmp2 = ir.RotateRight(x, ir.Imm8(13));
    const IR::U32 tmp3 = ir.RotateRight(x, ir.Imm8(22));

    return ir.Eor(tmp1, ir.Eor(tmp2, tmp3));
}

IR::U32 SHAhashSIGMA1(IREmitter& ir, IR::U32 x) {
    const IR::U32 tmp1 = ir.RotateRight(x, ir.Imm8(6));
    const IR::U32 tmp2 = ir.RotateRight(x, ir.Imm8(11));
    const IR::U32 tmp3 = ir.RotateRight(x, ir.Imm8(25));

    return ir.Eor(tmp1, ir.Eor(tmp2, tmp3));
}

enum class SHA256HashPart {
    Part1,
    Part2
};

IR::U128 SHA256hash(IREmitter& ir, IR::U128 x, IR::U128 y, IR::U128 w, SHA256HashPart part) {
    for (size_t i = 0; i < 4; i++) {
        const IR::U32 low_x = ir.VectorGetElement(32, x, 0);
        const IR::U32 after_low_x = ir.VectorGetElement(32, x, 1);
        const IR::U32 before_high_x = ir.VectorGetElement(32, x, 2);
        const IR::U32 high_x = ir.VectorGetElement(32, x, 3);

        const IR::U32 low_y = ir.VectorGetElement(32, y, 0);
        const IR::U32 after_low_y = ir.VectorGetElement(32, y, 1);
        const IR::U32 before_high_y = ir.VectorGetElement(32, y, 2);
        const IR::U32 high_y = ir.VectorGetElement(32, y, 3);

        const IR::U32 choice = SHAchoose(ir, low_y, after_low_y, before_high_y);
        const IR::U32 majority = SHAmajority(ir, low_x, after_low_x, before_high_x);

        const IR::U32 t = [&] {
            const IR::U32 w_element = ir.VectorGetElement(32, w, i);
            const IR::U32 sig = SHAhashSIGMA1(ir, low_y);

            return ir.Add(high_y, ir.Add(sig, ir.Add(choice, w_element)));
        }();

        const IR::U32 new_low_x = ir.Add(t, ir.Add(SHAhashSIGMA0(ir, low_x), majority));
        const IR::U32 new_low_y = ir.Add(t, high_x);

        // Shuffle all words left by 1 element: [3, 2, 1, 0] -> [2, 1, 0, 3]
        const IR::U128 shuffled_x = ir.VectorShuffleWords(x, 0b10010011);
        const IR::U128 shuffled_y = ir.VectorShuffleWords(y, 0b10010011);

        x = ir.VectorSetElement(32, shuffled_x, 0, new_low_x);
        y = ir.VectorSetElement(32, shuffled_y, 0, new_low_y);
    }

    if (part == SHA256HashPart::Part1) {
        return x;
    }

    return y;
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
    const IR::U128 d = ir.GetQ(Vd);
    const IR::U128 n = ir.GetQ(Vn);

    const IR::U128 t = [&] {
        // Shuffle the upper three elements down: [3, 2, 1, 0] -> [0, 3, 2, 1]
        const IR::U128 shuffled = ir.VectorShuffleWords(d, 0b00111001);

        return ir.VectorSetElement(32, shuffled, 3, ir.VectorGetElement(32, n, 0));
    }();

    IR::U128 result = ir.ZeroVector();
    for (size_t i = 0; i < 4; i++) {
        const IR::U32 modified_element = [&] {
            const IR::U32 element = ir.VectorGetElement(32, t, i);
            const IR::U32 tmp1 = ir.RotateRight(element, ir.Imm8(7));
            const IR::U32 tmp2 = ir.RotateRight(element, ir.Imm8(18));
            const IR::U32 tmp3 = ir.LogicalShiftRight(element, ir.Imm8(3));

            return ir.Eor(tmp1, ir.Eor(tmp2, tmp3));
        }();

        const IR::U32 d_element = ir.VectorGetElement(32, d, i);
        result = ir.VectorSetElement(32, result, i, ir.Add(modified_element, d_element));
    }

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA256SU1(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 d = ir.GetQ(Vd);
    const IR::U128 m = ir.GetQ(Vm);
    const IR::U128 n = ir.GetQ(Vn);

    const IR::U128 T0 = [&] {
        const IR::U32 low_m = ir.VectorGetElement(32, m, 0);
        const IR::U128 shuffled_n = ir.VectorShuffleWords(n, 0b00111001);

        return ir.VectorSetElement(32, shuffled_n, 3, low_m);
    }();

    const IR::U128 lower_half = [&] {
        const IR::U128 T = ir.VectorShuffleWords(m, 0b01001110);
        const IR::U128 tmp1 = ir.VectorRotateRight(32, T, 17);
        const IR::U128 tmp2 = ir.VectorRotateRight(32, T, 19);
        const IR::U128 tmp3 = ir.VectorLogicalShiftRight(32, T, 10);
        const IR::U128 tmp4 = ir.VectorEor(tmp1, ir.VectorEor(tmp2, tmp3));
        const IR::U128 tmp5 = ir.VectorAdd(32, tmp4, ir.VectorAdd(32, d, T0));
        return ir.VectorZeroUpper(tmp5);
    }();

    const IR::U64 upper_half = [&] {
        const IR::U128 tmp1 = ir.VectorRotateRight(32, lower_half, 17);
        const IR::U128 tmp2 = ir.VectorRotateRight(32, lower_half, 19);
        const IR::U128 tmp3 = ir.VectorLogicalShiftRight(32, lower_half, 10);
        const IR::U128 tmp4 = ir.VectorEor(tmp1, ir.VectorEor(tmp2, tmp3));

        // Shuffle the top two 32-bit elements downwards [3, 2, 1, 0] -> [1, 0, 3, 2]
        const IR::U128 shuffled_d = ir.VectorShuffleWords(d, 0b01001110);
        const IR::U128 shuffled_T0 = ir.VectorShuffleWords(T0, 0b01001110);

        const IR::U128 tmp5 = ir.VectorAdd(32, tmp4, ir.VectorAdd(32, shuffled_d, shuffled_T0));
        return ir.VectorGetElement(64, tmp5, 0);
    }();

    const IR::U128 result = ir.VectorSetElement(64, lower_half, 1, upper_half);

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA256H(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 result = SHA256hash(ir, ir.GetQ(Vd), ir.GetQ(Vn), ir.GetQ(Vm), SHA256HashPart::Part1);
    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA256H2(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 result = SHA256hash(ir, ir.GetQ(Vn), ir.GetQ(Vd), ir.GetQ(Vm), SHA256HashPart::Part2);
    ir.SetQ(Vd, result);
    return true;
}

}  // namespace Dynarmic::A64

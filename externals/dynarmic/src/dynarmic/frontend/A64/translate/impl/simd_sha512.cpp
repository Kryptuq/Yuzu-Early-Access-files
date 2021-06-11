/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
IR::U64 MakeSig(IREmitter& ir, IR::U64 data, u8 first_rot_amount, u8 second_rot_amount, u8 shift_amount) {
    const IR::U64 tmp1 = ir.RotateRight(data, ir.Imm8(first_rot_amount));
    const IR::U64 tmp2 = ir.RotateRight(data, ir.Imm8(second_rot_amount));
    const IR::U64 tmp3 = ir.LogicalShiftRight(data, ir.Imm8(shift_amount));

    return ir.Eor(tmp1, ir.Eor(tmp2, tmp3));
}

IR::U64 MakeMNSig(IREmitter& ir, IR::U64 data, u8 first_rot_amount, u8 second_rot_amount, u8 third_rot_amount) {
    const IR::U64 tmp1 = ir.RotateRight(data, ir.Imm8(first_rot_amount));
    const IR::U64 tmp2 = ir.RotateRight(data, ir.Imm8(second_rot_amount));
    const IR::U64 tmp3 = ir.RotateRight(data, ir.Imm8(third_rot_amount));

    return ir.Eor(tmp1, ir.Eor(tmp2, tmp3));
}

enum class SHA512HashPart {
    Part1,
    Part2,
};

IR::U128 SHA512Hash(IREmitter& ir, Vec Vm, Vec Vn, Vec Vd, SHA512HashPart part) {
    const IR::U128 x = ir.GetQ(Vn);
    const IR::U128 y = ir.GetQ(Vm);
    const IR::U128 w = ir.GetQ(Vd);

    const IR::U64 lower_x = ir.VectorGetElement(64, x, 0);
    const IR::U64 upper_x = ir.VectorGetElement(64, x, 1);

    const IR::U64 lower_y = ir.VectorGetElement(64, y, 0);
    const IR::U64 upper_y = ir.VectorGetElement(64, y, 1);

    const auto make_sigma = [&](IR::U64 data) {
        if (part == SHA512HashPart::Part1) {
            return MakeMNSig(ir, data, 14, 18, 41);
        }
        return MakeMNSig(ir, data, 28, 34, 39);
    };

    const auto make_partial_half = [&](IR::U64 a, IR::U64 b, IR::U64 c) {
        const IR::U64 tmp1 = ir.And(a, b);

        if (part == SHA512HashPart::Part1) {
            const IR::U64 tmp2 = ir.And(ir.Not(a), c);
            return ir.Eor(tmp1, tmp2);
        }

        const IR::U64 tmp2 = ir.And(a, c);
        const IR::U64 tmp3 = ir.And(upper_y, lower_y);
        return ir.Eor(tmp1, ir.Eor(tmp2, tmp3));
    };

    const IR::U64 Vtmp = [&] {
        const IR::U64 partial = [&] {
            if (part == SHA512HashPart::Part1) {
                return make_partial_half(upper_y, lower_x, upper_x);
            }
            return make_partial_half(lower_x, upper_y, lower_y);
        }();
        const IR::U64 upper_w = ir.VectorGetElement(64, w, 1);
        const IR::U64 sig = [&] {
            if (part == SHA512HashPart::Part1) {
                return make_sigma(upper_y);
            }
            return make_sigma(lower_y);
        }();

        return ir.Add(partial, ir.Add(sig, upper_w));
    }();

    const IR::U128 low_result = [&] {
        const IR::U64 tmp = [&]() -> IR::U64 {
            if (part == SHA512HashPart::Part1) {
                return ir.Add(Vtmp, lower_y);
            }
            return Vtmp;
        }();
        const IR::U64 partial = [&] {
            if (part == SHA512HashPart::Part1) {
                return make_partial_half(tmp, upper_y, lower_x);
            }
            return make_partial_half(Vtmp, lower_y, upper_y);
        }();
        const IR::U64 sig = make_sigma(tmp);
        const IR::U64 lower_w = ir.VectorGetElement(64, w, 0);

        return ir.ZeroExtendToQuad(ir.Add(partial, ir.Add(sig, lower_w)));
    }();

    return ir.VectorSetElement(64, low_result, 1, Vtmp);
}

enum class SM4RotationType {
    SM4E,
    SM4EKEY
};

IR::U32 SM4Rotation(IREmitter& ir, IR::U32 intval, IR::U32 round_result_low_word, SM4RotationType type) {
    if (type == SM4RotationType::SM4E) {
        const IR::U32 tmp1 = ir.RotateRight(intval, ir.Imm8(30));
        const IR::U32 tmp2 = ir.RotateRight(intval, ir.Imm8(22));
        const IR::U32 tmp3 = ir.RotateRight(intval, ir.Imm8(14));
        const IR::U32 tmp4 = ir.RotateRight(intval, ir.Imm8(8));
        const IR::U32 tmp5 = ir.Eor(intval, ir.Eor(tmp1, ir.Eor(tmp2, ir.Eor(tmp3, tmp4))));

        return ir.Eor(tmp5, round_result_low_word);
    }

    const IR::U32 tmp1 = ir.RotateRight(intval, ir.Imm8(19));
    const IR::U32 tmp2 = ir.RotateRight(intval, ir.Imm8(9));
    return ir.Eor(round_result_low_word, ir.Eor(intval, ir.Eor(tmp1, tmp2)));
}

IR::U128 SM4Hash(IREmitter& ir, Vec Vn, Vec Vd, SM4RotationType type) {
    const IR::U128 n = ir.GetQ(Vn);
    IR::U128 roundresult = ir.GetQ(Vd);

    for (size_t i = 0; i < 4; i++) {
        const IR::U32 round_key = ir.VectorGetElement(32, n, i);

        const IR::U32 upper_round = ir.VectorGetElement(32, roundresult, 3);
        const IR::U32 before_upper_round = ir.VectorGetElement(32, roundresult, 2);
        const IR::U32 after_lower_round = ir.VectorGetElement(32, roundresult, 1);

        IR::U128 intval_vec = ir.ZeroExtendToQuad(ir.Eor(upper_round, ir.Eor(before_upper_round, ir.Eor(after_lower_round, round_key))));

        for (size_t j = 0; j < 4; j++) {
            const IR::U8 byte_element = ir.VectorGetElement(8, intval_vec, j);
            intval_vec = ir.VectorSetElement(8, intval_vec, j, ir.SM4AccessSubstitutionBox(byte_element));
        }

        const IR::U32 intval_low_word = ir.VectorGetElement(32, intval_vec, 0);
        const IR::U32 round_result_low_word = ir.VectorGetElement(32, roundresult, 0);
        const IR::U32 intval = SM4Rotation(ir, intval_low_word, round_result_low_word, type);
        roundresult = ir.VectorShuffleWords(roundresult, 0b00111001);
        roundresult = ir.VectorSetElement(32, roundresult, 3, intval);
    }

    return roundresult;
}
}  // Anonymous namespace

bool TranslatorVisitor::SHA512SU0(Vec Vn, Vec Vd) {
    const IR::U128 x = ir.GetQ(Vn);
    const IR::U128 w = ir.GetQ(Vd);

    const IR::U64 lower_x = ir.VectorGetElement(64, x, 0);
    const IR::U64 lower_w = ir.VectorGetElement(64, w, 0);
    const IR::U64 upper_w = ir.VectorGetElement(64, w, 1);

    const auto make_sig0 = [&](IR::U64 data) {
        return MakeSig(ir, data, 1, 8, 7);
    };

    const IR::U128 low_result = ir.ZeroExtendToQuad(ir.Add(lower_w, make_sig0(upper_w)));
    const IR::U64 high_result = ir.Add(upper_w, make_sig0(lower_x));
    const IR::U128 result = ir.VectorSetElement(64, low_result, 1, high_result);

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA512SU1(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 x = ir.GetQ(Vn);
    const IR::U128 y = ir.GetQ(Vm);
    const IR::U128 w = ir.GetQ(Vd);

    const auto make_sig1 = [&](IR::U64 data) {
        return MakeSig(ir, data, 19, 61, 6);
    };

    const IR::U128 sig_vector = [&] {
        const IR::U64 lower_x = ir.VectorGetElement(64, x, 0);
        const IR::U64 upper_x = ir.VectorGetElement(64, x, 1);

        const IR::U128 low_result = ir.ZeroExtendToQuad(make_sig1(lower_x));
        return ir.VectorSetElement(64, low_result, 1, make_sig1(upper_x));
    }();

    const IR::U128 result = ir.VectorAdd(64, w, ir.VectorAdd(64, y, sig_vector));

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA512H(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 result = SHA512Hash(ir, Vm, Vn, Vd, SHA512HashPart::Part1);
    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SHA512H2(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 result = SHA512Hash(ir, Vm, Vn, Vd, SHA512HashPart::Part2);
    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::RAX1(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 m = ir.GetQ(Vm);
    const IR::U128 n = ir.GetQ(Vn);

    const IR::U128 rotated_m = ir.VectorRotateLeft(64, m, 1);
    const IR::U128 result = ir.VectorEor(n, rotated_m);

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::XAR(Vec Vm, Imm<6> imm6, Vec Vn, Vec Vd) {
    const IR::U128 m = ir.GetQ(Vm);
    const IR::U128 n = ir.GetQ(Vn);

    const IR::U128 tmp = ir.VectorEor(m, n);
    const IR::U128 result = ir.VectorRotateRight(64, tmp, imm6.ZeroExtend<u8>());

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SM3PARTW1(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 d = ir.GetQ(Vd);
    const IR::U128 m = ir.GetQ(Vm);
    const IR::U128 n = ir.GetQ(Vn);

    const IR::U128 eor_d_n = ir.VectorEor(d, n);

    const IR::U128 result_low_three_words = [&] {
        // Move the top-most 3 words down one element (i.e. [3, 2, 1, 0] -> [0, 3, 2, 1])
        const IR::U128 shuffled_m = ir.VectorShuffleWords(m, 0b00111001);

        // We treat the uppermost word as junk data and don't touch/use it explicitly for now.
        // Given we don't do anything with it yet, the fact we EOR into it doesn't matter.
        return ir.VectorEor(eor_d_n, ir.VectorRotateLeft(32, shuffled_m, 15));
    }();

    IR::U128 result = result_low_three_words;
    for (size_t i = 0; i < 4; i++) {
        if (i == 3) {
            const IR::U32 top_eor_d_n = ir.VectorGetElement(32, eor_d_n, 3);
            const IR::U32 low_result_word = ir.VectorGetElement(32, result, 0);
            const IR::U32 top_result_word = ir.Eor(top_eor_d_n, ir.RotateRight(low_result_word, ir.Imm8(17)));

            // Now the uppermost word is well-defined data.
            result = ir.VectorSetElement(32, result, 3, top_result_word);
        }

        const IR::U32 word = ir.VectorGetElement(32, result, i);
        const IR::U32 modified = ir.Eor(word, ir.Eor(ir.RotateRight(word, ir.Imm8(17)),
                                                     ir.RotateRight(word, ir.Imm8(9))));

        result = ir.VectorSetElement(32, result, i, modified);
    }

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SM3PARTW2(Vec Vm, Vec Vn, Vec Vd) {
    const IR::U128 d = ir.GetQ(Vd);
    const IR::U128 m = ir.GetQ(Vm);
    const IR::U128 n = ir.GetQ(Vn);

    const IR::U128 temp = ir.VectorEor(n, ir.VectorRotateLeft(32, m, 7));
    const IR::U128 temp_result = ir.VectorEor(d, temp);
    const IR::U32 temp2 = [&] {
        const IR::U32 rotate1 = ir.RotateRight(ir.VectorGetElement(32, temp, 0), ir.Imm8(17));
        const IR::U32 rotate2 = ir.RotateRight(rotate1, ir.Imm8(17));
        const IR::U32 rotate3 = ir.RotateRight(rotate1, ir.Imm8(9));

        return ir.Eor(rotate1, ir.Eor(rotate2, rotate3));
    }();

    const IR::U32 high_temp_result = ir.VectorGetElement(32, temp_result, 3);
    const IR::U32 replacement = ir.Eor(high_temp_result, temp2);
    const IR::U128 result = ir.VectorSetElement(32, temp_result, 3, replacement);

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::SM4E(Vec Vn, Vec Vd) {
    ir.SetQ(Vd, SM4Hash(ir, Vn, Vd, SM4RotationType::SM4E));
    return true;
}

bool TranslatorVisitor::SM4EKEY(Vec Vm, Vec Vn, Vec Vd) {
    ir.SetQ(Vd, SM4Hash(ir, Vm, Vn, SM4RotationType::SM4EKEY));
    return true;
}

}  // namespace Dynarmic::A64

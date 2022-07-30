/* This file is part of the dynarmic project.
 * Copyright (c) 2022 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/ir_emitter.h"
#include "dynarmic/ir/microinstruction.h"
#include "dynarmic/ir/opcodes.h"
#include "dynarmic/ir/opt/passes.h"

namespace Dynarmic::Optimization {

namespace {

void PolyfillSHA256MessageSchedule0(IR::IREmitter& ir, IR::Inst& inst) {
    const IR::U128 x = (IR::U128)inst.GetArg(0);
    const IR::U128 y = (IR::U128)inst.GetArg(1);

    const IR::U128 t = ir.VectorExtract(x, y, 32);

    IR::U128 result = ir.ZeroVector();
    for (size_t i = 0; i < 4; i++) {
        const IR::U32 modified_element = [&] {
            const IR::U32 element = ir.VectorGetElement(32, t, i);
            const IR::U32 tmp1 = ir.RotateRight(element, ir.Imm8(7));
            const IR::U32 tmp2 = ir.RotateRight(element, ir.Imm8(18));
            const IR::U32 tmp3 = ir.LogicalShiftRight(element, ir.Imm8(3));

            return ir.Eor(tmp1, ir.Eor(tmp2, tmp3));
        }();

        result = ir.VectorSetElement(32, result, i, modified_element);
    }
    result = ir.VectorAdd(32, result, x);

    inst.ReplaceUsesWith(result);
}

void PolyfillSHA256MessageSchedule1(IR::IREmitter& ir, IR::Inst& inst) {
    const IR::U128 x = (IR::U128)inst.GetArg(0);
    const IR::U128 y = (IR::U128)inst.GetArg(1);
    const IR::U128 z = (IR::U128)inst.GetArg(2);

    const IR::U128 T0 = ir.VectorExtract(y, z, 32);

    const IR::U128 lower_half = [&] {
        const IR::U128 T = ir.VectorShuffleWords(z, 0b01001110);
        const IR::U128 tmp1 = ir.VectorRotateRight(32, T, 17);
        const IR::U128 tmp2 = ir.VectorRotateRight(32, T, 19);
        const IR::U128 tmp3 = ir.VectorLogicalShiftRight(32, T, 10);
        const IR::U128 tmp4 = ir.VectorEor(tmp1, ir.VectorEor(tmp2, tmp3));
        const IR::U128 tmp5 = ir.VectorAdd(32, tmp4, ir.VectorAdd(32, x, T0));
        return ir.VectorZeroUpper(tmp5);
    }();

    const IR::U64 upper_half = [&] {
        const IR::U128 tmp1 = ir.VectorRotateRight(32, lower_half, 17);
        const IR::U128 tmp2 = ir.VectorRotateRight(32, lower_half, 19);
        const IR::U128 tmp3 = ir.VectorLogicalShiftRight(32, lower_half, 10);
        const IR::U128 tmp4 = ir.VectorEor(tmp1, ir.VectorEor(tmp2, tmp3));

        // Shuffle the top two 32-bit elements downwards [3, 2, 1, 0] -> [1, 0, 3, 2]
        const IR::U128 shuffled_d = ir.VectorShuffleWords(x, 0b01001110);
        const IR::U128 shuffled_T0 = ir.VectorShuffleWords(T0, 0b01001110);

        const IR::U128 tmp5 = ir.VectorAdd(32, tmp4, ir.VectorAdd(32, shuffled_d, shuffled_T0));
        return ir.VectorGetElement(64, tmp5, 0);
    }();

    const IR::U128 result = ir.VectorSetElement(64, lower_half, 1, upper_half);

    inst.ReplaceUsesWith(result);
}

IR::U32 SHAchoose(IR::IREmitter& ir, IR::U32 x, IR::U32 y, IR::U32 z) {
    return ir.Eor(ir.And(ir.Eor(y, z), x), z);
}

IR::U32 SHAmajority(IR::IREmitter& ir, IR::U32 x, IR::U32 y, IR::U32 z) {
    return ir.Or(ir.And(x, y), ir.And(ir.Or(x, y), z));
}

IR::U32 SHAhashSIGMA0(IR::IREmitter& ir, IR::U32 x) {
    const IR::U32 tmp1 = ir.RotateRight(x, ir.Imm8(2));
    const IR::U32 tmp2 = ir.RotateRight(x, ir.Imm8(13));
    const IR::U32 tmp3 = ir.RotateRight(x, ir.Imm8(22));

    return ir.Eor(tmp1, ir.Eor(tmp2, tmp3));
}

IR::U32 SHAhashSIGMA1(IR::IREmitter& ir, IR::U32 x) {
    const IR::U32 tmp1 = ir.RotateRight(x, ir.Imm8(6));
    const IR::U32 tmp2 = ir.RotateRight(x, ir.Imm8(11));
    const IR::U32 tmp3 = ir.RotateRight(x, ir.Imm8(25));

    return ir.Eor(tmp1, ir.Eor(tmp2, tmp3));
}

void PolyfillSHA256Hash(IR::IREmitter& ir, IR::Inst& inst) {
    IR::U128 x = (IR::U128)inst.GetArg(0);
    IR::U128 y = (IR::U128)inst.GetArg(1);
    const IR::U128 w = (IR::U128)inst.GetArg(2);
    const bool part1 = inst.GetArg(3).GetU1();

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

    inst.ReplaceUsesWith(part1 ? x : y);
}

}  // namespace

void PolyfillPass(IR::Block& block, const PolyfillOptions& polyfill) {
    if (polyfill == PolyfillOptions{}) {
        return;
    }

    IR::IREmitter ir{block};

    for (auto& inst : block) {
        ir.SetInsertionPointBefore(&inst);

        switch (inst.GetOpcode()) {
        case IR::Opcode::SHA256MessageSchedule0:
            if (polyfill.sha256) {
                PolyfillSHA256MessageSchedule0(ir, inst);
            }
            break;
        case IR::Opcode::SHA256MessageSchedule1:
            if (polyfill.sha256) {
                PolyfillSHA256MessageSchedule1(ir, inst);
            }
            break;
        case IR::Opcode::SHA256Hash:
            if (polyfill.sha256) {
                PolyfillSHA256Hash(ir, inst);
            }
            break;
        default:
            break;
        }
    }
}

}  // namespace Dynarmic::Optimization

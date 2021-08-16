/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

bool TranslatorVisitor::AND_imm(bool sf, bool N, Imm<6> immr, Imm<6> imms, Reg Rn, Reg Rd) {
    if (!sf && N) {
        return ReservedValue();
    }

    u64 imm;
    if (auto masks = DecodeBitMasks(N, imms, immr, true)) {
        imm = masks->wmask;
    } else {
        return ReservedValue();
    }

    const size_t datasize = sf ? 64 : 32;
    const auto operand1 = X(datasize, Rn);

    const auto result = ir.And(operand1, I(datasize, imm));
    if (Rd == Reg::SP) {
        SP(datasize, result);
    } else {
        X(datasize, Rd, result);
    }

    return true;
}

bool TranslatorVisitor::ORR_imm(bool sf, bool N, Imm<6> immr, Imm<6> imms, Reg Rn, Reg Rd) {
    if (!sf && N) {
        return ReservedValue();
    }

    u64 imm;
    if (auto masks = DecodeBitMasks(N, imms, immr, true)) {
        imm = masks->wmask;
    } else {
        return ReservedValue();
    }

    const size_t datasize = sf ? 64 : 32;
    const auto operand1 = X(datasize, Rn);

    const auto result = ir.Or(operand1, I(datasize, imm));
    if (Rd == Reg::SP) {
        SP(datasize, result);
    } else {
        X(datasize, Rd, result);
    }

    return true;
}

bool TranslatorVisitor::EOR_imm(bool sf, bool N, Imm<6> immr, Imm<6> imms, Reg Rn, Reg Rd) {
    if (!sf && N) {
        return ReservedValue();
    }

    u64 imm;
    if (auto masks = DecodeBitMasks(N, imms, immr, true)) {
        imm = masks->wmask;
    } else {
        return ReservedValue();
    }

    const size_t datasize = sf ? 64 : 32;
    const auto operand1 = X(datasize, Rn);

    const auto result = ir.Eor(operand1, I(datasize, imm));
    if (Rd == Reg::SP) {
        SP(datasize, result);
    } else {
        X(datasize, Rd, result);
    }

    return true;
}

bool TranslatorVisitor::ANDS_imm(bool sf, bool N, Imm<6> immr, Imm<6> imms, Reg Rn, Reg Rd) {
    if (!sf && N) {
        return ReservedValue();
    }

    u64 imm;
    if (auto masks = DecodeBitMasks(N, imms, immr, true)) {
        imm = masks->wmask;
    } else {
        return ReservedValue();
    }

    const size_t datasize = sf ? 64 : 32;
    const auto operand1 = X(datasize, Rn);
    const auto result = ir.And(operand1, I(datasize, imm));

    ir.SetNZCV(ir.NZCVFrom(result));
    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::AND_shift(bool sf, Imm<2> shift, Reg Rm, Imm<6> imm6, Reg Rn, Reg Rd) {
    if (!sf && imm6.Bit<5>()) {
        return ReservedValue();
    }

    const size_t datasize = sf ? 64 : 32;
    const u8 shift_amount = imm6.ZeroExtend<u8>();

    const auto operand1 = X(datasize, Rn);
    const auto operand2 = ShiftReg(datasize, Rm, shift, ir.Imm8(shift_amount));
    const auto result = ir.And(operand1, operand2);

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::BIC_shift(bool sf, Imm<2> shift, Reg Rm, Imm<6> imm6, Reg Rn, Reg Rd) {
    if (!sf && imm6.Bit<5>()) {
        return ReservedValue();
    }

    const size_t datasize = sf ? 64 : 32;
    const u8 shift_amount = imm6.ZeroExtend<u8>();

    const auto operand1 = X(datasize, Rn);
    const auto operand2 = ShiftReg(datasize, Rm, shift, ir.Imm8(shift_amount));
    const auto result = ir.AndNot(operand1, operand2);

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::ORR_shift(bool sf, Imm<2> shift, Reg Rm, Imm<6> imm6, Reg Rn, Reg Rd) {
    if (!sf && imm6.Bit<5>()) {
        return ReservedValue();
    }

    const size_t datasize = sf ? 64 : 32;
    const u8 shift_amount = imm6.ZeroExtend<u8>();

    const auto operand1 = X(datasize, Rn);
    const auto operand2 = ShiftReg(datasize, Rm, shift, ir.Imm8(shift_amount));
    const auto result = ir.Or(operand1, operand2);

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::ORN_shift(bool sf, Imm<2> shift, Reg Rm, Imm<6> imm6, Reg Rn, Reg Rd) {
    if (!sf && imm6.Bit<5>()) {
        return ReservedValue();
    }

    const size_t datasize = sf ? 64 : 32;
    const u8 shift_amount = imm6.ZeroExtend<u8>();

    const auto operand1 = X(datasize, Rn);
    const auto operand2 = ir.Not(ShiftReg(datasize, Rm, shift, ir.Imm8(shift_amount)));
    const auto result = ir.Or(operand1, operand2);

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::EOR_shift(bool sf, Imm<2> shift, Reg Rm, Imm<6> imm6, Reg Rn, Reg Rd) {
    if (!sf && imm6.Bit<5>()) {
        return ReservedValue();
    }

    const size_t datasize = sf ? 64 : 32;
    const u8 shift_amount = imm6.ZeroExtend<u8>();

    const auto operand1 = X(datasize, Rn);
    const auto operand2 = ShiftReg(datasize, Rm, shift, ir.Imm8(shift_amount));
    const auto result = ir.Eor(operand1, operand2);

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::EON(bool sf, Imm<2> shift, Reg Rm, Imm<6> imm6, Reg Rn, Reg Rd) {
    if (!sf && imm6.Bit<5>()) {
        return ReservedValue();
    }

    const size_t datasize = sf ? 64 : 32;
    const u8 shift_amount = imm6.ZeroExtend<u8>();

    const auto operand1 = X(datasize, Rn);
    const auto operand2 = ir.Not(ShiftReg(datasize, Rm, shift, ir.Imm8(shift_amount)));
    const auto result = ir.Eor(operand1, operand2);

    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::ANDS_shift(bool sf, Imm<2> shift, Reg Rm, Imm<6> imm6, Reg Rn, Reg Rd) {
    if (!sf && imm6.Bit<5>()) {
        return ReservedValue();
    }

    const size_t datasize = sf ? 64 : 32;
    const u8 shift_amount = imm6.ZeroExtend<u8>();

    const auto operand1 = X(datasize, Rn);
    const auto operand2 = ShiftReg(datasize, Rm, shift, ir.Imm8(shift_amount));
    const auto result = ir.And(operand1, operand2);

    ir.SetNZCV(ir.NZCVFrom(result));
    X(datasize, Rd, result);
    return true;
}

bool TranslatorVisitor::BICS(bool sf, Imm<2> shift, Reg Rm, Imm<6> imm6, Reg Rn, Reg Rd) {
    if (!sf && imm6.Bit<5>()) {
        return ReservedValue();
    }

    const size_t datasize = sf ? 64 : 32;
    const u8 shift_amount = imm6.ZeroExtend<u8>();

    const auto operand1 = X(datasize, Rn);
    const auto operand2 = ShiftReg(datasize, Rm, shift, ir.Imm8(shift_amount));
    const auto result = ir.AndNot(operand1, operand2);

    ir.SetNZCV(ir.NZCVFrom(result));
    X(datasize, Rd, result);
    return true;
}

}  // namespace Dynarmic::A64

/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <mcl/assert.hpp>
#include <mcl/bit/bit_field.hpp>

#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"

namespace Dynarmic::A32 {

bool TranslatorVisitor::asimd_VMOV_imm(Imm<1> a, bool D, Imm<1> b, Imm<1> c, Imm<1> d, size_t Vd, Imm<4> cmode, bool Q, bool op, Imm<1> e, Imm<1> f, Imm<1> g, Imm<1> h) {
    if (Q && mcl::bit::get_bit<0>(Vd)) {
        return UndefinedInstruction();
    }

    const auto d_reg = ToVector(Q, Vd, D);
    const auto imm = AdvSIMDExpandImm(op, cmode, concatenate(a, b, c, d, e, f, g, h));

    // VMOV
    const auto mov = [&] {
        const auto imm64 = ir.Imm64(imm);
        if (Q) {
            ir.SetVector(d_reg, ir.VectorBroadcast(64, imm64));
        } else {
            ir.SetExtendedRegister(d_reg, imm64);
        }
        return true;
    };

    // VMVN
    // mvn is a predefined macro in arm64 MSVC
    const auto mvn_ = [&] {
        const auto imm64 = ir.Imm64(~imm);
        if (Q) {
            ir.SetVector(d_reg, ir.VectorBroadcast(64, imm64));
        } else {
            ir.SetExtendedRegister(d_reg, imm64);
        }
        return true;
    };

    // VORR
    const auto orr = [&] {
        const auto imm64 = ir.Imm64(imm);
        if (Q) {
            const auto reg_value = ir.GetVector(d_reg);
            ir.SetVector(d_reg, ir.VectorOr(reg_value, ir.VectorBroadcast(64, imm64)));
        } else {
            const auto reg_value = ir.GetExtendedRegister(d_reg);
            ir.SetExtendedRegister(d_reg, ir.Or(reg_value, imm64));
        }
        return true;
    };

    // VBIC
    const auto bic = [&] {
        const auto imm64 = ir.Imm64(~imm);
        if (Q) {
            const auto reg_value = ir.GetVector(d_reg);
            ir.SetVector(d_reg, ir.VectorAnd(reg_value, ir.VectorBroadcast(64, imm64)));
        } else {
            const auto reg_value = ir.GetExtendedRegister(d_reg);
            ir.SetExtendedRegister(d_reg, ir.And(reg_value, imm64));
        }
        return true;
    };

    switch (concatenate(cmode, Imm<1>{op}).ZeroExtend()) {
    case 0b00000:
    case 0b00100:
    case 0b01000:
    case 0b01100:
    case 0b10000:
    case 0b10100:
    case 0b11000:
    case 0b11010:
    case 0b11100:
    case 0b11101:
    case 0b11110:
        return mov();
    case 0b11111:
        return UndefinedInstruction();
    case 0b00001:
    case 0b00101:
    case 0b01001:
    case 0b01101:
    case 0b10001:
    case 0b10101:
    case 0b11001:
    case 0b11011:
        return mvn_();
    case 0b00010:
    case 0b00110:
    case 0b01010:
    case 0b01110:
    case 0b10010:
    case 0b10110:
        return orr();
    case 0b00011:
    case 0b00111:
    case 0b01011:
    case 0b01111:
    case 0b10011:
    case 0b10111:
        return bic();
    }

    UNREACHABLE();
}

}  // namespace Dynarmic::A32

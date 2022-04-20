/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <mcl/assert.hpp>
#include <mcl/bit/bit_count.hpp>
#include <mcl/bit/bit_field.hpp>

#include "dynarmic/frontend/A32/translate/impl/a32_translate_impl.h"

namespace Dynarmic::A32 {

static bool TableLookup(TranslatorVisitor& v, bool is_vtbl, bool D, size_t Vn, size_t Vd, size_t len, bool N, bool M, size_t Vm) {
    const size_t length = len + 1;
    const auto d = ToVector(false, Vd, D);
    const auto m = ToVector(false, Vm, M);
    const auto n = ToVector(false, Vn, N);

    if (RegNumber(n) + length > 32) {
        return v.UnpredictableInstruction();
    }

    const IR::Table table = v.ir.VectorTable([&] {
        std::vector<IR::U64> result;
        for (size_t i = 0; i < length; ++i) {
            result.emplace_back(v.ir.GetExtendedRegister(n + i));
        }
        return result;
    }());
    const IR::U64 indicies = v.ir.GetExtendedRegister(m);
    const IR::U64 defaults = is_vtbl ? v.ir.Imm64(0) : IR::U64{v.ir.GetExtendedRegister(d)};
    const IR::U64 result = v.ir.VectorTableLookup(defaults, table, indicies);

    v.ir.SetExtendedRegister(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VEXT(bool D, size_t Vn, size_t Vd, Imm<4> imm4, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (mcl::bit::get_bit<0>(Vd) || mcl::bit::get_bit<0>(Vn) || mcl::bit::get_bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (!Q && imm4.Bit<3>()) {
        return UndefinedInstruction();
    }

    const u8 position = 8 * imm4.ZeroExtend<u8>();
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.GetVector(m);
    const auto result = Q ? ir.VectorExtract(reg_n, reg_m, position) : ir.VectorExtractLower(reg_n, reg_m, position);

    ir.SetVector(d, result);
    return true;
}

bool TranslatorVisitor::asimd_VTBL(bool D, size_t Vn, size_t Vd, size_t len, bool N, bool M, size_t Vm) {
    return TableLookup(*this, true, D, Vn, Vd, len, N, M, Vm);
}

bool TranslatorVisitor::asimd_VTBX(bool D, size_t Vn, size_t Vd, size_t len, bool N, bool M, size_t Vm) {
    return TableLookup(*this, false, D, Vn, Vd, len, N, M, Vm);
}

bool TranslatorVisitor::asimd_VDUP_scalar(bool D, Imm<4> imm4, size_t Vd, bool Q, bool M, size_t Vm) {
    if (Q && mcl::bit::get_bit<0>(Vd)) {
        return UndefinedInstruction();
    }

    if (imm4.Bits<0, 2>() == 0b000) {
        return UndefinedInstruction();
    }

    const size_t imm4_lsb = mcl::bit::lowest_set_bit(imm4.ZeroExtend());
    const size_t esize = 8u << imm4_lsb;
    const size_t index = imm4.ZeroExtend() >> (imm4_lsb + 1);
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(false, Vm, M);

    const auto reg_m = ir.GetVector(m);
    const auto result = ir.VectorBroadcastElement(esize, reg_m, index);

    ir.SetVector(d, result);
    return true;
}

}  // namespace Dynarmic::A32

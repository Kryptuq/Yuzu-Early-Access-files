/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {

static bool TableLookup(TranslatorVisitor& v, bool Q, Vec Vm, Imm<2> len, bool is_tbl, size_t Vn, Vec Vd) {
    const size_t datasize = Q ? 128 : 64;

    const IR::Table table = v.ir.VectorTable([&] {
        std::vector<IR::U128> result;
        for (size_t i = 0; i < len.ZeroExtend<size_t>() + 1; ++i) {
            result.emplace_back(v.ir.GetQ(static_cast<Vec>((Vn + i) % 32)));
        }
        return result;
    }());

    const IR::U128 indicies = v.ir.GetQ(Vm);
    const IR::U128 defaults = is_tbl ? v.ir.ZeroVector() : v.ir.GetQ(Vd);

    const IR::U128 result = v.ir.VectorTableLookup(defaults, table, indicies);

    v.V(datasize, Vd, datasize == 128 ? result : v.ir.VectorZeroUpper(result));
    return true;
}

bool TranslatorVisitor::TBL(bool Q, Vec Vm, Imm<2> len, size_t Vn, Vec Vd) {
    return TableLookup(*this, Q, Vm, len, true, Vn, Vd);
}

bool TranslatorVisitor::TBX(bool Q, Vec Vm, Imm<2> len, size_t Vn, Vec Vd) {
    return TableLookup(*this, Q, Vm, len, false, Vn, Vd);
}

}  // namespace Dynarmic::A64

/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <tuple>

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
enum class Transposition {
    TRN1,
    TRN2,
};

bool VectorTranspose(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, Transposition type) {
    if (!Q && size == 0b11) {
        return v.ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const u8 esize = static_cast<u8>(8 << size.ZeroExtend());

    const IR::U128 m = v.V(datasize, Vm);
    const IR::U128 n = v.V(datasize, Vn);
    const IR::U128 result = v.ir.VectorTranspose(esize, n, m, type == Transposition::TRN2);

    v.V(datasize, Vd, result);
    return true;
}

enum class UnzipType {
    Even,
    Odd,
};

bool VectorUnzip(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd, UnzipType type) {
    if (size == 0b11 && !Q) {
        return v.ReservedValue();
    }

    const size_t datasize = Q ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();

    const IR::U128 n = v.V(datasize, Vn);
    const IR::U128 m = v.V(datasize, Vm);
    const IR::U128 result = type == UnzipType::Even
                              ? (Q ? v.ir.VectorDeinterleaveEven(esize, n, m) : v.ir.VectorDeinterleaveEvenLower(esize, n, m))
                              : (Q ? v.ir.VectorDeinterleaveOdd(esize, n, m) : v.ir.VectorDeinterleaveOddLower(esize, n, m));

    v.V(datasize, Vd, result);
    return true;
}
}  // Anonymous namespace

bool TranslatorVisitor::TRN1(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return VectorTranspose(*this, Q, size, Vm, Vn, Vd, Transposition::TRN1);
}

bool TranslatorVisitor::TRN2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return VectorTranspose(*this, Q, size, Vm, Vn, Vd, Transposition::TRN2);
}

bool TranslatorVisitor::UZP1(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return VectorUnzip(*this, Q, size, Vm, Vn, Vd, UnzipType::Even);
}

bool TranslatorVisitor::UZP2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    return VectorUnzip(*this, Q, size, Vm, Vn, Vd, UnzipType::Odd);
}

bool TranslatorVisitor::ZIP1(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = ir.VectorInterleaveLower(esize, operand1, operand2);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::ZIP2(bool Q, Imm<2> size, Vec Vm, Vec Vn, Vec Vd) {
    if (size == 0b11 && !Q) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend<size_t>();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 result = [&] {
        if (Q) {
            return ir.VectorInterleaveUpper(esize, operand1, operand2);
        }

        // TODO: Urgh.
        const IR::U128 interleaved = ir.VectorInterleaveLower(esize, operand1, operand2);
        return ir.VectorZeroUpper(ir.VectorShuffleWords(interleaved, 0b01001110));
    }();

    V(datasize, Vd, result);
    return true;
}

}  // namespace Dynarmic::A64

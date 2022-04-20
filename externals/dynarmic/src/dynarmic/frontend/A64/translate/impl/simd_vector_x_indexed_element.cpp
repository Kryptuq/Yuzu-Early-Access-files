/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <utility>

#include <mcl/assert.hpp>

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
std::pair<size_t, Vec> Combine(Imm<2> size, Imm<1> H, Imm<1> L, Imm<1> M, Imm<4> Vmlo) {
    if (size == 0b01) {
        return {concatenate(H, L, M).ZeroExtend(), Vmlo.ZeroExtend<Vec>()};
    }

    return {concatenate(H, L).ZeroExtend(), concatenate(M, Vmlo).ZeroExtend<Vec>()};
}

enum class ExtraBehavior {
    None,
    Extended,
    Accumulate,
    Subtract,
};

bool MultiplyByElement(TranslatorVisitor& v, bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd, ExtraBehavior extra_behavior) {
    if (size != 0b01 && size != 0b10) {
        return v.ReservedValue();
    }

    const auto [index, Vm] = Combine(size, H, L, M, Vmlo);
    const size_t idxdsize = H == 1 ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = v.ir.VectorBroadcastElement(esize, v.V(idxdsize, Vm), index);
    const IR::U128 operand3 = v.V(datasize, Vd);

    IR::U128 result = v.ir.VectorMultiply(esize, operand1, operand2);
    if (extra_behavior == ExtraBehavior::Accumulate) {
        result = v.ir.VectorAdd(esize, operand3, result);
    } else if (extra_behavior == ExtraBehavior::Subtract) {
        result = v.ir.VectorSub(esize, operand3, result);
    }

    v.V(datasize, Vd, result);
    return true;
}

bool FPMultiplyByElement(TranslatorVisitor& v, bool Q, bool sz, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd, ExtraBehavior extra_behavior) {
    if (sz && L == 1) {
        return v.ReservedValue();
    }
    if (sz && !Q) {
        return v.ReservedValue();
    }

    const size_t idxdsize = H == 1 ? 128 : 64;
    const size_t index = sz ? H.ZeroExtend() : concatenate(H, L).ZeroExtend();
    const Vec Vm = concatenate(M, Vmlo).ZeroExtend<Vec>();
    const size_t esize = sz ? 64 : 32;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = Q ? v.ir.VectorBroadcastElement(esize, v.V(idxdsize, Vm), index) : v.ir.VectorBroadcastElementLower(esize, v.V(idxdsize, Vm), index);
    const IR::U128 operand3 = v.V(datasize, Vd);

    const IR::U128 result = [&] {
        switch (extra_behavior) {
        case ExtraBehavior::None:
            return v.ir.FPVectorMul(esize, operand1, operand2);
        case ExtraBehavior::Extended:
            return v.ir.FPVectorMulX(esize, operand1, operand2);
        case ExtraBehavior::Accumulate:
            return v.ir.FPVectorMulAdd(esize, operand3, operand1, operand2);
        case ExtraBehavior::Subtract:
            return v.ir.FPVectorMulAdd(esize, operand3, v.ir.FPVectorNeg(esize, operand1), operand2);
        }
        UNREACHABLE();
    }();
    v.V(datasize, Vd, result);
    return true;
}

bool FPMultiplyByElementHalfPrecision(TranslatorVisitor& v, bool Q, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd, ExtraBehavior extra_behavior) {
    const size_t idxdsize = H == 1 ? 128 : 64;
    const size_t index = concatenate(H, L, M).ZeroExtend();
    const Vec Vm = Vmlo.ZeroExtend<Vec>();
    const size_t esize = 16;
    const size_t datasize = Q ? 128 : 64;

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = Q ? v.ir.VectorBroadcastElement(esize, v.V(idxdsize, Vm), index) : v.ir.VectorBroadcastElementLower(esize, v.V(idxdsize, Vm), index);
    const IR::U128 operand3 = v.V(datasize, Vd);

    // TODO: We currently don't implement half-precision paths for
    //       regular multiplies and extended multiplies.
    const IR::U128 result = [&] {
        switch (extra_behavior) {
        case ExtraBehavior::None:
            break;
        case ExtraBehavior::Extended:
            break;
        case ExtraBehavior::Accumulate:
            return v.ir.FPVectorMulAdd(esize, operand3, operand1, operand2);
        case ExtraBehavior::Subtract:
            return v.ir.FPVectorMulAdd(esize, operand3, v.ir.FPVectorNeg(esize, operand1), operand2);
        }
        UNREACHABLE();
    }();
    v.V(datasize, Vd, result);
    return true;
}

using ExtensionFunction = IR::U32 (IREmitter::*)(const IR::UAny&);

bool DotProduct(TranslatorVisitor& v, bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd, ExtensionFunction extension) {
    if (size != 0b10) {
        return v.ReservedValue();
    }

    const Vec Vm = concatenate(M, Vmlo).ZeroExtend<Vec>();
    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;
    const size_t elements = datasize / esize;
    const size_t index = concatenate(H, L).ZeroExtend();

    const IR::U128 operand1 = v.V(datasize, Vn);
    const IR::U128 operand2 = v.V(128, Vm);
    IR::U128 result = v.V(datasize, Vd);

    for (size_t i = 0; i < elements; i++) {
        IR::U32 res_element = v.ir.Imm32(0);

        for (size_t j = 0; j < 4; j++) {
            const IR::U32 elem1 = (v.ir.*extension)(v.ir.VectorGetElement(8, operand1, 4 * i + j));
            const IR::U32 elem2 = (v.ir.*extension)(v.ir.VectorGetElement(8, operand2, 4 * index + j));

            res_element = v.ir.Add(res_element, v.ir.Mul(elem1, elem2));
        }

        res_element = v.ir.Add(v.ir.VectorGetElement(32, result, i), res_element);
        result = v.ir.VectorSetElement(32, result, i, res_element);
    }

    v.V(datasize, Vd, result);
    return true;
}

enum class Signedness {
    Signed,
    Unsigned
};

bool MultiplyLong(TranslatorVisitor& v, bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd, ExtraBehavior extra_behavior, Signedness sign) {
    if (size == 0b00 || size == 0b11) {
        return v.ReservedValue();
    }

    const size_t idxsize = H == 1 ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = 64;
    const auto [index, Vm] = Combine(size, H, L, M, Vmlo);

    const auto extend_operands = [&](const IR::U128& lhs, const IR::U128& rhs) {
        if (sign == Signedness::Signed) {
            return std::make_pair(v.ir.VectorSignExtend(esize, lhs),
                                  v.ir.VectorSignExtend(esize, rhs));
        }

        return std::make_pair(v.ir.VectorZeroExtend(esize, lhs),
                              v.ir.VectorZeroExtend(esize, rhs));
    };

    const IR::U128 operand1 = v.Vpart(datasize, Vn, Q);
    const IR::U128 operand2 = v.V(idxsize, Vm);
    const IR::U128 index_vector = v.ir.VectorBroadcastElement(esize, operand2, index);

    const IR::U128 result = [&] {
        const auto [extended_op1, extended_index] = extend_operands(operand1, index_vector);
        const IR::U128 product = v.ir.VectorMultiply(2 * esize, extended_op1, extended_index);
        if (extra_behavior == ExtraBehavior::None) {
            return product;
        }

        const IR::U128 operand3 = v.V(2 * datasize, Vd);
        if (extra_behavior == ExtraBehavior::Accumulate) {
            return v.ir.VectorAdd(2 * esize, operand3, product);
        }

        return v.ir.VectorSub(2 * esize, operand3, product);
    }();

    v.V(2 * datasize, Vd, result);
    return true;
}
}  // Anonymous namespace

bool TranslatorVisitor::MLA_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyByElement(*this, Q, size, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Accumulate);
}

bool TranslatorVisitor::MLS_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyByElement(*this, Q, size, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Subtract);
}

bool TranslatorVisitor::MUL_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyByElement(*this, Q, size, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::None);
}

bool TranslatorVisitor::FCMLA_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<2> rot, Imm<1> H, Vec Vn, Vec Vd) {
    if (size == 0b00 || size == 0b11) {
        return ReservedValue();
    }

    if (size == 0b01 && H == 1 && Q == 0) {
        return ReservedValue();
    }

    if (size == 0b10 && (L == 1 || Q == 0)) {
        return ReservedValue();
    }

    const size_t esize = 8U << size.ZeroExtend();

    // TODO: We don't support the half-precision floating point variant yet.
    if (esize == 16) {
        return InterpretThisInstruction();
    }

    const size_t index = [=] {
        if (size == 0b01) {
            return concatenate(H, L).ZeroExtend();
        }
        return H.ZeroExtend();
    }();

    const Vec Vm = concatenate(M, Vmlo).ZeroExtend<Vec>();

    const size_t datasize = Q ? 128 : 64;
    const size_t num_elements = datasize / esize;
    const size_t num_iterations = num_elements / 2;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vm);
    const IR::U128 operand3 = V(datasize, Vd);
    IR::U128 result = ir.ZeroVector();

    IR::U32U64 element1;
    IR::U32U64 element2;
    IR::U32U64 element3;
    IR::U32U64 element4;
    for (size_t e = 0; e < num_iterations; ++e) {
        const size_t first = e * 2;
        const size_t second = first + 1;

        const size_t index_first = index * 2;
        const size_t index_second = index_first + 1;

        switch (rot.ZeroExtend()) {
        case 0b00:  // 0 degrees
            element1 = ir.VectorGetElement(esize, operand2, index_first);
            element2 = ir.VectorGetElement(esize, operand1, first);
            element3 = ir.VectorGetElement(esize, operand2, index_second);
            element4 = ir.VectorGetElement(esize, operand1, first);
            break;
        case 0b01:  // 90 degrees
            element1 = ir.FPNeg(ir.VectorGetElement(esize, operand2, index_second));
            element2 = ir.VectorGetElement(esize, operand1, second);
            element3 = ir.VectorGetElement(esize, operand2, index_first);
            element4 = ir.VectorGetElement(esize, operand1, second);
            break;
        case 0b10:  // 180 degrees
            element1 = ir.FPNeg(ir.VectorGetElement(esize, operand2, index_first));
            element2 = ir.VectorGetElement(esize, operand1, first);
            element3 = ir.FPNeg(ir.VectorGetElement(esize, operand2, index_second));
            element4 = ir.VectorGetElement(esize, operand1, first);
            break;
        case 0b11:  // 270 degrees
            element1 = ir.VectorGetElement(esize, operand2, index_second);
            element2 = ir.VectorGetElement(esize, operand1, second);
            element3 = ir.FPNeg(ir.VectorGetElement(esize, operand2, index_first));
            element4 = ir.VectorGetElement(esize, operand1, second);
            break;
        }

        const IR::U32U64 operand3_elem1 = ir.VectorGetElement(esize, operand3, first);
        const IR::U32U64 operand3_elem2 = ir.VectorGetElement(esize, operand3, second);

        result = ir.VectorSetElement(esize, result, first, ir.FPMulAdd(operand3_elem1, element2, element1));
        result = ir.VectorSetElement(esize, result, second, ir.FPMulAdd(operand3_elem2, element4, element3));
    }

    ir.SetQ(Vd, result);
    return true;
}

bool TranslatorVisitor::FMLA_elt_3(bool Q, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return FPMultiplyByElementHalfPrecision(*this, Q, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Accumulate);
}

bool TranslatorVisitor::FMLA_elt_4(bool Q, bool sz, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return FPMultiplyByElement(*this, Q, sz, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Accumulate);
}

bool TranslatorVisitor::FMLS_elt_3(bool Q, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return FPMultiplyByElementHalfPrecision(*this, Q, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Subtract);
}

bool TranslatorVisitor::FMLS_elt_4(bool Q, bool sz, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return FPMultiplyByElement(*this, Q, sz, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Subtract);
}

bool TranslatorVisitor::FMUL_elt_4(bool Q, bool sz, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return FPMultiplyByElement(*this, Q, sz, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::None);
}

bool TranslatorVisitor::FMULX_elt_4(bool Q, bool sz, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return FPMultiplyByElement(*this, Q, sz, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Extended);
}

bool TranslatorVisitor::SMLAL_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyLong(*this, Q, size, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Accumulate, Signedness::Signed);
}

bool TranslatorVisitor::SMLSL_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyLong(*this, Q, size, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Subtract, Signedness::Signed);
}

bool TranslatorVisitor::SMULL_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyLong(*this, Q, size, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::None, Signedness::Signed);
}

bool TranslatorVisitor::SQDMULL_elt_2(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    if (size == 0b00 || size == 0b11) {
        return ReservedValue();
    }

    const size_t part = Q ? 1 : 0;
    const size_t idxsize = H == 1 ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = 64;
    const auto [index, Vm] = Combine(size, H, L, M, Vmlo);

    const IR::U128 operand1 = Vpart(datasize, Vn, part);
    const IR::U128 operand2 = V(idxsize, Vm);
    const IR::U128 index_vector = ir.VectorBroadcastElement(esize, operand2, index);
    const IR::U128 result = ir.VectorSignedSaturatedDoublingMultiplyLong(esize, operand1, index_vector);

    V(128, Vd, result);
    return true;
}

bool TranslatorVisitor::SQDMULH_elt_2(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    if (size == 0b00 || size == 0b11) {
        return ReservedValue();
    }

    const size_t idxsize = H == 1 ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;
    const auto [index, Vm] = Combine(size, H, L, M, Vmlo);

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(idxsize, Vm);
    const IR::U128 index_vector = ir.VectorBroadcastElement(esize, operand2, index);
    const IR::U128 result = ir.VectorSignedSaturatedDoublingMultiply(esize, operand1, index_vector).upper;

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SQRDMULH_elt_2(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    if (size == 0b00 || size == 0b11) {
        return ReservedValue();
    }

    const size_t idxsize = H == 1 ? 128 : 64;
    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;
    const auto [index, Vm] = Combine(size, H, L, M, Vmlo);

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(idxsize, Vm);
    const IR::U128 index_vector = ir.VectorBroadcastElement(esize, operand2, index);
    const IR::UpperAndLower multiply = ir.VectorSignedSaturatedDoublingMultiply(esize, operand1, index_vector);
    const IR::U128 result = ir.VectorAdd(esize, multiply.upper, ir.VectorLogicalShiftRight(esize, multiply.lower, static_cast<u8>(esize - 1)));

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SDOT_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return DotProduct(*this, Q, size, L, M, Vmlo, H, Vn, Vd, &IREmitter::SignExtendToWord);
}

bool TranslatorVisitor::UDOT_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return DotProduct(*this, Q, size, L, M, Vmlo, H, Vn, Vd, &IREmitter::ZeroExtendToWord);
}

bool TranslatorVisitor::UMLAL_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyLong(*this, Q, size, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Accumulate, Signedness::Unsigned);
}

bool TranslatorVisitor::UMLSL_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyLong(*this, Q, size, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::Subtract, Signedness::Unsigned);
}

bool TranslatorVisitor::UMULL_elt(bool Q, Imm<2> size, Imm<1> L, Imm<1> M, Imm<4> Vmlo, Imm<1> H, Vec Vn, Vec Vd) {
    return MultiplyLong(*this, Q, size, L, M, Vmlo, H, Vn, Vd, ExtraBehavior::None, Signedness::Unsigned);
}

}  // namespace Dynarmic::A64

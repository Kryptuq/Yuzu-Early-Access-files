/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
enum class Signedness {
    Signed,
    Unsigned
};

bool LongAdd(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vn, Vec Vd, Signedness sign) {
    if ((size == 0b10 && !Q) || size == 0b11) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;
    const size_t elements = datasize / esize;

    const IR::U128 operand = v.V(datasize, Vn);

    const auto get_element = [&](IR::U128 vec, size_t element) {
        const auto vec_element = v.ir.VectorGetElement(esize, vec, element);

        if (sign == Signedness::Signed) {
            return v.ir.SignExtendToLong(vec_element);
        }

        return v.ir.ZeroExtendToLong(vec_element);
    };

    IR::U64 sum = get_element(operand, 0);
    for (size_t i = 1; i < elements; i++) {
        sum = v.ir.Add(sum, get_element(operand, i));
    }

    if (size == 0b00) {
        v.V(datasize, Vd, v.ir.ZeroExtendToQuad(v.ir.LeastSignificantHalf(sum)));
    } else if (size == 0b01) {
        v.V(datasize, Vd, v.ir.ZeroExtendToQuad(v.ir.LeastSignificantWord(sum)));
    } else {
        v.V(datasize, Vd, v.ir.ZeroExtendToQuad(sum));
    }

    return true;
}

enum class MinMaxOperation {
    Max,
    MaxNumeric,
    Min,
    MinNumeric,
};

bool FPMinMax(TranslatorVisitor& v, bool Q, bool sz, Vec Vn, Vec Vd, MinMaxOperation operation) {
    if (!Q || sz) {
        return v.ReservedValue();
    }

    const size_t esize = 32;
    const size_t datasize = 128;
    const size_t elements = datasize / esize;

    const IR::U128 operand = v.V(datasize, Vn);

    const auto op = [&](const IR::U32U64& lhs, const IR::U32U64& rhs) {
        switch (operation) {
        case MinMaxOperation::Max:
            return v.ir.FPMax(lhs, rhs);
        case MinMaxOperation::MaxNumeric:
            return v.ir.FPMaxNumeric(lhs, rhs);
        case MinMaxOperation::Min:
            return v.ir.FPMin(lhs, rhs);
        case MinMaxOperation::MinNumeric:
            return v.ir.FPMinNumeric(lhs, rhs);
        default:
            UNREACHABLE();
        }
    };

    const auto reduce = [&](size_t start, size_t end) {
        IR::U32U64 result = v.ir.VectorGetElement(esize, operand, start);

        for (size_t i = start + 1; i < end; i++) {
            const IR::U32U64 element = v.ir.VectorGetElement(esize, operand, i);

            result = op(result, element);
        }

        return result;
    };

    const IR::U32U64 hi = reduce(elements / 2, elements);
    const IR::U32U64 lo = reduce(0, elements / 2);
    const IR::U32U64 result = op(lo, hi);

    v.V_scalar(esize, Vd, result);
    return true;
}

enum class ScalarMinMaxOperation {
    Max,
    Min,
};

bool ScalarMinMax(TranslatorVisitor& v, bool Q, Imm<2> size, Vec Vn, Vec Vd, ScalarMinMaxOperation operation, Signedness sign) {
    if ((size == 0b10 && !Q) || size == 0b11) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;
    const size_t elements = datasize / esize;

    const auto get_element = [&](IR::U128 vec, size_t element) {
        const auto vec_element = v.ir.VectorGetElement(esize, vec, element);

        if (sign == Signedness::Signed) {
            return v.ir.SignExtendToWord(vec_element);
        }

        return v.ir.ZeroExtendToWord(vec_element);
    };

    const auto op_func = [&](const auto& a, const auto& b) {
        switch (operation) {
        case ScalarMinMaxOperation::Max:
            if (sign == Signedness::Signed) {
                return v.ir.MaxSigned(a, b);
            }
            return v.ir.MaxUnsigned(a, b);

        case ScalarMinMaxOperation::Min:
            if (sign == Signedness::Signed) {
                return v.ir.MinSigned(a, b);
            }
            return v.ir.MinUnsigned(a, b);

        default:
            UNREACHABLE();
        }
    };

    const IR::U128 operand = v.V(datasize, Vn);

    IR::U32 value = get_element(operand, 0);
    for (size_t i = 1; i < elements; i++) {
        value = op_func(value, get_element(operand, i));
    }

    if (size == 0b00) {
        v.V(datasize, Vd, v.ir.ZeroExtendToQuad(v.ir.LeastSignificantByte(value)));
    } else if (size == 0b01) {
        v.V(datasize, Vd, v.ir.ZeroExtendToQuad(v.ir.LeastSignificantHalf(value)));
    } else {
        v.V(datasize, Vd, v.ir.ZeroExtendToQuad(value));
    }

    return true;
}
}  // Anonymous namespace

bool TranslatorVisitor::ADDV(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    if ((size == 0b10 && !Q) || size == 0b11) {
        return ReservedValue();
    }

    const size_t esize = 8 << size.ZeroExtend();
    const size_t datasize = Q ? 128 : 64;
    const size_t elements = datasize / esize;

    const IR::U128 operand = V(datasize, Vn);

    const auto get_element = [&](IR::U128 vec, size_t element) {
        return ir.ZeroExtendToWord(ir.VectorGetElement(esize, vec, element));
    };

    IR::U32 sum = get_element(operand, 0);
    for (size_t i = 1; i < elements; i++) {
        sum = ir.Add(sum, get_element(operand, i));
    }

    if (size == 0b00) {
        V(datasize, Vd, ir.ZeroExtendToQuad(ir.LeastSignificantByte(sum)));
    } else if (size == 0b01) {
        V(datasize, Vd, ir.ZeroExtendToQuad(ir.LeastSignificantHalf(sum)));
    } else {
        V(datasize, Vd, ir.ZeroExtendToQuad(sum));
    }

    return true;
}

bool TranslatorVisitor::FMAXNMV_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FPMinMax(*this, Q, sz, Vn, Vd, MinMaxOperation::MaxNumeric);
}

bool TranslatorVisitor::FMAXV_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FPMinMax(*this, Q, sz, Vn, Vd, MinMaxOperation::Max);
}

bool TranslatorVisitor::FMINNMV_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FPMinMax(*this, Q, sz, Vn, Vd, MinMaxOperation::MinNumeric);
}

bool TranslatorVisitor::FMINV_2(bool Q, bool sz, Vec Vn, Vec Vd) {
    return FPMinMax(*this, Q, sz, Vn, Vd, MinMaxOperation::Min);
}

bool TranslatorVisitor::SADDLV(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return LongAdd(*this, Q, size, Vn, Vd, Signedness::Signed);
}

bool TranslatorVisitor::SMAXV(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return ScalarMinMax(*this, Q, size, Vn, Vd, ScalarMinMaxOperation::Max, Signedness::Signed);
}

bool TranslatorVisitor::SMINV(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return ScalarMinMax(*this, Q, size, Vn, Vd, ScalarMinMaxOperation::Min, Signedness::Signed);
}

bool TranslatorVisitor::UADDLV(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return LongAdd(*this, Q, size, Vn, Vd, Signedness::Unsigned);
}

bool TranslatorVisitor::UMAXV(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return ScalarMinMax(*this, Q, size, Vn, Vd, ScalarMinMaxOperation::Max, Signedness::Unsigned);
}

bool TranslatorVisitor::UMINV(bool Q, Imm<2> size, Vec Vn, Vec Vd) {
    return ScalarMinMax(*this, Q, size, Vn, Vd, ScalarMinMaxOperation::Min, Signedness::Unsigned);
}
}  // namespace Dynarmic::A64

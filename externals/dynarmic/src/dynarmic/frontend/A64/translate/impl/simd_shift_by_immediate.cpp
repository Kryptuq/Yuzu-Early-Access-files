/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/fp/rounding_mode.h"
#include "dynarmic/frontend/A64/translate/impl/impl.h"

namespace Dynarmic::A64 {
namespace {
enum class Rounding {
    None,
    Round
};

enum class Accumulating {
    None,
    Accumulate
};

enum class Signedness {
    Signed,
    Unsigned
};

enum class Narrowing {
    Truncation,
    SaturateToUnsigned,
    SaturateToSigned,
};

enum class SaturatingShiftLeftType {
    Signed,
    Unsigned,
    SignedWithUnsignedSaturation,
};

enum class FloatConversionDirection {
    FixedToFloat,
    FloatToFixed,
};

IR::U128 PerformRoundingCorrection(TranslatorVisitor& v, size_t esize, u64 round_value, IR::U128 original, IR::U128 shifted) {
    const IR::U128 round_const = v.ir.VectorBroadcast(esize, v.I(esize, round_value));
    const IR::U128 round_correction = v.ir.VectorEqual(esize, v.ir.VectorAnd(original, round_const), round_const);
    return v.ir.VectorSub(esize, shifted, round_correction);
}

bool ShiftRight(TranslatorVisitor& v, bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd, Rounding rounding, Accumulating accumulating, Signedness signedness) {
    if (immh == 0b0000) {
        return v.DecodeError();
    }

    if (immh.Bit<3>() && !Q) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << Common::HighestSetBit(immh.ZeroExtend());
    const size_t datasize = Q ? 128 : 64;

    const u8 shift_amount = static_cast<u8>(2 * esize) - concatenate(immh, immb).ZeroExtend<u8>();

    const IR::U128 operand = v.V(datasize, Vn);

    IR::U128 result = [&] {
        if (signedness == Signedness::Signed) {
            return v.ir.VectorArithmeticShiftRight(esize, operand, shift_amount);
        }
        return v.ir.VectorLogicalShiftRight(esize, operand, shift_amount);
    }();

    if (rounding == Rounding::Round) {
        const u64 round_value = 1ULL << (shift_amount - 1);
        result = PerformRoundingCorrection(v, esize, round_value, operand, result);
    }

    if (accumulating == Accumulating::Accumulate) {
        const IR::U128 accumulator = v.V(datasize, Vd);
        result = v.ir.VectorAdd(esize, result, accumulator);
    }

    v.V(datasize, Vd, result);
    return true;
}

bool ShiftRightNarrowing(TranslatorVisitor& v, bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd, Rounding rounding, Narrowing narrowing, Signedness signedness) {
    if (immh == 0b0000) {
        return v.DecodeError();
    }

    if (immh.Bit<3>()) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << Common::HighestSetBit(immh.ZeroExtend());
    const size_t source_esize = 2 * esize;
    const size_t part = Q ? 1 : 0;

    const u8 shift_amount = static_cast<u8>(source_esize - concatenate(immh, immb).ZeroExtend());

    const IR::U128 operand = v.V(128, Vn);

    IR::U128 wide_result = [&] {
        if (signedness == Signedness::Signed) {
            return v.ir.VectorArithmeticShiftRight(source_esize, operand, shift_amount);
        }
        return v.ir.VectorLogicalShiftRight(source_esize, operand, shift_amount);
    }();

    if (rounding == Rounding::Round) {
        const u64 round_value = 1ULL << (shift_amount - 1);
        wide_result = PerformRoundingCorrection(v, source_esize, round_value, operand, wide_result);
    }

    const IR::U128 result = [&] {
        switch (narrowing) {
        case Narrowing::Truncation:
            return v.ir.VectorNarrow(source_esize, wide_result);
        case Narrowing::SaturateToUnsigned:
            if (signedness == Signedness::Signed) {
                return v.ir.VectorSignedSaturatedNarrowToUnsigned(source_esize, wide_result);
            }
            return v.ir.VectorUnsignedSaturatedNarrow(source_esize, wide_result);
        case Narrowing::SaturateToSigned:
            ASSERT(signedness == Signedness::Signed);
            return v.ir.VectorSignedSaturatedNarrowToSigned(source_esize, wide_result);
        }
        UNREACHABLE();
    }();

    v.Vpart(64, Vd, part, result);
    return true;
}

bool ShiftLeftLong(TranslatorVisitor& v, bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd, Signedness signedness) {
    if (immh == 0b0000) {
        return v.DecodeError();
    }

    if (immh.Bit<3>()) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << Common::HighestSetBit(immh.ZeroExtend());
    const size_t datasize = 64;
    const size_t part = Q ? 1 : 0;

    const u8 shift_amount = concatenate(immh, immb).ZeroExtend<u8>() - static_cast<u8>(esize);

    const IR::U128 operand = v.Vpart(datasize, Vn, part);
    const IR::U128 expanded_operand = [&] {
        if (signedness == Signedness::Signed) {
            return v.ir.VectorSignExtend(esize, operand);
        }
        return v.ir.VectorZeroExtend(esize, operand);
    }();
    const IR::U128 result = v.ir.VectorLogicalShiftLeft(2 * esize, expanded_operand, shift_amount);

    v.V(2 * datasize, Vd, result);
    return true;
}

bool SaturatingShiftLeft(TranslatorVisitor& v, bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd, SaturatingShiftLeftType type) {
    if (!Q && immh.Bit<3>()) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << Common::HighestSetBit(immh.ZeroExtend());
    const size_t datasize = Q ? 128 : 64;
    const size_t shift = concatenate(immh, immb).ZeroExtend() - esize;

    const IR::U128 operand = v.V(datasize, Vn);
    const IR::U128 shift_vec = v.ir.VectorBroadcast(esize, v.I(esize, shift));
    const IR::U128 result = [&] {
        if (type == SaturatingShiftLeftType::Signed) {
            return v.ir.VectorSignedSaturatedShiftLeft(esize, operand, shift_vec);
        }

        if (type == SaturatingShiftLeftType::Unsigned) {
            return v.ir.VectorUnsignedSaturatedShiftLeft(esize, operand, shift_vec);
        }

        return v.ir.VectorSignedSaturatedShiftLeftUnsigned(esize, operand, shift_vec);
    }();

    v.V(datasize, Vd, result);
    return true;
}

bool ConvertFloat(TranslatorVisitor& v, bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd, Signedness signedness, FloatConversionDirection direction, FP::RoundingMode rounding_mode) {
    if (immh == 0b0000) {
        return v.DecodeError();
    }

    if (immh == 0b0001 || immh == 0b0010 || immh == 0b0011) {
        return v.ReservedValue();
    }

    if (immh.Bit<3>() && !Q) {
        return v.ReservedValue();
    }

    const size_t esize = 8 << Common::HighestSetBit(immh.ZeroExtend());
    const size_t datasize = Q ? 128 : 64;

    const u8 fbits = static_cast<u8>(esize * 2) - concatenate(immh, immb).ZeroExtend<u8>();

    const IR::U128 operand = v.V(datasize, Vn);
    const IR::U128 result = [&] {
        switch (direction) {
        case FloatConversionDirection::FixedToFloat:
            return signedness == Signedness::Signed
                     ? v.ir.FPVectorFromSignedFixed(esize, operand, fbits, rounding_mode)
                     : v.ir.FPVectorFromUnsignedFixed(esize, operand, fbits, rounding_mode);
        case FloatConversionDirection::FloatToFixed:
            return signedness == Signedness::Signed
                     ? v.ir.FPVectorToSignedFixed(esize, operand, fbits, rounding_mode)
                     : v.ir.FPVectorToUnsignedFixed(esize, operand, fbits, rounding_mode);
        }
        UNREACHABLE();
    }();

    v.V(datasize, Vd, result);
    return true;
}

}  // Anonymous namespace

bool TranslatorVisitor::SSHR_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRight(*this, Q, immh, immb, Vn, Vd, Rounding::None, Accumulating::None, Signedness::Signed);
}

bool TranslatorVisitor::SRSHR_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRight(*this, Q, immh, immb, Vn, Vd, Rounding::Round, Accumulating::None, Signedness::Signed);
}

bool TranslatorVisitor::SRSRA_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRight(*this, Q, immh, immb, Vn, Vd, Rounding::Round, Accumulating::Accumulate, Signedness::Signed);
}

bool TranslatorVisitor::SSRA_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRight(*this, Q, immh, immb, Vn, Vd, Rounding::None, Accumulating::Accumulate, Signedness::Signed);
}

bool TranslatorVisitor::SHL_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    if (immh == 0b0000) {
        return DecodeError();
    }
    if (immh.Bit<3>() && !Q) {
        return ReservedValue();
    }
    const size_t esize = 8 << Common::HighestSetBit(immh.ZeroExtend());
    const size_t datasize = Q ? 128 : 64;

    const u8 shift_amount = concatenate(immh, immb).ZeroExtend<u8>() - static_cast<u8>(esize);

    const IR::U128 operand = V(datasize, Vn);
    const IR::U128 result = ir.VectorLogicalShiftLeft(esize, operand, shift_amount);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SHRN(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRightNarrowing(*this, Q, immh, immb, Vn, Vd, Rounding::None, Narrowing::Truncation, Signedness::Unsigned);
}

bool TranslatorVisitor::RSHRN(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRightNarrowing(*this, Q, immh, immb, Vn, Vd, Rounding::Round, Narrowing::Truncation, Signedness::Unsigned);
}

bool TranslatorVisitor::SQSHL_imm_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return SaturatingShiftLeft(*this, Q, immh, immb, Vn, Vd, SaturatingShiftLeftType::Signed);
}

bool TranslatorVisitor::SQSHLU_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return SaturatingShiftLeft(*this, Q, immh, immb, Vn, Vd, SaturatingShiftLeftType::SignedWithUnsignedSaturation);
}

bool TranslatorVisitor::SQSHRN_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRightNarrowing(*this, Q, immh, immb, Vn, Vd, Rounding::None, Narrowing::SaturateToSigned, Signedness::Signed);
}

bool TranslatorVisitor::SQRSHRN_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRightNarrowing(*this, Q, immh, immb, Vn, Vd, Rounding::Round, Narrowing::SaturateToSigned, Signedness::Signed);
}

bool TranslatorVisitor::SQSHRUN_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRightNarrowing(*this, Q, immh, immb, Vn, Vd, Rounding::None, Narrowing::SaturateToUnsigned, Signedness::Signed);
}

bool TranslatorVisitor::SQRSHRUN_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRightNarrowing(*this, Q, immh, immb, Vn, Vd, Rounding::Round, Narrowing::SaturateToUnsigned, Signedness::Signed);
}

bool TranslatorVisitor::UQSHL_imm_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return SaturatingShiftLeft(*this, Q, immh, immb, Vn, Vd, SaturatingShiftLeftType::Unsigned);
}

bool TranslatorVisitor::UQSHRN_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRightNarrowing(*this, Q, immh, immb, Vn, Vd, Rounding::None, Narrowing::SaturateToUnsigned, Signedness::Unsigned);
}

bool TranslatorVisitor::UQRSHRN_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRightNarrowing(*this, Q, immh, immb, Vn, Vd, Rounding::Round, Narrowing::SaturateToUnsigned, Signedness::Unsigned);
}

bool TranslatorVisitor::SSHLL(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftLeftLong(*this, Q, immh, immb, Vn, Vd, Signedness::Signed);
}

bool TranslatorVisitor::URSHR_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRight(*this, Q, immh, immb, Vn, Vd, Rounding::Round, Accumulating::None, Signedness::Unsigned);
}

bool TranslatorVisitor::URSRA_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRight(*this, Q, immh, immb, Vn, Vd, Rounding::Round, Accumulating::Accumulate, Signedness::Unsigned);
}

bool TranslatorVisitor::USHR_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRight(*this, Q, immh, immb, Vn, Vd, Rounding::None, Accumulating::None, Signedness::Unsigned);
}

bool TranslatorVisitor::USRA_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftRight(*this, Q, immh, immb, Vn, Vd, Rounding::None, Accumulating::Accumulate, Signedness::Unsigned);
}

bool TranslatorVisitor::USHLL(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ShiftLeftLong(*this, Q, immh, immb, Vn, Vd, Signedness::Unsigned);
}

bool TranslatorVisitor::SRI_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    if (immh == 0b0000) {
        return DecodeError();
    }

    if (!Q && immh.Bit<3>()) {
        return ReservedValue();
    }

    const size_t esize = 8 << Common::HighestSetBit(immh.ZeroExtend());
    const size_t datasize = Q ? 128 : 64;

    const u8 shift_amount = static_cast<u8>((esize * 2) - concatenate(immh, immb).ZeroExtend<u8>());
    const u64 mask = shift_amount == esize ? 0 : Common::Ones<u64>(esize) >> shift_amount;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vd);

    const IR::U128 shifted = ir.VectorLogicalShiftRight(esize, operand1, shift_amount);
    const IR::U128 mask_vec = ir.VectorBroadcast(esize, I(esize, mask));
    const IR::U128 result = ir.VectorOr(ir.VectorAndNot(operand2, mask_vec), shifted);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SLI_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    if (immh == 0b0000) {
        return DecodeError();
    }

    if (!Q && immh.Bit<3>()) {
        return ReservedValue();
    }

    const size_t esize = 8 << Common::HighestSetBit(immh.ZeroExtend());
    const size_t datasize = Q ? 128 : 64;

    const u8 shift_amount = concatenate(immh, immb).ZeroExtend<u8>() - static_cast<u8>(esize);
    const u64 mask = Common::Ones<u64>(esize) << shift_amount;

    const IR::U128 operand1 = V(datasize, Vn);
    const IR::U128 operand2 = V(datasize, Vd);

    const IR::U128 shifted = ir.VectorLogicalShiftLeft(esize, operand1, shift_amount);
    const IR::U128 mask_vec = ir.VectorBroadcast(esize, I(esize, mask));
    const IR::U128 result = ir.VectorOr(ir.VectorAndNot(operand2, mask_vec), shifted);

    V(datasize, Vd, result);
    return true;
}

bool TranslatorVisitor::SCVTF_fix_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ConvertFloat(*this, Q, immh, immb, Vn, Vd, Signedness::Signed, FloatConversionDirection::FixedToFloat, ir.current_location->FPCR().RMode());
}

bool TranslatorVisitor::UCVTF_fix_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ConvertFloat(*this, Q, immh, immb, Vn, Vd, Signedness::Unsigned, FloatConversionDirection::FixedToFloat, ir.current_location->FPCR().RMode());
}

bool TranslatorVisitor::FCVTZS_fix_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ConvertFloat(*this, Q, immh, immb, Vn, Vd, Signedness::Signed, FloatConversionDirection::FloatToFixed, FP::RoundingMode::TowardsZero);
}

bool TranslatorVisitor::FCVTZU_fix_2(bool Q, Imm<4> immh, Imm<3> immb, Vec Vn, Vec Vd) {
    return ConvertFloat(*this, Q, immh, immb, Vn, Vd, Signedness::Unsigned, FloatConversionDirection::FloatToFixed, FP::RoundingMode::TowardsZero);
}

}  // namespace Dynarmic::A64

/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/ir/ir_emitter.h"

#include "dynarmic/common/assert.h"
#include "dynarmic/common/cast_util.h"
#include "dynarmic/ir/opcodes.h"

namespace Dynarmic::IR {

U1 IREmitter::Imm1(bool imm1) const {
    return U1(Value(imm1));
}

U8 IREmitter::Imm8(u8 imm8) const {
    return U8(Value(imm8));
}

U16 IREmitter::Imm16(u16 imm16) const {
    return U16(Value(imm16));
}

U32 IREmitter::Imm32(u32 imm32) const {
    return U32(Value(imm32));
}

U64 IREmitter::Imm64(u64 imm64) const {
    return U64(Value(imm64));
}

void IREmitter::PushRSB(const LocationDescriptor& return_location) {
    Inst(Opcode::PushRSB, IR::Value(return_location.Value()));
}

U64 IREmitter::Pack2x32To1x64(const U32& lo, const U32& hi) {
    return Inst<U64>(Opcode::Pack2x32To1x64, lo, hi);
}

U128 IREmitter::Pack2x64To1x128(const U64& lo, const U64& hi) {
    return Inst<U128>(Opcode::Pack2x64To1x128, lo, hi);
}

UAny IREmitter::LeastSignificant(size_t bitsize, const U32U64& value) {
    switch (bitsize) {
    case 8:
        return LeastSignificantByte(value);
    case 16:
        return LeastSignificantHalf(value);
    case 32:
        if (value.GetType() == Type::U32) {
            return value;
        }
        return LeastSignificantWord(value);
    case 64:
        ASSERT(value.GetType() == Type::U64);
        return value;
    }
    ASSERT_FALSE("Invalid bitsize");
}

U32 IREmitter::LeastSignificantWord(const U64& value) {
    return Inst<U32>(Opcode::LeastSignificantWord, value);
}

U16 IREmitter::LeastSignificantHalf(U32U64 value) {
    if (value.GetType() == Type::U64) {
        value = LeastSignificantWord(value);
    }
    return Inst<U16>(Opcode::LeastSignificantHalf, value);
}

U8 IREmitter::LeastSignificantByte(U32U64 value) {
    if (value.GetType() == Type::U64) {
        value = LeastSignificantWord(value);
    }
    return Inst<U8>(Opcode::LeastSignificantByte, value);
}

ResultAndCarry<U32> IREmitter::MostSignificantWord(const U64& value) {
    const auto result = Inst<U32>(Opcode::MostSignificantWord, value);
    const auto carry_out = Inst<U1>(Opcode::GetCarryFromOp, result);
    return {result, carry_out};
}

U1 IREmitter::MostSignificantBit(const U32& value) {
    return Inst<U1>(Opcode::MostSignificantBit, value);
}

U1 IREmitter::IsZero(const U32& value) {
    return Inst<U1>(Opcode::IsZero32, value);
}

U1 IREmitter::IsZero(const U64& value) {
    return Inst<U1>(Opcode::IsZero64, value);
}

U1 IREmitter::IsZero(const U32U64& value) {
    if (value.GetType() == Type::U32) {
        return Inst<U1>(Opcode::IsZero32, value);
    } else {
        return Inst<U1>(Opcode::IsZero64, value);
    }
}

U1 IREmitter::TestBit(const U32U64& value, const U8& bit) {
    if (value.GetType() == Type::U32) {
        return Inst<U1>(Opcode::TestBit, IndeterminateExtendToLong(value), bit);
    } else {
        return Inst<U1>(Opcode::TestBit, value, bit);
    }
}

U32 IREmitter::ConditionalSelect(Cond cond, const U32& a, const U32& b) {
    return Inst<U32>(Opcode::ConditionalSelect32, Value{cond}, a, b);
}

U64 IREmitter::ConditionalSelect(Cond cond, const U64& a, const U64& b) {
    return Inst<U64>(Opcode::ConditionalSelect64, Value{cond}, a, b);
}

NZCV IREmitter::ConditionalSelect(Cond cond, const NZCV& a, const NZCV& b) {
    return Inst<NZCV>(Opcode::ConditionalSelectNZCV, Value{cond}, a, b);
}

U32U64 IREmitter::ConditionalSelect(Cond cond, const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());
    if (a.GetType() == Type::U32) {
        return Inst<U32>(Opcode::ConditionalSelect32, Value{cond}, a, b);
    } else {
        return Inst<U64>(Opcode::ConditionalSelect64, Value{cond}, a, b);
    }
}

NZCV IREmitter::NZCVFromPackedFlags(const U32& a) {
    return Inst<NZCV>(Opcode::NZCVFromPackedFlags, a);
}

NZCV IREmitter::NZCVFrom(const Value& value) {
    return Inst<NZCV>(Opcode::GetNZCVFromOp, value);
}

ResultAndCarry<U32> IREmitter::LogicalShiftLeft(const U32& value_in, const U8& shift_amount, const U1& carry_in) {
    const auto result = Inst<U32>(Opcode::LogicalShiftLeft32, value_in, shift_amount, carry_in);
    const auto carry_out = Inst<U1>(Opcode::GetCarryFromOp, result);
    return {result, carry_out};
}

ResultAndCarry<U32> IREmitter::LogicalShiftRight(const U32& value_in, const U8& shift_amount, const U1& carry_in) {
    const auto result = Inst<U32>(Opcode::LogicalShiftRight32, value_in, shift_amount, carry_in);
    const auto carry_out = Inst<U1>(Opcode::GetCarryFromOp, result);
    return {result, carry_out};
}

ResultAndCarry<U32> IREmitter::ArithmeticShiftRight(const U32& value_in, const U8& shift_amount, const U1& carry_in) {
    const auto result = Inst<U32>(Opcode::ArithmeticShiftRight32, value_in, shift_amount, carry_in);
    const auto carry_out = Inst<U1>(Opcode::GetCarryFromOp, result);
    return {result, carry_out};
}

ResultAndCarry<U32> IREmitter::RotateRight(const U32& value_in, const U8& shift_amount, const U1& carry_in) {
    const auto result = Inst<U32>(Opcode::RotateRight32, value_in, shift_amount, carry_in);
    const auto carry_out = Inst<U1>(Opcode::GetCarryFromOp, result);
    return {result, carry_out};
}

ResultAndCarry<U32> IREmitter::RotateRightExtended(const U32& value_in, const U1& carry_in) {
    const auto result = Inst<U32>(Opcode::RotateRightExtended, value_in, carry_in);
    const auto carry_out = Inst<U1>(Opcode::GetCarryFromOp, result);
    return {result, carry_out};
}

U32U64 IREmitter::LogicalShiftLeft(const U32U64& value_in, const U8& shift_amount) {
    if (value_in.GetType() == Type::U32) {
        return Inst<U32>(Opcode::LogicalShiftLeft32, value_in, shift_amount, Imm1(0));
    } else {
        return Inst<U64>(Opcode::LogicalShiftLeft64, value_in, shift_amount);
    }
}

U32U64 IREmitter::LogicalShiftRight(const U32U64& value_in, const U8& shift_amount) {
    if (value_in.GetType() == Type::U32) {
        return Inst<U32>(Opcode::LogicalShiftRight32, value_in, shift_amount, Imm1(0));
    } else {
        return Inst<U64>(Opcode::LogicalShiftRight64, value_in, shift_amount);
    }
}

U32U64 IREmitter::ArithmeticShiftRight(const U32U64& value_in, const U8& shift_amount) {
    if (value_in.GetType() == Type::U32) {
        return Inst<U32>(Opcode::ArithmeticShiftRight32, value_in, shift_amount, Imm1(0));
    } else {
        return Inst<U64>(Opcode::ArithmeticShiftRight64, value_in, shift_amount);
    }
}

U32U64 IREmitter::RotateRight(const U32U64& value_in, const U8& shift_amount) {
    if (value_in.GetType() == Type::U32) {
        return Inst<U32>(Opcode::RotateRight32, value_in, shift_amount, Imm1(0));
    } else {
        return Inst<U64>(Opcode::RotateRight64, value_in, shift_amount);
    }
}

U32U64 IREmitter::LogicalShiftLeftMasked(const U32U64& value_in, const U32U64& shift_amount) {
    ASSERT(value_in.GetType() == shift_amount.GetType());
    if (value_in.GetType() == Type::U32) {
        return Inst<U32>(Opcode::LogicalShiftLeftMasked32, value_in, shift_amount);
    } else {
        return Inst<U64>(Opcode::LogicalShiftLeftMasked64, value_in, shift_amount);
    }
}

U32U64 IREmitter::LogicalShiftRightMasked(const U32U64& value_in, const U32U64& shift_amount) {
    ASSERT(value_in.GetType() == shift_amount.GetType());
    if (value_in.GetType() == Type::U32) {
        return Inst<U32>(Opcode::LogicalShiftRightMasked32, value_in, shift_amount);
    } else {
        return Inst<U64>(Opcode::LogicalShiftRightMasked64, value_in, shift_amount);
    }
}

U32U64 IREmitter::ArithmeticShiftRightMasked(const U32U64& value_in, const U32U64& shift_amount) {
    ASSERT(value_in.GetType() == shift_amount.GetType());
    if (value_in.GetType() == Type::U32) {
        return Inst<U32>(Opcode::ArithmeticShiftRightMasked32, value_in, shift_amount);
    } else {
        return Inst<U64>(Opcode::ArithmeticShiftRightMasked64, value_in, shift_amount);
    }
}

U32U64 IREmitter::RotateRightMasked(const U32U64& value_in, const U32U64& shift_amount) {
    ASSERT(value_in.GetType() == shift_amount.GetType());
    if (value_in.GetType() == Type::U32) {
        return Inst<U32>(Opcode::RotateRightMasked32, value_in, shift_amount);
    } else {
        return Inst<U64>(Opcode::RotateRightMasked64, value_in, shift_amount);
    }
}

ResultAndCarryAndOverflow<U32> IREmitter::AddWithCarry(const U32& a, const U32& b, const U1& carry_in) {
    const auto result = Inst<U32>(Opcode::Add32, a, b, carry_in);
    const auto carry_out = Inst<U1>(Opcode::GetCarryFromOp, result);
    const auto overflow = Inst<U1>(Opcode::GetOverflowFromOp, result);
    return {result, carry_out, overflow};
}

U32U64 IREmitter::AddWithCarry(const U32U64& a, const U32U64& b, const U1& carry_in) {
    ASSERT(a.GetType() == b.GetType());
    if (a.GetType() == Type::U32) {
        return Inst<U32>(Opcode::Add32, a, b, carry_in);
    } else {
        return Inst<U64>(Opcode::Add64, a, b, carry_in);
    }
}

U32U64 IREmitter::Add(const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());
    if (a.GetType() == Type::U32) {
        return Inst<U32>(Opcode::Add32, a, b, Imm1(0));
    } else {
        return Inst<U64>(Opcode::Add64, a, b, Imm1(0));
    }
}

ResultAndCarryAndOverflow<U32> IREmitter::SubWithCarry(const U32& a, const U32& b, const U1& carry_in) {
    // This is equivalent to AddWithCarry(a, Not(b), carry_in).
    const auto result = Inst<U32>(Opcode::Sub32, a, b, carry_in);
    const auto carry_out = Inst<U1>(Opcode::GetCarryFromOp, result);
    const auto overflow = Inst<U1>(Opcode::GetOverflowFromOp, result);
    return {result, carry_out, overflow};
}

U32U64 IREmitter::SubWithCarry(const U32U64& a, const U32U64& b, const U1& carry_in) {
    ASSERT(a.GetType() == b.GetType());
    if (a.GetType() == Type::U32) {
        return Inst<U32>(Opcode::Sub32, a, b, carry_in);
    } else {
        return Inst<U64>(Opcode::Sub64, a, b, carry_in);
    }
}

U32U64 IREmitter::Sub(const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());
    if (a.GetType() == Type::U32) {
        return Inst<U32>(Opcode::Sub32, a, b, Imm1(1));
    } else {
        return Inst<U64>(Opcode::Sub64, a, b, Imm1(1));
    }
}

U32U64 IREmitter::Mul(const U32U64& a, const U32U64& b) {
    if (a.GetType() == Type::U32) {
        return Inst<U32>(Opcode::Mul32, a, b);
    }

    return Inst<U64>(Opcode::Mul64, a, b);
}

U64 IREmitter::UnsignedMultiplyHigh(const U64& a, const U64& b) {
    return Inst<U64>(Opcode::UnsignedMultiplyHigh64, a, b);
}

U64 IREmitter::SignedMultiplyHigh(const U64& a, const U64& b) {
    return Inst<U64>(Opcode::SignedMultiplyHigh64, a, b);
}

U32U64 IREmitter::UnsignedDiv(const U32U64& a, const U32U64& b) {
    if (a.GetType() == Type::U32) {
        return Inst<U32>(Opcode::UnsignedDiv32, a, b);
    }

    return Inst<U64>(Opcode::UnsignedDiv64, a, b);
}

U32U64 IREmitter::SignedDiv(const U32U64& a, const U32U64& b) {
    if (a.GetType() == Type::U32) {
        return Inst<U32>(Opcode::SignedDiv32, a, b);
    }

    return Inst<U64>(Opcode::SignedDiv64, a, b);
}

U32U64 IREmitter::And(const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());
    if (a.GetType() == Type::U32) {
        return Inst<U32>(Opcode::And32, a, b);
    } else {
        return Inst<U64>(Opcode::And64, a, b);
    }
}

U32U64 IREmitter::Eor(const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());
    if (a.GetType() == Type::U32) {
        return Inst<U32>(Opcode::Eor32, a, b);
    } else {
        return Inst<U64>(Opcode::Eor64, a, b);
    }
}

U32U64 IREmitter::Or(const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());
    if (a.GetType() == Type::U32) {
        return Inst<U32>(Opcode::Or32, a, b);
    } else {
        return Inst<U64>(Opcode::Or64, a, b);
    }
}

U32U64 IREmitter::Not(const U32U64& a) {
    if (a.GetType() == Type::U32) {
        return Inst<U32>(Opcode::Not32, a);
    } else {
        return Inst<U64>(Opcode::Not64, a);
    }
}

U64 IREmitter::SignExtendToLong(const UAny& a) {
    switch (a.GetType()) {
    case Type::U8:
        return Inst<U64>(Opcode::SignExtendByteToLong, a);
    case Type::U16:
        return Inst<U64>(Opcode::SignExtendHalfToLong, a);
    case Type::U32:
        return Inst<U64>(Opcode::SignExtendWordToLong, a);
    case Type::U64:
        return U64(a);
    default:
        UNREACHABLE();
    }
}

U32 IREmitter::SignExtendToWord(const UAny& a) {
    switch (a.GetType()) {
    case Type::U8:
        return Inst<U32>(Opcode::SignExtendByteToWord, a);
    case Type::U16:
        return Inst<U32>(Opcode::SignExtendHalfToWord, a);
    case Type::U32:
        return U32(a);
    case Type::U64:
        return Inst<U32>(Opcode::LeastSignificantWord, a);
    default:
        UNREACHABLE();
    }
}

U64 IREmitter::SignExtendWordToLong(const U32& a) {
    return Inst<U64>(Opcode::SignExtendWordToLong, a);
}

U32 IREmitter::SignExtendHalfToWord(const U16& a) {
    return Inst<U32>(Opcode::SignExtendHalfToWord, a);
}

U32 IREmitter::SignExtendByteToWord(const U8& a) {
    return Inst<U32>(Opcode::SignExtendByteToWord, a);
}

U64 IREmitter::ZeroExtendToLong(const UAny& a) {
    switch (a.GetType()) {
    case Type::U8:
        return Inst<U64>(Opcode::ZeroExtendByteToLong, a);
    case Type::U16:
        return Inst<U64>(Opcode::ZeroExtendHalfToLong, a);
    case Type::U32:
        return Inst<U64>(Opcode::ZeroExtendWordToLong, a);
    case Type::U64:
        return U64(a);
    default:
        UNREACHABLE();
    }
}

U32 IREmitter::ZeroExtendToWord(const UAny& a) {
    switch (a.GetType()) {
    case Type::U8:
        return Inst<U32>(Opcode::ZeroExtendByteToWord, a);
    case Type::U16:
        return Inst<U32>(Opcode::ZeroExtendHalfToWord, a);
    case Type::U32:
        return U32(a);
    case Type::U64:
        return Inst<U32>(Opcode::LeastSignificantWord, a);
    default:
        UNREACHABLE();
    }
}

U128 IREmitter::ZeroExtendToQuad(const UAny& a) {
    return Inst<U128>(Opcode::ZeroExtendLongToQuad, ZeroExtendToLong(a));
}

U64 IREmitter::ZeroExtendWordToLong(const U32& a) {
    return Inst<U64>(Opcode::ZeroExtendWordToLong, a);
}

U32 IREmitter::ZeroExtendHalfToWord(const U16& a) {
    return Inst<U32>(Opcode::ZeroExtendHalfToWord, a);
}

U32 IREmitter::ZeroExtendByteToWord(const U8& a) {
    return Inst<U32>(Opcode::ZeroExtendByteToWord, a);
}

U32 IREmitter::IndeterminateExtendToWord(const UAny& a) {
    // TODO: Implement properly
    return ZeroExtendToWord(a);
}

U64 IREmitter::IndeterminateExtendToLong(const UAny& a) {
    // TODO: Implement properly
    return ZeroExtendToLong(a);
}

U32 IREmitter::ByteReverseWord(const U32& a) {
    return Inst<U32>(Opcode::ByteReverseWord, a);
}

U16 IREmitter::ByteReverseHalf(const U16& a) {
    return Inst<U16>(Opcode::ByteReverseHalf, a);
}

U64 IREmitter::ByteReverseDual(const U64& a) {
    return Inst<U64>(Opcode::ByteReverseDual, a);
}

U32U64 IREmitter::CountLeadingZeros(const U32U64& a) {
    if (a.GetType() == IR::Type::U32) {
        return Inst<U32>(Opcode::CountLeadingZeros32, a);
    }

    return Inst<U64>(Opcode::CountLeadingZeros64, a);
}

U32U64 IREmitter::ExtractRegister(const U32U64& a, const U32U64& b, const U8& lsb) {
    if (a.GetType() == IR::Type::U32) {
        return Inst<U32>(Opcode::ExtractRegister32, a, b, lsb);
    }

    return Inst<U64>(Opcode::ExtractRegister64, a, b, lsb);
}

U32U64 IREmitter::ReplicateBit(const U32U64& a, u8 bit) {
    if (a.GetType() == IR::Type::U32) {
        ASSERT(bit < 32);
        return Inst<U32>(Opcode::ReplicateBit32, a, Imm8(bit));
    }

    ASSERT(bit < 64);
    return Inst<U64>(Opcode::ReplicateBit64, a, Imm8(bit));
}

U32U64 IREmitter::MaxSigned(const U32U64& a, const U32U64& b) {
    if (a.GetType() == IR::Type::U32) {
        return Inst<U32>(Opcode::MaxSigned32, a, b);
    }

    return Inst<U64>(Opcode::MaxSigned64, a, b);
}

U32U64 IREmitter::MaxUnsigned(const U32U64& a, const U32U64& b) {
    if (a.GetType() == IR::Type::U32) {
        return Inst<U32>(Opcode::MaxUnsigned32, a, b);
    }

    return Inst<U64>(Opcode::MaxUnsigned64, a, b);
}

U32U64 IREmitter::MinSigned(const U32U64& a, const U32U64& b) {
    if (a.GetType() == IR::Type::U32) {
        return Inst<U32>(Opcode::MinSigned32, a, b);
    }

    return Inst<U64>(Opcode::MinSigned64, a, b);
}

U32U64 IREmitter::MinUnsigned(const U32U64& a, const U32U64& b) {
    if (a.GetType() == IR::Type::U32) {
        return Inst<U32>(Opcode::MinUnsigned32, a, b);
    }

    return Inst<U64>(Opcode::MinUnsigned64, a, b);
}

ResultAndOverflow<UAny> IREmitter::SignedSaturatedAdd(const UAny& a, const UAny& b) {
    ASSERT(a.GetType() == b.GetType());
    const auto result = [&]() -> IR::UAny {
        switch (a.GetType()) {
        case IR::Type::U8:
            return Inst<U8>(Opcode::SignedSaturatedAdd8, a, b);
        case IR::Type::U16:
            return Inst<U16>(Opcode::SignedSaturatedAdd16, a, b);
        case IR::Type::U32:
            return Inst<U32>(Opcode::SignedSaturatedAdd32, a, b);
        case IR::Type::U64:
            return Inst<U64>(Opcode::SignedSaturatedAdd64, a, b);
        default:
            return IR::UAny{};
        }
    }();
    const auto overflow = Inst<U1>(Opcode::GetOverflowFromOp, result);
    return {result, overflow};
}

ResultAndOverflow<UAny> IREmitter::SignedSaturatedDoublingMultiplyReturnHigh(const UAny& a, const UAny& b) {
    ASSERT(a.GetType() == b.GetType());
    const auto result = [&]() -> IR::UAny {
        switch (a.GetType()) {
        case IR::Type::U16:
            return Inst<U16>(Opcode::SignedSaturatedDoublingMultiplyReturnHigh16, a, b);
        case IR::Type::U32:
            return Inst<U32>(Opcode::SignedSaturatedDoublingMultiplyReturnHigh32, a, b);
        default:
            UNREACHABLE();
        }
    }();

    const auto overflow = Inst<U1>(Opcode::GetOverflowFromOp, result);
    return {result, overflow};
}

ResultAndOverflow<UAny> IREmitter::SignedSaturatedSub(const UAny& a, const UAny& b) {
    ASSERT(a.GetType() == b.GetType());
    const auto result = [&]() -> IR::UAny {
        switch (a.GetType()) {
        case IR::Type::U8:
            return Inst<U8>(Opcode::SignedSaturatedSub8, a, b);
        case IR::Type::U16:
            return Inst<U16>(Opcode::SignedSaturatedSub16, a, b);
        case IR::Type::U32:
            return Inst<U32>(Opcode::SignedSaturatedSub32, a, b);
        case IR::Type::U64:
            return Inst<U64>(Opcode::SignedSaturatedSub64, a, b);
        default:
            return IR::UAny{};
        }
    }();
    const auto overflow = Inst<U1>(Opcode::GetOverflowFromOp, result);
    return {result, overflow};
}

ResultAndOverflow<U32> IREmitter::SignedSaturation(const U32& a, size_t bit_size_to_saturate_to) {
    ASSERT(bit_size_to_saturate_to >= 1 && bit_size_to_saturate_to <= 32);
    const auto result = Inst<U32>(Opcode::SignedSaturation, a, Imm8(static_cast<u8>(bit_size_to_saturate_to)));
    const auto overflow = Inst<U1>(Opcode::GetOverflowFromOp, result);
    return {result, overflow};
}

ResultAndOverflow<UAny> IREmitter::UnsignedSaturatedAdd(const UAny& a, const UAny& b) {
    ASSERT(a.GetType() == b.GetType());
    const auto result = [&]() -> IR::UAny {
        switch (a.GetType()) {
        case IR::Type::U8:
            return Inst<U8>(Opcode::UnsignedSaturatedAdd8, a, b);
        case IR::Type::U16:
            return Inst<U16>(Opcode::UnsignedSaturatedAdd16, a, b);
        case IR::Type::U32:
            return Inst<U32>(Opcode::UnsignedSaturatedAdd32, a, b);
        case IR::Type::U64:
            return Inst<U64>(Opcode::UnsignedSaturatedAdd64, a, b);
        default:
            return IR::UAny{};
        }
    }();
    const auto overflow = Inst<U1>(Opcode::GetOverflowFromOp, result);
    return {result, overflow};
}

ResultAndOverflow<UAny> IREmitter::UnsignedSaturatedSub(const UAny& a, const UAny& b) {
    ASSERT(a.GetType() == b.GetType());
    const auto result = [&]() -> IR::UAny {
        switch (a.GetType()) {
        case IR::Type::U8:
            return Inst<U8>(Opcode::UnsignedSaturatedSub8, a, b);
        case IR::Type::U16:
            return Inst<U16>(Opcode::UnsignedSaturatedSub16, a, b);
        case IR::Type::U32:
            return Inst<U32>(Opcode::UnsignedSaturatedSub32, a, b);
        case IR::Type::U64:
            return Inst<U64>(Opcode::UnsignedSaturatedSub64, a, b);
        default:
            return IR::UAny{};
        }
    }();
    const auto overflow = Inst<U1>(Opcode::GetOverflowFromOp, result);
    return {result, overflow};
}

ResultAndOverflow<U32> IREmitter::UnsignedSaturation(const U32& a, size_t bit_size_to_saturate_to) {
    ASSERT(bit_size_to_saturate_to <= 31);
    const auto result = Inst<U32>(Opcode::UnsignedSaturation, a, Imm8(static_cast<u8>(bit_size_to_saturate_to)));
    const auto overflow = Inst<U1>(Opcode::GetOverflowFromOp, result);
    return {result, overflow};
}

U128 IREmitter::VectorSignedSaturatedAdd(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorSignedSaturatedAdd8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorSignedSaturatedAdd16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorSignedSaturatedAdd32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorSignedSaturatedAdd64, a, b);
    default:
        UNREACHABLE();
    }
}

U128 IREmitter::VectorSignedSaturatedSub(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorSignedSaturatedSub8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorSignedSaturatedSub16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorSignedSaturatedSub32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorSignedSaturatedSub64, a, b);
    default:
        UNREACHABLE();
    }
}

U128 IREmitter::VectorUnsignedSaturatedAdd(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedAdd8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedAdd16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedAdd32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedAdd64, a, b);
    default:
        UNREACHABLE();
    }
}

U128 IREmitter::VectorUnsignedSaturatedSub(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedSub8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedSub16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedSub32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedSub64, a, b);
    default:
        UNREACHABLE();
    }
}

ResultAndGE<U32> IREmitter::PackedAddU8(const U32& a, const U32& b) {
    const auto result = Inst<U32>(Opcode::PackedAddU8, a, b);
    const auto ge = Inst<U32>(Opcode::GetGEFromOp, result);
    return {result, ge};
}

ResultAndGE<U32> IREmitter::PackedAddS8(const U32& a, const U32& b) {
    const auto result = Inst<U32>(Opcode::PackedAddS8, a, b);
    const auto ge = Inst<U32>(Opcode::GetGEFromOp, result);
    return {result, ge};
}

ResultAndGE<U32> IREmitter::PackedAddU16(const U32& a, const U32& b) {
    const auto result = Inst<U32>(Opcode::PackedAddU16, a, b);
    const auto ge = Inst<U32>(Opcode::GetGEFromOp, result);
    return {result, ge};
}

ResultAndGE<U32> IREmitter::PackedAddS16(const U32& a, const U32& b) {
    const auto result = Inst<U32>(Opcode::PackedAddS16, a, b);
    const auto ge = Inst<U32>(Opcode::GetGEFromOp, result);
    return {result, ge};
}

ResultAndGE<U32> IREmitter::PackedSubU8(const U32& a, const U32& b) {
    const auto result = Inst<U32>(Opcode::PackedSubU8, a, b);
    const auto ge = Inst<U32>(Opcode::GetGEFromOp, result);
    return {result, ge};
}

ResultAndGE<U32> IREmitter::PackedSubS8(const U32& a, const U32& b) {
    const auto result = Inst<U32>(Opcode::PackedSubS8, a, b);
    const auto ge = Inst<U32>(Opcode::GetGEFromOp, result);
    return {result, ge};
}

ResultAndGE<U32> IREmitter::PackedSubU16(const U32& a, const U32& b) {
    const auto result = Inst<U32>(Opcode::PackedSubU16, a, b);
    const auto ge = Inst<U32>(Opcode::GetGEFromOp, result);
    return {result, ge};
}

ResultAndGE<U32> IREmitter::PackedSubS16(const U32& a, const U32& b) {
    const auto result = Inst<U32>(Opcode::PackedSubS16, a, b);
    const auto ge = Inst<U32>(Opcode::GetGEFromOp, result);
    return {result, ge};
}

ResultAndGE<U32> IREmitter::PackedAddSubU16(const U32& a, const U32& b) {
    const auto result = Inst<U32>(Opcode::PackedAddSubU16, a, b);
    const auto ge = Inst<U32>(Opcode::GetGEFromOp, result);
    return {result, ge};
}

ResultAndGE<U32> IREmitter::PackedAddSubS16(const U32& a, const U32& b) {
    const auto result = Inst<U32>(Opcode::PackedAddSubS16, a, b);
    const auto ge = Inst<U32>(Opcode::GetGEFromOp, result);
    return {result, ge};
}

ResultAndGE<U32> IREmitter::PackedSubAddU16(const U32& a, const U32& b) {
    const auto result = Inst<U32>(Opcode::PackedSubAddU16, a, b);
    const auto ge = Inst<U32>(Opcode::GetGEFromOp, result);
    return {result, ge};
}

ResultAndGE<U32> IREmitter::PackedSubAddS16(const U32& a, const U32& b) {
    const auto result = Inst<U32>(Opcode::PackedSubAddS16, a, b);
    const auto ge = Inst<U32>(Opcode::GetGEFromOp, result);
    return {result, ge};
}

U32 IREmitter::PackedHalvingAddU8(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedHalvingAddU8, a, b);
}

U32 IREmitter::PackedHalvingAddS8(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedHalvingAddS8, a, b);
}

U32 IREmitter::PackedHalvingSubU8(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedHalvingSubU8, a, b);
}

U32 IREmitter::PackedHalvingSubS8(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedHalvingSubS8, a, b);
}

U32 IREmitter::PackedHalvingAddU16(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedHalvingAddU16, a, b);
}

U32 IREmitter::PackedHalvingAddS16(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedHalvingAddS16, a, b);
}

U32 IREmitter::PackedHalvingSubU16(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedHalvingSubU16, a, b);
}

U32 IREmitter::PackedHalvingSubS16(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedHalvingSubS16, a, b);
}

U32 IREmitter::PackedHalvingAddSubU16(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedHalvingAddSubU16, a, b);
}

U32 IREmitter::PackedHalvingAddSubS16(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedHalvingAddSubS16, a, b);
}

U32 IREmitter::PackedHalvingSubAddU16(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedHalvingSubAddU16, a, b);
}

U32 IREmitter::PackedHalvingSubAddS16(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedHalvingSubAddS16, a, b);
}

U32 IREmitter::PackedSaturatedAddU8(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedSaturatedAddU8, a, b);
}

U32 IREmitter::PackedSaturatedAddS8(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedSaturatedAddS8, a, b);
}

U32 IREmitter::PackedSaturatedSubU8(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedSaturatedSubU8, a, b);
}

U32 IREmitter::PackedSaturatedSubS8(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedSaturatedSubS8, a, b);
}

U32 IREmitter::PackedSaturatedAddU16(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedSaturatedAddU16, a, b);
}

U32 IREmitter::PackedSaturatedAddS16(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedSaturatedAddS16, a, b);
}

U32 IREmitter::PackedSaturatedSubU16(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedSaturatedSubU16, a, b);
}

U32 IREmitter::PackedSaturatedSubS16(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedSaturatedSubS16, a, b);
}

U32 IREmitter::PackedAbsDiffSumS8(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedAbsDiffSumS8, a, b);
}

U32 IREmitter::PackedSelect(const U32& ge, const U32& a, const U32& b) {
    return Inst<U32>(Opcode::PackedSelect, ge, a, b);
}

U32 IREmitter::CRC32Castagnoli8(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::CRC32Castagnoli8, a, b);
}

U32 IREmitter::CRC32Castagnoli16(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::CRC32Castagnoli16, a, b);
}

U32 IREmitter::CRC32Castagnoli32(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::CRC32Castagnoli32, a, b);
}

U32 IREmitter::CRC32Castagnoli64(const U32& a, const U64& b) {
    return Inst<U32>(Opcode::CRC32Castagnoli64, a, b);
}

U32 IREmitter::CRC32ISO8(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::CRC32ISO8, a, b);
}

U32 IREmitter::CRC32ISO16(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::CRC32ISO16, a, b);
}

U32 IREmitter::CRC32ISO32(const U32& a, const U32& b) {
    return Inst<U32>(Opcode::CRC32ISO32, a, b);
}

U32 IREmitter::CRC32ISO64(const U32& a, const U64& b) {
    return Inst<U32>(Opcode::CRC32ISO64, a, b);
}

U128 IREmitter::AESDecryptSingleRound(const U128& a) {
    return Inst<U128>(Opcode::AESDecryptSingleRound, a);
}

U128 IREmitter::AESEncryptSingleRound(const U128& a) {
    return Inst<U128>(Opcode::AESEncryptSingleRound, a);
}

U128 IREmitter::AESInverseMixColumns(const U128& a) {
    return Inst<U128>(Opcode::AESInverseMixColumns, a);
}

U128 IREmitter::AESMixColumns(const U128& a) {
    return Inst<U128>(Opcode::AESMixColumns, a);
}

U8 IREmitter::SM4AccessSubstitutionBox(const U8& a) {
    return Inst<U8>(Opcode::SM4AccessSubstitutionBox, a);
}

UAny IREmitter::VectorGetElement(size_t esize, const U128& a, size_t index) {
    ASSERT_MSG(esize * index < 128, "Invalid index");
    switch (esize) {
    case 8:
        return Inst<U8>(Opcode::VectorGetElement8, a, Imm8(static_cast<u8>(index)));
    case 16:
        return Inst<U16>(Opcode::VectorGetElement16, a, Imm8(static_cast<u8>(index)));
    case 32:
        return Inst<U32>(Opcode::VectorGetElement32, a, Imm8(static_cast<u8>(index)));
    case 64:
        return Inst<U64>(Opcode::VectorGetElement64, a, Imm8(static_cast<u8>(index)));
    default:
        UNREACHABLE();
    }
}

U128 IREmitter::VectorSetElement(size_t esize, const U128& a, size_t index, const IR::UAny& elem) {
    ASSERT_MSG(esize * index < 128, "Invalid index");
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorSetElement8, a, Imm8(static_cast<u8>(index)), elem);
    case 16:
        return Inst<U128>(Opcode::VectorSetElement16, a, Imm8(static_cast<u8>(index)), elem);
    case 32:
        return Inst<U128>(Opcode::VectorSetElement32, a, Imm8(static_cast<u8>(index)), elem);
    case 64:
        return Inst<U128>(Opcode::VectorSetElement64, a, Imm8(static_cast<u8>(index)), elem);
    default:
        UNREACHABLE();
    }
}

U128 IREmitter::VectorAbs(size_t esize, const U128& a) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorAbs8, a);
    case 16:
        return Inst<U128>(Opcode::VectorAbs16, a);
    case 32:
        return Inst<U128>(Opcode::VectorAbs32, a);
    case 64:
        return Inst<U128>(Opcode::VectorAbs64, a);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorAdd(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorAdd8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorAdd16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorAdd32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorAdd64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorAnd(const U128& a, const U128& b) {
    return Inst<U128>(Opcode::VectorAnd, a, b);
}

U128 IREmitter::VectorArithmeticShiftRight(size_t esize, const U128& a, u8 shift_amount) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorArithmeticShiftRight8, a, Imm8(shift_amount));
    case 16:
        return Inst<U128>(Opcode::VectorArithmeticShiftRight16, a, Imm8(shift_amount));
    case 32:
        return Inst<U128>(Opcode::VectorArithmeticShiftRight32, a, Imm8(shift_amount));
    case 64:
        return Inst<U128>(Opcode::VectorArithmeticShiftRight64, a, Imm8(shift_amount));
    }
    UNREACHABLE();
}

U128 IREmitter::VectorArithmeticVShift(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorArithmeticVShift8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorArithmeticVShift16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorArithmeticVShift32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorArithmeticVShift64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorBroadcastLower(size_t esize, const UAny& a) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorBroadcastLower8, U8(a));
    case 16:
        return Inst<U128>(Opcode::VectorBroadcastLower16, U16(a));
    case 32:
        return Inst<U128>(Opcode::VectorBroadcastLower32, U32(a));
    }
    UNREACHABLE();
}

U128 IREmitter::VectorBroadcast(size_t esize, const UAny& a) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorBroadcast8, U8(a));
    case 16:
        return Inst<U128>(Opcode::VectorBroadcast16, U16(a));
    case 32:
        return Inst<U128>(Opcode::VectorBroadcast32, U32(a));
    case 64:
        return Inst<U128>(Opcode::VectorBroadcast64, U64(a));
    }
    UNREACHABLE();
}

U128 IREmitter::VectorCountLeadingZeros(size_t esize, const U128& a) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorCountLeadingZeros8, a);
    case 16:
        return Inst<U128>(Opcode::VectorCountLeadingZeros16, a);
    case 32:
        return Inst<U128>(Opcode::VectorCountLeadingZeros32, a);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorDeinterleaveEven(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorDeinterleaveEven8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorDeinterleaveEven16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorDeinterleaveEven32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorDeinterleaveEven64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorDeinterleaveOdd(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorDeinterleaveOdd8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorDeinterleaveOdd16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorDeinterleaveOdd32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorDeinterleaveOdd64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorDeinterleaveEvenLower(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorDeinterleaveEvenLower8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorDeinterleaveEvenLower16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorDeinterleaveEvenLower32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorDeinterleaveEvenLower64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorDeinterleaveOddLower(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorDeinterleaveOddLower8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorDeinterleaveOddLower16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorDeinterleaveOddLower32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorDeinterleaveOddLower64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorEor(const U128& a, const U128& b) {
    return Inst<U128>(Opcode::VectorEor, a, b);
}

U128 IREmitter::VectorEqual(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorEqual8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorEqual16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorEqual32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorEqual64, a, b);
    case 128:
        return Inst<U128>(Opcode::VectorEqual128, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorExtract(const U128& a, const U128& b, size_t position) {
    ASSERT(position <= 128);
    return Inst<U128>(Opcode::VectorExtract, a, b, Imm8(static_cast<u8>(position)));
}

U128 IREmitter::VectorExtractLower(const U128& a, const U128& b, size_t position) {
    ASSERT(position <= 64);
    return Inst<U128>(Opcode::VectorExtractLower, a, b, Imm8(static_cast<u8>(position)));
}

U128 IREmitter::VectorGreaterSigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorGreaterS8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorGreaterS16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorGreaterS32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorGreaterS64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorGreaterEqualSigned(size_t esize, const U128& a, const U128& b) {
    return VectorOr(VectorGreaterSigned(esize, a, b), VectorEqual(esize, a, b));
}

U128 IREmitter::VectorGreaterEqualUnsigned(size_t esize, const U128& a, const U128& b) {
    return VectorEqual(esize, VectorMaxUnsigned(esize, a, b), a);
}

U128 IREmitter::VectorGreaterUnsigned(size_t esize, const U128& a, const U128& b) {
    return VectorNot(VectorEqual(esize, VectorMinUnsigned(esize, a, b), a));
}

U128 IREmitter::VectorHalvingAddSigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorHalvingAddS8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorHalvingAddS16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorHalvingAddS32, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorHalvingAddUnsigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorHalvingAddU8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorHalvingAddU16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorHalvingAddU32, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorHalvingSubSigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorHalvingSubS8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorHalvingSubS16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorHalvingSubS32, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorHalvingSubUnsigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorHalvingSubU8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorHalvingSubU16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorHalvingSubU32, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorInterleaveLower(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorInterleaveLower8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorInterleaveLower16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorInterleaveLower32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorInterleaveLower64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorInterleaveUpper(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorInterleaveUpper8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorInterleaveUpper16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorInterleaveUpper32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorInterleaveUpper64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorLessEqualSigned(size_t esize, const U128& a, const U128& b) {
    return VectorNot(VectorGreaterSigned(esize, a, b));
}

U128 IREmitter::VectorLessEqualUnsigned(size_t esize, const U128& a, const U128& b) {
    return VectorEqual(esize, VectorMinUnsigned(esize, a, b), a);
}

U128 IREmitter::VectorLessSigned(size_t esize, const U128& a, const U128& b) {
    return VectorNot(VectorOr(VectorGreaterSigned(esize, a, b), VectorEqual(esize, a, b)));
}

U128 IREmitter::VectorLessUnsigned(size_t esize, const U128& a, const U128& b) {
    return VectorNot(VectorEqual(esize, VectorMaxUnsigned(esize, a, b), a));
}

U128 IREmitter::VectorLogicalShiftLeft(size_t esize, const U128& a, u8 shift_amount) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorLogicalShiftLeft8, a, Imm8(shift_amount));
    case 16:
        return Inst<U128>(Opcode::VectorLogicalShiftLeft16, a, Imm8(shift_amount));
    case 32:
        return Inst<U128>(Opcode::VectorLogicalShiftLeft32, a, Imm8(shift_amount));
    case 64:
        return Inst<U128>(Opcode::VectorLogicalShiftLeft64, a, Imm8(shift_amount));
    }
    UNREACHABLE();
}

U128 IREmitter::VectorLogicalShiftRight(size_t esize, const U128& a, u8 shift_amount) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorLogicalShiftRight8, a, Imm8(shift_amount));
    case 16:
        return Inst<U128>(Opcode::VectorLogicalShiftRight16, a, Imm8(shift_amount));
    case 32:
        return Inst<U128>(Opcode::VectorLogicalShiftRight32, a, Imm8(shift_amount));
    case 64:
        return Inst<U128>(Opcode::VectorLogicalShiftRight64, a, Imm8(shift_amount));
    }
    UNREACHABLE();
}

U128 IREmitter::VectorLogicalVShift(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorLogicalVShift8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorLogicalVShift16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorLogicalVShift32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorLogicalVShift64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorMaxSigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorMaxS8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorMaxS16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorMaxS32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorMaxS64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorMaxUnsigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorMaxU8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorMaxU16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorMaxU32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorMaxU64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorMinSigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorMinS8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorMinS16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorMinS32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorMinS64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorMinUnsigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorMinU8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorMinU16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorMinU32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorMinU64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorMultiply(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorMultiply8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorMultiply16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorMultiply32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorMultiply64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorNarrow(size_t original_esize, const U128& a) {
    switch (original_esize) {
    case 16:
        return Inst<U128>(Opcode::VectorNarrow16, a);
    case 32:
        return Inst<U128>(Opcode::VectorNarrow32, a);
    case 64:
        return Inst<U128>(Opcode::VectorNarrow64, a);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorNot(const U128& a) {
    return Inst<U128>(Opcode::VectorNot, a);
}

U128 IREmitter::VectorOr(const U128& a, const U128& b) {
    return Inst<U128>(Opcode::VectorOr, a, b);
}

U128 IREmitter::VectorPairedAdd(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorPairedAdd8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorPairedAdd16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorPairedAdd32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorPairedAdd64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorPairedAddLower(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorPairedAddLower8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorPairedAddLower16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorPairedAddLower32, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorPairedAddSignedWiden(size_t original_esize, const U128& a) {
    switch (original_esize) {
    case 8:
        return Inst<U128>(Opcode::VectorPairedAddSignedWiden8, a);
    case 16:
        return Inst<U128>(Opcode::VectorPairedAddSignedWiden16, a);
    case 32:
        return Inst<U128>(Opcode::VectorPairedAddSignedWiden32, a);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorPairedAddUnsignedWiden(size_t original_esize, const U128& a) {
    switch (original_esize) {
    case 8:
        return Inst<U128>(Opcode::VectorPairedAddUnsignedWiden8, a);
    case 16:
        return Inst<U128>(Opcode::VectorPairedAddUnsignedWiden16, a);
    case 32:
        return Inst<U128>(Opcode::VectorPairedAddUnsignedWiden32, a);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorPairedMaxSigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorPairedMaxS8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorPairedMaxS16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorPairedMaxS32, a, b);
    default:
        UNREACHABLE();
    }
}

U128 IREmitter::VectorPairedMaxUnsigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorPairedMaxU8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorPairedMaxU16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorPairedMaxU32, a, b);
    default:
        UNREACHABLE();
    }
}

U128 IREmitter::VectorPairedMinSigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorPairedMinS8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorPairedMinS16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorPairedMinS32, a, b);
    default:
        UNREACHABLE();
    }
}

U128 IREmitter::VectorPairedMinUnsigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorPairedMinU8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorPairedMinU16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorPairedMinU32, a, b);
    default:
        UNREACHABLE();
    }
}

U128 IREmitter::VectorPolynomialMultiply(const U128& a, const U128& b) {
    return Inst<U128>(Opcode::VectorPolynomialMultiply8, a, b);
}

U128 IREmitter::VectorPolynomialMultiplyLong(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorPolynomialMultiplyLong8, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorPolynomialMultiplyLong64, a, b);
    default:
        UNREACHABLE();
    }
}

U128 IREmitter::VectorPopulationCount(const U128& a) {
    return Inst<U128>(Opcode::VectorPopulationCount, a);
}

U128 IREmitter::VectorReverseBits(const U128& a) {
    return Inst<U128>(Opcode::VectorReverseBits, a);
}

U128 IREmitter::VectorRotateLeft(size_t esize, const U128& a, u8 amount) {
    ASSERT(amount < esize);

    if (amount == 0) {
        return a;
    }

    return VectorOr(VectorLogicalShiftLeft(esize, a, amount),
                    VectorLogicalShiftRight(esize, a, static_cast<u8>(esize - amount)));
}

U128 IREmitter::VectorRotateRight(size_t esize, const U128& a, u8 amount) {
    ASSERT(amount < esize);

    if (amount == 0) {
        return a;
    }

    return VectorOr(VectorLogicalShiftRight(esize, a, amount),
                    VectorLogicalShiftLeft(esize, a, static_cast<u8>(esize - amount)));
}

U128 IREmitter::VectorRoundingHalvingAddSigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorRoundingHalvingAddS8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorRoundingHalvingAddS16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorRoundingHalvingAddS32, a, b);
    }

    UNREACHABLE();
}

U128 IREmitter::VectorRoundingHalvingAddUnsigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorRoundingHalvingAddU8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorRoundingHalvingAddU16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorRoundingHalvingAddU32, a, b);
    }

    UNREACHABLE();
}

U128 IREmitter::VectorRoundingShiftLeftSigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorRoundingShiftLeftS8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorRoundingShiftLeftS16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorRoundingShiftLeftS32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorRoundingShiftLeftS64, a, b);
    }

    UNREACHABLE();
}

U128 IREmitter::VectorRoundingShiftLeftUnsigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorRoundingShiftLeftU8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorRoundingShiftLeftU16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorRoundingShiftLeftU32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorRoundingShiftLeftU64, a, b);
    }

    UNREACHABLE();
}

U128 IREmitter::VectorShuffleHighHalfwords(const U128& a, u8 mask) {
    return Inst<U128>(Opcode::VectorShuffleHighHalfwords, a, mask);
}

U128 IREmitter::VectorShuffleLowHalfwords(const U128& a, u8 mask) {
    return Inst<U128>(Opcode::VectorShuffleLowHalfwords, a, mask);
}

U128 IREmitter::VectorShuffleWords(const U128& a, u8 mask) {
    return Inst<U128>(Opcode::VectorShuffleWords, a, mask);
}

U128 IREmitter::VectorSignExtend(size_t original_esize, const U128& a) {
    switch (original_esize) {
    case 8:
        return Inst<U128>(Opcode::VectorSignExtend8, a);
    case 16:
        return Inst<U128>(Opcode::VectorSignExtend16, a);
    case 32:
        return Inst<U128>(Opcode::VectorSignExtend32, a);
    case 64:
        return Inst<U128>(Opcode::VectorSignExtend64, a);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorSignedAbsoluteDifference(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorSignedAbsoluteDifference8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorSignedAbsoluteDifference16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorSignedAbsoluteDifference32, a, b);
    }
    UNREACHABLE();
}

UpperAndLower IREmitter::VectorSignedMultiply(size_t esize, const U128& a, const U128& b) {
    const Value multiply = [&] {
        switch (esize) {
        case 16:
            return Inst(Opcode::VectorSignedMultiply16, a, b);
        case 32:
            return Inst(Opcode::VectorSignedMultiply32, a, b);
        }
        UNREACHABLE();
    }();

    return {
        Inst<U128>(Opcode::GetUpperFromOp, multiply),
        Inst<U128>(Opcode::GetLowerFromOp, multiply),
    };
}

U128 IREmitter::VectorSignedSaturatedAbs(size_t esize, const U128& a) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorSignedSaturatedAbs8, a);
    case 16:
        return Inst<U128>(Opcode::VectorSignedSaturatedAbs16, a);
    case 32:
        return Inst<U128>(Opcode::VectorSignedSaturatedAbs32, a);
    case 64:
        return Inst<U128>(Opcode::VectorSignedSaturatedAbs64, a);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorSignedSaturatedAccumulateUnsigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorSignedSaturatedAccumulateUnsigned8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorSignedSaturatedAccumulateUnsigned16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorSignedSaturatedAccumulateUnsigned32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorSignedSaturatedAccumulateUnsigned64, a, b);
    }
    UNREACHABLE();
}

UpperAndLower IREmitter::VectorSignedSaturatedDoublingMultiply(size_t esize, const U128& a, const U128& b) {
    const Value multiply = [&] {
        switch (esize) {
        case 16:
            return Inst(Opcode::VectorSignedSaturatedDoublingMultiply16, a, b);
        case 32:
            return Inst(Opcode::VectorSignedSaturatedDoublingMultiply32, a, b);
        default:
            UNREACHABLE();
        }
    }();

    return {
        Inst<U128>(Opcode::GetUpperFromOp, multiply),
        Inst<U128>(Opcode::GetLowerFromOp, multiply),
    };
}

U128 IREmitter::VectorSignedSaturatedDoublingMultiplyLong(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 16:
        return Inst<U128>(Opcode::VectorSignedSaturatedDoublingMultiplyLong16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorSignedSaturatedDoublingMultiplyLong32, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorSignedSaturatedNarrowToSigned(size_t original_esize, const U128& a) {
    switch (original_esize) {
    case 16:
        return Inst<U128>(Opcode::VectorSignedSaturatedNarrowToSigned16, a);
    case 32:
        return Inst<U128>(Opcode::VectorSignedSaturatedNarrowToSigned32, a);
    case 64:
        return Inst<U128>(Opcode::VectorSignedSaturatedNarrowToSigned64, a);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorSignedSaturatedNarrowToUnsigned(size_t original_esize, const U128& a) {
    switch (original_esize) {
    case 16:
        return Inst<U128>(Opcode::VectorSignedSaturatedNarrowToUnsigned16, a);
    case 32:
        return Inst<U128>(Opcode::VectorSignedSaturatedNarrowToUnsigned32, a);
    case 64:
        return Inst<U128>(Opcode::VectorSignedSaturatedNarrowToUnsigned64, a);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorSignedSaturatedNeg(size_t esize, const U128& a) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorSignedSaturatedNeg8, a);
    case 16:
        return Inst<U128>(Opcode::VectorSignedSaturatedNeg16, a);
    case 32:
        return Inst<U128>(Opcode::VectorSignedSaturatedNeg32, a);
    case 64:
        return Inst<U128>(Opcode::VectorSignedSaturatedNeg64, a);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorSignedSaturatedShiftLeft(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorSignedSaturatedShiftLeft8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorSignedSaturatedShiftLeft16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorSignedSaturatedShiftLeft32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorSignedSaturatedShiftLeft64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorSignedSaturatedShiftLeftUnsigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorSignedSaturatedShiftLeftUnsigned8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorSignedSaturatedShiftLeftUnsigned16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorSignedSaturatedShiftLeftUnsigned32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorSignedSaturatedShiftLeftUnsigned64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorSub(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorSub8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorSub16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorSub32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorSub64, a, b);
    }
    UNREACHABLE();
}

Table IREmitter::VectorTable(std::vector<U64> values) {
    ASSERT(values.size() >= 1 && values.size() <= 4);
    values.resize(4);
    return Inst<Table>(Opcode::VectorTable, values[0], values[1], values[2], values[3]);
}

Table IREmitter::VectorTable(std::vector<U128> values) {
    ASSERT(values.size() >= 1 && values.size() <= 4);
    values.resize(4);
    return Inst<Table>(Opcode::VectorTable, values[0], values[1], values[2], values[3]);
}

U64 IREmitter::VectorTableLookup(const U64& defaults, const Table& table, const U64& indices) {
    ASSERT(table.GetInst()->GetArg(0).GetType() == Type::U64);
    return Inst<U64>(Opcode::VectorTableLookup64, defaults, table, indices);
}

U128 IREmitter::VectorTableLookup(const U128& defaults, const Table& table, const U128& indices) {
    ASSERT(table.GetInst()->GetArg(0).GetType() == Type::U128);
    return Inst<U128>(Opcode::VectorTableLookup128, defaults, table, indices);
}

U128 IREmitter::VectorTranspose(size_t esize, const U128& a, const U128& b, bool part) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorTranspose8, a, b, Imm1(part));
    case 16:
        return Inst<U128>(Opcode::VectorTranspose16, a, b, Imm1(part));
    case 32:
        return Inst<U128>(Opcode::VectorTranspose32, a, b, Imm1(part));
    case 64:
        return Inst<U128>(Opcode::VectorTranspose64, a, b, Imm1(part));
    }
    UNREACHABLE();
}

U128 IREmitter::VectorUnsignedAbsoluteDifference(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorUnsignedAbsoluteDifference8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorUnsignedAbsoluteDifference16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorUnsignedAbsoluteDifference32, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorUnsignedRecipEstimate(const U128& a) {
    return Inst<U128>(Opcode::VectorUnsignedRecipEstimate, a);
}

U128 IREmitter::VectorUnsignedRecipSqrtEstimate(const U128& a) {
    return Inst<U128>(Opcode::VectorUnsignedRecipSqrtEstimate, a);
}

U128 IREmitter::VectorUnsignedSaturatedAccumulateSigned(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedAccumulateSigned8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedAccumulateSigned16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedAccumulateSigned32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedAccumulateSigned64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorUnsignedSaturatedNarrow(size_t esize, const U128& a) {
    switch (esize) {
    case 16:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedNarrow16, a);
    case 32:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedNarrow32, a);
    case 64:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedNarrow64, a);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorUnsignedSaturatedShiftLeft(size_t esize, const U128& a, const U128& b) {
    switch (esize) {
    case 8:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedShiftLeft8, a, b);
    case 16:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedShiftLeft16, a, b);
    case 32:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedShiftLeft32, a, b);
    case 64:
        return Inst<U128>(Opcode::VectorUnsignedSaturatedShiftLeft64, a, b);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorZeroExtend(size_t original_esize, const U128& a) {
    switch (original_esize) {
    case 8:
        return Inst<U128>(Opcode::VectorZeroExtend8, a);
    case 16:
        return Inst<U128>(Opcode::VectorZeroExtend16, a);
    case 32:
        return Inst<U128>(Opcode::VectorZeroExtend32, a);
    case 64:
        return Inst<U128>(Opcode::VectorZeroExtend64, a);
    }
    UNREACHABLE();
}

U128 IREmitter::VectorZeroUpper(const U128& a) {
    return Inst<U128>(Opcode::VectorZeroUpper, a);
}

U128 IREmitter::ZeroVector() {
    return Inst<U128>(Opcode::ZeroVector);
}

U16U32U64 IREmitter::FPAbs(const U16U32U64& a) {
    switch (a.GetType()) {
    case Type::U16:
        return Inst<U16>(Opcode::FPAbs16, a);
    case Type::U32:
        return Inst<U32>(Opcode::FPAbs32, a);
    case Type::U64:
        return Inst<U64>(Opcode::FPAbs64, a);
    default:
        UNREACHABLE();
    }
}

U32U64 IREmitter::FPAdd(const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());

    switch (a.GetType()) {
    case Type::U32:
        return Inst<U32>(Opcode::FPAdd32, a, b);
    case Type::U64:
        return Inst<U64>(Opcode::FPAdd64, a, b);
    default:
        UNREACHABLE();
    }
}

NZCV IREmitter::FPCompare(const U32U64& a, const U32U64& b, bool exc_on_qnan) {
    ASSERT(a.GetType() == b.GetType());

    const IR::U1 exc_on_qnan_imm = Imm1(exc_on_qnan);

    switch (a.GetType()) {
    case Type::U32:
        return Inst<NZCV>(Opcode::FPCompare32, a, b, exc_on_qnan_imm);
    case Type::U64:
        return Inst<NZCV>(Opcode::FPCompare64, a, b, exc_on_qnan_imm);
    default:
        UNREACHABLE();
    }
}

U32U64 IREmitter::FPDiv(const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());

    switch (a.GetType()) {
    case Type::U32:
        return Inst<U32>(Opcode::FPDiv32, a, b);
    case Type::U64:
        return Inst<U64>(Opcode::FPDiv64, a, b);
    default:
        UNREACHABLE();
    }
}

U32U64 IREmitter::FPMax(const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());

    switch (a.GetType()) {
    case Type::U32:
        return Inst<U32>(Opcode::FPMax32, a, b);
    case Type::U64:
        return Inst<U64>(Opcode::FPMax64, a, b);
    default:
        UNREACHABLE();
    }
}

U32U64 IREmitter::FPMaxNumeric(const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());

    switch (a.GetType()) {
    case Type::U32:
        return Inst<U32>(Opcode::FPMaxNumeric32, a, b);
    case Type::U64:
        return Inst<U64>(Opcode::FPMaxNumeric64, a, b);
    default:
        UNREACHABLE();
    }
}

U32U64 IREmitter::FPMin(const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());

    switch (a.GetType()) {
    case Type::U32:
        return Inst<U32>(Opcode::FPMin32, a, b);
    case Type::U64:
        return Inst<U64>(Opcode::FPMin64, a, b);
    default:
        UNREACHABLE();
    }
}

U32U64 IREmitter::FPMinNumeric(const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());

    switch (a.GetType()) {
    case Type::U32:
        return Inst<U32>(Opcode::FPMinNumeric32, a, b);
    case Type::U64:
        return Inst<U64>(Opcode::FPMinNumeric64, a, b);
    default:
        UNREACHABLE();
    }
}

U32U64 IREmitter::FPMul(const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());

    switch (a.GetType()) {
    case Type::U32:
        return Inst<U32>(Opcode::FPMul32, a, b);
    case Type::U64:
        return Inst<U64>(Opcode::FPMul64, a, b);
    default:
        UNREACHABLE();
    }
}

U16U32U64 IREmitter::FPMulAdd(const U16U32U64& a, const U16U32U64& b, const U16U32U64& c) {
    ASSERT(a.GetType() == b.GetType());

    switch (a.GetType()) {
    case Type::U16:
        return Inst<U16>(Opcode::FPMulAdd16, a, b, c);
    case Type::U32:
        return Inst<U32>(Opcode::FPMulAdd32, a, b, c);
    case Type::U64:
        return Inst<U64>(Opcode::FPMulAdd64, a, b, c);
    default:
        UNREACHABLE();
    }
}

U32U64 IREmitter::FPMulX(const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());

    switch (a.GetType()) {
    case Type::U32:
        return Inst<U32>(Opcode::FPMulX32, a, b);
    case Type::U64:
        return Inst<U64>(Opcode::FPMulX64, a, b);
    default:
        UNREACHABLE();
    }
}

U16U32U64 IREmitter::FPNeg(const U16U32U64& a) {
    switch (a.GetType()) {
    case Type::U16:
        return Inst<U16>(Opcode::FPNeg16, a);
    case Type::U32:
        return Inst<U32>(Opcode::FPNeg32, a);
    case Type::U64:
        return Inst<U64>(Opcode::FPNeg64, a);
    default:
        UNREACHABLE();
    }
}

U16U32U64 IREmitter::FPRecipEstimate(const U16U32U64& a) {
    switch (a.GetType()) {
    case Type::U16:
        return Inst<U16>(Opcode::FPRecipEstimate16, a);
    case Type::U32:
        return Inst<U32>(Opcode::FPRecipEstimate32, a);
    case Type::U64:
        return Inst<U64>(Opcode::FPRecipEstimate64, a);
    default:
        UNREACHABLE();
    }
}

U16U32U64 IREmitter::FPRecipExponent(const U16U32U64& a) {
    switch (a.GetType()) {
    case Type::U16:
        return Inst<U16>(Opcode::FPRecipExponent16, a);
    case Type::U32:
        return Inst<U32>(Opcode::FPRecipExponent32, a);
    case Type::U64:
        return Inst<U64>(Opcode::FPRecipExponent64, a);
    default:
        UNREACHABLE();
    }
}

U16U32U64 IREmitter::FPRecipStepFused(const U16U32U64& a, const U16U32U64& b) {
    ASSERT(a.GetType() == b.GetType());

    switch (a.GetType()) {
    case Type::U16:
        return Inst<U16>(Opcode::FPRecipStepFused16, a, b);
    case Type::U32:
        return Inst<U32>(Opcode::FPRecipStepFused32, a, b);
    case Type::U64:
        return Inst<U64>(Opcode::FPRecipStepFused64, a, b);
    default:
        UNREACHABLE();
    }
}

U16U32U64 IREmitter::FPRoundInt(const U16U32U64& a, FP::RoundingMode rounding, bool exact) {
    const u8 rounding_value = static_cast<u8>(rounding);
    const IR::U1 exact_imm = Imm1(exact);

    switch (a.GetType()) {
    case Type::U16:
        return Inst<U16>(Opcode::FPRoundInt16, a, rounding_value, exact_imm);
    case Type::U32:
        return Inst<U32>(Opcode::FPRoundInt32, a, rounding_value, exact_imm);
    case Type::U64:
        return Inst<U64>(Opcode::FPRoundInt64, a, rounding_value, exact_imm);
    default:
        UNREACHABLE();
    }
}

U16U32U64 IREmitter::FPRSqrtEstimate(const U16U32U64& a) {
    switch (a.GetType()) {
    case Type::U16:
        return Inst<U16>(Opcode::FPRSqrtEstimate16, a);
    case Type::U32:
        return Inst<U32>(Opcode::FPRSqrtEstimate32, a);
    case Type::U64:
        return Inst<U64>(Opcode::FPRSqrtEstimate64, a);
    default:
        UNREACHABLE();
    }
}

U16U32U64 IREmitter::FPRSqrtStepFused(const U16U32U64& a, const U16U32U64& b) {
    ASSERT(a.GetType() == b.GetType());

    switch (a.GetType()) {
    case Type::U16:
        return Inst<U16>(Opcode::FPRSqrtStepFused16, a, b);
    case Type::U32:
        return Inst<U32>(Opcode::FPRSqrtStepFused32, a, b);
    case Type::U64:
        return Inst<U64>(Opcode::FPRSqrtStepFused64, a, b);
    default:
        UNREACHABLE();
    }
}

U32U64 IREmitter::FPSqrt(const U32U64& a) {
    switch (a.GetType()) {
    case Type::U32:
        return Inst<U32>(Opcode::FPSqrt32, a);
    case Type::U64:
        return Inst<U64>(Opcode::FPSqrt64, a);
    default:
        UNREACHABLE();
    }
}

U32U64 IREmitter::FPSub(const U32U64& a, const U32U64& b) {
    ASSERT(a.GetType() == b.GetType());

    switch (a.GetType()) {
    case Type::U32:
        return Inst<U32>(Opcode::FPSub32, a, b);
    case Type::U64:
        return Inst<U64>(Opcode::FPSub64, a, b);
    default:
        UNREACHABLE();
    }
}

U16 IREmitter::FPDoubleToHalf(const U64& a, FP::RoundingMode rounding) {
    return Inst<U16>(Opcode::FPDoubleToHalf, a, Imm8(static_cast<u8>(rounding)));
}

U32 IREmitter::FPDoubleToSingle(const U64& a, FP::RoundingMode rounding) {
    return Inst<U32>(Opcode::FPDoubleToSingle, a, Imm8(static_cast<u8>(rounding)));
}

U64 IREmitter::FPHalfToDouble(const U16& a, FP::RoundingMode rounding) {
    return Inst<U64>(Opcode::FPHalfToDouble, a, Imm8(static_cast<u8>(rounding)));
}

U32 IREmitter::FPHalfToSingle(const U16& a, FP::RoundingMode rounding) {
    return Inst<U32>(Opcode::FPHalfToSingle, a, Imm8(static_cast<u8>(rounding)));
}

U64 IREmitter::FPSingleToDouble(const U32& a, FP::RoundingMode rounding) {
    return Inst<U64>(Opcode::FPSingleToDouble, a, Imm8(static_cast<u8>(rounding)));
}

U16 IREmitter::FPSingleToHalf(const U32& a, FP::RoundingMode rounding) {
    return Inst<U16>(Opcode::FPSingleToHalf, a, Imm8(static_cast<u8>(rounding)));
}

U16 IREmitter::FPToFixedS16(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding) {
    ASSERT(fbits <= 16);

    const U8 fbits_imm = Imm8(static_cast<u8>(fbits));
    const U8 rounding_imm = Imm8(static_cast<u8>(rounding));

    switch (a.GetType()) {
    case Type::U16:
        return Inst<U16>(Opcode::FPHalfToFixedS16, a, fbits_imm, rounding_imm);
    case Type::U32:
        return Inst<U16>(Opcode::FPSingleToFixedS16, a, fbits_imm, rounding_imm);
    case Type::U64:
        return Inst<U16>(Opcode::FPDoubleToFixedS16, a, fbits_imm, rounding_imm);
    default:
        UNREACHABLE();
    }
}

U32 IREmitter::FPToFixedS32(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding) {
    ASSERT(fbits <= 32);

    const U8 fbits_imm = Imm8(static_cast<u8>(fbits));
    const U8 rounding_imm = Imm8(static_cast<u8>(rounding));

    switch (a.GetType()) {
    case Type::U16:
        return Inst<U32>(Opcode::FPHalfToFixedS32, a, fbits_imm, rounding_imm);
    case Type::U32:
        return Inst<U32>(Opcode::FPSingleToFixedS32, a, fbits_imm, rounding_imm);
    case Type::U64:
        return Inst<U32>(Opcode::FPDoubleToFixedS32, a, fbits_imm, rounding_imm);
    default:
        UNREACHABLE();
    }
}

U64 IREmitter::FPToFixedS64(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding) {
    ASSERT(fbits <= 64);

    const U8 fbits_imm = Imm8(static_cast<u8>(fbits));
    const U8 rounding_imm = Imm8(static_cast<u8>(rounding));

    switch (a.GetType()) {
    case Type::U16:
        return Inst<U64>(Opcode::FPHalfToFixedS64, a, fbits_imm, rounding_imm);
    case Type::U32:
        return Inst<U64>(Opcode::FPSingleToFixedS64, a, fbits_imm, rounding_imm);
    case Type::U64:
        return Inst<U64>(Opcode::FPDoubleToFixedS64, a, fbits_imm, rounding_imm);
    default:
        UNREACHABLE();
    }
}

U16 IREmitter::FPToFixedU16(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding) {
    ASSERT(fbits <= 16);

    const U8 fbits_imm = Imm8(static_cast<u8>(fbits));
    const U8 rounding_imm = Imm8(static_cast<u8>(rounding));

    switch (a.GetType()) {
    case Type::U16:
        return Inst<U16>(Opcode::FPHalfToFixedU16, a, fbits_imm, rounding_imm);
    case Type::U32:
        return Inst<U16>(Opcode::FPSingleToFixedU16, a, fbits_imm, rounding_imm);
    case Type::U64:
        return Inst<U16>(Opcode::FPDoubleToFixedU16, a, fbits_imm, rounding_imm);
    default:
        UNREACHABLE();
    }
}

U32 IREmitter::FPToFixedU32(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding) {
    ASSERT(fbits <= 32);

    const U8 fbits_imm = Imm8(static_cast<u8>(fbits));
    const U8 rounding_imm = Imm8(static_cast<u8>(rounding));

    switch (a.GetType()) {
    case Type::U16:
        return Inst<U32>(Opcode::FPHalfToFixedU32, a, fbits_imm, rounding_imm);
    case Type::U32:
        return Inst<U32>(Opcode::FPSingleToFixedU32, a, fbits_imm, rounding_imm);
    case Type::U64:
        return Inst<U32>(Opcode::FPDoubleToFixedU32, a, fbits_imm, rounding_imm);
    default:
        UNREACHABLE();
    }
}

U64 IREmitter::FPToFixedU64(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding) {
    ASSERT(fbits <= 64);

    const U8 fbits_imm = Imm8(static_cast<u8>(fbits));
    const U8 rounding_imm = Imm8(static_cast<u8>(rounding));

    switch (a.GetType()) {
    case Type::U16:
        return Inst<U64>(Opcode::FPHalfToFixedU64, a, fbits_imm, rounding_imm);
    case Type::U32:
        return Inst<U64>(Opcode::FPSingleToFixedU64, a, fbits_imm, rounding_imm);
    case Type::U64:
        return Inst<U64>(Opcode::FPDoubleToFixedU64, a, fbits_imm, rounding_imm);
    default:
        UNREACHABLE();
    }
}

U32 IREmitter::FPSignedFixedToSingle(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding) {
    ASSERT(fbits <= (a.GetType() == Type::U16 ? 16 : (a.GetType() == Type::U32 ? 32 : 64)));

    const IR::U8 fbits_imm = Imm8(static_cast<u8>(fbits));
    const IR::U8 rounding_imm = Imm8(static_cast<u8>(rounding));

    switch (a.GetType()) {
    case Type::U16:
        return Inst<U32>(Opcode::FPFixedS16ToSingle, a, fbits_imm, rounding_imm);
    case Type::U32:
        return Inst<U32>(Opcode::FPFixedS32ToSingle, a, fbits_imm, rounding_imm);
    case Type::U64:
        return Inst<U32>(Opcode::FPFixedS64ToSingle, a, fbits_imm, rounding_imm);
    default:
        UNREACHABLE();
    }
}

U32 IREmitter::FPUnsignedFixedToSingle(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding) {
    ASSERT(fbits <= (a.GetType() == Type::U16 ? 16 : (a.GetType() == Type::U32 ? 32 : 64)));

    const IR::U8 fbits_imm = Imm8(static_cast<u8>(fbits));
    const IR::U8 rounding_imm = Imm8(static_cast<u8>(rounding));

    switch (a.GetType()) {
    case Type::U16:
        return Inst<U32>(Opcode::FPFixedU16ToSingle, a, fbits_imm, rounding_imm);
    case Type::U32:
        return Inst<U32>(Opcode::FPFixedU32ToSingle, a, fbits_imm, rounding_imm);
    case Type::U64:
        return Inst<U32>(Opcode::FPFixedU64ToSingle, a, fbits_imm, rounding_imm);
    default:
        UNREACHABLE();
    }
}

U64 IREmitter::FPSignedFixedToDouble(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding) {
    ASSERT(fbits <= (a.GetType() == Type::U16 ? 16 : (a.GetType() == Type::U32 ? 32 : 64)));

    const IR::U8 fbits_imm = Imm8(static_cast<u8>(fbits));
    const IR::U8 rounding_imm = Imm8(static_cast<u8>(rounding));

    switch (a.GetType()) {
    case Type::U16:
        return Inst<U64>(Opcode::FPFixedS16ToDouble, a, fbits_imm, rounding_imm);
    case Type::U32:
        return Inst<U64>(Opcode::FPFixedS32ToDouble, a, fbits_imm, rounding_imm);
    case Type::U64:
        return Inst<U64>(Opcode::FPFixedS64ToDouble, a, fbits_imm, rounding_imm);
    default:
        UNREACHABLE();
    }
}

U64 IREmitter::FPUnsignedFixedToDouble(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding) {
    ASSERT(fbits <= (a.GetType() == Type::U16 ? 16 : (a.GetType() == Type::U32 ? 32 : 64)));

    const IR::U8 fbits_imm = Imm8(static_cast<u8>(fbits));
    const IR::U8 rounding_imm = Imm8(static_cast<u8>(rounding));

    switch (a.GetType()) {
    case Type::U16:
        return Inst<U64>(Opcode::FPFixedU16ToDouble, a, fbits_imm, rounding_imm);
    case Type::U32:
        return Inst<U64>(Opcode::FPFixedU32ToDouble, a, fbits_imm, rounding_imm);
    case Type::U64:
        return Inst<U64>(Opcode::FPFixedU64ToDouble, a, fbits_imm, rounding_imm);
    default:
        UNREACHABLE();
    }
}

U128 IREmitter::FPVectorAbs(size_t esize, const U128& a) {
    switch (esize) {
    case 16:
        return Inst<U128>(Opcode::FPVectorAbs16, a);
    case 32:
        return Inst<U128>(Opcode::FPVectorAbs32, a);
    case 64:
        return Inst<U128>(Opcode::FPVectorAbs64, a);
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorAdd(size_t esize, const U128& a, const U128& b, bool fpcr_controlled) {
    switch (esize) {
    case 32:
        return Inst<U128>(Opcode::FPVectorAdd32, a, b, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorAdd64, a, b, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorDiv(size_t esize, const U128& a, const U128& b, bool fpcr_controlled) {
    switch (esize) {
    case 32:
        return Inst<U128>(Opcode::FPVectorDiv32, a, b, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorDiv64, a, b, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorEqual(size_t esize, const U128& a, const U128& b, bool fpcr_controlled) {
    switch (esize) {
    case 16:
        return Inst<U128>(Opcode::FPVectorEqual16, a, b, Imm1(fpcr_controlled));
    case 32:
        return Inst<U128>(Opcode::FPVectorEqual32, a, b, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorEqual64, a, b, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorFromHalf(size_t esize, const U128& a, FP::RoundingMode rounding, bool fpcr_controlled) {
    ASSERT(esize == 32);
    return Inst<U128>(Opcode::FPVectorFromHalf32, a, Imm8(static_cast<u8>(rounding)), Imm1(fpcr_controlled));
}

U128 IREmitter::FPVectorFromSignedFixed(size_t esize, const U128& a, size_t fbits, FP::RoundingMode rounding, bool fpcr_controlled) {
    ASSERT(fbits <= esize);
    switch (esize) {
    case 32:
        return Inst<U128>(Opcode::FPVectorFromSignedFixed32, a, Imm8(static_cast<u8>(fbits)), Imm8(static_cast<u8>(rounding)), Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorFromSignedFixed64, a, Imm8(static_cast<u8>(fbits)), Imm8(static_cast<u8>(rounding)), Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorFromUnsignedFixed(size_t esize, const U128& a, size_t fbits, FP::RoundingMode rounding, bool fpcr_controlled) {
    ASSERT(fbits <= esize);
    switch (esize) {
    case 32:
        return Inst<U128>(Opcode::FPVectorFromUnsignedFixed32, a, Imm8(static_cast<u8>(fbits)), Imm8(static_cast<u8>(rounding)), Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorFromUnsignedFixed64, a, Imm8(static_cast<u8>(fbits)), Imm8(static_cast<u8>(rounding)), Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorGreater(size_t esize, const U128& a, const U128& b, bool fpcr_controlled) {
    switch (esize) {
    case 32:
        return Inst<U128>(Opcode::FPVectorGreater32, a, b, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorGreater64, a, b, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorGreaterEqual(size_t esize, const U128& a, const U128& b, bool fpcr_controlled) {
    switch (esize) {
    case 32:
        return Inst<U128>(Opcode::FPVectorGreaterEqual32, a, b, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorGreaterEqual64, a, b, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorMax(size_t esize, const U128& a, const U128& b, bool fpcr_controlled) {
    switch (esize) {
    case 32:
        return Inst<U128>(Opcode::FPVectorMax32, a, b, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorMax64, a, b, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorMin(size_t esize, const U128& a, const U128& b, bool fpcr_controlled) {
    switch (esize) {
    case 32:
        return Inst<U128>(Opcode::FPVectorMin32, a, b, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorMin64, a, b, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorMul(size_t esize, const U128& a, const U128& b, bool fpcr_controlled) {
    switch (esize) {
    case 32:
        return Inst<U128>(Opcode::FPVectorMul32, a, b, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorMul64, a, b, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorMulAdd(size_t esize, const U128& a, const U128& b, const U128& c, bool fpcr_controlled) {
    switch (esize) {
    case 16:
        return Inst<U128>(Opcode::FPVectorMulAdd16, a, b, c, Imm1(fpcr_controlled));
    case 32:
        return Inst<U128>(Opcode::FPVectorMulAdd32, a, b, c, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorMulAdd64, a, b, c, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorMulX(size_t esize, const U128& a, const U128& b, bool fpcr_controlled) {
    switch (esize) {
    case 32:
        return Inst<U128>(Opcode::FPVectorMulX32, a, b, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorMulX64, a, b, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorNeg(size_t esize, const U128& a) {
    switch (esize) {
    case 16:
        return Inst<U128>(Opcode::FPVectorNeg16, a);
    case 32:
        return Inst<U128>(Opcode::FPVectorNeg32, a);
    case 64:
        return Inst<U128>(Opcode::FPVectorNeg64, a);
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorPairedAdd(size_t esize, const U128& a, const U128& b, bool fpcr_controlled) {
    switch (esize) {
    case 32:
        return Inst<U128>(Opcode::FPVectorPairedAdd32, a, b, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorPairedAdd64, a, b, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorPairedAddLower(size_t esize, const U128& a, const U128& b, bool fpcr_controlled) {
    switch (esize) {
    case 32:
        return Inst<U128>(Opcode::FPVectorPairedAddLower32, a, b, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorPairedAddLower64, a, b, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorRecipEstimate(size_t esize, const U128& a, bool fpcr_controlled) {
    switch (esize) {
    case 16:
        return Inst<U128>(Opcode::FPVectorRecipEstimate16, a, Imm1(fpcr_controlled));
    case 32:
        return Inst<U128>(Opcode::FPVectorRecipEstimate32, a, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorRecipEstimate64, a, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorRecipStepFused(size_t esize, const U128& a, const U128& b, bool fpcr_controlled) {
    switch (esize) {
    case 16:
        return Inst<U128>(Opcode::FPVectorRecipStepFused16, a, b, Imm1(fpcr_controlled));
    case 32:
        return Inst<U128>(Opcode::FPVectorRecipStepFused32, a, b, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorRecipStepFused64, a, b, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorRoundInt(size_t esize, const U128& operand, FP::RoundingMode rounding, bool exact, bool fpcr_controlled) {
    const IR::U8 rounding_imm = Imm8(static_cast<u8>(rounding));
    const IR::U1 exact_imm = Imm1(exact);

    switch (esize) {
    case 16:
        return Inst<U128>(Opcode::FPVectorRoundInt16, operand, rounding_imm, exact_imm, Imm1(fpcr_controlled));
    case 32:
        return Inst<U128>(Opcode::FPVectorRoundInt32, operand, rounding_imm, exact_imm, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorRoundInt64, operand, rounding_imm, exact_imm, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorRSqrtEstimate(size_t esize, const U128& a, bool fpcr_controlled) {
    switch (esize) {
    case 16:
        return Inst<U128>(Opcode::FPVectorRSqrtEstimate16, a, Imm1(fpcr_controlled));
    case 32:
        return Inst<U128>(Opcode::FPVectorRSqrtEstimate32, a, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorRSqrtEstimate64, a, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorRSqrtStepFused(size_t esize, const U128& a, const U128& b, bool fpcr_controlled) {
    switch (esize) {
    case 16:
        return Inst<U128>(Opcode::FPVectorRSqrtStepFused16, a, b, Imm1(fpcr_controlled));
    case 32:
        return Inst<U128>(Opcode::FPVectorRSqrtStepFused32, a, b, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorRSqrtStepFused64, a, b, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorSqrt(size_t esize, const U128& a, bool fpcr_controlled) {
    switch (esize) {
    case 32:
        return Inst<U128>(Opcode::FPVectorSqrt32, a, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorSqrt64, a, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorSub(size_t esize, const U128& a, const U128& b, bool fpcr_controlled) {
    switch (esize) {
    case 32:
        return Inst<U128>(Opcode::FPVectorSub32, a, b, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorSub64, a, b, Imm1(fpcr_controlled));
    }
    UNREACHABLE();
}

U128 IREmitter::FPVectorToHalf(size_t esize, const U128& a, FP::RoundingMode rounding, bool fpcr_controlled) {
    ASSERT(esize == 32);
    return Inst<U128>(Opcode::FPVectorToHalf32, a, Imm8(static_cast<u8>(rounding)), Imm1(fpcr_controlled));
}

U128 IREmitter::FPVectorToSignedFixed(size_t esize, const U128& a, size_t fbits, FP::RoundingMode rounding, bool fpcr_controlled) {
    ASSERT(fbits <= esize);

    const U8 fbits_imm = Imm8(static_cast<u8>(fbits));
    const U8 rounding_imm = Imm8(static_cast<u8>(rounding));

    switch (esize) {
    case 16:
        return Inst<U128>(Opcode::FPVectorToSignedFixed16, a, fbits_imm, rounding_imm, Imm1(fpcr_controlled));
    case 32:
        return Inst<U128>(Opcode::FPVectorToSignedFixed32, a, fbits_imm, rounding_imm, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorToSignedFixed64, a, fbits_imm, rounding_imm, Imm1(fpcr_controlled));
    }

    UNREACHABLE();
}

U128 IREmitter::FPVectorToUnsignedFixed(size_t esize, const U128& a, size_t fbits, FP::RoundingMode rounding, bool fpcr_controlled) {
    ASSERT(fbits <= esize);

    const U8 fbits_imm = Imm8(static_cast<u8>(fbits));
    const U8 rounding_imm = Imm8(static_cast<u8>(rounding));

    switch (esize) {
    case 16:
        return Inst<U128>(Opcode::FPVectorToUnsignedFixed16, a, fbits_imm, rounding_imm, Imm1(fpcr_controlled));
    case 32:
        return Inst<U128>(Opcode::FPVectorToUnsignedFixed32, a, fbits_imm, rounding_imm, Imm1(fpcr_controlled));
    case 64:
        return Inst<U128>(Opcode::FPVectorToUnsignedFixed64, a, fbits_imm, rounding_imm, Imm1(fpcr_controlled));
    }

    UNREACHABLE();
}

void IREmitter::Breakpoint() {
    Inst(Opcode::Breakpoint);
}

void IREmitter::CallHostFunction(void (*fn)(void)) {
    Inst(Opcode::CallHostFunction, Imm64(Common::BitCast<u64>(fn)));
}

void IREmitter::CallHostFunction(void (*fn)(u64), const U64& arg1) {
    Inst(Opcode::CallHostFunction, Imm64(Common::BitCast<u64>(fn)), arg1);
}

void IREmitter::CallHostFunction(void (*fn)(u64, u64), const U64& arg1, const U64& arg2) {
    Inst(Opcode::CallHostFunction, Imm64(Common::BitCast<u64>(fn)), arg1, arg2);
}

void IREmitter::CallHostFunction(void (*fn)(u64, u64, u64), const U64& arg1, const U64& arg2, const U64& arg3) {
    Inst(Opcode::CallHostFunction, Imm64(Common::BitCast<u64>(fn)), arg1, arg2, arg3);
}

void IREmitter::SetTerm(const Terminal& terminal) {
    block.SetTerminal(terminal);
}

}  // namespace Dynarmic::IR

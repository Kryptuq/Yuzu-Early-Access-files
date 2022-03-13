/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "dynarmic/common/common_types.h"
#include "dynarmic/ir/access_type.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/location_descriptor.h"
#include "dynarmic/ir/terminal.h"
#include "dynarmic/ir/value.h"

namespace Dynarmic::FP {
enum class RoundingMode;
}  // namespace Dynarmic::FP

// ARM JIT Microinstruction Intermediate Representation
//
// This intermediate representation is an SSA IR. It is designed primarily for analysis,
// though it can be lowered into a reduced form for interpretation. Each IR node (Value)
// is a microinstruction of an idealised ARM CPU. The choice of microinstructions is made
// not based on any existing microarchitecture but on ease of implementation.

namespace Dynarmic::IR {

enum class Opcode;

template<typename T>
struct ResultAndCarry {
    T result;
    U1 carry;
};

template<typename T>
struct ResultAndOverflow {
    T result;
    U1 overflow;
};

template<typename T>
struct ResultAndCarryAndOverflow {
    T result;
    U1 carry;
    U1 overflow;
};

template<typename T>
struct ResultAndGE {
    T result;
    U32 ge;
};

struct UpperAndLower {
    U128 upper;
    U128 lower;
};

enum class MemOp {
    LOAD,
    STORE,
    PREFETCH,
};

/**
 * Convenience class to construct a basic block of the intermediate representation.
 * `block` is the resulting block.
 * The user of this class updates `current_location` as appropriate.
 */
class IREmitter {
public:
    explicit IREmitter(Block& block)
            : block(block), insertion_point(block.end()) {}

    Block& block;

    U1 Imm1(bool value) const;
    U8 Imm8(u8 value) const;
    U16 Imm16(u16 value) const;
    U32 Imm32(u32 value) const;
    U64 Imm64(u64 value) const;

    void PushRSB(const LocationDescriptor& return_location);

    U64 Pack2x32To1x64(const U32& lo, const U32& hi);
    U128 Pack2x64To1x128(const U64& lo, const U64& hi);
    UAny LeastSignificant(size_t bitsize, const U32U64& value);
    U32 LeastSignificantWord(const U64& value);
    U16 LeastSignificantHalf(U32U64 value);
    U8 LeastSignificantByte(U32U64 value);
    ResultAndCarry<U32> MostSignificantWord(const U64& value);
    U1 MostSignificantBit(const U32& value);
    U1 IsZero(const U32& value);
    U1 IsZero(const U64& value);
    U1 IsZero(const U32U64& value);
    U1 TestBit(const U32U64& value, const U8& bit);
    U32 ConditionalSelect(Cond cond, const U32& a, const U32& b);
    U64 ConditionalSelect(Cond cond, const U64& a, const U64& b);
    NZCV ConditionalSelect(Cond cond, const NZCV& a, const NZCV& b);
    U32U64 ConditionalSelect(Cond cond, const U32U64& a, const U32U64& b);

    NZCV NZCVFromPackedFlags(const U32& a);
    // This pseudo-instruction may only be added to instructions that support it.
    NZCV NZCVFrom(const Value& value);

    ResultAndCarry<U32> LogicalShiftLeft(const U32& value_in, const U8& shift_amount, const U1& carry_in);
    ResultAndCarry<U32> LogicalShiftRight(const U32& value_in, const U8& shift_amount, const U1& carry_in);
    ResultAndCarry<U32> ArithmeticShiftRight(const U32& value_in, const U8& shift_amount, const U1& carry_in);
    ResultAndCarry<U32> RotateRight(const U32& value_in, const U8& shift_amount, const U1& carry_in);
    U32U64 LogicalShiftLeft(const U32U64& value_in, const U8& shift_amount);
    U32U64 LogicalShiftRight(const U32U64& value_in, const U8& shift_amount);
    U32U64 ArithmeticShiftRight(const U32U64& value_in, const U8& shift_amount);
    U32U64 RotateRight(const U32U64& value_in, const U8& shift_amount);
    U32U64 LogicalShiftLeftMasked(const U32U64& value_in, const U32U64& shift_amount);
    U32U64 LogicalShiftRightMasked(const U32U64& value_in, const U32U64& shift_amount);
    U32U64 ArithmeticShiftRightMasked(const U32U64& value_in, const U32U64& shift_amount);
    U32U64 RotateRightMasked(const U32U64& value_in, const U32U64& shift_amount);
    ResultAndCarry<U32> RotateRightExtended(const U32& value_in, const U1& carry_in);
    U32U64 AddWithCarry(const U32U64& a, const U32U64& b, const U1& carry_in);
    U32U64 SubWithCarry(const U32U64& a, const U32U64& b, const U1& carry_in);
    U32U64 Add(const U32U64& a, const U32U64& b);
    U32U64 Sub(const U32U64& a, const U32U64& b);
    U32U64 Mul(const U32U64& a, const U32U64& b);
    U64 UnsignedMultiplyHigh(const U64& a, const U64& b);
    U64 SignedMultiplyHigh(const U64& a, const U64& b);
    U32U64 UnsignedDiv(const U32U64& a, const U32U64& b);
    U32U64 SignedDiv(const U32U64& a, const U32U64& b);
    U32U64 And(const U32U64& a, const U32U64& b);
    U32U64 AndNot(const U32U64& a, const U32U64& b);
    U32U64 Eor(const U32U64& a, const U32U64& b);
    U32U64 Or(const U32U64& a, const U32U64& b);
    U32U64 Not(const U32U64& a);
    U32 SignExtendToWord(const UAny& a);
    U64 SignExtendToLong(const UAny& a);
    U32 SignExtendByteToWord(const U8& a);
    U32 SignExtendHalfToWord(const U16& a);
    U64 SignExtendWordToLong(const U32& a);
    U32 ZeroExtendToWord(const UAny& a);
    U64 ZeroExtendToLong(const UAny& a);
    U128 ZeroExtendToQuad(const UAny& a);
    U32 ZeroExtendByteToWord(const U8& a);
    U32 ZeroExtendHalfToWord(const U16& a);
    U64 ZeroExtendWordToLong(const U32& a);
    U32 IndeterminateExtendToWord(const UAny& a);
    U64 IndeterminateExtendToLong(const UAny& a);
    U32 ByteReverseWord(const U32& a);
    U16 ByteReverseHalf(const U16& a);
    U64 ByteReverseDual(const U64& a);
    U32U64 CountLeadingZeros(const U32U64& a);
    U32U64 ExtractRegister(const U32U64& a, const U32U64& b, const U8& lsb);
    U32U64 ReplicateBit(const U32U64& a, u8 bit);
    U32U64 MaxSigned(const U32U64& a, const U32U64& b);
    U32U64 MaxUnsigned(const U32U64& a, const U32U64& b);
    U32U64 MinSigned(const U32U64& a, const U32U64& b);
    U32U64 MinUnsigned(const U32U64& a, const U32U64& b);

    ResultAndOverflow<UAny> SignedSaturatedAdd(const UAny& a, const UAny& b);
    ResultAndOverflow<UAny> SignedSaturatedDoublingMultiplyReturnHigh(const UAny& a, const UAny& b);
    ResultAndOverflow<UAny> SignedSaturatedSub(const UAny& a, const UAny& b);
    ResultAndOverflow<U32> SignedSaturation(const U32& a, size_t bit_size_to_saturate_to);
    ResultAndOverflow<UAny> UnsignedSaturatedAdd(const UAny& a, const UAny& b);
    ResultAndOverflow<UAny> UnsignedSaturatedSub(const UAny& a, const UAny& b);
    ResultAndOverflow<U32> UnsignedSaturation(const U32& a, size_t bit_size_to_saturate_to);

    U128 VectorSignedSaturatedAdd(size_t esize, const U128& a, const U128& b);
    U128 VectorSignedSaturatedSub(size_t esize, const U128& a, const U128& b);
    U128 VectorUnsignedSaturatedAdd(size_t esize, const U128& a, const U128& b);
    U128 VectorUnsignedSaturatedSub(size_t esize, const U128& a, const U128& b);

    ResultAndGE<U32> PackedAddU8(const U32& a, const U32& b);
    ResultAndGE<U32> PackedAddS8(const U32& a, const U32& b);
    ResultAndGE<U32> PackedAddU16(const U32& a, const U32& b);
    ResultAndGE<U32> PackedAddS16(const U32& a, const U32& b);
    ResultAndGE<U32> PackedSubU8(const U32& a, const U32& b);
    ResultAndGE<U32> PackedSubS8(const U32& a, const U32& b);
    ResultAndGE<U32> PackedSubU16(const U32& a, const U32& b);
    ResultAndGE<U32> PackedSubS16(const U32& a, const U32& b);
    ResultAndGE<U32> PackedAddSubU16(const U32& a, const U32& b);
    ResultAndGE<U32> PackedAddSubS16(const U32& a, const U32& b);
    ResultAndGE<U32> PackedSubAddU16(const U32& a, const U32& b);
    ResultAndGE<U32> PackedSubAddS16(const U32& a, const U32& b);
    U32 PackedHalvingAddU8(const U32& a, const U32& b);
    U32 PackedHalvingAddS8(const U32& a, const U32& b);
    U32 PackedHalvingSubU8(const U32& a, const U32& b);
    U32 PackedHalvingSubS8(const U32& a, const U32& b);
    U32 PackedHalvingAddU16(const U32& a, const U32& b);
    U32 PackedHalvingAddS16(const U32& a, const U32& b);
    U32 PackedHalvingSubU16(const U32& a, const U32& b);
    U32 PackedHalvingSubS16(const U32& a, const U32& b);
    U32 PackedHalvingAddSubU16(const U32& a, const U32& b);
    U32 PackedHalvingAddSubS16(const U32& a, const U32& b);
    U32 PackedHalvingSubAddU16(const U32& a, const U32& b);
    U32 PackedHalvingSubAddS16(const U32& a, const U32& b);
    U32 PackedSaturatedAddU8(const U32& a, const U32& b);
    U32 PackedSaturatedAddS8(const U32& a, const U32& b);
    U32 PackedSaturatedSubU8(const U32& a, const U32& b);
    U32 PackedSaturatedSubS8(const U32& a, const U32& b);
    U32 PackedSaturatedAddU16(const U32& a, const U32& b);
    U32 PackedSaturatedAddS16(const U32& a, const U32& b);
    U32 PackedSaturatedSubU16(const U32& a, const U32& b);
    U32 PackedSaturatedSubS16(const U32& a, const U32& b);
    U32 PackedAbsDiffSumS8(const U32& a, const U32& b);
    U32 PackedSelect(const U32& ge, const U32& a, const U32& b);

    U32 CRC32Castagnoli8(const U32& a, const U32& b);
    U32 CRC32Castagnoli16(const U32& a, const U32& b);
    U32 CRC32Castagnoli32(const U32& a, const U32& b);
    U32 CRC32Castagnoli64(const U32& a, const U64& b);
    U32 CRC32ISO8(const U32& a, const U32& b);
    U32 CRC32ISO16(const U32& a, const U32& b);
    U32 CRC32ISO32(const U32& a, const U32& b);
    U32 CRC32ISO64(const U32& a, const U64& b);

    U128 AESDecryptSingleRound(const U128& a);
    U128 AESEncryptSingleRound(const U128& a);
    U128 AESInverseMixColumns(const U128& a);
    U128 AESMixColumns(const U128& a);

    U8 SM4AccessSubstitutionBox(const U8& a);

    UAny VectorGetElement(size_t esize, const U128& a, size_t index);
    U128 VectorSetElement(size_t esize, const U128& a, size_t index, const UAny& elem);
    U128 VectorAbs(size_t esize, const U128& a);
    U128 VectorAdd(size_t esize, const U128& a, const U128& b);
    U128 VectorAnd(const U128& a, const U128& b);
    U128 VectorAndNot(const U128& a, const U128& b);
    U128 VectorArithmeticShiftRight(size_t esize, const U128& a, u8 shift_amount);
    U128 VectorArithmeticVShift(size_t esize, const U128& a, const U128& b);
    U128 VectorBroadcast(size_t esize, const UAny& a);
    U128 VectorBroadcastLower(size_t esize, const UAny& a);
    U128 VectorBroadcastElement(size_t esize, const U128& a, size_t index);
    U128 VectorBroadcastElementLower(size_t esize, const U128& a, size_t index);
    U128 VectorCountLeadingZeros(size_t esize, const U128& a);
    U128 VectorEor(const U128& a, const U128& b);
    U128 VectorDeinterleaveEven(size_t esize, const U128& a, const U128& b);
    U128 VectorDeinterleaveEvenLower(size_t esize, const U128& a, const U128& b);
    U128 VectorDeinterleaveOdd(size_t esize, const U128& a, const U128& b);
    U128 VectorDeinterleaveOddLower(size_t esize, const U128& a, const U128& b);
    U128 VectorEqual(size_t esize, const U128& a, const U128& b);
    U128 VectorExtract(const U128& a, const U128& b, size_t position);
    U128 VectorExtractLower(const U128& a, const U128& b, size_t position);
    U128 VectorGreaterEqualSigned(size_t esize, const U128& a, const U128& b);
    U128 VectorGreaterEqualUnsigned(size_t esize, const U128& a, const U128& b);
    U128 VectorGreaterSigned(size_t esize, const U128& a, const U128& b);
    U128 VectorGreaterUnsigned(size_t esize, const U128& a, const U128& b);
    U128 VectorHalvingAddSigned(size_t esize, const U128& a, const U128& b);
    U128 VectorHalvingAddUnsigned(size_t esize, const U128& a, const U128& b);
    U128 VectorHalvingSubSigned(size_t esize, const U128& a, const U128& b);
    U128 VectorHalvingSubUnsigned(size_t esize, const U128& a, const U128& b);
    U128 VectorInterleaveLower(size_t esize, const U128& a, const U128& b);
    U128 VectorInterleaveUpper(size_t esize, const U128& a, const U128& b);
    U128 VectorLessEqualSigned(size_t esize, const U128& a, const U128& b);
    U128 VectorLessEqualUnsigned(size_t esize, const U128& a, const U128& b);
    U128 VectorLessSigned(size_t esize, const U128& a, const U128& b);
    U128 VectorLessUnsigned(size_t esize, const U128& a, const U128& b);
    U128 VectorLogicalShiftLeft(size_t esize, const U128& a, u8 shift_amount);
    U128 VectorLogicalShiftRight(size_t esize, const U128& a, u8 shift_amount);
    U128 VectorLogicalVShift(size_t esize, const U128& a, const U128& b);
    U128 VectorMaxSigned(size_t esize, const U128& a, const U128& b);
    U128 VectorMaxUnsigned(size_t esize, const U128& a, const U128& b);
    U128 VectorMinSigned(size_t esize, const U128& a, const U128& b);
    U128 VectorMinUnsigned(size_t esize, const U128& a, const U128& b);
    U128 VectorMultiply(size_t esize, const U128& a, const U128& b);
    U128 VectorNarrow(size_t original_esize, const U128& a);
    U128 VectorNot(const U128& a);
    U128 VectorOr(const U128& a, const U128& b);
    U128 VectorPairedAdd(size_t esize, const U128& a, const U128& b);
    U128 VectorPairedAddLower(size_t esize, const U128& a, const U128& b);
    U128 VectorPairedAddSignedWiden(size_t original_esize, const U128& a);
    U128 VectorPairedAddUnsignedWiden(size_t original_esize, const U128& a);
    U128 VectorPairedMaxSigned(size_t esize, const U128& a, const U128& b);
    U128 VectorPairedMaxUnsigned(size_t esize, const U128& a, const U128& b);
    U128 VectorPairedMinSigned(size_t esize, const U128& a, const U128& b);
    U128 VectorPairedMinUnsigned(size_t esize, const U128& a, const U128& b);
    U128 VectorPolynomialMultiply(const U128& a, const U128& b);
    U128 VectorPolynomialMultiplyLong(size_t esize, const U128& a, const U128& b);
    U128 VectorPopulationCount(const U128& a);
    U128 VectorReverseBits(const U128& a);
    U128 VectorReduceAdd(size_t esize, const U128& a);
    U128 VectorRotateLeft(size_t esize, const U128& a, u8 amount);
    U128 VectorRotateRight(size_t esize, const U128& a, u8 amount);
    U128 VectorRoundingHalvingAddSigned(size_t esize, const U128& a, const U128& b);
    U128 VectorRoundingHalvingAddUnsigned(size_t esize, const U128& a, const U128& b);
    U128 VectorRoundingShiftLeftSigned(size_t esize, const U128& a, const U128& b);
    U128 VectorRoundingShiftLeftUnsigned(size_t esize, const U128& a, const U128& b);
    U128 VectorShuffleHighHalfwords(const U128& a, u8 mask);
    U128 VectorShuffleLowHalfwords(const U128& a, u8 mask);
    U128 VectorShuffleWords(const U128& a, u8 mask);
    U128 VectorSignExtend(size_t original_esize, const U128& a);
    U128 VectorSignedAbsoluteDifference(size_t esize, const U128& a, const U128& b);
    UpperAndLower VectorSignedMultiply(size_t esize, const U128& a, const U128& b);
    U128 VectorSignedSaturatedAbs(size_t esize, const U128& a);
    U128 VectorSignedSaturatedAccumulateUnsigned(size_t esize, const U128& a, const U128& b);
    UpperAndLower VectorSignedSaturatedDoublingMultiply(size_t esize, const U128& a, const U128& b);
    U128 VectorSignedSaturatedDoublingMultiplyLong(size_t esize, const U128& a, const U128& b);
    U128 VectorSignedSaturatedNarrowToSigned(size_t original_esize, const U128& a);
    U128 VectorSignedSaturatedNarrowToUnsigned(size_t original_esize, const U128& a);
    U128 VectorSignedSaturatedNeg(size_t esize, const U128& a);
    U128 VectorSignedSaturatedShiftLeft(size_t esize, const U128& a, const U128& b);
    U128 VectorSignedSaturatedShiftLeftUnsigned(size_t esize, const U128& a, const U128& b);
    U128 VectorSub(size_t esize, const U128& a, const U128& b);
    Table VectorTable(std::vector<U64> values);
    Table VectorTable(std::vector<U128> values);
    U64 VectorTableLookup(const U64& defaults, const Table& table, const U64& indices);
    U128 VectorTableLookup(const U128& defaults, const Table& table, const U128& indices);
    U128 VectorTranspose(size_t esize, const U128& a, const U128& b, bool part);
    U128 VectorUnsignedAbsoluteDifference(size_t esize, const U128& a, const U128& b);
    U128 VectorUnsignedRecipEstimate(const U128& a);
    U128 VectorUnsignedRecipSqrtEstimate(const U128& a);
    U128 VectorUnsignedSaturatedAccumulateSigned(size_t esize, const U128& a, const U128& b);
    U128 VectorUnsignedSaturatedNarrow(size_t esize, const U128& a);
    U128 VectorUnsignedSaturatedShiftLeft(size_t esize, const U128& a, const U128& b);
    U128 VectorZeroExtend(size_t original_esize, const U128& a);
    U128 VectorZeroUpper(const U128& a);
    U128 ZeroVector();

    U16U32U64 FPAbs(const U16U32U64& a);
    U32U64 FPAdd(const U32U64& a, const U32U64& b);
    NZCV FPCompare(const U32U64& a, const U32U64& b, bool exc_on_qnan);
    U32U64 FPDiv(const U32U64& a, const U32U64& b);
    U32U64 FPMax(const U32U64& a, const U32U64& b);
    U32U64 FPMaxNumeric(const U32U64& a, const U32U64& b);
    U32U64 FPMin(const U32U64& a, const U32U64& b);
    U32U64 FPMinNumeric(const U32U64& a, const U32U64& b);
    U32U64 FPMul(const U32U64& a, const U32U64& b);
    U16U32U64 FPMulAdd(const U16U32U64& addend, const U16U32U64& op1, const U16U32U64& op2);
    U32U64 FPMulX(const U32U64& a, const U32U64& b);
    U16U32U64 FPNeg(const U16U32U64& a);
    U16U32U64 FPRecipEstimate(const U16U32U64& a);
    U16U32U64 FPRecipExponent(const U16U32U64& a);
    U16U32U64 FPRecipStepFused(const U16U32U64& a, const U16U32U64& b);
    U16U32U64 FPRoundInt(const U16U32U64& a, FP::RoundingMode rounding, bool exact);
    U16U32U64 FPRSqrtEstimate(const U16U32U64& a);
    U16U32U64 FPRSqrtStepFused(const U16U32U64& a, const U16U32U64& b);
    U32U64 FPSqrt(const U32U64& a);
    U32U64 FPSub(const U32U64& a, const U32U64& b);
    U16 FPDoubleToHalf(const U64& a, FP::RoundingMode rounding);
    U32 FPDoubleToSingle(const U64& a, FP::RoundingMode rounding);
    U64 FPHalfToDouble(const U16& a, FP::RoundingMode rounding);
    U32 FPHalfToSingle(const U16& a, FP::RoundingMode rounding);
    U16 FPSingleToHalf(const U32& a, FP::RoundingMode rounding);
    U64 FPSingleToDouble(const U32& a, FP::RoundingMode rounding);
    U16 FPToFixedS16(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding);
    U32 FPToFixedS32(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding);
    U64 FPToFixedS64(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding);
    U16 FPToFixedU16(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding);
    U32 FPToFixedU32(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding);
    U64 FPToFixedU64(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding);
    U32 FPSignedFixedToSingle(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding);
    U32 FPUnsignedFixedToSingle(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding);
    U64 FPSignedFixedToDouble(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding);
    U64 FPUnsignedFixedToDouble(const U16U32U64& a, size_t fbits, FP::RoundingMode rounding);

    U128 FPVectorAbs(size_t esize, const U128& a);
    U128 FPVectorAdd(size_t esize, const U128& a, const U128& b, bool fpcr_controlled = true);
    U128 FPVectorDiv(size_t esize, const U128& a, const U128& b, bool fpcr_controlled = true);
    U128 FPVectorEqual(size_t esize, const U128& a, const U128& b, bool fpcr_controlled = true);
    U128 FPVectorFromHalf(size_t esize, const U128& a, FP::RoundingMode rounding, bool fpcr_controlled = true);
    U128 FPVectorFromSignedFixed(size_t esize, const U128& a, size_t fbits, FP::RoundingMode rounding, bool fpcr_controlled = true);
    U128 FPVectorFromUnsignedFixed(size_t esize, const U128& a, size_t fbits, FP::RoundingMode rounding, bool fpcr_controlled = true);
    U128 FPVectorGreater(size_t esize, const U128& a, const U128& b, bool fpcr_controlled = true);
    U128 FPVectorGreaterEqual(size_t esize, const U128& a, const U128& b, bool fpcr_controlled = true);
    U128 FPVectorMax(size_t esize, const U128& a, const U128& b, bool fpcr_controlled = true);
    U128 FPVectorMin(size_t esize, const U128& a, const U128& b, bool fpcr_controlled = true);
    U128 FPVectorMul(size_t esize, const U128& a, const U128& b, bool fpcr_controlled = true);
    U128 FPVectorMulAdd(size_t esize, const U128& addend, const U128& op1, const U128& op2, bool fpcr_controlled = true);
    U128 FPVectorMulX(size_t esize, const U128& a, const U128& b, bool fpcr_controlled = true);
    U128 FPVectorNeg(size_t esize, const U128& a);
    U128 FPVectorPairedAdd(size_t esize, const U128& a, const U128& b, bool fpcr_controlled = true);
    U128 FPVectorPairedAddLower(size_t esize, const U128& a, const U128& b, bool fpcr_controlled = true);
    U128 FPVectorRecipEstimate(size_t esize, const U128& a, bool fpcr_controlled = true);
    U128 FPVectorRecipStepFused(size_t esize, const U128& a, const U128& b, bool fpcr_controlled = true);
    U128 FPVectorRoundInt(size_t esize, const U128& operand, FP::RoundingMode rounding, bool exact, bool fpcr_controlled = true);
    U128 FPVectorRSqrtEstimate(size_t esize, const U128& a, bool fpcr_controlled = true);
    U128 FPVectorRSqrtStepFused(size_t esize, const U128& a, const U128& b, bool fpcr_controlled = true);
    U128 FPVectorSqrt(size_t esize, const U128& a, bool fpcr_controlled = true);
    U128 FPVectorSub(size_t esize, const U128& a, const U128& b, bool fpcr_controlled = true);
    U128 FPVectorToHalf(size_t esize, const U128& a, FP::RoundingMode rounding, bool fpcr_controlled = true);
    U128 FPVectorToSignedFixed(size_t esize, const U128& a, size_t fbits, FP::RoundingMode rounding, bool fpcr_controlled = true);
    U128 FPVectorToUnsignedFixed(size_t esize, const U128& a, size_t fbits, FP::RoundingMode rounding, bool fpcr_controlled = true);

    void Breakpoint();
    void CallHostFunction(void (*fn)(void));
    void CallHostFunction(void (*fn)(u64), const U64& arg1);
    void CallHostFunction(void (*fn)(u64, u64), const U64& arg1, const U64& arg2);
    void CallHostFunction(void (*fn)(u64, u64, u64), const U64& arg1, const U64& arg2, const U64& arg3);

    void SetTerm(const Terminal& terminal);

    void SetInsertionPoint(IR::Inst* new_insertion_point) {
        insertion_point = IR::Block::iterator{*new_insertion_point};
    }

    void SetInsertionPoint(IR::Block::iterator new_insertion_point) {
        insertion_point = new_insertion_point;
    }

protected:
    IR::Block::iterator insertion_point;

    template<typename T = Value, typename... Args>
    T Inst(Opcode op, Args... args) {
        auto iter = block.PrependNewInst(insertion_point, op, {Value(args)...});
        return T(Value(&*iter));
    }
};

}  // namespace Dynarmic::IR

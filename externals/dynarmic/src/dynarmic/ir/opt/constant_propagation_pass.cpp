/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <optional>

#include "dynarmic/common/assert.h"
#include "dynarmic/common/bit_util.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/common/safe_ops.h"
#include "dynarmic/ir/basic_block.h"
#include "dynarmic/ir/ir_emitter.h"
#include "dynarmic/ir/opcodes.h"
#include "dynarmic/ir/opt/passes.h"

namespace Dynarmic::Optimization {

using Op = Dynarmic::IR::Opcode;

namespace {

// Tiny helper to avoid the need to store based off the opcode
// bit size all over the place within folding functions.
void ReplaceUsesWith(IR::Inst& inst, bool is_32_bit, u64 value) {
    if (is_32_bit) {
        inst.ReplaceUsesWith(IR::Value{static_cast<u32>(value)});
    } else {
        inst.ReplaceUsesWith(IR::Value{value});
    }
}

IR::Value Value(bool is_32_bit, u64 value) {
    return is_32_bit ? IR::Value{static_cast<u32>(value)} : IR::Value{value};
}

template<typename ImmFn>
bool FoldCommutative(IR::Inst& inst, bool is_32_bit, ImmFn imm_fn) {
    const auto lhs = inst.GetArg(0);
    const auto rhs = inst.GetArg(1);

    const bool is_lhs_immediate = lhs.IsImmediate();
    const bool is_rhs_immediate = rhs.IsImmediate();

    if (is_lhs_immediate && is_rhs_immediate) {
        const u64 result = imm_fn(lhs.GetImmediateAsU64(), rhs.GetImmediateAsU64());
        ReplaceUsesWith(inst, is_32_bit, result);
        return false;
    }

    if (is_lhs_immediate && !is_rhs_immediate) {
        const IR::Inst* rhs_inst = rhs.GetInstRecursive();
        if (rhs_inst->GetOpcode() == inst.GetOpcode() && rhs_inst->GetArg(1).IsImmediate()) {
            const u64 combined = imm_fn(lhs.GetImmediateAsU64(), rhs_inst->GetArg(1).GetImmediateAsU64());
            inst.SetArg(0, rhs_inst->GetArg(0));
            inst.SetArg(1, Value(is_32_bit, combined));
        } else {
            // Normalize
            inst.SetArg(0, rhs);
            inst.SetArg(1, lhs);
        }
    }

    if (!is_lhs_immediate && is_rhs_immediate) {
        const IR::Inst* lhs_inst = lhs.GetInstRecursive();
        if (lhs_inst->GetOpcode() == inst.GetOpcode() && lhs_inst->GetArg(1).IsImmediate()) {
            const u64 combined = imm_fn(rhs.GetImmediateAsU64(), lhs_inst->GetArg(1).GetImmediateAsU64());
            inst.SetArg(0, lhs_inst->GetArg(0));
            inst.SetArg(1, Value(is_32_bit, combined));
        }
    }

    return true;
}

void FoldAdd(IR::Inst& inst, bool is_32_bit) {
    const auto lhs = inst.GetArg(0);
    const auto rhs = inst.GetArg(1);
    const auto carry = inst.GetArg(2);

    if (lhs.IsImmediate() && !rhs.IsImmediate()) {
        // Normalize
        inst.SetArg(0, rhs);
        inst.SetArg(1, lhs);
        FoldAdd(inst, is_32_bit);
        return;
    }

    if (inst.HasAssociatedPseudoOperation()) {
        return;
    }

    if (!lhs.IsImmediate() && rhs.IsImmediate()) {
        const IR::Inst* lhs_inst = lhs.GetInstRecursive();
        if (lhs_inst->GetOpcode() == inst.GetOpcode() && lhs_inst->GetArg(1).IsImmediate() && lhs_inst->GetArg(2).IsImmediate()) {
            const u64 combined = rhs.GetImmediateAsU64() + lhs_inst->GetArg(1).GetImmediateAsU64() + lhs_inst->GetArg(2).GetU1();
            inst.SetArg(0, lhs_inst->GetArg(0));
            inst.SetArg(1, Value(is_32_bit, combined));
            return;
        }
    }

    if (inst.AreAllArgsImmediates()) {
        const u64 result = lhs.GetImmediateAsU64() + rhs.GetImmediateAsU64() + carry.GetU1();
        ReplaceUsesWith(inst, is_32_bit, result);
        return;
    }
}

// Folds AND operations based on the following:
//
// 1. imm_x & imm_y -> result
// 2. x & 0 -> 0
// 3. 0 & y -> 0
// 4. x & y -> y (where x has all bits set to 1)
// 5. x & y -> x (where y has all bits set to 1)
//
void FoldAND(IR::Inst& inst, bool is_32_bit) {
    if (FoldCommutative(inst, is_32_bit, [](u64 a, u64 b) { return a & b; })) {
        const auto rhs = inst.GetArg(1);
        if (rhs.IsZero()) {
            ReplaceUsesWith(inst, is_32_bit, 0);
        } else if (rhs.HasAllBitsSet()) {
            inst.ReplaceUsesWith(inst.GetArg(0));
        }
    }
}

// Folds byte reversal opcodes based on the following:
//
// 1. imm -> swap(imm)
//
void FoldByteReverse(IR::Inst& inst, Op op) {
    const auto operand = inst.GetArg(0);

    if (!operand.IsImmediate()) {
        return;
    }

    if (op == Op::ByteReverseWord) {
        const u32 result = Common::SwapBytes32(static_cast<u32>(operand.GetImmediateAsU64()));
        inst.ReplaceUsesWith(IR::Value{result});
    } else if (op == Op::ByteReverseHalf) {
        const u16 result = Common::SwapBytes16(static_cast<u16>(operand.GetImmediateAsU64()));
        inst.ReplaceUsesWith(IR::Value{result});
    } else {
        const u64 result = Common::SwapBytes64(operand.GetImmediateAsU64());
        inst.ReplaceUsesWith(IR::Value{result});
    }
}

// Folds division operations based on the following:
//
// 1. x / 0 -> 0 (NOTE: This is an ARM-specific behavior defined in the architecture reference manual)
// 2. imm_x / imm_y -> result
// 3. x / 1 -> x
//
void FoldDivide(IR::Inst& inst, bool is_32_bit, bool is_signed) {
    const auto rhs = inst.GetArg(1);

    if (rhs.IsZero()) {
        ReplaceUsesWith(inst, is_32_bit, 0);
        return;
    }

    const auto lhs = inst.GetArg(0);
    if (lhs.IsImmediate() && rhs.IsImmediate()) {
        if (is_signed) {
            const s64 result = lhs.GetImmediateAsS64() / rhs.GetImmediateAsS64();
            ReplaceUsesWith(inst, is_32_bit, static_cast<u64>(result));
        } else {
            const u64 result = lhs.GetImmediateAsU64() / rhs.GetImmediateAsU64();
            ReplaceUsesWith(inst, is_32_bit, result);
        }
    } else if (rhs.IsUnsignedImmediate(1)) {
        inst.ReplaceUsesWith(IR::Value{lhs});
    }
}

// Folds EOR operations based on the following:
//
// 1. imm_x ^ imm_y -> result
// 2. x ^ 0 -> x
// 3. 0 ^ y -> y
//
void FoldEOR(IR::Inst& inst, bool is_32_bit) {
    if (FoldCommutative(inst, is_32_bit, [](u64 a, u64 b) { return a ^ b; })) {
        const auto rhs = inst.GetArg(1);
        if (rhs.IsZero()) {
            inst.ReplaceUsesWith(inst.GetArg(0));
        }
    }
}

void FoldLeastSignificantByte(IR::Inst& inst) {
    if (!inst.AreAllArgsImmediates()) {
        return;
    }

    const auto operand = inst.GetArg(0);
    inst.ReplaceUsesWith(IR::Value{static_cast<u8>(operand.GetImmediateAsU64())});
}

void FoldLeastSignificantHalf(IR::Inst& inst) {
    if (!inst.AreAllArgsImmediates()) {
        return;
    }

    const auto operand = inst.GetArg(0);
    inst.ReplaceUsesWith(IR::Value{static_cast<u16>(operand.GetImmediateAsU64())});
}

void FoldLeastSignificantWord(IR::Inst& inst) {
    if (!inst.AreAllArgsImmediates()) {
        return;
    }

    const auto operand = inst.GetArg(0);
    inst.ReplaceUsesWith(IR::Value{static_cast<u32>(operand.GetImmediateAsU64())});
}

void FoldMostSignificantBit(IR::Inst& inst) {
    if (!inst.AreAllArgsImmediates()) {
        return;
    }

    const auto operand = inst.GetArg(0);
    inst.ReplaceUsesWith(IR::Value{(operand.GetImmediateAsU64() >> 31) != 0});
}

void FoldMostSignificantWord(IR::Inst& inst) {
    IR::Inst* carry_inst = inst.GetAssociatedPseudoOperation(Op::GetCarryFromOp);

    if (!inst.AreAllArgsImmediates()) {
        return;
    }

    const auto operand = inst.GetArg(0);
    if (carry_inst) {
        carry_inst->ReplaceUsesWith(IR::Value{Common::Bit<31>(operand.GetImmediateAsU64())});
    }
    inst.ReplaceUsesWith(IR::Value{static_cast<u32>(operand.GetImmediateAsU64() >> 32)});
}

// Folds multiplication operations based on the following:
//
// 1. imm_x * imm_y -> result
// 2. x * 0 -> 0
// 3. 0 * y -> 0
// 4. x * 1 -> x
// 5. 1 * y -> y
//
void FoldMultiply(IR::Inst& inst, bool is_32_bit) {
    if (FoldCommutative(inst, is_32_bit, [](u64 a, u64 b) { return a * b; })) {
        const auto rhs = inst.GetArg(1);
        if (rhs.IsZero()) {
            ReplaceUsesWith(inst, is_32_bit, 0);
        } else if (rhs.IsUnsignedImmediate(1)) {
            inst.ReplaceUsesWith(inst.GetArg(0));
        }
    }
}

// Folds NOT operations if the contained value is an immediate.
void FoldNOT(IR::Inst& inst, bool is_32_bit) {
    const auto operand = inst.GetArg(0);

    if (!operand.IsImmediate()) {
        return;
    }

    const u64 result = ~operand.GetImmediateAsU64();
    ReplaceUsesWith(inst, is_32_bit, result);
}

// Folds OR operations based on the following:
//
// 1. imm_x | imm_y -> result
// 2. x | 0 -> x
// 3. 0 | y -> y
//
void FoldOR(IR::Inst& inst, bool is_32_bit) {
    if (FoldCommutative(inst, is_32_bit, [](u64 a, u64 b) { return a | b; })) {
        const auto rhs = inst.GetArg(1);
        if (rhs.IsZero()) {
            inst.ReplaceUsesWith(inst.GetArg(0));
        }
    }
}

bool FoldShifts(IR::Inst& inst) {
    IR::Inst* carry_inst = inst.GetAssociatedPseudoOperation(Op::GetCarryFromOp);

    // The 32-bit variants can contain 3 arguments, while the
    // 64-bit variants only contain 2.
    if (inst.NumArgs() == 3 && !carry_inst) {
        inst.SetArg(2, IR::Value(false));
    }

    const auto shift_amount = inst.GetArg(1);
    if (shift_amount.IsZero()) {
        if (carry_inst) {
            carry_inst->ReplaceUsesWith(inst.GetArg(2));
        }
        inst.ReplaceUsesWith(inst.GetArg(0));
        return false;
    }

    if (!inst.AreAllArgsImmediates() || carry_inst) {
        return false;
    }

    return true;
}

void FoldSignExtendXToWord(IR::Inst& inst) {
    if (!inst.AreAllArgsImmediates()) {
        return;
    }

    const s64 value = inst.GetArg(0).GetImmediateAsS64();
    inst.ReplaceUsesWith(IR::Value{static_cast<u32>(value)});
}

void FoldSignExtendXToLong(IR::Inst& inst) {
    if (!inst.AreAllArgsImmediates()) {
        return;
    }

    const s64 value = inst.GetArg(0).GetImmediateAsS64();
    inst.ReplaceUsesWith(IR::Value{static_cast<u64>(value)});
}

void FoldSub(IR::Inst& inst, bool is_32_bit) {
    if (!inst.AreAllArgsImmediates() || inst.HasAssociatedPseudoOperation()) {
        return;
    }

    const auto lhs = inst.GetArg(0);
    const auto rhs = inst.GetArg(1);
    const auto carry = inst.GetArg(2);

    const u64 result = lhs.GetImmediateAsU64() + (~rhs.GetImmediateAsU64()) + carry.GetU1();
    ReplaceUsesWith(inst, is_32_bit, result);
}

void FoldZeroExtendXToWord(IR::Inst& inst) {
    if (!inst.AreAllArgsImmediates()) {
        return;
    }

    const u64 value = inst.GetArg(0).GetImmediateAsU64();
    inst.ReplaceUsesWith(IR::Value{static_cast<u32>(value)});
}

void FoldZeroExtendXToLong(IR::Inst& inst) {
    if (!inst.AreAllArgsImmediates()) {
        return;
    }

    const u64 value = inst.GetArg(0).GetImmediateAsU64();
    inst.ReplaceUsesWith(IR::Value{value});
}
}  // Anonymous namespace

void ConstantPropagation(IR::Block& block) {
    for (auto& inst : block) {
        const auto opcode = inst.GetOpcode();

        switch (opcode) {
        case Op::LeastSignificantWord:
            FoldLeastSignificantWord(inst);
            break;
        case Op::MostSignificantWord:
            FoldMostSignificantWord(inst);
            break;
        case Op::LeastSignificantHalf:
            FoldLeastSignificantHalf(inst);
            break;
        case Op::LeastSignificantByte:
            FoldLeastSignificantByte(inst);
            break;
        case Op::MostSignificantBit:
            FoldMostSignificantBit(inst);
            break;
        case Op::IsZero32:
            if (inst.AreAllArgsImmediates()) {
                inst.ReplaceUsesWith(IR::Value{inst.GetArg(0).GetU32() == 0});
            }
            break;
        case Op::IsZero64:
            if (inst.AreAllArgsImmediates()) {
                inst.ReplaceUsesWith(IR::Value{inst.GetArg(0).GetU64() == 0});
            }
            break;
        case Op::LogicalShiftLeft32:
            if (FoldShifts(inst)) {
                ReplaceUsesWith(inst, true, Safe::LogicalShiftLeft<u32>(inst.GetArg(0).GetU32(), inst.GetArg(1).GetU8()));
            }
            break;
        case Op::LogicalShiftLeft64:
            if (FoldShifts(inst)) {
                ReplaceUsesWith(inst, false, Safe::LogicalShiftLeft<u64>(inst.GetArg(0).GetU64(), inst.GetArg(1).GetU8()));
            }
            break;
        case Op::LogicalShiftRight32:
            if (FoldShifts(inst)) {
                ReplaceUsesWith(inst, true, Safe::LogicalShiftRight<u32>(inst.GetArg(0).GetU32(), inst.GetArg(1).GetU8()));
            }
            break;
        case Op::LogicalShiftRight64:
            if (FoldShifts(inst)) {
                ReplaceUsesWith(inst, false, Safe::LogicalShiftRight<u64>(inst.GetArg(0).GetU64(), inst.GetArg(1).GetU8()));
            }
            break;
        case Op::ArithmeticShiftRight32:
            if (FoldShifts(inst)) {
                ReplaceUsesWith(inst, true, Safe::ArithmeticShiftRight<u32>(inst.GetArg(0).GetU32(), inst.GetArg(1).GetU8()));
            }
            break;
        case Op::ArithmeticShiftRight64:
            if (FoldShifts(inst)) {
                ReplaceUsesWith(inst, false, Safe::ArithmeticShiftRight<u64>(inst.GetArg(0).GetU64(), inst.GetArg(1).GetU8()));
            }
            break;
        case Op::RotateRight32:
            if (FoldShifts(inst)) {
                ReplaceUsesWith(inst, true, Common::RotateRight<u32>(inst.GetArg(0).GetU32(), inst.GetArg(1).GetU8()));
            }
            break;
        case Op::RotateRight64:
            if (FoldShifts(inst)) {
                ReplaceUsesWith(inst, false, Common::RotateRight<u64>(inst.GetArg(0).GetU64(), inst.GetArg(1).GetU8()));
            }
            break;
        case Op::LogicalShiftLeftMasked32:
            if (inst.AreAllArgsImmediates()) {
                ReplaceUsesWith(inst, true, inst.GetArg(0).GetU32() << (inst.GetArg(1).GetU32() & 0x1f));
            }
            break;
        case Op::LogicalShiftLeftMasked64:
            if (inst.AreAllArgsImmediates()) {
                ReplaceUsesWith(inst, false, inst.GetArg(0).GetU64() << (inst.GetArg(1).GetU64() & 0x3f));
            }
            break;
        case Op::LogicalShiftRightMasked32:
            if (inst.AreAllArgsImmediates()) {
                ReplaceUsesWith(inst, true, inst.GetArg(0).GetU32() >> (inst.GetArg(1).GetU32() & 0x1f));
            }
            break;
        case Op::LogicalShiftRightMasked64:
            if (inst.AreAllArgsImmediates()) {
                ReplaceUsesWith(inst, false, inst.GetArg(0).GetU64() >> (inst.GetArg(1).GetU64() & 0x3f));
            }
            break;
        case Op::ArithmeticShiftRightMasked32:
            if (inst.AreAllArgsImmediates()) {
                ReplaceUsesWith(inst, true, static_cast<s32>(inst.GetArg(0).GetU32()) >> (inst.GetArg(1).GetU32() & 0x1f));
            }
            break;
        case Op::ArithmeticShiftRightMasked64:
            if (inst.AreAllArgsImmediates()) {
                ReplaceUsesWith(inst, false, static_cast<s64>(inst.GetArg(0).GetU64()) >> (inst.GetArg(1).GetU64() & 0x3f));
            }
            break;
        case Op::RotateRightMasked32:
            if (inst.AreAllArgsImmediates()) {
                ReplaceUsesWith(inst, true, Common::RotateRight<u32>(inst.GetArg(0).GetU32(), inst.GetArg(1).GetU32()));
            }
            break;
        case Op::RotateRightMasked64:
            if (inst.AreAllArgsImmediates()) {
                ReplaceUsesWith(inst, false, Common::RotateRight<u64>(inst.GetArg(0).GetU64(), inst.GetArg(1).GetU64()));
            }
            break;
        case Op::Add32:
        case Op::Add64:
            FoldAdd(inst, opcode == Op::Add32);
            break;
        case Op::Sub32:
        case Op::Sub64:
            FoldSub(inst, opcode == Op::Sub32);
            break;
        case Op::Mul32:
        case Op::Mul64:
            FoldMultiply(inst, opcode == Op::Mul32);
            break;
        case Op::SignedDiv32:
        case Op::SignedDiv64:
            FoldDivide(inst, opcode == Op::SignedDiv32, true);
            break;
        case Op::UnsignedDiv32:
        case Op::UnsignedDiv64:
            FoldDivide(inst, opcode == Op::UnsignedDiv32, false);
            break;
        case Op::And32:
        case Op::And64:
            FoldAND(inst, opcode == Op::And32);
            break;
        case Op::Eor32:
        case Op::Eor64:
            FoldEOR(inst, opcode == Op::Eor32);
            break;
        case Op::Or32:
        case Op::Or64:
            FoldOR(inst, opcode == Op::Or32);
            break;
        case Op::Not32:
        case Op::Not64:
            FoldNOT(inst, opcode == Op::Not32);
            break;
        case Op::SignExtendByteToWord:
        case Op::SignExtendHalfToWord:
            FoldSignExtendXToWord(inst);
            break;
        case Op::SignExtendByteToLong:
        case Op::SignExtendHalfToLong:
        case Op::SignExtendWordToLong:
            FoldSignExtendXToLong(inst);
            break;
        case Op::ZeroExtendByteToWord:
        case Op::ZeroExtendHalfToWord:
            FoldZeroExtendXToWord(inst);
            break;
        case Op::ZeroExtendByteToLong:
        case Op::ZeroExtendHalfToLong:
        case Op::ZeroExtendWordToLong:
            FoldZeroExtendXToLong(inst);
            break;
        case Op::ByteReverseWord:
        case Op::ByteReverseHalf:
        case Op::ByteReverseDual:
            FoldByteReverse(inst, opcode);
            break;
        default:
            break;
        }
    }
}

}  // namespace Dynarmic::Optimization

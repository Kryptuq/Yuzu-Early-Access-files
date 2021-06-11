// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <utility>

#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using std::move;
using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;

namespace {
constexpr u64 NUM_CONDITION_CODES = 4;
constexpr u64 NUM_PREDICATES = 7;
} // namespace

u32 ShaderIR::DecodeRegisterSetPredicate(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);

    Node apply_mask = [this, opcode, instr] {
        switch (opcode->get().GetId()) {
        case OpCode::Id::R2P_IMM:
        case OpCode::Id::P2R_IMM:
            return Immediate(static_cast<u32>(instr.p2r_r2p.immediate_mask));
        default:
            UNREACHABLE();
            return Immediate(0);
        }
    }();

    const u32 offset = static_cast<u32>(instr.p2r_r2p.byte) * 8;

    const bool cc = instr.p2r_r2p.mode == Tegra::Shader::R2pMode::Cc;
    const u64 num_entries = cc ? NUM_CONDITION_CODES : NUM_PREDICATES;
    const auto get_entry = [this, cc](u64 entry) {
        return cc ? GetInternalFlag(static_cast<InternalFlag>(entry)) : GetPredicate(entry);
    };

    switch (opcode->get().GetId()) {
    case OpCode::Id::R2P_IMM: {
        Node mask = GetRegister(instr.gpr8);

        for (u64 entry = 0; entry < num_entries; ++entry) {
            const u32 shift = static_cast<u32>(entry);

            Node apply = BitfieldExtract(apply_mask, shift, 1);
            Node condition = Operation(OperationCode::LogicalUNotEqual, apply, Immediate(0));

            Node compare = BitfieldExtract(mask, offset + shift, 1);
            Node value = Operation(OperationCode::LogicalUNotEqual, move(compare), Immediate(0));

            Node code = Operation(OperationCode::LogicalAssign, get_entry(entry), move(value));
            bb.push_back(Conditional(condition, {move(code)}));
        }
        break;
    }
    case OpCode::Id::P2R_IMM: {
        Node value = Immediate(0);
        for (u64 entry = 0; entry < num_entries; ++entry) {
            Node bit = Operation(OperationCode::Select, get_entry(entry), Immediate(1U << entry),
                                 Immediate(0));
            value = Operation(OperationCode::UBitwiseOr, move(value), move(bit));
        }
        value = Operation(OperationCode::UBitwiseAnd, move(value), apply_mask);
        value = BitfieldInsert(GetRegister(instr.gpr8), move(value), offset, 8);

        SetRegister(bb, instr.gpr0, move(value));
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Unhandled P2R/R2R instruction: {}", opcode->get().GetName());
        break;
    }

    return pc;
}

} // namespace VideoCommon::Shader

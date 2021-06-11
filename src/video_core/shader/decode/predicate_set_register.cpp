// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;

u32 ShaderIR::DecodePredicateSetRegister(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};

    UNIMPLEMENTED_IF_MSG(instr.generates_cc,
                         "Condition codes generation in PSET is not implemented");

    const Node op_a = GetPredicate(instr.pset.pred12, instr.pset.neg_pred12 != 0);
    const Node op_b = GetPredicate(instr.pset.pred29, instr.pset.neg_pred29 != 0);
    const Node first_pred = Operation(GetPredicateCombiner(instr.pset.cond), op_a, op_b);

    const Node second_pred = GetPredicate(instr.pset.pred39, instr.pset.neg_pred39 != 0);

    const OperationCode combiner = GetPredicateCombiner(instr.pset.op);
    const Node predicate = Operation(combiner, first_pred, second_pred);

    const Node true_value = instr.pset.bf ? Immediate(1.0f) : Immediate(0xffffffff);
    const Node false_value = instr.pset.bf ? Immediate(0.0f) : Immediate(0);
    const Node value =
        Operation(OperationCode::Select, PRECISE, predicate, true_value, false_value);

    if (instr.pset.bf) {
        SetInternalFlagsFromFloat(bb, value, instr.generates_cc);
    } else {
        SetInternalFlagsFromInteger(bb, value, instr.generates_cc);
    }
    SetRegister(bb, instr.gpr0, value);

    return pc;
}

} // namespace VideoCommon::Shader

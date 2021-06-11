// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <array>
#include <cmath>

#include "common/assert.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/node.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/registry.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using Tegra::Shader::Attribute;
using Tegra::Shader::Instruction;
using Tegra::Shader::IpaMode;
using Tegra::Shader::Pred;
using Tegra::Shader::PredCondition;
using Tegra::Shader::PredOperation;
using Tegra::Shader::Register;

ShaderIR::ShaderIR(const ProgramCode& program_code_, u32 main_offset_, CompilerSettings settings_,
                   Registry& registry_)
    : program_code{program_code_}, main_offset{main_offset_}, settings{settings_}, registry{
                                                                                       registry_} {
    Decode();
    PostDecode();
}

ShaderIR::~ShaderIR() = default;

Node ShaderIR::GetRegister(Register reg) {
    if (reg != Register::ZeroIndex) {
        used_registers.insert(static_cast<u32>(reg));
    }
    return MakeNode<GprNode>(reg);
}

Node ShaderIR::GetCustomVariable(u32 id) {
    return MakeNode<CustomVarNode>(id);
}

Node ShaderIR::GetImmediate19(Instruction instr) {
    return Immediate(instr.alu.GetImm20_19());
}

Node ShaderIR::GetImmediate32(Instruction instr) {
    return Immediate(instr.alu.GetImm20_32());
}

Node ShaderIR::GetConstBuffer(u64 index_, u64 offset_) {
    const auto index = static_cast<u32>(index_);
    const auto offset = static_cast<u32>(offset_);

    used_cbufs.try_emplace(index).first->second.MarkAsUsed(offset);

    return MakeNode<CbufNode>(index, Immediate(offset));
}

Node ShaderIR::GetConstBufferIndirect(u64 index_, u64 offset_, Node node) {
    const auto index = static_cast<u32>(index_);
    const auto offset = static_cast<u32>(offset_);

    used_cbufs.try_emplace(index).first->second.MarkAsUsedIndirect();

    Node final_offset = [&] {
        // Attempt to inline constant buffer without a variable offset. This is done to allow
        // tracking LDC calls.
        if (const auto gpr = std::get_if<GprNode>(&*node)) {
            if (gpr->GetIndex() == Register::ZeroIndex) {
                return Immediate(offset);
            }
        }
        return Operation(OperationCode::UAdd, NO_PRECISE, std::move(node), Immediate(offset));
    }();
    return MakeNode<CbufNode>(index, std::move(final_offset));
}

Node ShaderIR::GetPredicate(u64 pred_, bool negated) {
    const auto pred = static_cast<Pred>(pred_);
    if (pred != Pred::UnusedIndex && pred != Pred::NeverExecute) {
        used_predicates.insert(pred);
    }

    return MakeNode<PredicateNode>(pred, negated);
}

Node ShaderIR::GetPredicate(bool immediate) {
    return GetPredicate(static_cast<u64>(immediate ? Pred::UnusedIndex : Pred::NeverExecute));
}

Node ShaderIR::GetInputAttribute(Attribute::Index index, u64 element, Node buffer) {
    MarkAttributeUsage(index, element);
    used_input_attributes.emplace(index);
    return MakeNode<AbufNode>(index, static_cast<u32>(element), std::move(buffer));
}

Node ShaderIR::GetPhysicalInputAttribute(Tegra::Shader::Register physical_address, Node buffer) {
    uses_physical_attributes = true;
    return MakeNode<AbufNode>(GetRegister(physical_address), buffer);
}

Node ShaderIR::GetOutputAttribute(Attribute::Index index, u64 element, Node buffer) {
    MarkAttributeUsage(index, element);
    used_output_attributes.insert(index);
    return MakeNode<AbufNode>(index, static_cast<u32>(element), std::move(buffer));
}

Node ShaderIR::GetInternalFlag(InternalFlag flag, bool negated) const {
    Node node = MakeNode<InternalFlagNode>(flag);
    if (negated) {
        return Operation(OperationCode::LogicalNegate, std::move(node));
    }
    return node;
}

Node ShaderIR::GetLocalMemory(Node address) {
    return MakeNode<LmemNode>(std::move(address));
}

Node ShaderIR::GetSharedMemory(Node address) {
    return MakeNode<SmemNode>(std::move(address));
}

Node ShaderIR::GetTemporary(u32 id) {
    return GetRegister(Register::ZeroIndex + 1 + id);
}

Node ShaderIR::GetOperandAbsNegFloat(Node value, bool absolute, bool negate) {
    if (absolute) {
        value = Operation(OperationCode::FAbsolute, NO_PRECISE, std::move(value));
    }
    if (negate) {
        value = Operation(OperationCode::FNegate, NO_PRECISE, std::move(value));
    }
    return value;
}

Node ShaderIR::GetSaturatedFloat(Node value, bool saturate) {
    if (!saturate) {
        return value;
    }

    Node positive_zero = Immediate(std::copysignf(0, 1));
    Node positive_one = Immediate(1.0f);
    return Operation(OperationCode::FClamp, NO_PRECISE, std::move(value), std::move(positive_zero),
                     std::move(positive_one));
}

Node ShaderIR::ConvertIntegerSize(Node value, Register::Size size, bool is_signed) {
    switch (size) {
    case Register::Size::Byte:
        value = SignedOperation(OperationCode::ILogicalShiftLeft, is_signed, NO_PRECISE,
                                std::move(value), Immediate(24));
        value = SignedOperation(OperationCode::IArithmeticShiftRight, is_signed, NO_PRECISE,
                                std::move(value), Immediate(24));
        return value;
    case Register::Size::Short:
        value = SignedOperation(OperationCode::ILogicalShiftLeft, is_signed, NO_PRECISE,
                                std::move(value), Immediate(16));
        value = SignedOperation(OperationCode::IArithmeticShiftRight, is_signed, NO_PRECISE,
                                std::move(value), Immediate(16));
        return value;
    case Register::Size::Word:
        // Default - do nothing
        return value;
    default:
        UNREACHABLE_MSG("Unimplemented conversion size: {}", size);
        return value;
    }
}

Node ShaderIR::GetOperandAbsNegInteger(Node value, bool absolute, bool negate, bool is_signed) {
    if (!is_signed) {
        // Absolute or negate on an unsigned is pointless
        return value;
    }
    if (absolute) {
        value = Operation(OperationCode::IAbsolute, NO_PRECISE, std::move(value));
    }
    if (negate) {
        value = Operation(OperationCode::INegate, NO_PRECISE, std::move(value));
    }
    return value;
}

Node ShaderIR::UnpackHalfImmediate(Instruction instr, bool has_negation) {
    Node value = Immediate(instr.half_imm.PackImmediates());
    if (!has_negation) {
        return value;
    }

    Node first_negate = GetPredicate(instr.half_imm.first_negate != 0);
    Node second_negate = GetPredicate(instr.half_imm.second_negate != 0);

    return Operation(OperationCode::HNegate, NO_PRECISE, std::move(value), std::move(first_negate),
                     std::move(second_negate));
}

Node ShaderIR::UnpackHalfFloat(Node value, Tegra::Shader::HalfType type) {
    return Operation(OperationCode::HUnpack, type, std::move(value));
}

Node ShaderIR::HalfMerge(Node dest, Node src, Tegra::Shader::HalfMerge merge) {
    switch (merge) {
    case Tegra::Shader::HalfMerge::H0_H1:
        return src;
    case Tegra::Shader::HalfMerge::F32:
        return Operation(OperationCode::HMergeF32, std::move(src));
    case Tegra::Shader::HalfMerge::Mrg_H0:
        return Operation(OperationCode::HMergeH0, std::move(dest), std::move(src));
    case Tegra::Shader::HalfMerge::Mrg_H1:
        return Operation(OperationCode::HMergeH1, std::move(dest), std::move(src));
    }
    UNREACHABLE();
    return src;
}

Node ShaderIR::GetOperandAbsNegHalf(Node value, bool absolute, bool negate) {
    if (absolute) {
        value = Operation(OperationCode::HAbsolute, NO_PRECISE, std::move(value));
    }
    if (negate) {
        value = Operation(OperationCode::HNegate, NO_PRECISE, std::move(value), GetPredicate(true),
                          GetPredicate(true));
    }
    return value;
}

Node ShaderIR::GetSaturatedHalfFloat(Node value, bool saturate) {
    if (!saturate) {
        return value;
    }

    Node positive_zero = Immediate(std::copysignf(0, 1));
    Node positive_one = Immediate(1.0f);
    return Operation(OperationCode::HClamp, NO_PRECISE, std::move(value), std::move(positive_zero),
                     std::move(positive_one));
}

Node ShaderIR::GetPredicateComparisonFloat(PredCondition condition, Node op_a, Node op_b) {
    if (condition == PredCondition::T) {
        return GetPredicate(true);
    } else if (condition == PredCondition::F) {
        return GetPredicate(false);
    }

    static constexpr std::array comparison_table{
        OperationCode(0),
        OperationCode::LogicalFOrdLessThan,       // LT
        OperationCode::LogicalFOrdEqual,          // EQ
        OperationCode::LogicalFOrdLessEqual,      // LE
        OperationCode::LogicalFOrdGreaterThan,    // GT
        OperationCode::LogicalFOrdNotEqual,       // NE
        OperationCode::LogicalFOrdGreaterEqual,   // GE
        OperationCode::LogicalFOrdered,           // NUM
        OperationCode::LogicalFUnordered,         // NAN
        OperationCode::LogicalFUnordLessThan,     // LTU
        OperationCode::LogicalFUnordEqual,        // EQU
        OperationCode::LogicalFUnordLessEqual,    // LEU
        OperationCode::LogicalFUnordGreaterThan,  // GTU
        OperationCode::LogicalFUnordNotEqual,     // NEU
        OperationCode::LogicalFUnordGreaterEqual, // GEU
    };
    const std::size_t index = static_cast<std::size_t>(condition);
    ASSERT_MSG(index < std::size(comparison_table), "Invalid condition={}", index);

    return Operation(comparison_table[index], op_a, op_b);
}

Node ShaderIR::GetPredicateComparisonInteger(PredCondition condition, bool is_signed, Node op_a,
                                             Node op_b) {
    static constexpr std::array comparison_table{
        std::pair{PredCondition::LT, OperationCode::LogicalILessThan},
        std::pair{PredCondition::EQ, OperationCode::LogicalIEqual},
        std::pair{PredCondition::LE, OperationCode::LogicalILessEqual},
        std::pair{PredCondition::GT, OperationCode::LogicalIGreaterThan},
        std::pair{PredCondition::NE, OperationCode::LogicalINotEqual},
        std::pair{PredCondition::GE, OperationCode::LogicalIGreaterEqual},
    };

    const auto comparison =
        std::find_if(comparison_table.cbegin(), comparison_table.cend(),
                     [condition](const auto entry) { return condition == entry.first; });
    UNIMPLEMENTED_IF_MSG(comparison == comparison_table.cend(),
                         "Unknown predicate comparison operation");

    return SignedOperation(comparison->second, is_signed, NO_PRECISE, std::move(op_a),
                           std::move(op_b));
}

Node ShaderIR::GetPredicateComparisonHalf(Tegra::Shader::PredCondition condition, Node op_a,
                                          Node op_b) {
    static constexpr std::array comparison_table{
        std::pair{PredCondition::LT, OperationCode::Logical2HLessThan},
        std::pair{PredCondition::EQ, OperationCode::Logical2HEqual},
        std::pair{PredCondition::LE, OperationCode::Logical2HLessEqual},
        std::pair{PredCondition::GT, OperationCode::Logical2HGreaterThan},
        std::pair{PredCondition::NE, OperationCode::Logical2HNotEqual},
        std::pair{PredCondition::GE, OperationCode::Logical2HGreaterEqual},
        std::pair{PredCondition::LTU, OperationCode::Logical2HLessThanWithNan},
        std::pair{PredCondition::LEU, OperationCode::Logical2HLessEqualWithNan},
        std::pair{PredCondition::GTU, OperationCode::Logical2HGreaterThanWithNan},
        std::pair{PredCondition::NEU, OperationCode::Logical2HNotEqualWithNan},
        std::pair{PredCondition::GEU, OperationCode::Logical2HGreaterEqualWithNan},
    };

    const auto comparison =
        std::find_if(comparison_table.cbegin(), comparison_table.cend(),
                     [condition](const auto entry) { return condition == entry.first; });
    UNIMPLEMENTED_IF_MSG(comparison == comparison_table.cend(),
                         "Unknown predicate comparison operation");

    return Operation(comparison->second, NO_PRECISE, std::move(op_a), std::move(op_b));
}

OperationCode ShaderIR::GetPredicateCombiner(PredOperation operation) {
    static constexpr std::array operation_table{
        OperationCode::LogicalAnd,
        OperationCode::LogicalOr,
        OperationCode::LogicalXor,
    };

    const auto index = static_cast<std::size_t>(operation);
    if (index >= operation_table.size()) {
        UNIMPLEMENTED_MSG("Unknown predicate operation.");
        return {};
    }

    return operation_table[index];
}

Node ShaderIR::GetConditionCode(ConditionCode cc) const {
    switch (cc) {
    case ConditionCode::NEU:
        return GetInternalFlag(InternalFlag::Zero, true);
    case ConditionCode::FCSM_TR:
        UNIMPLEMENTED_MSG("EXIT.FCSM_TR is not implemented");
        return MakeNode<PredicateNode>(Pred::NeverExecute, false);
    default:
        UNIMPLEMENTED_MSG("Unimplemented condition code: {}", cc);
        return MakeNode<PredicateNode>(Pred::NeverExecute, false);
    }
}

void ShaderIR::SetRegister(NodeBlock& bb, Register dest, Node src) {
    bb.push_back(Operation(OperationCode::Assign, GetRegister(dest), std::move(src)));
}

void ShaderIR::SetPredicate(NodeBlock& bb, u64 dest, Node src) {
    bb.push_back(Operation(OperationCode::LogicalAssign, GetPredicate(dest), std::move(src)));
}

void ShaderIR::SetInternalFlag(NodeBlock& bb, InternalFlag flag, Node value) {
    bb.push_back(Operation(OperationCode::LogicalAssign, GetInternalFlag(flag), std::move(value)));
}

void ShaderIR::SetLocalMemory(NodeBlock& bb, Node address, Node value) {
    bb.push_back(
        Operation(OperationCode::Assign, GetLocalMemory(std::move(address)), std::move(value)));
}

void ShaderIR::SetSharedMemory(NodeBlock& bb, Node address, Node value) {
    bb.push_back(
        Operation(OperationCode::Assign, GetSharedMemory(std::move(address)), std::move(value)));
}

void ShaderIR::SetTemporary(NodeBlock& bb, u32 id, Node value) {
    SetRegister(bb, Register::ZeroIndex + 1 + id, std::move(value));
}

void ShaderIR::SetInternalFlagsFromFloat(NodeBlock& bb, Node value, bool sets_cc) {
    if (!sets_cc) {
        return;
    }
    Node zerop = Operation(OperationCode::LogicalFOrdEqual, std::move(value), Immediate(0.0f));
    SetInternalFlag(bb, InternalFlag::Zero, std::move(zerop));
    LOG_WARNING(HW_GPU, "Condition codes implementation is incomplete");
}

void ShaderIR::SetInternalFlagsFromInteger(NodeBlock& bb, Node value, bool sets_cc) {
    if (!sets_cc) {
        return;
    }
    Node zerop = Operation(OperationCode::LogicalIEqual, std::move(value), Immediate(0));
    SetInternalFlag(bb, InternalFlag::Zero, std::move(zerop));
    LOG_WARNING(HW_GPU, "Condition codes implementation is incomplete");
}

Node ShaderIR::BitfieldExtract(Node value, u32 offset, u32 bits) {
    return Operation(OperationCode::UBitfieldExtract, NO_PRECISE, std::move(value),
                     Immediate(offset), Immediate(bits));
}

Node ShaderIR::BitfieldInsert(Node base, Node insert, u32 offset, u32 bits) {
    return Operation(OperationCode::UBitfieldInsert, NO_PRECISE, base, insert, Immediate(offset),
                     Immediate(bits));
}

void ShaderIR::MarkAttributeUsage(Attribute::Index index, u64 element) {
    switch (index) {
    case Attribute::Index::LayerViewportPointSize:
        switch (element) {
        case 0:
            UNIMPLEMENTED();
            break;
        case 1:
            uses_layer = true;
            break;
        case 2:
            uses_viewport_index = true;
            break;
        case 3:
            uses_point_size = true;
            break;
        }
        break;
    case Attribute::Index::TessCoordInstanceIDVertexID:
        switch (element) {
        case 2:
            uses_instance_id = true;
            break;
        case 3:
            uses_vertex_id = true;
            break;
        }
        break;
    case Attribute::Index::ClipDistances0123:
    case Attribute::Index::ClipDistances4567: {
        const u64 clip_index = (index == Attribute::Index::ClipDistances4567 ? 4 : 0) + element;
        used_clip_distances.at(clip_index) = true;
        break;
    }
    case Attribute::Index::FrontColor:
    case Attribute::Index::FrontSecondaryColor:
    case Attribute::Index::BackColor:
    case Attribute::Index::BackSecondaryColor:
        uses_legacy_varyings = true;
        break;
    default:
        if (index >= Attribute::Index::TexCoord_0 && index <= Attribute::Index::TexCoord_7) {
            uses_legacy_varyings = true;
        }
        break;
    }
}

std::size_t ShaderIR::DeclareAmend(Node new_amend) {
    const auto id = amend_code.size();
    amend_code.push_back(std::move(new_amend));
    return id;
}

u32 ShaderIR::NewCustomVariable() {
    return num_custom_variables++;
}

} // namespace VideoCommon::Shader

// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include <limits>
#include <set>

#include <fmt/format.h>

#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/engines/shader_header.h"
#include "video_core/shader/control_flow.h"
#include "video_core/shader/memory_util.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;

namespace {

void DeduceTextureHandlerSize(VideoCore::GuestDriverProfile& gpu_driver,
                              const std::list<SamplerEntry>& used_samplers) {
    if (gpu_driver.IsTextureHandlerSizeKnown() || used_samplers.size() <= 1) {
        return;
    }
    u32 count{};
    std::vector<u32> bound_offsets;
    for (const auto& sampler : used_samplers) {
        if (sampler.is_bindless) {
            continue;
        }
        ++count;
        bound_offsets.emplace_back(sampler.offset);
    }
    if (count > 1) {
        gpu_driver.DeduceTextureHandlerSize(std::move(bound_offsets));
    }
}

std::optional<u32> TryDeduceSamplerSize(const SamplerEntry& sampler_to_deduce,
                                        VideoCore::GuestDriverProfile& gpu_driver,
                                        const std::list<SamplerEntry>& used_samplers) {
    const u32 base_offset = sampler_to_deduce.offset;
    u32 max_offset{std::numeric_limits<u32>::max()};
    for (const auto& sampler : used_samplers) {
        if (sampler.is_bindless) {
            continue;
        }
        if (sampler.offset > base_offset) {
            max_offset = std::min(sampler.offset, max_offset);
        }
    }
    if (max_offset == std::numeric_limits<u32>::max()) {
        return std::nullopt;
    }
    return ((max_offset - base_offset) * 4) / gpu_driver.GetTextureHandlerSize();
}

} // Anonymous namespace

class ASTDecoder {
public:
    explicit ASTDecoder(ShaderIR& ir_) : ir(ir_) {}

    void operator()(ASTProgram& ast) {
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
    }

    void operator()(ASTIfThen& ast) {
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
    }

    void operator()(ASTIfElse& ast) {
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
    }

    void operator()(ASTBlockEncoded& ast) {}

    void operator()(ASTBlockDecoded& ast) {}

    void operator()(ASTVarSet& ast) {}

    void operator()(ASTLabel& ast) {}

    void operator()(ASTGoto& ast) {}

    void operator()(ASTDoWhile& ast) {
        ASTNode current = ast.nodes.GetFirst();
        while (current) {
            Visit(current);
            current = current->GetNext();
        }
    }

    void operator()(ASTReturn& ast) {}

    void operator()(ASTBreak& ast) {}

    void Visit(ASTNode& node) {
        std::visit(*this, *node->GetInnerData());
        if (node->IsBlockEncoded()) {
            auto block = std::get_if<ASTBlockEncoded>(node->GetInnerData());
            NodeBlock bb = ir.DecodeRange(block->start, block->end);
            node->TransformBlockEncoded(std::move(bb));
        }
    }

private:
    ShaderIR& ir;
};

void ShaderIR::Decode() {
    std::memcpy(&header, program_code.data(), sizeof(Tegra::Shader::Header));

    decompiled = false;
    auto info = ScanFlow(program_code, main_offset, settings, registry);
    auto& shader_info = *info;
    coverage_begin = shader_info.start;
    coverage_end = shader_info.end;
    switch (shader_info.settings.depth) {
    case CompileDepth::FlowStack: {
        for (const auto& block : shader_info.blocks) {
            basic_blocks.insert({block.start, DecodeRange(block.start, block.end + 1)});
        }
        break;
    }
    case CompileDepth::NoFlowStack: {
        disable_flow_stack = true;
        const auto insert_block = [this](NodeBlock& nodes, u32 label) {
            if (label == static_cast<u32>(exit_branch)) {
                return;
            }
            basic_blocks.insert({label, nodes});
        };
        const auto& blocks = shader_info.blocks;
        NodeBlock current_block;
        u32 current_label = static_cast<u32>(exit_branch);
        for (const auto& block : blocks) {
            if (shader_info.labels.contains(block.start)) {
                insert_block(current_block, current_label);
                current_block.clear();
                current_label = block.start;
            }
            if (!block.ignore_branch) {
                DecodeRangeInner(current_block, block.start, block.end);
                InsertControlFlow(current_block, block);
            } else {
                DecodeRangeInner(current_block, block.start, block.end + 1);
            }
        }
        insert_block(current_block, current_label);
        break;
    }
    case CompileDepth::DecompileBackwards:
    case CompileDepth::FullDecompile: {
        program_manager = std::move(shader_info.manager);
        disable_flow_stack = true;
        decompiled = true;
        ASTDecoder decoder{*this};
        ASTNode program = GetASTProgram();
        decoder.Visit(program);
        break;
    }
    default:
        LOG_CRITICAL(HW_GPU, "Unknown decompilation mode!");
        [[fallthrough]];
    case CompileDepth::BruteForce: {
        const auto shader_end = static_cast<u32>(program_code.size());
        coverage_begin = main_offset;
        coverage_end = shader_end;
        for (u32 label = main_offset; label < shader_end; ++label) {
            basic_blocks.insert({label, DecodeRange(label, label + 1)});
        }
        break;
    }
    }
    if (settings.depth != shader_info.settings.depth) {
        LOG_WARNING(
            HW_GPU, "Decompiling to this setting \"{}\" failed, downgrading to this setting \"{}\"",
            CompileDepthAsString(settings.depth), CompileDepthAsString(shader_info.settings.depth));
    }
}

NodeBlock ShaderIR::DecodeRange(u32 begin, u32 end) {
    NodeBlock basic_block;
    DecodeRangeInner(basic_block, begin, end);
    return basic_block;
}

void ShaderIR::DecodeRangeInner(NodeBlock& bb, u32 begin, u32 end) {
    for (u32 pc = begin; pc < (begin > end ? MAX_PROGRAM_LENGTH : end);) {
        pc = DecodeInstr(bb, pc);
    }
}

void ShaderIR::InsertControlFlow(NodeBlock& bb, const ShaderBlock& block) {
    const auto apply_conditions = [&](const Condition& cond, Node n) -> Node {
        Node result = n;
        if (cond.cc != ConditionCode::T) {
            result = Conditional(GetConditionCode(cond.cc), {result});
        }
        if (cond.predicate != Pred::UnusedIndex) {
            u32 pred = static_cast<u32>(cond.predicate);
            const bool is_neg = pred > 7;
            if (is_neg) {
                pred -= 8;
            }
            result = Conditional(GetPredicate(pred, is_neg), {result});
        }
        return result;
    };
    if (std::holds_alternative<SingleBranch>(*block.branch)) {
        auto branch = std::get_if<SingleBranch>(block.branch.get());
        if (branch->address < 0) {
            if (branch->kill) {
                Node n = Operation(OperationCode::Discard);
                n = apply_conditions(branch->condition, n);
                bb.push_back(n);
                global_code.push_back(n);
                return;
            }
            Node n = Operation(OperationCode::Exit);
            n = apply_conditions(branch->condition, n);
            bb.push_back(n);
            global_code.push_back(n);
            return;
        }
        Node n = Operation(OperationCode::Branch, Immediate(branch->address));
        n = apply_conditions(branch->condition, n);
        bb.push_back(n);
        global_code.push_back(n);
        return;
    }
    auto multi_branch = std::get_if<MultiBranch>(block.branch.get());
    Node op_a = GetRegister(multi_branch->gpr);
    for (auto& branch_case : multi_branch->branches) {
        Node n = Operation(OperationCode::Branch, Immediate(branch_case.address));
        Node op_b = Immediate(branch_case.cmp_value);
        Node condition =
            GetPredicateComparisonInteger(Tegra::Shader::PredCondition::EQ, false, op_a, op_b);
        auto result = Conditional(condition, {n});
        bb.push_back(result);
        global_code.push_back(result);
    }
}

u32 ShaderIR::DecodeInstr(NodeBlock& bb, u32 pc) {
    // Ignore sched instructions when generating code.
    if (IsSchedInstruction(pc, main_offset)) {
        return pc + 1;
    }

    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);
    const u32 nv_address = ConvertAddressToNvidiaSpace(pc);

    // Decoding failure
    if (!opcode) {
        UNIMPLEMENTED_MSG("Unhandled instruction: {0:x}", instr.value);
        bb.push_back(Comment(fmt::format("{:05x} Unimplemented Shader instruction (0x{:016x})",
                                         nv_address, instr.value)));
        return pc + 1;
    }

    bb.push_back(Comment(
        fmt::format("{:05x} {} (0x{:016x})", nv_address, opcode->get().GetName(), instr.value)));

    using Tegra::Shader::Pred;
    UNIMPLEMENTED_IF_MSG(instr.pred.full_pred == Pred::NeverExecute,
                         "NeverExecute predicate not implemented");

    static const std::map<OpCode::Type, u32 (ShaderIR::*)(NodeBlock&, u32)> decoders = {
        {OpCode::Type::Arithmetic, &ShaderIR::DecodeArithmetic},
        {OpCode::Type::ArithmeticImmediate, &ShaderIR::DecodeArithmeticImmediate},
        {OpCode::Type::Bfe, &ShaderIR::DecodeBfe},
        {OpCode::Type::Bfi, &ShaderIR::DecodeBfi},
        {OpCode::Type::Shift, &ShaderIR::DecodeShift},
        {OpCode::Type::ArithmeticInteger, &ShaderIR::DecodeArithmeticInteger},
        {OpCode::Type::ArithmeticIntegerImmediate, &ShaderIR::DecodeArithmeticIntegerImmediate},
        {OpCode::Type::ArithmeticHalf, &ShaderIR::DecodeArithmeticHalf},
        {OpCode::Type::ArithmeticHalfImmediate, &ShaderIR::DecodeArithmeticHalfImmediate},
        {OpCode::Type::Ffma, &ShaderIR::DecodeFfma},
        {OpCode::Type::Hfma2, &ShaderIR::DecodeHfma2},
        {OpCode::Type::Conversion, &ShaderIR::DecodeConversion},
        {OpCode::Type::Warp, &ShaderIR::DecodeWarp},
        {OpCode::Type::Memory, &ShaderIR::DecodeMemory},
        {OpCode::Type::Texture, &ShaderIR::DecodeTexture},
        {OpCode::Type::Image, &ShaderIR::DecodeImage},
        {OpCode::Type::FloatSetPredicate, &ShaderIR::DecodeFloatSetPredicate},
        {OpCode::Type::IntegerSetPredicate, &ShaderIR::DecodeIntegerSetPredicate},
        {OpCode::Type::HalfSetPredicate, &ShaderIR::DecodeHalfSetPredicate},
        {OpCode::Type::PredicateSetRegister, &ShaderIR::DecodePredicateSetRegister},
        {OpCode::Type::PredicateSetPredicate, &ShaderIR::DecodePredicateSetPredicate},
        {OpCode::Type::RegisterSetPredicate, &ShaderIR::DecodeRegisterSetPredicate},
        {OpCode::Type::FloatSet, &ShaderIR::DecodeFloatSet},
        {OpCode::Type::IntegerSet, &ShaderIR::DecodeIntegerSet},
        {OpCode::Type::HalfSet, &ShaderIR::DecodeHalfSet},
        {OpCode::Type::Video, &ShaderIR::DecodeVideo},
        {OpCode::Type::Xmad, &ShaderIR::DecodeXmad},
    };

    std::vector<Node> tmp_block;
    if (const auto decoder = decoders.find(opcode->get().GetType()); decoder != decoders.end()) {
        pc = (this->*decoder->second)(tmp_block, pc);
    } else {
        pc = DecodeOther(tmp_block, pc);
    }

    // Some instructions (like SSY) don't have a predicate field, they are always unconditionally
    // executed.
    const bool can_be_predicated = OpCode::IsPredicatedInstruction(opcode->get().GetId());
    const auto pred_index = static_cast<u32>(instr.pred.pred_index);

    if (can_be_predicated && pred_index != static_cast<u32>(Pred::UnusedIndex)) {
        const Node conditional =
            Conditional(GetPredicate(pred_index, instr.negate_pred != 0), std::move(tmp_block));
        global_code.push_back(conditional);
        bb.push_back(conditional);
    } else {
        for (auto& node : tmp_block) {
            global_code.push_back(node);
            bb.push_back(node);
        }
    }

    return pc + 1;
}

void ShaderIR::PostDecode() {
    // Deduce texture handler size if needed
    auto gpu_driver = registry.AccessGuestDriverProfile();
    DeduceTextureHandlerSize(gpu_driver, used_samplers);
    // Deduce Indexed Samplers
    if (!uses_indexed_samplers) {
        return;
    }
    for (auto& sampler : used_samplers) {
        if (!sampler.is_indexed) {
            continue;
        }
        if (const auto size = TryDeduceSamplerSize(sampler, gpu_driver, used_samplers)) {
            sampler.size = *size;
        } else {
            LOG_CRITICAL(HW_GPU, "Failed to deduce size of indexed sampler");
            sampler.size = 1;
        }
    }
}

} // namespace VideoCommon::Shader

// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <memory>
#include <vector>
#include <queue>

#include "common/settings.h"
#include "shader_recompiler/exception.h"
#include "shader_recompiler/frontend/ir/basic_block.h"
#include "shader_recompiler/frontend/ir/ir_emitter.h"
#include "shader_recompiler/frontend/ir/post_order.h"
#include "shader_recompiler/frontend/maxwell/structured_control_flow.h"
#include "shader_recompiler/frontend/maxwell/translate/translate.h"
#include "shader_recompiler/frontend/maxwell/translate_program.h"
#include "shader_recompiler/host_translate_info.h"
#include "shader_recompiler/ir_opt/passes.h"

namespace Shader::Maxwell {
namespace {
IR::BlockList GenerateBlocks(const IR::AbstractSyntaxList& syntax_list) {
    size_t num_syntax_blocks{};
    for (const auto& node : syntax_list) {
        if (node.type == IR::AbstractSyntaxNode::Type::Block) {
            ++num_syntax_blocks;
        }
    }
    IR::BlockList blocks;
    blocks.reserve(num_syntax_blocks);
    u32 order_index{};
    for (const auto& node : syntax_list) {
        if (node.type == IR::AbstractSyntaxNode::Type::Block) {
            blocks.push_back(node.data.block);
            blocks.back()->SetOrder(order_index++);
        }
    }
    return blocks;
}

void RemoveUnreachableBlocks(IR::Program& program) {
    // Some blocks might be unreachable if a function call exists unconditionally
    // If this happens the number of blocks and post order blocks will mismatch
    if (program.blocks.size() == program.post_order_blocks.size()) {
        return;
    }
    const auto begin{program.blocks.begin() + 1};
    const auto end{program.blocks.end()};
    const auto pred{[](IR::Block* block) { return block->ImmPredecessors().empty(); }};
    program.blocks.erase(std::remove_if(begin, end, pred), end);
}

void CollectInterpolationInfo(Environment& env, IR::Program& program) {
    if (program.stage != Stage::Fragment) {
        return;
    }
    const ProgramHeader& sph{env.SPH()};
    for (size_t index = 0; index < IR::NUM_GENERICS; ++index) {
        std::optional<PixelImap> imap;
        for (const PixelImap value : sph.ps.GenericInputMap(static_cast<u32>(index))) {
            if (value == PixelImap::Unused) {
                continue;
            }
            if (imap && imap != value) {
                throw NotImplementedException("Per component interpolation");
            }
            imap = value;
        }
        if (!imap) {
            continue;
        }
        program.info.interpolation[index] = [&] {
            switch (*imap) {
            case PixelImap::Unused:
            case PixelImap::Perspective:
                return Interpolation::Smooth;
            case PixelImap::Constant:
                return Interpolation::Flat;
            case PixelImap::ScreenLinear:
                return Interpolation::NoPerspective;
            }
            throw NotImplementedException("Unknown interpolation {}", *imap);
        }();
    }
}

void AddNVNStorageBuffers(IR::Program& program) {
    if (!program.info.uses_global_memory) {
        return;
    }
    const u32 driver_cbuf{0};
    const u32 descriptor_size{0x10};
    const u32 num_buffers{16};
    const u32 base{[&] {
        switch (program.stage) {
        case Stage::VertexA:
        case Stage::VertexB:
            return 0x110u;
        case Stage::TessellationControl:
            return 0x210u;
        case Stage::TessellationEval:
            return 0x310u;
        case Stage::Geometry:
            return 0x410u;
        case Stage::Fragment:
            return 0x510u;
        case Stage::Compute:
            return 0x310u;
        }
        throw InvalidArgument("Invalid stage {}", program.stage);
    }()};
    auto& descs{program.info.storage_buffers_descriptors};
    for (u32 index = 0; index < num_buffers; ++index) {
        if (!program.info.nvn_buffer_used[index]) {
            continue;
        }
        const u32 offset{base + index * descriptor_size};
        const auto it{std::ranges::find(descs, offset, &StorageBufferDescriptor::cbuf_offset)};
        if (it != descs.end()) {
            it->is_written |= program.info.stores_global_memory;
            continue;
        }
        descs.push_back({
            .cbuf_index = driver_cbuf,
            .cbuf_offset = offset,
            .count = 1,
            .is_written = program.info.stores_global_memory,
        });
    }
}

bool IsLegacyAttribute(IR::Attribute attribute) {
    return (attribute >= IR::Attribute::ColorFrontDiffuseR &&
            attribute <= IR::Attribute::ColorBackSpecularA) ||
           attribute == IR::Attribute::FogCoordinate ||
           (attribute >= IR::Attribute::FixedFncTexture0S &&
            attribute <= IR::Attribute::FixedFncTexture9Q);
}

std::map<IR::Attribute, IR::Attribute> GenerateLegacyToGenericMappings(
    const VaryingState& state, std::queue<IR::Attribute> unused_generics,
    const std::map<IR::Attribute, IR::Attribute>& previous_stage_mapping) {
    std::map<IR::Attribute, IR::Attribute> mapping;
    auto update_mapping = [&mapping, &unused_generics, previous_stage_mapping](IR::Attribute attr,
                                                                               size_t count) {
        if (previous_stage_mapping.find(attr) != previous_stage_mapping.end()) {
            for (size_t i = 0; i < count; ++i) {
                mapping.insert({attr + i, previous_stage_mapping.at(attr + i)});
            }
        } else {
            for (size_t i = 0; i < count; ++i) {
                mapping.insert({attr + i, unused_generics.front() + i});
            }
            unused_generics.pop();
        }
    };
    for (size_t index = 0; index < 4; ++index) {
        auto attr = IR::Attribute::ColorFrontDiffuseR + index * 4;
        if (state.AnyComponent(attr)) {
            update_mapping(attr, 4);
        }
    }
    if (state[IR::Attribute::FogCoordinate]) {
        update_mapping(IR::Attribute::FogCoordinate, 1);
    }
    for (size_t index = 0; index < IR::NUM_FIXEDFNCTEXTURE; ++index) {
        auto attr = IR::Attribute::FixedFncTexture0S + index * 4;
        if (state.AnyComponent(attr)) {
            update_mapping(attr, 4);
        }
    }
    return mapping;
}
} // Anonymous namespace

IR::Program TranslateProgram(ObjectPool<IR::Inst>& inst_pool, ObjectPool<IR::Block>& block_pool,
                             Environment& env, Flow::CFG& cfg, const HostTranslateInfo& host_info) {
    IR::Program program;
    program.syntax_list = BuildASL(inst_pool, block_pool, env, cfg, host_info);
    program.blocks = GenerateBlocks(program.syntax_list);
    program.post_order_blocks = PostOrder(program.syntax_list.front());
    program.stage = env.ShaderStage();
    program.local_memory_size = env.LocalMemorySize();
    switch (program.stage) {
    case Stage::TessellationControl: {
        const ProgramHeader& sph{env.SPH()};
        program.invocations = sph.common2.threads_per_input_primitive;
        break;
    }
    case Stage::Geometry: {
        const ProgramHeader& sph{env.SPH()};
        program.output_topology = sph.common3.output_topology;
        program.output_vertices = sph.common4.max_output_vertices;
        program.invocations = sph.common2.threads_per_input_primitive;
        program.is_geometry_passthrough = sph.common0.geometry_passthrough != 0;
        if (program.is_geometry_passthrough) {
            const auto& mask{env.GpPassthroughMask()};
            for (size_t i = 0; i < program.info.passthrough.mask.size(); ++i) {
                program.info.passthrough.mask[i] = ((mask[i / 32] >> (i % 32)) & 1) == 0;
            }
        }
        break;
    }
    case Stage::Compute:
        program.workgroup_size = env.WorkgroupSize();
        program.shared_memory_size = env.SharedMemorySize();
        break;
    default:
        break;
    }
    RemoveUnreachableBlocks(program);

    // Replace instructions before the SSA rewrite
    if (!host_info.support_float16) {
        Optimization::LowerFp16ToFp32(program);
    }
    if (!host_info.support_int64) {
        Optimization::LowerInt64ToInt32(program);
    }
    Optimization::SsaRewritePass(program);

    Optimization::ConstantPropagationPass(program);

    Optimization::PositionPass(env, program);

    Optimization::GlobalMemoryToStorageBufferPass(program);
    Optimization::TexturePass(env, program, host_info);

    if (Settings::values.resolution_info.active) {
        Optimization::RescalingPass(program);
    }
    Optimization::DeadCodeEliminationPass(program);
    if (Settings::values.renderer_debug) {
        Optimization::VerificationPass(program);
    }
    Optimization::CollectShaderInfoPass(env, program);
    Optimization::LayerPass(program, host_info);

    CollectInterpolationInfo(env, program);
    AddNVNStorageBuffers(program);
    return program;
}

IR::Program MergeDualVertexPrograms(IR::Program& vertex_a, IR::Program& vertex_b,
                                    Environment& env_vertex_b) {
    IR::Program result{};
    Optimization::VertexATransformPass(vertex_a);
    Optimization::VertexBTransformPass(vertex_b);
    for (const auto& term : vertex_a.syntax_list) {
        if (term.type != IR::AbstractSyntaxNode::Type::Return) {
            result.syntax_list.push_back(term);
        }
    }
    result.syntax_list.insert(result.syntax_list.end(), vertex_b.syntax_list.begin(),
                              vertex_b.syntax_list.end());
    result.blocks = GenerateBlocks(result.syntax_list);
    result.post_order_blocks = vertex_b.post_order_blocks;
    for (const auto& block : vertex_a.post_order_blocks) {
        result.post_order_blocks.push_back(block);
    }
    result.stage = Stage::VertexB;
    result.info = vertex_a.info;
    result.local_memory_size = std::max(vertex_a.local_memory_size, vertex_b.local_memory_size);
    result.info.loads.mask |= vertex_b.info.loads.mask;
    result.info.stores.mask |= vertex_b.info.stores.mask;

    Optimization::JoinTextureInfo(result.info, vertex_b.info);
    Optimization::JoinStorageInfo(result.info, vertex_b.info);
    Optimization::DeadCodeEliminationPass(result);
    if (Settings::values.renderer_debug) {
        Optimization::VerificationPass(result);
    }
    Optimization::CollectShaderInfoPass(env_vertex_b, result);
    return result;
}

void ConvertLegacyToGeneric(IR::Program& program, const Shader::RuntimeInfo& runtime_info) {
    auto& stores = program.info.stores;
    if (stores.Legacy()) {
        std::queue<IR::Attribute> unused_output_generics{};
        for (size_t index = 0; index < IR::NUM_GENERICS; ++index) {
            if (!stores.Generic(index)) {
                unused_output_generics.push(IR::Attribute::Generic0X + index * 4);
            }
        }
        program.info.legacy_stores_mapping =
            GenerateLegacyToGenericMappings(stores, unused_output_generics, {});
        for (IR::Block* const block : program.post_order_blocks) {
            for (IR::Inst& inst : block->Instructions()) {
                switch (inst.GetOpcode()) {
                case IR::Opcode::SetAttribute: {
                    const auto attr = inst.Arg(0).Attribute();
                    if (IsLegacyAttribute(attr)) {
                        stores.Set(program.info.legacy_stores_mapping[attr], true);
                        inst.SetArg(0, Shader::IR::Value(program.info.legacy_stores_mapping[attr]));
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }
    }

    auto& loads = program.info.loads;
    if (loads.Legacy()) {
        std::queue<IR::Attribute> unused_input_generics{};
        for (size_t index = 0; index < IR::NUM_GENERICS; ++index) {
            const AttributeType input_type{runtime_info.generic_input_types[index]};
            if (!runtime_info.previous_stage_stores.Generic(index) || !loads.Generic(index) ||
                input_type == AttributeType::Disabled) {
                unused_input_generics.push(IR::Attribute::Generic0X + index * 4);
            }
        }
        auto mappings = GenerateLegacyToGenericMappings(
            loads, unused_input_generics, runtime_info.previous_stage_legacy_stores_mapping);
        for (IR::Block* const block : program.post_order_blocks) {
            for (IR::Inst& inst : block->Instructions()) {
                switch (inst.GetOpcode()) {
                case IR::Opcode::GetAttribute: {
                    const auto attr = inst.Arg(0).Attribute();
                    if (IsLegacyAttribute(attr)) {
                        loads.Set(mappings[attr], true);
                        inst.SetArg(0, Shader::IR::Value(mappings[attr]));
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }
    }
}

IR::Program GenerateGeometryPassthrough(ObjectPool<IR::Inst>& inst_pool,
                                        ObjectPool<IR::Block>& block_pool,
                                        const HostTranslateInfo& host_info,
                                        IR::Program& source_program,
                                        Shader::OutputTopology output_topology) {
    IR::Program program;
    program.stage = Stage::Geometry;
    program.output_topology = output_topology;
    switch (output_topology) {
    case OutputTopology::PointList:
        program.output_vertices = 1;
        break;
    case OutputTopology::LineStrip:
        program.output_vertices = 2;
        break;
    default:
        program.output_vertices = 3;
        break;
    }

    program.is_geometry_passthrough = false;
    program.info.loads.mask = source_program.info.stores.mask;
    program.info.stores.mask = source_program.info.stores.mask;
    program.info.stores.Set(IR::Attribute::Layer, true);
    program.info.stores.Set(source_program.info.emulated_layer, false);

    IR::Block* current_block = block_pool.Create(inst_pool);
    auto& node{program.syntax_list.emplace_back()};
    node.type = IR::AbstractSyntaxNode::Type::Block;
    node.data.block = current_block;

    IR::IREmitter ir{*current_block};
    for (u32 i = 0; i < program.output_vertices; i++) {
        // Assign generics from input
        for (u32 j = 0; j < 32; j++) {
            if (!program.info.stores.Generic(j)) {
                continue;
            }

            const IR::Attribute attr = IR::Attribute::Generic0X + (j * 4);
            ir.SetAttribute(attr + 0, ir.GetAttribute(attr + 0, ir.Imm32(i)), ir.Imm32(0));
            ir.SetAttribute(attr + 1, ir.GetAttribute(attr + 1, ir.Imm32(i)), ir.Imm32(0));
            ir.SetAttribute(attr + 2, ir.GetAttribute(attr + 2, ir.Imm32(i)), ir.Imm32(0));
            ir.SetAttribute(attr + 3, ir.GetAttribute(attr + 3, ir.Imm32(i)), ir.Imm32(0));
        }

        // Assign position from input
        const IR::Attribute attr = IR::Attribute::PositionX;
        ir.SetAttribute(attr + 0, ir.GetAttribute(attr + 0, ir.Imm32(i)), ir.Imm32(0));
        ir.SetAttribute(attr + 1, ir.GetAttribute(attr + 1, ir.Imm32(i)), ir.Imm32(0));
        ir.SetAttribute(attr + 2, ir.GetAttribute(attr + 2, ir.Imm32(i)), ir.Imm32(0));
        ir.SetAttribute(attr + 3, ir.GetAttribute(attr + 3, ir.Imm32(i)), ir.Imm32(0));

        // Assign layer
        ir.SetAttribute(IR::Attribute::Layer, ir.GetAttribute(source_program.info.emulated_layer),
                        ir.Imm32(0));

        // Emit vertex
        ir.EmitVertex(ir.Imm32(0));
    }
    ir.EndPrimitive(ir.Imm32(0));

    IR::Block* return_block{block_pool.Create(inst_pool)};
    IR::IREmitter{*return_block}.Epilogue();
    current_block->AddBranch(return_block);

    auto& merge{program.syntax_list.emplace_back()};
    merge.type = IR::AbstractSyntaxNode::Type::Block;
    merge.data.block = return_block;
    program.syntax_list.emplace_back().type = IR::AbstractSyntaxNode::Type::Return;

    program.blocks = GenerateBlocks(program.syntax_list);
    program.post_order_blocks = PostOrder(program.syntax_list.front());
    Optimization::SsaRewritePass(program);

    return program;
}

} // namespace Shader::Maxwell

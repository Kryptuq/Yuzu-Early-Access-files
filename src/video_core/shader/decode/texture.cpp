// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <vector>
#include <fmt/format.h>

#include "common/assert.h"
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/shader/node_helper.h"
#include "video_core/shader/registry.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

using Tegra::Shader::Instruction;
using Tegra::Shader::OpCode;
using Tegra::Shader::Register;
using Tegra::Shader::TextureMiscMode;
using Tegra::Shader::TextureProcessMode;
using Tegra::Shader::TextureType;

static std::size_t GetCoordCount(TextureType texture_type) {
    switch (texture_type) {
    case TextureType::Texture1D:
        return 1;
    case TextureType::Texture2D:
        return 2;
    case TextureType::Texture3D:
    case TextureType::TextureCube:
        return 3;
    default:
        UNIMPLEMENTED_MSG("Unhandled texture type: {}", texture_type);
        return 0;
    }
}

u32 ShaderIR::DecodeTexture(NodeBlock& bb, u32 pc) {
    const Instruction instr = {program_code[pc]};
    const auto opcode = OpCode::Decode(instr);
    bool is_bindless = false;
    switch (opcode->get().GetId()) {
    case OpCode::Id::TEX: {
        const TextureType texture_type{instr.tex.texture_type};
        const bool is_array = instr.tex.array != 0;
        const bool is_aoffi = instr.tex.UsesMiscMode(TextureMiscMode::AOFFI);
        const bool depth_compare = instr.tex.UsesMiscMode(TextureMiscMode::DC);
        const auto process_mode = instr.tex.GetTextureProcessMode();
        WriteTexInstructionFloat(
            bb, instr,
            GetTexCode(instr, texture_type, process_mode, depth_compare, is_array, is_aoffi, {}));
        break;
    }
    case OpCode::Id::TEX_B: {
        UNIMPLEMENTED_IF_MSG(instr.tex.UsesMiscMode(TextureMiscMode::AOFFI),
                             "AOFFI is not implemented");

        const TextureType texture_type{instr.tex_b.texture_type};
        const bool is_array = instr.tex_b.array != 0;
        const bool is_aoffi = instr.tex.UsesMiscMode(TextureMiscMode::AOFFI);
        const bool depth_compare = instr.tex_b.UsesMiscMode(TextureMiscMode::DC);
        const auto process_mode = instr.tex_b.GetTextureProcessMode();
        WriteTexInstructionFloat(bb, instr,
                                 GetTexCode(instr, texture_type, process_mode, depth_compare,
                                            is_array, is_aoffi, {instr.gpr20}));
        break;
    }
    case OpCode::Id::TEXS: {
        const TextureType texture_type{instr.texs.GetTextureType()};
        const bool is_array{instr.texs.IsArrayTexture()};
        const bool depth_compare = instr.texs.UsesMiscMode(TextureMiscMode::DC);
        const auto process_mode = instr.texs.GetTextureProcessMode();

        const Node4 components =
            GetTexsCode(instr, texture_type, process_mode, depth_compare, is_array);

        if (instr.texs.fp32_flag) {
            WriteTexsInstructionFloat(bb, instr, components);
        } else {
            WriteTexsInstructionHalfFloat(bb, instr, components);
        }
        break;
    }
    case OpCode::Id::TLD4_B: {
        is_bindless = true;
        [[fallthrough]];
    }
    case OpCode::Id::TLD4: {
        UNIMPLEMENTED_IF_MSG(instr.tld4.UsesMiscMode(TextureMiscMode::NDV),
                             "NDV is not implemented");
        const auto texture_type = instr.tld4.texture_type.Value();
        const bool depth_compare = is_bindless ? instr.tld4_b.UsesMiscMode(TextureMiscMode::DC)
                                               : instr.tld4.UsesMiscMode(TextureMiscMode::DC);
        const bool is_array = instr.tld4.array != 0;
        const bool is_aoffi = is_bindless ? instr.tld4_b.UsesMiscMode(TextureMiscMode::AOFFI)
                                          : instr.tld4.UsesMiscMode(TextureMiscMode::AOFFI);
        const bool is_ptp = is_bindless ? instr.tld4_b.UsesMiscMode(TextureMiscMode::PTP)
                                        : instr.tld4.UsesMiscMode(TextureMiscMode::PTP);
        WriteTexInstructionFloat(bb, instr,
                                 GetTld4Code(instr, texture_type, depth_compare, is_array, is_aoffi,
                                             is_ptp, is_bindless));
        break;
    }
    case OpCode::Id::TLD4S: {
        constexpr std::size_t num_coords = 2;
        const bool is_aoffi = instr.tld4s.UsesMiscMode(TextureMiscMode::AOFFI);
        const bool is_depth_compare = instr.tld4s.UsesMiscMode(TextureMiscMode::DC);
        const Node op_a = GetRegister(instr.gpr8);
        const Node op_b = GetRegister(instr.gpr20);

        // TODO(Subv): Figure out how the sampler type is encoded in the TLD4S instruction.
        std::vector<Node> coords;
        std::vector<Node> aoffi;
        Node depth_compare;
        if (is_depth_compare) {
            // Note: TLD4S coordinate encoding works just like TEXS's
            const Node op_y = GetRegister(instr.gpr8.Value() + 1);
            coords.push_back(op_a);
            coords.push_back(op_y);
            if (is_aoffi) {
                aoffi = GetAoffiCoordinates(op_b, num_coords, true);
                depth_compare = GetRegister(instr.gpr20.Value() + 1);
            } else {
                depth_compare = op_b;
            }
        } else {
            // There's no depth compare
            coords.push_back(op_a);
            if (is_aoffi) {
                coords.push_back(GetRegister(instr.gpr8.Value() + 1));
                aoffi = GetAoffiCoordinates(op_b, num_coords, true);
            } else {
                coords.push_back(op_b);
            }
        }
        const Node component = Immediate(static_cast<u32>(instr.tld4s.component));

        SamplerInfo info;
        info.is_shadow = is_depth_compare;
        const std::optional<SamplerEntry> sampler = GetSampler(instr.sampler, info);

        Node4 values;
        for (u32 element = 0; element < values.size(); ++element) {
            MetaTexture meta{*sampler, {}, depth_compare, aoffi,   {}, {},
                             {},       {}, component,     element, {}};
            values[element] = Operation(OperationCode::TextureGather, meta, coords);
        }

        if (instr.tld4s.fp16_flag) {
            WriteTexsInstructionHalfFloat(bb, instr, values, true);
        } else {
            WriteTexsInstructionFloat(bb, instr, values, true);
        }
        break;
    }
    case OpCode::Id::TXD_B:
        is_bindless = true;
        [[fallthrough]];
    case OpCode::Id::TXD: {
        UNIMPLEMENTED_IF_MSG(instr.txd.UsesMiscMode(TextureMiscMode::AOFFI),
                             "AOFFI is not implemented");

        const bool is_array = instr.txd.is_array != 0;
        const auto derivate_reg = instr.gpr20.Value();
        const auto texture_type = instr.txd.texture_type.Value();
        const auto coord_count = GetCoordCount(texture_type);
        u64 base_reg = instr.gpr8.Value();
        Node index_var;
        SamplerInfo info;
        info.type = texture_type;
        info.is_array = is_array;
        const std::optional<SamplerEntry> sampler =
            is_bindless ? GetBindlessSampler(base_reg, info, index_var)
                        : GetSampler(instr.sampler, info);
        Node4 values;
        if (!sampler) {
            std::generate(values.begin(), values.end(), [this] { return Immediate(0); });
            WriteTexInstructionFloat(bb, instr, values);
            break;
        }

        if (is_bindless) {
            base_reg++;
        }

        std::vector<Node> coords;
        std::vector<Node> derivates;
        for (std::size_t i = 0; i < coord_count; ++i) {
            coords.push_back(GetRegister(base_reg + i));
            const std::size_t derivate = i * 2;
            derivates.push_back(GetRegister(derivate_reg + derivate));
            derivates.push_back(GetRegister(derivate_reg + derivate + 1));
        }

        Node array_node = {};
        if (is_array) {
            const Node info_reg = GetRegister(base_reg + coord_count);
            array_node = BitfieldExtract(info_reg, 0, 16);
        }

        for (u32 element = 0; element < values.size(); ++element) {
            MetaTexture meta{*sampler, array_node, {}, {},      {},       derivates,
                             {},       {},         {}, element, index_var};
            values[element] = Operation(OperationCode::TextureGradient, std::move(meta), coords);
        }

        WriteTexInstructionFloat(bb, instr, values);

        break;
    }
    case OpCode::Id::TXQ_B:
        is_bindless = true;
        [[fallthrough]];
    case OpCode::Id::TXQ: {
        Node index_var;
        const std::optional<SamplerEntry> sampler =
            is_bindless ? GetBindlessSampler(instr.gpr8, {}, index_var)
                        : GetSampler(instr.sampler, {});

        if (!sampler) {
            u32 indexer = 0;
            for (u32 element = 0; element < 4; ++element) {
                if (!instr.txq.IsComponentEnabled(element)) {
                    continue;
                }
                const Node value = Immediate(0);
                SetTemporary(bb, indexer++, value);
            }
            for (u32 i = 0; i < indexer; ++i) {
                SetRegister(bb, instr.gpr0.Value() + i, GetTemporary(i));
            }
            break;
        }

        u32 indexer = 0;
        switch (instr.txq.query_type) {
        case Tegra::Shader::TextureQueryType::Dimension: {
            for (u32 element = 0; element < 4; ++element) {
                if (!instr.txq.IsComponentEnabled(element)) {
                    continue;
                }
                MetaTexture meta{*sampler, {}, {}, {}, {}, {}, {}, {}, {}, element, index_var};
                const Node value =
                    Operation(OperationCode::TextureQueryDimensions, meta,
                              GetRegister(instr.gpr8.Value() + (is_bindless ? 1 : 0)));
                SetTemporary(bb, indexer++, value);
            }
            for (u32 i = 0; i < indexer; ++i) {
                SetRegister(bb, instr.gpr0.Value() + i, GetTemporary(i));
            }
            break;
        }
        default:
            UNIMPLEMENTED_MSG("Unhandled texture query type: {}", instr.txq.query_type.Value());
        }
        break;
    }
    case OpCode::Id::TMML_B:
        is_bindless = true;
        [[fallthrough]];
    case OpCode::Id::TMML: {
        UNIMPLEMENTED_IF_MSG(instr.tmml.UsesMiscMode(Tegra::Shader::TextureMiscMode::NDV),
                             "NDV is not implemented");

        const auto texture_type = instr.tmml.texture_type.Value();
        const bool is_array = instr.tmml.array != 0;
        SamplerInfo info;
        info.type = texture_type;
        info.is_array = is_array;
        Node index_var;
        const std::optional<SamplerEntry> sampler =
            is_bindless ? GetBindlessSampler(instr.gpr20, info, index_var)
                        : GetSampler(instr.sampler, info);

        if (!sampler) {
            u32 indexer = 0;
            for (u32 element = 0; element < 2; ++element) {
                if (!instr.tmml.IsComponentEnabled(element)) {
                    continue;
                }
                const Node value = Immediate(0);
                SetTemporary(bb, indexer++, value);
            }
            for (u32 i = 0; i < indexer; ++i) {
                SetRegister(bb, instr.gpr0.Value() + i, GetTemporary(i));
            }
            break;
        }

        const u64 base_index = is_array ? 1 : 0;
        const u64 num_components = [texture_type] {
            switch (texture_type) {
            case TextureType::Texture1D:
                return 1;
            case TextureType::Texture2D:
                return 2;
            case TextureType::TextureCube:
                return 3;
            default:
                UNIMPLEMENTED_MSG("Unhandled texture type {}", texture_type);
                return 2;
            }
        }();
        // TODO: What's the array component used for?

        std::vector<Node> coords;
        coords.reserve(num_components);
        for (u64 component = 0; component < num_components; ++component) {
            coords.push_back(GetRegister(instr.gpr8.Value() + base_index + component));
        }

        u32 indexer = 0;
        for (u32 element = 0; element < 2; ++element) {
            if (!instr.tmml.IsComponentEnabled(element)) {
                continue;
            }
            MetaTexture meta{*sampler, {}, {}, {}, {}, {}, {}, {}, {}, element, index_var};
            Node value = Operation(OperationCode::TextureQueryLod, meta, coords);
            SetTemporary(bb, indexer++, std::move(value));
        }
        for (u32 i = 0; i < indexer; ++i) {
            SetRegister(bb, instr.gpr0.Value() + i, GetTemporary(i));
        }
        break;
    }
    case OpCode::Id::TLD: {
        UNIMPLEMENTED_IF_MSG(instr.tld.aoffi, "AOFFI is not implemented");
        UNIMPLEMENTED_IF_MSG(instr.tld.ms, "MS is not implemented");
        UNIMPLEMENTED_IF_MSG(instr.tld.cl, "CL is not implemented");

        WriteTexInstructionFloat(bb, instr, GetTldCode(instr));
        break;
    }
    case OpCode::Id::TLDS: {
        const TextureType texture_type{instr.tlds.GetTextureType()};
        const bool is_array{instr.tlds.IsArrayTexture()};

        UNIMPLEMENTED_IF_MSG(instr.tlds.UsesMiscMode(TextureMiscMode::AOFFI),
                             "AOFFI is not implemented");
        UNIMPLEMENTED_IF_MSG(instr.tlds.UsesMiscMode(TextureMiscMode::MZ), "MZ is not implemented");

        const Node4 components = GetTldsCode(instr, texture_type, is_array);

        if (instr.tlds.fp32_flag) {
            WriteTexsInstructionFloat(bb, instr, components);
        } else {
            WriteTexsInstructionHalfFloat(bb, instr, components);
        }
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Unhandled memory instruction: {}", opcode->get().GetName());
    }

    return pc;
}

ShaderIR::SamplerInfo ShaderIR::GetSamplerInfo(
    SamplerInfo info, std::optional<Tegra::Engines::SamplerDescriptor> sampler) {
    if (info.IsComplete()) {
        return info;
    }
    if (!sampler) {
        LOG_WARNING(HW_GPU, "Unknown sampler info");
        info.type = info.type.value_or(Tegra::Shader::TextureType::Texture2D);
        info.is_array = info.is_array.value_or(false);
        info.is_shadow = info.is_shadow.value_or(false);
        info.is_buffer = info.is_buffer.value_or(false);
        return info;
    }
    info.type = info.type.value_or(sampler->texture_type);
    info.is_array = info.is_array.value_or(sampler->is_array != 0);
    info.is_shadow = info.is_shadow.value_or(sampler->is_shadow != 0);
    info.is_buffer = info.is_buffer.value_or(sampler->is_buffer != 0);
    return info;
}

std::optional<SamplerEntry> ShaderIR::GetSampler(Tegra::Shader::Sampler sampler,
                                                 SamplerInfo sampler_info) {
    const u32 offset = static_cast<u32>(sampler.index.Value());
    const auto info = GetSamplerInfo(sampler_info, registry.ObtainBoundSampler(offset));

    // If this sampler has already been used, return the existing mapping.
    const auto it =
        std::find_if(used_samplers.begin(), used_samplers.end(),
                     [offset](const SamplerEntry& entry) { return entry.offset == offset; });
    if (it != used_samplers.end()) {
        ASSERT(!it->is_bindless && it->type == info.type && it->is_array == info.is_array &&
               it->is_shadow == info.is_shadow && it->is_buffer == info.is_buffer);
        return *it;
    }

    // Otherwise create a new mapping for this sampler
    const auto next_index = static_cast<u32>(used_samplers.size());
    return used_samplers.emplace_back(next_index, offset, *info.type, *info.is_array,
                                      *info.is_shadow, *info.is_buffer, false);
}

std::optional<SamplerEntry> ShaderIR::GetBindlessSampler(Tegra::Shader::Register reg,
                                                         SamplerInfo info, Node& index_var) {
    const Node sampler_register = GetRegister(reg);
    const auto [base_node, tracked_sampler_info] =
        TrackBindlessSampler(sampler_register, global_code, static_cast<s64>(global_code.size()));
    if (!base_node) {
        UNREACHABLE();
        return std::nullopt;
    }

    if (const auto sampler_info = std::get_if<BindlessSamplerNode>(&*tracked_sampler_info)) {
        const u32 buffer = sampler_info->index;
        const u32 offset = sampler_info->offset;
        info = GetSamplerInfo(info, registry.ObtainBindlessSampler(buffer, offset));

        // If this sampler has already been used, return the existing mapping.
        const auto it = std::find_if(used_samplers.begin(), used_samplers.end(),
                                     [buffer, offset](const SamplerEntry& entry) {
                                         return entry.buffer == buffer && entry.offset == offset;
                                     });
        if (it != used_samplers.end()) {
            ASSERT(it->is_bindless && it->type == info.type && it->is_array == info.is_array &&
                   it->is_shadow == info.is_shadow);
            return *it;
        }

        // Otherwise create a new mapping for this sampler
        const auto next_index = static_cast<u32>(used_samplers.size());
        return used_samplers.emplace_back(next_index, offset, buffer, *info.type, *info.is_array,
                                          *info.is_shadow, *info.is_buffer, false);
    }
    if (const auto sampler_info = std::get_if<SeparateSamplerNode>(&*tracked_sampler_info)) {
        const std::pair indices = sampler_info->indices;
        const std::pair offsets = sampler_info->offsets;
        info = GetSamplerInfo(info, registry.ObtainSeparateSampler(indices, offsets));

        // Try to use an already created sampler if it exists
        const auto it =
            std::find_if(used_samplers.begin(), used_samplers.end(),
                         [indices, offsets](const SamplerEntry& entry) {
                             return offsets == std::pair{entry.offset, entry.secondary_offset} &&
                                    indices == std::pair{entry.buffer, entry.secondary_buffer};
                         });
        if (it != used_samplers.end()) {
            ASSERT(it->is_separated && it->type == info.type && it->is_array == info.is_array &&
                   it->is_shadow == info.is_shadow && it->is_buffer == info.is_buffer);
            return *it;
        }

        // Otherwise create a new mapping for this sampler
        const u32 next_index = static_cast<u32>(used_samplers.size());
        return used_samplers.emplace_back(next_index, offsets, indices, *info.type, *info.is_array,
                                          *info.is_shadow, *info.is_buffer);
    }
    if (const auto sampler_info = std::get_if<ArraySamplerNode>(&*tracked_sampler_info)) {
        const u32 base_offset = sampler_info->base_offset / 4;
        index_var = GetCustomVariable(sampler_info->bindless_var);
        info = GetSamplerInfo(info, registry.ObtainBoundSampler(base_offset));

        // If this sampler has already been used, return the existing mapping.
        const auto it = std::find_if(
            used_samplers.begin(), used_samplers.end(),
            [base_offset](const SamplerEntry& entry) { return entry.offset == base_offset; });
        if (it != used_samplers.end()) {
            ASSERT(!it->is_bindless && it->type == info.type && it->is_array == info.is_array &&
                   it->is_shadow == info.is_shadow && it->is_buffer == info.is_buffer &&
                   it->is_indexed);
            return *it;
        }

        uses_indexed_samplers = true;
        // Otherwise create a new mapping for this sampler
        const auto next_index = static_cast<u32>(used_samplers.size());
        return used_samplers.emplace_back(next_index, base_offset, *info.type, *info.is_array,
                                          *info.is_shadow, *info.is_buffer, true);
    }
    return std::nullopt;
}

void ShaderIR::WriteTexInstructionFloat(NodeBlock& bb, Instruction instr, const Node4& components) {
    u32 dest_elem = 0;
    for (u32 elem = 0; elem < 4; ++elem) {
        if (!instr.tex.IsComponentEnabled(elem)) {
            // Skip disabled components
            continue;
        }
        SetTemporary(bb, dest_elem++, components[elem]);
    }
    // After writing values in temporals, move them to the real registers
    for (u32 i = 0; i < dest_elem; ++i) {
        SetRegister(bb, instr.gpr0.Value() + i, GetTemporary(i));
    }
}

void ShaderIR::WriteTexsInstructionFloat(NodeBlock& bb, Instruction instr, const Node4& components,
                                         bool ignore_mask) {
    // TEXS has two destination registers and a swizzle. The first two elements in the swizzle
    // go into gpr0+0 and gpr0+1, and the rest goes into gpr28+0 and gpr28+1

    u32 dest_elem = 0;
    for (u32 component = 0; component < 4; ++component) {
        if (!instr.texs.IsComponentEnabled(component) && !ignore_mask)
            continue;
        SetTemporary(bb, dest_elem++, components[component]);
    }

    for (u32 i = 0; i < dest_elem; ++i) {
        if (i < 2) {
            // Write the first two swizzle components to gpr0 and gpr0+1
            SetRegister(bb, instr.gpr0.Value() + i % 2, GetTemporary(i));
        } else {
            ASSERT(instr.texs.HasTwoDestinations());
            // Write the rest of the swizzle components to gpr28 and gpr28+1
            SetRegister(bb, instr.gpr28.Value() + i % 2, GetTemporary(i));
        }
    }
}

void ShaderIR::WriteTexsInstructionHalfFloat(NodeBlock& bb, Instruction instr,
                                             const Node4& components, bool ignore_mask) {
    // TEXS.F16 destionation registers are packed in two registers in pairs (just like any half
    // float instruction).

    Node4 values;
    u32 dest_elem = 0;
    for (u32 component = 0; component < 4; ++component) {
        if (!instr.texs.IsComponentEnabled(component) && !ignore_mask)
            continue;
        values[dest_elem++] = components[component];
    }
    if (dest_elem == 0)
        return;

    std::generate(values.begin() + dest_elem, values.end(), [&]() { return Immediate(0); });

    const Node first_value = Operation(OperationCode::HPack2, values[0], values[1]);
    if (dest_elem <= 2) {
        SetRegister(bb, instr.gpr0, first_value);
        return;
    }

    SetTemporary(bb, 0, first_value);
    SetTemporary(bb, 1, Operation(OperationCode::HPack2, values[2], values[3]));

    SetRegister(bb, instr.gpr0, GetTemporary(0));
    SetRegister(bb, instr.gpr28, GetTemporary(1));
}

Node4 ShaderIR::GetTextureCode(Instruction instr, TextureType texture_type,
                               TextureProcessMode process_mode, std::vector<Node> coords,
                               Node array, Node depth_compare, u32 bias_offset,
                               std::vector<Node> aoffi,
                               std::optional<Tegra::Shader::Register> bindless_reg) {
    const bool is_array = array != nullptr;
    const bool is_shadow = depth_compare != nullptr;
    const bool is_bindless = bindless_reg.has_value();

    ASSERT_MSG(texture_type != TextureType::Texture3D || !is_array || !is_shadow,
               "Illegal texture type");

    SamplerInfo info;
    info.type = texture_type;
    info.is_array = is_array;
    info.is_shadow = is_shadow;
    info.is_buffer = false;

    Node index_var;
    const std::optional<SamplerEntry> sampler =
        is_bindless ? GetBindlessSampler(*bindless_reg, info, index_var)
                    : GetSampler(instr.sampler, info);
    if (!sampler) {
        return {Immediate(0), Immediate(0), Immediate(0), Immediate(0)};
    }

    const bool lod_needed = process_mode == TextureProcessMode::LZ ||
                            process_mode == TextureProcessMode::LL ||
                            process_mode == TextureProcessMode::LLA;
    const OperationCode opcode = lod_needed ? OperationCode::TextureLod : OperationCode::Texture;

    Node bias;
    Node lod;
    switch (process_mode) {
    case TextureProcessMode::None:
        break;
    case TextureProcessMode::LZ:
        lod = Immediate(0.0f);
        break;
    case TextureProcessMode::LB:
        // If present, lod or bias are always stored in the register indexed by the gpr20 field with
        // an offset depending on the usage of the other registers.
        bias = GetRegister(instr.gpr20.Value() + bias_offset);
        break;
    case TextureProcessMode::LL:
        lod = GetRegister(instr.gpr20.Value() + bias_offset);
        break;
    default:
        UNIMPLEMENTED_MSG("Unimplemented process mode={}", process_mode);
        break;
    }

    Node4 values;
    for (u32 element = 0; element < values.size(); ++element) {
        MetaTexture meta{*sampler, array, depth_compare, aoffi,    {}, {}, bias,
                         lod,      {},    element,       index_var};
        values[element] = Operation(opcode, meta, coords);
    }

    return values;
}

Node4 ShaderIR::GetTexCode(Instruction instr, TextureType texture_type,
                           TextureProcessMode process_mode, bool depth_compare, bool is_array,
                           bool is_aoffi, std::optional<Tegra::Shader::Register> bindless_reg) {
    const bool lod_bias_enabled{
        (process_mode != TextureProcessMode::None && process_mode != TextureProcessMode::LZ)};

    const bool is_bindless = bindless_reg.has_value();

    u64 parameter_register = instr.gpr20.Value();
    if (is_bindless) {
        ++parameter_register;
    }

    const u32 bias_lod_offset = (is_bindless ? 1 : 0);
    if (lod_bias_enabled) {
        ++parameter_register;
    }

    const auto coord_counts = ValidateAndGetCoordinateElement(texture_type, depth_compare, is_array,
                                                              lod_bias_enabled, 4, 5);
    const auto coord_count = std::get<0>(coord_counts);
    // If enabled arrays index is always stored in the gpr8 field
    const u64 array_register = instr.gpr8.Value();
    // First coordinate index is the gpr8 or gpr8 + 1 when arrays are used
    const u64 coord_register = array_register + (is_array ? 1 : 0);

    std::vector<Node> coords;
    for (std::size_t i = 0; i < coord_count; ++i) {
        coords.push_back(GetRegister(coord_register + i));
    }
    // 1D.DC in OpenGL the 2nd component is ignored.
    if (depth_compare && !is_array && texture_type == TextureType::Texture1D) {
        coords.push_back(Immediate(0.0f));
    }

    const Node array = is_array ? GetRegister(array_register) : nullptr;

    std::vector<Node> aoffi;
    if (is_aoffi) {
        aoffi = GetAoffiCoordinates(GetRegister(parameter_register++), coord_count, false);
    }

    Node dc;
    if (depth_compare) {
        // Depth is always stored in the register signaled by gpr20 or in the next register if lod
        // or bias are used
        dc = GetRegister(parameter_register++);
    }

    return GetTextureCode(instr, texture_type, process_mode, coords, array, dc, bias_lod_offset,
                          aoffi, bindless_reg);
}

Node4 ShaderIR::GetTexsCode(Instruction instr, TextureType texture_type,
                            TextureProcessMode process_mode, bool depth_compare, bool is_array) {
    const bool lod_bias_enabled =
        (process_mode != TextureProcessMode::None && process_mode != TextureProcessMode::LZ);

    const auto coord_counts = ValidateAndGetCoordinateElement(texture_type, depth_compare, is_array,
                                                              lod_bias_enabled, 4, 4);
    const auto coord_count = std::get<0>(coord_counts);

    // If enabled arrays index is always stored in the gpr8 field
    const u64 array_register = instr.gpr8.Value();
    // First coordinate index is stored in gpr8 field or (gpr8 + 1) when arrays are used
    const u64 coord_register = array_register + (is_array ? 1 : 0);
    const u64 last_coord_register =
        (is_array || !(lod_bias_enabled || depth_compare) || (coord_count > 2))
            ? static_cast<u64>(instr.gpr20.Value())
            : coord_register + 1;
    const u32 bias_offset = coord_count > 2 ? 1 : 0;

    std::vector<Node> coords;
    for (std::size_t i = 0; i < coord_count; ++i) {
        const bool last = (i == (coord_count - 1)) && (coord_count > 1);
        coords.push_back(GetRegister(last ? last_coord_register : coord_register + i));
    }

    const Node array = is_array ? GetRegister(array_register) : nullptr;

    Node dc;
    if (depth_compare) {
        // Depth is always stored in the register signaled by gpr20 or in the next register if lod
        // or bias are used
        const u64 depth_register = instr.gpr20.Value() + (lod_bias_enabled ? 1 : 0);
        dc = GetRegister(depth_register);
    }

    return GetTextureCode(instr, texture_type, process_mode, coords, array, dc, bias_offset, {},
                          {});
}

Node4 ShaderIR::GetTld4Code(Instruction instr, TextureType texture_type, bool depth_compare,
                            bool is_array, bool is_aoffi, bool is_ptp, bool is_bindless) {
    ASSERT_MSG(!(is_aoffi && is_ptp), "AOFFI and PTP can't be enabled at the same time");

    const std::size_t coord_count = GetCoordCount(texture_type);

    // If enabled arrays index is always stored in the gpr8 field
    const u64 array_register = instr.gpr8.Value();
    // First coordinate index is the gpr8 or gpr8 + 1 when arrays are used
    const u64 coord_register = array_register + (is_array ? 1 : 0);

    std::vector<Node> coords;
    for (std::size_t i = 0; i < coord_count; ++i) {
        coords.push_back(GetRegister(coord_register + i));
    }

    u64 parameter_register = instr.gpr20.Value();

    SamplerInfo info;
    info.type = texture_type;
    info.is_array = is_array;
    info.is_shadow = depth_compare;

    Node index_var;
    const std::optional<SamplerEntry> sampler =
        is_bindless ? GetBindlessSampler(parameter_register++, info, index_var)
                    : GetSampler(instr.sampler, info);
    Node4 values;
    if (!sampler) {
        for (u32 element = 0; element < values.size(); ++element) {
            values[element] = Immediate(0);
        }
        return values;
    }

    std::vector<Node> aoffi, ptp;
    if (is_aoffi) {
        aoffi = GetAoffiCoordinates(GetRegister(parameter_register++), coord_count, true);
    } else if (is_ptp) {
        ptp = GetPtpCoordinates(
            {GetRegister(parameter_register++), GetRegister(parameter_register++)});
    }

    Node dc;
    if (depth_compare) {
        dc = GetRegister(parameter_register++);
    }

    const Node component = is_bindless ? Immediate(static_cast<u32>(instr.tld4_b.component))
                                       : Immediate(static_cast<u32>(instr.tld4.component));

    for (u32 element = 0; element < values.size(); ++element) {
        auto coords_copy = coords;
        MetaTexture meta{
            *sampler, GetRegister(array_register), dc, aoffi, ptp, {}, {}, {}, component, element,
            index_var};
        values[element] = Operation(OperationCode::TextureGather, meta, std::move(coords_copy));
    }

    return values;
}

Node4 ShaderIR::GetTldCode(Tegra::Shader::Instruction instr) {
    const auto texture_type{instr.tld.texture_type};
    const bool is_array{instr.tld.is_array != 0};
    const bool lod_enabled{instr.tld.GetTextureProcessMode() == TextureProcessMode::LL};
    const std::size_t coord_count{GetCoordCount(texture_type)};

    u64 gpr8_cursor{instr.gpr8.Value()};
    const Node array_register{is_array ? GetRegister(gpr8_cursor++) : nullptr};

    std::vector<Node> coords;
    coords.reserve(coord_count);
    for (std::size_t i = 0; i < coord_count; ++i) {
        coords.push_back(GetRegister(gpr8_cursor++));
    }

    u64 gpr20_cursor{instr.gpr20.Value()};
    // const Node bindless_register{is_bindless ? GetRegister(gpr20_cursor++) : nullptr};
    const Node lod{lod_enabled ? GetRegister(gpr20_cursor++) : Immediate(0u)};
    // const Node aoffi_register{is_aoffi ? GetRegister(gpr20_cursor++) : nullptr};
    // const Node multisample{is_multisample ? GetRegister(gpr20_cursor++) : nullptr};

    const std::optional<SamplerEntry> sampler = GetSampler(instr.sampler, {});

    Node4 values;
    for (u32 element = 0; element < values.size(); ++element) {
        auto coords_copy = coords;
        MetaTexture meta{*sampler, array_register, {}, {}, {}, {}, {}, lod, {}, element, {}};
        values[element] = Operation(OperationCode::TexelFetch, meta, std::move(coords_copy));
    }

    return values;
}

Node4 ShaderIR::GetTldsCode(Instruction instr, TextureType texture_type, bool is_array) {
    SamplerInfo info;
    info.type = texture_type;
    info.is_array = is_array;
    info.is_shadow = false;
    const std::optional<SamplerEntry> sampler = GetSampler(instr.sampler, info);

    const std::size_t type_coord_count = GetCoordCount(texture_type);
    const bool lod_enabled = instr.tlds.GetTextureProcessMode() == TextureProcessMode::LL;
    const bool aoffi_enabled = instr.tlds.UsesMiscMode(TextureMiscMode::AOFFI);

    // If enabled arrays index is always stored in the gpr8 field
    const u64 array_register = instr.gpr8.Value();
    // if is array gpr20 is used
    const u64 coord_register = is_array ? instr.gpr20.Value() : instr.gpr8.Value();

    const u64 last_coord_register =
        ((type_coord_count > 2) || (type_coord_count == 2 && !lod_enabled)) && !is_array
            ? static_cast<u64>(instr.gpr20.Value())
            : coord_register + 1;

    std::vector<Node> coords;
    for (std::size_t i = 0; i < type_coord_count; ++i) {
        const bool last = (i == (type_coord_count - 1)) && (type_coord_count > 1);
        coords.push_back(
            GetRegister(last && !aoffi_enabled ? last_coord_register : coord_register + i));
    }

    const Node array = is_array ? GetRegister(array_register) : nullptr;
    // When lod is used always is in gpr20
    const Node lod = lod_enabled ? GetRegister(instr.gpr20) : Immediate(0);

    std::vector<Node> aoffi;
    if (aoffi_enabled) {
        aoffi = GetAoffiCoordinates(GetRegister(instr.gpr20), type_coord_count, false);
    }

    Node4 values;
    for (u32 element = 0; element < values.size(); ++element) {
        auto coords_copy = coords;
        MetaTexture meta{*sampler, array, {}, aoffi, {}, {}, {}, lod, {}, element, {}};
        values[element] = Operation(OperationCode::TexelFetch, meta, std::move(coords_copy));
    }
    return values;
}

std::tuple<std::size_t, std::size_t> ShaderIR::ValidateAndGetCoordinateElement(
    TextureType texture_type, bool depth_compare, bool is_array, bool lod_bias_enabled,
    std::size_t max_coords, std::size_t max_inputs) {
    const std::size_t coord_count = GetCoordCount(texture_type);

    std::size_t total_coord_count = coord_count + (is_array ? 1 : 0) + (depth_compare ? 1 : 0);
    const std::size_t total_reg_count = total_coord_count + (lod_bias_enabled ? 1 : 0);
    if (total_coord_count > max_coords || total_reg_count > max_inputs) {
        UNIMPLEMENTED_MSG("Unsupported Texture operation");
        total_coord_count = std::min(total_coord_count, max_coords);
    }
    // 1D.DC OpenGL is using a vec3 but 2nd component is ignored later.
    total_coord_count +=
        (depth_compare && !is_array && texture_type == TextureType::Texture1D) ? 1 : 0;

    return {coord_count, total_coord_count};
}

std::vector<Node> ShaderIR::GetAoffiCoordinates(Node aoffi_reg, std::size_t coord_count,
                                                bool is_tld4) {
    const std::array coord_offsets = is_tld4 ? std::array{0U, 8U, 16U} : std::array{0U, 4U, 8U};
    const u32 size = is_tld4 ? 6 : 4;
    const s32 wrap_value = is_tld4 ? 32 : 8;
    const s32 diff_value = is_tld4 ? 64 : 16;
    const u32 mask = (1U << size) - 1;

    std::vector<Node> aoffi;
    aoffi.reserve(coord_count);

    const auto aoffi_immediate{
        TrackImmediate(aoffi_reg, global_code, static_cast<s64>(global_code.size()))};
    if (!aoffi_immediate) {
        // Variable access, not supported on AMD.
        LOG_WARNING(HW_GPU,
                    "AOFFI constant folding failed, some hardware might have graphical issues");
        for (std::size_t coord = 0; coord < coord_count; ++coord) {
            const Node value = BitfieldExtract(aoffi_reg, coord_offsets[coord], size);
            const Node condition =
                Operation(OperationCode::LogicalIGreaterEqual, value, Immediate(wrap_value));
            const Node negative = Operation(OperationCode::IAdd, value, Immediate(-diff_value));
            aoffi.push_back(Operation(OperationCode::Select, condition, negative, value));
        }
        return aoffi;
    }

    for (std::size_t coord = 0; coord < coord_count; ++coord) {
        s32 value = (*aoffi_immediate >> coord_offsets[coord]) & mask;
        if (value >= wrap_value) {
            value -= diff_value;
        }
        aoffi.push_back(Immediate(value));
    }
    return aoffi;
}

std::vector<Node> ShaderIR::GetPtpCoordinates(std::array<Node, 2> ptp_regs) {
    static constexpr u32 num_entries = 8;

    std::vector<Node> ptp;
    ptp.reserve(num_entries);

    const auto global_size = static_cast<s64>(global_code.size());
    const std::optional low = TrackImmediate(ptp_regs[0], global_code, global_size);
    const std::optional high = TrackImmediate(ptp_regs[1], global_code, global_size);
    if (!low || !high) {
        for (u32 entry = 0; entry < num_entries; ++entry) {
            const u32 reg = entry / 4;
            const u32 offset = entry % 4;
            const Node value = BitfieldExtract(ptp_regs[reg], offset * 8, 6);
            const Node condition =
                Operation(OperationCode::LogicalIGreaterEqual, value, Immediate(32));
            const Node negative = Operation(OperationCode::IAdd, value, Immediate(-64));
            ptp.push_back(Operation(OperationCode::Select, condition, negative, value));
        }
        return ptp;
    }

    const u64 immediate = (static_cast<u64>(*high) << 32) | static_cast<u64>(*low);
    for (u32 entry = 0; entry < num_entries; ++entry) {
        s32 value = (immediate >> (entry * 8)) & 0b111111;
        if (value >= 32) {
            value -= 64;
        }
        ptp.push_back(Immediate(value));
    }

    return ptp;
}

} // namespace VideoCommon::Shader

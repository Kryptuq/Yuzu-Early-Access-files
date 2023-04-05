// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/container/static_vector.hpp>

#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/backend/spirv/emit_spirv_instructions.h"
#include "shader_recompiler/backend/spirv/spirv_emit_context.h"
#include "shader_recompiler/frontend/ir/modifiers.h"

namespace Shader::Backend::SPIRV {
namespace {
class ImageOperands {
public:
    explicit ImageOperands(EmitContext& ctx, bool has_bias, bool has_lod, bool has_lod_clamp,
                           Id lod, const IR::Value& offset) {
        if (has_bias) {
            const Id bias{has_lod_clamp ? ctx.OpCompositeExtract(ctx.F32[1], lod, 0) : lod};
            Add(spv::ImageOperandsMask::Bias, bias);
        }
        if (has_lod) {
            const Id lod_value{has_lod_clamp ? ctx.OpCompositeExtract(ctx.F32[1], lod, 0) : lod};
            Add(spv::ImageOperandsMask::Lod, lod_value);
        }
        AddOffset(ctx, offset);
        if (has_lod_clamp) {
            const Id lod_clamp{has_bias ? ctx.OpCompositeExtract(ctx.F32[1], lod, 1) : lod};
            Add(spv::ImageOperandsMask::MinLod, lod_clamp);
        }
    }

    explicit ImageOperands(EmitContext& ctx, const IR::Value& offset, const IR::Value& offset2) {
        if (offset2.IsEmpty()) {
            if (offset.IsEmpty()) {
                return;
            }
            Add(spv::ImageOperandsMask::Offset, ctx.Def(offset));
            return;
        }
        const std::array values{offset.InstRecursive(), offset2.InstRecursive()};
        if (!values[0]->AreAllArgsImmediates() || !values[1]->AreAllArgsImmediates()) {
            LOG_WARNING(Shader_SPIRV, "Not all arguments in PTP are immediate, ignoring");
            return;
        }
        const IR::Opcode opcode{values[0]->GetOpcode()};
        if (opcode != values[1]->GetOpcode() || opcode != IR::Opcode::CompositeConstructU32x4) {
            throw LogicError("Invalid PTP arguments");
        }
        auto read{[&](unsigned int a, unsigned int b) { return values[a]->Arg(b).U32(); }};

        const Id offsets{ctx.ConstantComposite(
            ctx.TypeArray(ctx.U32[2], ctx.Const(4U)), ctx.Const(read(0, 0), read(0, 1)),
            ctx.Const(read(0, 2), read(0, 3)), ctx.Const(read(1, 0), read(1, 1)),
            ctx.Const(read(1, 2), read(1, 3)))};
        Add(spv::ImageOperandsMask::ConstOffsets, offsets);
    }

    explicit ImageOperands(Id offset, Id lod, Id ms) {
        if (Sirit::ValidId(lod)) {
            Add(spv::ImageOperandsMask::Lod, lod);
        }
        if (Sirit::ValidId(offset)) {
            Add(spv::ImageOperandsMask::Offset, offset);
        }
        if (Sirit::ValidId(ms)) {
            Add(spv::ImageOperandsMask::Sample, ms);
        }
    }

    explicit ImageOperands(EmitContext& ctx, bool has_lod_clamp, Id derivates, u32 num_derivates,
                           Id offset, Id lod_clamp) {
        if (!Sirit::ValidId(derivates)) {
            throw LogicError("Derivates must be present");
        }
        boost::container::static_vector<Id, 3> deriv_x_accum;
        boost::container::static_vector<Id, 3> deriv_y_accum;
        for (u32 i = 0; i < num_derivates; ++i) {
            deriv_x_accum.push_back(ctx.OpCompositeExtract(ctx.F32[1], derivates, i * 2));
            deriv_y_accum.push_back(ctx.OpCompositeExtract(ctx.F32[1], derivates, i * 2 + 1));
        }
        const Id derivates_X{ctx.OpCompositeConstruct(
            ctx.F32[num_derivates], std::span{deriv_x_accum.data(), deriv_x_accum.size()})};
        const Id derivates_Y{ctx.OpCompositeConstruct(
            ctx.F32[num_derivates], std::span{deriv_y_accum.data(), deriv_y_accum.size()})};
        Add(spv::ImageOperandsMask::Grad, derivates_X, derivates_Y);
        if (Sirit::ValidId(offset)) {
            Add(spv::ImageOperandsMask::Offset, offset);
        }
        if (has_lod_clamp) {
            Add(spv::ImageOperandsMask::MinLod, lod_clamp);
        }
    }

    std::span<const Id> Span() const noexcept {
        return std::span{operands.data(), operands.size()};
    }

    std::optional<spv::ImageOperandsMask> MaskOptional() const noexcept {
        return mask != spv::ImageOperandsMask{} ? std::make_optional(mask) : std::nullopt;
    }

    spv::ImageOperandsMask Mask() const noexcept {
        return mask;
    }

private:
    void AddOffset(EmitContext& ctx, const IR::Value& offset) {
        if (offset.IsEmpty()) {
            return;
        }
        if (offset.IsImmediate()) {
            Add(spv::ImageOperandsMask::ConstOffset, ctx.SConst(static_cast<s32>(offset.U32())));
            return;
        }
        IR::Inst* const inst{offset.InstRecursive()};
        if (inst->AreAllArgsImmediates()) {
            switch (inst->GetOpcode()) {
            case IR::Opcode::CompositeConstructU32x2:
                Add(spv::ImageOperandsMask::ConstOffset,
                    ctx.SConst(static_cast<s32>(inst->Arg(0).U32()),
                               static_cast<s32>(inst->Arg(1).U32())));
                return;
            case IR::Opcode::CompositeConstructU32x3:
                Add(spv::ImageOperandsMask::ConstOffset,
                    ctx.SConst(static_cast<s32>(inst->Arg(0).U32()),
                               static_cast<s32>(inst->Arg(1).U32()),
                               static_cast<s32>(inst->Arg(2).U32())));
                return;
            case IR::Opcode::CompositeConstructU32x4:
                Add(spv::ImageOperandsMask::ConstOffset,
                    ctx.SConst(static_cast<s32>(inst->Arg(0).U32()),
                               static_cast<s32>(inst->Arg(1).U32()),
                               static_cast<s32>(inst->Arg(2).U32()),
                               static_cast<s32>(inst->Arg(3).U32())));
                return;
            default:
                break;
            }
        }
        Add(spv::ImageOperandsMask::Offset, ctx.Def(offset));
    }

    void Add(spv::ImageOperandsMask new_mask, Id value) {
        mask = static_cast<spv::ImageOperandsMask>(static_cast<unsigned>(mask) |
                                                   static_cast<unsigned>(new_mask));
        operands.push_back(value);
    }

    void Add(spv::ImageOperandsMask new_mask, Id value_1, Id value_2) {
        mask = static_cast<spv::ImageOperandsMask>(static_cast<unsigned>(mask) |
                                                   static_cast<unsigned>(new_mask));
        operands.push_back(value_1);
        operands.push_back(value_2);
    }

    boost::container::static_vector<Id, 4> operands;
    spv::ImageOperandsMask mask{};
};

Id Texture(EmitContext& ctx, IR::TextureInstInfo info, [[maybe_unused]] const IR::Value& index) {
    const TextureDefinition& def{ctx.textures.at(info.descriptor_index)};
    if (def.count > 1) {
        const Id pointer{ctx.OpAccessChain(def.pointer_type, def.id, ctx.Def(index))};
        return ctx.OpLoad(def.sampled_type, pointer);
    } else {
        return ctx.OpLoad(def.sampled_type, def.id);
    }
}

Id TextureImage(EmitContext& ctx, IR::TextureInstInfo info, const IR::Value& index) {
    if (!index.IsImmediate() || index.U32() != 0) {
        throw NotImplementedException("Indirect image indexing");
    }
    if (info.type == TextureType::Buffer) {
        const TextureBufferDefinition& def{ctx.texture_buffers.at(info.descriptor_index)};
        if (def.count > 1) {
            throw NotImplementedException("Indirect texture sample");
        }
        const Id sampler_id{def.id};
        const Id id{ctx.OpLoad(ctx.sampled_texture_buffer_type, sampler_id)};
        return ctx.OpImage(ctx.image_buffer_type, id);
    } else {
        const TextureDefinition& def{ctx.textures.at(info.descriptor_index)};
        if (def.count > 1) {
            throw NotImplementedException("Indirect texture sample");
        }
        return ctx.OpImage(def.image_type, ctx.OpLoad(def.sampled_type, def.id));
    }
}

Id Image(EmitContext& ctx, const IR::Value& index, IR::TextureInstInfo info) {
    if (!index.IsImmediate() || index.U32() != 0) {
        throw NotImplementedException("Indirect image indexing");
    }
    if (info.type == TextureType::Buffer) {
        const ImageBufferDefinition def{ctx.image_buffers.at(info.descriptor_index)};
        return ctx.OpLoad(def.image_type, def.id);
    } else {
        const ImageDefinition def{ctx.images.at(info.descriptor_index)};
        return ctx.OpLoad(def.image_type, def.id);
    }
}

bool IsTextureMsaa(EmitContext& ctx, const IR::TextureInstInfo& info) {
    if (info.type == TextureType::Buffer) {
        return false;
    }
    return ctx.textures.at(info.descriptor_index).is_multisample;
}

Id Decorate(EmitContext& ctx, IR::Inst* inst, Id sample) {
    const auto info{inst->Flags<IR::TextureInstInfo>()};
    if (info.relaxed_precision != 0) {
        ctx.Decorate(sample, spv::Decoration::RelaxedPrecision);
    }
    return sample;
}

template <typename MethodPtrType, typename... Args>
Id Emit(MethodPtrType sparse_ptr, MethodPtrType non_sparse_ptr, EmitContext& ctx, IR::Inst* inst,
        Id result_type, Args&&... args) {
    IR::Inst* const sparse{inst->GetAssociatedPseudoOperation(IR::Opcode::GetSparseFromOp)};
    if (!sparse) {
        return Decorate(ctx, inst, (ctx.*non_sparse_ptr)(result_type, std::forward<Args>(args)...));
    }
    const Id struct_type{ctx.TypeStruct(ctx.U32[1], result_type)};
    const Id sample{(ctx.*sparse_ptr)(struct_type, std::forward<Args>(args)...)};
    const Id resident_code{ctx.OpCompositeExtract(ctx.U32[1], sample, 0U)};
    sparse->SetDefinition(ctx.OpImageSparseTexelsResident(ctx.U1, resident_code));
    sparse->Invalidate();
    Decorate(ctx, inst, sample);
    return ctx.OpCompositeExtract(result_type, sample, 1U);
}

Id IsScaled(EmitContext& ctx, const IR::Value& index, Id member_index, u32 base_index) {
    const Id push_constant_u32{ctx.TypePointer(spv::StorageClass::PushConstant, ctx.U32[1])};
    Id bit{};
    if (index.IsImmediate()) {
        // Use BitwiseAnd instead of BitfieldExtract for better codegen on Nvidia OpenGL.
        // LOP32I.NZ is used to set the predicate rather than BFE+ISETP.
        const u32 index_value{index.U32() + base_index};
        const Id word_index{ctx.Const(index_value / 32)};
        const Id bit_index_mask{ctx.Const(1u << (index_value % 32))};
        const Id pointer{ctx.OpAccessChain(push_constant_u32, ctx.rescaling_push_constants,
                                           member_index, word_index)};
        const Id word{ctx.OpLoad(ctx.U32[1], pointer)};
        bit = ctx.OpBitwiseAnd(ctx.U32[1], word, bit_index_mask);
    } else {
        Id index_value{ctx.Def(index)};
        if (base_index != 0) {
            index_value = ctx.OpIAdd(ctx.U32[1], index_value, ctx.Const(base_index));
        }
        const Id bit_index{ctx.OpBitwiseAnd(ctx.U32[1], index_value, ctx.Const(31u))};
        bit = ctx.OpBitFieldUExtract(ctx.U32[1], index_value, bit_index, ctx.Const(1u));
    }
    return ctx.OpINotEqual(ctx.U1, bit, ctx.u32_zero_value);
}

Id BitTest(EmitContext& ctx, Id mask, Id bit) {
    const Id shifted{ctx.OpShiftRightLogical(ctx.U32[1], mask, bit)};
    const Id bit_value{ctx.OpBitwiseAnd(ctx.U32[1], shifted, ctx.Const(1u))};
    return ctx.OpINotEqual(ctx.U1, bit_value, ctx.u32_zero_value);
}

Id ImageGatherSubpixelOffset(EmitContext& ctx, const IR::TextureInstInfo& info, Id texture,
                             Id coords) {
    // Apply a subpixel offset of 1/512 the texel size of the texture to ensure same rounding on
    // AMD hardware as on Maxwell or other Nvidia architectures.
    const auto calculate_offset{[&](size_t dim) -> std::array<Id, 2> {
        const Id nudge{ctx.Const(0x1p-9f)};
        const Id image_size{ctx.OpImageQuerySizeLod(ctx.U32[dim], texture, ctx.u32_zero_value)};
        const Id offset_x{ctx.OpFDiv(
            ctx.F32[1], nudge,
            ctx.OpConvertUToF(ctx.F32[1], ctx.OpCompositeExtract(ctx.U32[1], image_size, 0)))};
        const Id offset_y{ctx.OpFDiv(
            ctx.F32[1], nudge,
            ctx.OpConvertUToF(ctx.F32[1], ctx.OpCompositeExtract(ctx.U32[1], image_size, 1)))};
        return {ctx.OpFAdd(ctx.F32[1], ctx.OpCompositeExtract(ctx.F32[1], coords, 0), offset_x),
                ctx.OpFAdd(ctx.F32[1], ctx.OpCompositeExtract(ctx.F32[1], coords, 1), offset_y)};
    }};
    switch (info.type) {
    case TextureType::Color2D:
    case TextureType::Color2DRect: {
        const auto offset{calculate_offset(2)};
        return ctx.OpCompositeConstruct(ctx.F32[2], offset[0], offset[1]);
    }
    case TextureType::ColorArray2D:
    case TextureType::ColorCube: {
        const auto offset{calculate_offset(3)};
        return ctx.OpCompositeConstruct(ctx.F32[3], offset[0], offset[1],
                                        ctx.OpCompositeExtract(ctx.F32[1], coords, 2));
    }
    default:
        return coords;
    }
}
} // Anonymous namespace

Id EmitBindlessImageSampleImplicitLod(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBindlessImageSampleExplicitLod(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBindlessImageSampleDrefImplicitLod(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBindlessImageSampleDrefExplicitLod(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBindlessImageGather(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBindlessImageGatherDref(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBindlessImageFetch(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBindlessImageQueryDimensions(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBindlessImageQueryLod(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBindlessImageGradient(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBindlessImageRead(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBindlessImageWrite(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBoundImageSampleImplicitLod(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBoundImageSampleExplicitLod(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBoundImageSampleDrefImplicitLod(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBoundImageSampleDrefExplicitLod(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBoundImageGather(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBoundImageGatherDref(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBoundImageFetch(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBoundImageQueryDimensions(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBoundImageQueryLod(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBoundImageGradient(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBoundImageRead(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitBoundImageWrite(EmitContext&) {
    throw LogicError("Unreachable instruction");
}

Id EmitImageSampleImplicitLod(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords,
                              Id bias_lc, const IR::Value& offset) {
    const auto info{inst->Flags<IR::TextureInstInfo>()};
    if (ctx.stage == Stage::Fragment) {
        const ImageOperands operands(ctx, info.has_bias != 0, false, info.has_lod_clamp != 0,
                                     bias_lc, offset);
        return Emit(&EmitContext::OpImageSparseSampleImplicitLod,
                    &EmitContext::OpImageSampleImplicitLod, ctx, inst, ctx.F32[4],
                    Texture(ctx, info, index), coords, operands.MaskOptional(), operands.Span());
    } else {
        // We can't use implicit lods on non-fragment stages on SPIR-V. Maxwell hardware behaves as
        // if the lod was explicitly zero.  This may change on Turing with implicit compute
        // derivatives
        const Id lod{ctx.Const(0.0f)};
        const ImageOperands operands(ctx, false, true, info.has_lod_clamp != 0, lod, offset);
        return Emit(&EmitContext::OpImageSparseSampleExplicitLod,
                    &EmitContext::OpImageSampleExplicitLod, ctx, inst, ctx.F32[4],
                    Texture(ctx, info, index), coords, operands.Mask(), operands.Span());
    }
}

Id EmitImageSampleExplicitLod(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords,
                              Id lod, const IR::Value& offset) {
    const auto info{inst->Flags<IR::TextureInstInfo>()};
    const ImageOperands operands(ctx, false, true, false, lod, offset);
    return Emit(&EmitContext::OpImageSparseSampleExplicitLod,
                &EmitContext::OpImageSampleExplicitLod, ctx, inst, ctx.F32[4],
                Texture(ctx, info, index), coords, operands.Mask(), operands.Span());
}

Id EmitImageSampleDrefImplicitLod(EmitContext& ctx, IR::Inst* inst, const IR::Value& index,
                                  Id coords, Id dref, Id bias_lc, const IR::Value& offset) {
    const auto info{inst->Flags<IR::TextureInstInfo>()};
    if (ctx.stage == Stage::Fragment) {
        const ImageOperands operands(ctx, info.has_bias != 0, false, info.has_lod_clamp != 0,
                                     bias_lc, offset);
        return Emit(&EmitContext::OpImageSparseSampleDrefImplicitLod,
                    &EmitContext::OpImageSampleDrefImplicitLod, ctx, inst, ctx.F32[1],
                    Texture(ctx, info, index), coords, dref, operands.MaskOptional(),
                    operands.Span());
    } else {
        // Implicit lods in compute behave on hardware as if sampling from LOD 0.
        // This check is to ensure all drivers behave this way.
        const Id lod{ctx.Const(0.0f)};
        const ImageOperands operands(ctx, false, true, false, lod, offset);
        return Emit(&EmitContext::OpImageSparseSampleDrefExplicitLod,
                    &EmitContext::OpImageSampleDrefExplicitLod, ctx, inst, ctx.F32[1],
                    Texture(ctx, info, index), coords, dref, operands.Mask(), operands.Span());
    }
}

Id EmitImageSampleDrefExplicitLod(EmitContext& ctx, IR::Inst* inst, const IR::Value& index,
                                  Id coords, Id dref, Id lod, const IR::Value& offset) {
    const auto info{inst->Flags<IR::TextureInstInfo>()};
    const ImageOperands operands(ctx, false, true, false, lod, offset);
    return Emit(&EmitContext::OpImageSparseSampleDrefExplicitLod,
                &EmitContext::OpImageSampleDrefExplicitLod, ctx, inst, ctx.F32[1],
                Texture(ctx, info, index), coords, dref, operands.Mask(), operands.Span());
}

Id EmitImageGather(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords,
                   const IR::Value& offset, const IR::Value& offset2) {
    const auto info{inst->Flags<IR::TextureInstInfo>()};
    const ImageOperands operands(ctx, offset, offset2);
    if (ctx.profile.need_gather_subpixel_offset) {
        coords = ImageGatherSubpixelOffset(ctx, info, TextureImage(ctx, info, index), coords);
    }
    return Emit(&EmitContext::OpImageSparseGather, &EmitContext::OpImageGather, ctx, inst,
                ctx.F32[4], Texture(ctx, info, index), coords, ctx.Const(info.gather_component),
                operands.MaskOptional(), operands.Span());
}

Id EmitImageGatherDref(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords,
                       const IR::Value& offset, const IR::Value& offset2, Id dref) {
    const auto info{inst->Flags<IR::TextureInstInfo>()};
    const ImageOperands operands(ctx, offset, offset2);
    if (ctx.profile.need_gather_subpixel_offset) {
        coords = ImageGatherSubpixelOffset(ctx, info, TextureImage(ctx, info, index), coords);
    }
    return Emit(&EmitContext::OpImageSparseDrefGather, &EmitContext::OpImageDrefGather, ctx, inst,
                ctx.F32[4], Texture(ctx, info, index), coords, dref, operands.MaskOptional(),
                operands.Span());
}

Id EmitImageFetch(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords, Id offset,
                  Id lod, Id ms) {
    const auto info{inst->Flags<IR::TextureInstInfo>()};
    if (info.type == TextureType::Buffer) {
        lod = Id{};
    }
    if (Sirit::ValidId(ms)) {
        // This image is multisampled, lod must be implicit
        lod = Id{};
    }
    const ImageOperands operands(offset, lod, ms);
    return Emit(&EmitContext::OpImageSparseFetch, &EmitContext::OpImageFetch, ctx, inst, ctx.F32[4],
                TextureImage(ctx, info, index), coords, operands.MaskOptional(), operands.Span());
}

Id EmitImageQueryDimensions(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id lod,
                            const IR::Value& skip_mips_val) {
    const auto info{inst->Flags<IR::TextureInstInfo>()};
    const Id image{TextureImage(ctx, info, index)};
    const Id zero{ctx.u32_zero_value};
    const bool skip_mips{skip_mips_val.U1()};
    const auto mips{[&] { return skip_mips ? zero : ctx.OpImageQueryLevels(ctx.U32[1], image); }};
    const bool is_msaa{IsTextureMsaa(ctx, info)};
    const bool uses_lod{!is_msaa && info.type != TextureType::Buffer};
    const auto query{[&](Id type) {
        return uses_lod ? ctx.OpImageQuerySizeLod(type, image, lod)
                        : ctx.OpImageQuerySize(type, image);
    }};
    switch (info.type) {
    case TextureType::Color1D:
        return ctx.OpCompositeConstruct(ctx.U32[4], query(ctx.U32[1]), zero, zero, mips());
    case TextureType::ColorArray1D:
    case TextureType::Color2D:
    case TextureType::ColorCube:
    case TextureType::Color2DRect:
        return ctx.OpCompositeConstruct(ctx.U32[4], query(ctx.U32[2]), zero, mips());
    case TextureType::ColorArray2D:
    case TextureType::Color3D:
    case TextureType::ColorArrayCube:
        return ctx.OpCompositeConstruct(ctx.U32[4], query(ctx.U32[3]), mips());
    case TextureType::Buffer:
        return ctx.OpCompositeConstruct(ctx.U32[4], query(ctx.U32[1]), zero, zero, mips());
    }
    throw LogicError("Unspecified image type {}", info.type.Value());
}

Id EmitImageQueryLod(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords) {
    const auto info{inst->Flags<IR::TextureInstInfo>()};
    const Id zero{ctx.f32_zero_value};
    const Id sampler{Texture(ctx, info, index)};
    return ctx.OpCompositeConstruct(ctx.F32[4], ctx.OpImageQueryLod(ctx.F32[2], sampler, coords),
                                    zero, zero);
}

Id EmitImageGradient(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords,
                     Id derivates, Id offset, Id lod_clamp) {
    const auto info{inst->Flags<IR::TextureInstInfo>()};
    const ImageOperands operands(ctx, info.has_lod_clamp != 0, derivates, info.num_derivates,
                                 offset, lod_clamp);
    return Emit(&EmitContext::OpImageSparseSampleExplicitLod,
                &EmitContext::OpImageSampleExplicitLod, ctx, inst, ctx.F32[4],
                Texture(ctx, info, index), coords, operands.Mask(), operands.Span());
}

Id EmitImageRead(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords) {
    const auto info{inst->Flags<IR::TextureInstInfo>()};
    if (info.image_format == ImageFormat::Typeless && !ctx.profile.support_typeless_image_loads) {
        LOG_WARNING(Shader_SPIRV, "Typeless image read not supported by host");
        return ctx.ConstantNull(ctx.U32[4]);
    }
    return Emit(&EmitContext::OpImageSparseRead, &EmitContext::OpImageRead, ctx, inst, ctx.U32[4],
                Image(ctx, index, info), coords, std::nullopt, std::span<const Id>{});
}

void EmitImageWrite(EmitContext& ctx, IR::Inst* inst, const IR::Value& index, Id coords, Id color) {
    const auto info{inst->Flags<IR::TextureInstInfo>()};
    ctx.OpImageWrite(Image(ctx, index, info), coords, color);
}

Id EmitIsTextureScaled(EmitContext& ctx, const IR::Value& index) {
    if (ctx.profile.unified_descriptor_binding) {
        const Id member_index{ctx.Const(ctx.rescaling_textures_member_index)};
        return IsScaled(ctx, index, member_index, ctx.texture_rescaling_index);
    } else {
        const Id composite{ctx.OpLoad(ctx.F32[4], ctx.rescaling_uniform_constant)};
        const Id mask_f32{ctx.OpCompositeExtract(ctx.F32[1], composite, 0u)};
        const Id mask{ctx.OpBitcast(ctx.U32[1], mask_f32)};
        return BitTest(ctx, mask, ctx.Def(index));
    }
}

Id EmitIsImageScaled(EmitContext& ctx, const IR::Value& index) {
    if (ctx.profile.unified_descriptor_binding) {
        const Id member_index{ctx.Const(ctx.rescaling_images_member_index)};
        return IsScaled(ctx, index, member_index, ctx.image_rescaling_index);
    } else {
        const Id composite{ctx.OpLoad(ctx.F32[4], ctx.rescaling_uniform_constant)};
        const Id mask_f32{ctx.OpCompositeExtract(ctx.F32[1], composite, 1u)};
        const Id mask{ctx.OpBitcast(ctx.U32[1], mask_f32)};
        return BitTest(ctx, mask, ctx.Def(index));
    }
}

} // namespace Shader::Backend::SPIRV

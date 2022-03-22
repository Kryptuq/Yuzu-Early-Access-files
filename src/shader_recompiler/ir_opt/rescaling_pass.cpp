// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/settings.h"
#include "shader_recompiler/environment.h"
#include "shader_recompiler/frontend/ir/ir_emitter.h"
#include "shader_recompiler/frontend/ir/modifiers.h"
#include "shader_recompiler/frontend/ir/program.h"
#include "shader_recompiler/frontend/ir/value.h"
#include "shader_recompiler/ir_opt/passes.h"
#include "shader_recompiler/shader_info.h"

namespace Shader::Optimization {
namespace {
[[nodiscard]] bool IsTextureTypeRescalable(TextureType type) {
    switch (type) {
    case TextureType::Color2D:
    case TextureType::ColorArray2D:
        return true;
    case TextureType::Color1D:
    case TextureType::ColorArray1D:
    case TextureType::Color3D:
    case TextureType::ColorCube:
    case TextureType::ColorArrayCube:
    case TextureType::Buffer:
        break;
    }
    return false;
}

void VisitMark(IR::Block& block, IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::ShuffleIndex:
    case IR::Opcode::ShuffleUp:
    case IR::Opcode::ShuffleDown:
    case IR::Opcode::ShuffleButterfly: {
        const IR::Value shfl_arg{inst.Arg(0)};
        if (shfl_arg.IsImmediate()) {
            break;
        }
        const IR::Inst* const arg_inst{shfl_arg.InstRecursive()};
        if (arg_inst->GetOpcode() != IR::Opcode::BitCastU32F32) {
            break;
        }
        const IR::Value bitcast_arg{arg_inst->Arg(0)};
        if (bitcast_arg.IsImmediate()) {
            break;
        }
        IR::Inst* const bitcast_inst{bitcast_arg.InstRecursive()};
        bool must_patch_outside = false;
        if (bitcast_inst->GetOpcode() == IR::Opcode::GetAttribute) {
            const IR::Attribute attr{bitcast_inst->Arg(0).Attribute()};
            switch (attr) {
            case IR::Attribute::PositionX:
            case IR::Attribute::PositionY:
                bitcast_inst->SetFlags<u32>(0xDEADBEEF);
                must_patch_outside = true;
                break;
            default:
                break;
            }
        }
        if (must_patch_outside) {
            const auto it{IR::Block::InstructionList::s_iterator_to(inst)};
            IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
            const IR::F32 new_inst{&*block.PrependNewInst(it, inst)};
            const IR::F32 up_factor{ir.FPRecip(ir.ResolutionDownFactor())};
            const IR::Value converted{ir.FPMul(new_inst, up_factor)};
            inst.ReplaceUsesWith(converted);
        }
        break;
    }

    default:
        break;
    }
}

void PatchFragCoord(IR::Block& block, IR::Inst& inst) {
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const IR::F32 down_factor{ir.ResolutionDownFactor()};
    const IR::F32 frag_coord{ir.GetAttribute(inst.Arg(0).Attribute())};
    const IR::F32 downscaled_frag_coord{ir.FPMul(frag_coord, down_factor)};
    inst.ReplaceUsesWith(downscaled_frag_coord);
}

void PatchPointSize(IR::Block& block, IR::Inst& inst) {
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const IR::F32 point_value{inst.Arg(1)};
    const IR::F32 up_factor{ir.FPRecip(ir.ResolutionDownFactor())};
    const IR::F32 upscaled_point_value{ir.FPMul(point_value, up_factor)};
    inst.SetArg(1, upscaled_point_value);
}

[[nodiscard]] IR::U32 Scale(IR::IREmitter& ir, const IR::U1& is_scaled, const IR::U32& value) {
    IR::U32 scaled_value{value};
    if (const u32 up_scale = Settings::values.resolution_info.up_scale; up_scale != 1) {
        scaled_value = ir.IMul(scaled_value, ir.Imm32(up_scale));
    }
    if (const u32 down_shift = Settings::values.resolution_info.down_shift; down_shift != 0) {
        scaled_value = ir.ShiftRightArithmetic(scaled_value, ir.Imm32(down_shift));
    }
    return IR::U32{ir.Select(is_scaled, scaled_value, value)};
}

[[nodiscard]] IR::U32 SubScale(IR::IREmitter& ir, const IR::U1& is_scaled, const IR::U32& value,
                               const IR::Attribute attrib) {
    const IR::F32 up_factor{ir.Imm32(Settings::values.resolution_info.up_factor)};
    const IR::F32 base{ir.FPMul(ir.ConvertUToF(32, 32, value), up_factor)};
    const IR::F32 frag_coord{ir.GetAttribute(attrib)};
    const IR::F32 down_factor{ir.Imm32(Settings::values.resolution_info.down_factor)};
    const IR::F32 floor{ir.FPMul(up_factor, ir.FPFloor(ir.FPMul(frag_coord, down_factor)))};
    const IR::F16F32F64 deviation{ir.FPAdd(base, ir.FPAdd(frag_coord, ir.FPNeg(floor)))};
    return IR::U32{ir.Select(is_scaled, ir.ConvertFToU(32, deviation), value)};
}

[[nodiscard]] IR::U32 DownScale(IR::IREmitter& ir, const IR::U1& is_scaled, const IR::U32& value) {
    IR::U32 scaled_value{value};
    if (const u32 down_shift = Settings::values.resolution_info.down_shift; down_shift != 0) {
        scaled_value = ir.ShiftLeftLogical(scaled_value, ir.Imm32(down_shift));
    }
    if (const u32 up_scale = Settings::values.resolution_info.up_scale; up_scale != 1) {
        scaled_value = ir.IDiv(scaled_value, ir.Imm32(up_scale));
    }
    return IR::U32{ir.Select(is_scaled, scaled_value, value)};
}

void PatchImageQueryDimensions(IR::Block& block, IR::Inst& inst) {
    const auto it{IR::Block::InstructionList::s_iterator_to(inst)};
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const auto info{inst.Flags<IR::TextureInstInfo>()};
    const IR::U1 is_scaled{ir.IsTextureScaled(ir.Imm32(info.descriptor_index))};
    switch (info.type) {
    case TextureType::Color2D:
    case TextureType::ColorArray2D: {
        const IR::Value new_inst{&*block.PrependNewInst(it, inst)};
        const IR::U32 width{DownScale(ir, is_scaled, IR::U32{ir.CompositeExtract(new_inst, 0)})};
        const IR::U32 height{DownScale(ir, is_scaled, IR::U32{ir.CompositeExtract(new_inst, 1)})};
        const IR::Value replacement{ir.CompositeConstruct(
            width, height, ir.CompositeExtract(new_inst, 2), ir.CompositeExtract(new_inst, 3))};
        inst.ReplaceUsesWith(replacement);
        break;
    }
    case TextureType::Color1D:
    case TextureType::ColorArray1D:
    case TextureType::Color3D:
    case TextureType::ColorCube:
    case TextureType::ColorArrayCube:
    case TextureType::Buffer:
        // Nothing to patch here
        break;
    }
}

void ScaleIntegerComposite(IR::IREmitter& ir, IR::Inst& inst, const IR::U1& is_scaled,
                           size_t index) {
    const IR::Value composite{inst.Arg(index)};
    if (composite.IsEmpty()) {
        return;
    }
    const auto info{inst.Flags<IR::TextureInstInfo>()};
    const IR::U32 x{Scale(ir, is_scaled, IR::U32{ir.CompositeExtract(composite, 0)})};
    const IR::U32 y{Scale(ir, is_scaled, IR::U32{ir.CompositeExtract(composite, 1)})};
    switch (info.type) {
    case TextureType::Color2D:
        inst.SetArg(index, ir.CompositeConstruct(x, y));
        break;
    case TextureType::ColorArray2D: {
        const IR::U32 z{ir.CompositeExtract(composite, 2)};
        inst.SetArg(index, ir.CompositeConstruct(x, y, z));
        break;
    }
    case TextureType::Color1D:
    case TextureType::ColorArray1D:
    case TextureType::Color3D:
    case TextureType::ColorCube:
    case TextureType::ColorArrayCube:
    case TextureType::Buffer:
        // Nothing to patch here
        break;
    }
}

void ScaleIntegerOffsetComposite(IR::IREmitter& ir, IR::Inst& inst, const IR::U1& is_scaled,
                                 size_t index) {
    const IR::Value composite{inst.Arg(index)};
    if (composite.IsEmpty()) {
        return;
    }
    const auto info{inst.Flags<IR::TextureInstInfo>()};
    const IR::U32 x{Scale(ir, is_scaled, IR::U32{ir.CompositeExtract(composite, 0)})};
    const IR::U32 y{Scale(ir, is_scaled, IR::U32{ir.CompositeExtract(composite, 1)})};
    switch (info.type) {
    case TextureType::ColorArray2D:
    case TextureType::Color2D:
        inst.SetArg(index, ir.CompositeConstruct(x, y));
        break;
    case TextureType::Color1D:
    case TextureType::ColorArray1D:
    case TextureType::Color3D:
    case TextureType::ColorCube:
    case TextureType::ColorArrayCube:
    case TextureType::Buffer:
        // Nothing to patch here
        break;
    }
}

void SubScaleCoord(IR::IREmitter& ir, IR::Inst& inst, const IR::U1& is_scaled) {
    const auto info{inst.Flags<IR::TextureInstInfo>()};
    const IR::Value coord{inst.Arg(1)};
    const IR::U32 coord_x{ir.CompositeExtract(coord, 0)};
    const IR::U32 coord_y{ir.CompositeExtract(coord, 1)};

    const IR::U32 scaled_x{SubScale(ir, is_scaled, coord_x, IR::Attribute::PositionX)};
    const IR::U32 scaled_y{SubScale(ir, is_scaled, coord_y, IR::Attribute::PositionY)};
    switch (info.type) {
    case TextureType::Color2D:
        inst.SetArg(1, ir.CompositeConstruct(scaled_x, scaled_y));
        break;
    case TextureType::ColorArray2D: {
        const IR::U32 z{ir.CompositeExtract(coord, 2)};
        inst.SetArg(1, ir.CompositeConstruct(scaled_x, scaled_y, z));
        break;
    }
    case TextureType::Color1D:
    case TextureType::ColorArray1D:
    case TextureType::Color3D:
    case TextureType::ColorCube:
    case TextureType::ColorArrayCube:
    case TextureType::Buffer:
        // Nothing to patch here
        break;
    }
}

void SubScaleImageFetch(IR::Block& block, IR::Inst& inst) {
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const auto info{inst.Flags<IR::TextureInstInfo>()};
    if (!IsTextureTypeRescalable(info.type)) {
        return;
    }
    const IR::U1 is_scaled{ir.IsTextureScaled(ir.Imm32(info.descriptor_index))};
    SubScaleCoord(ir, inst, is_scaled);
    // Scale ImageFetch offset
    ScaleIntegerOffsetComposite(ir, inst, is_scaled, 2);
}

void SubScaleImageRead(IR::Block& block, IR::Inst& inst) {
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const auto info{inst.Flags<IR::TextureInstInfo>()};
    if (!IsTextureTypeRescalable(info.type)) {
        return;
    }
    const IR::U1 is_scaled{ir.IsImageScaled(ir.Imm32(info.descriptor_index))};
    SubScaleCoord(ir, inst, is_scaled);
}

void PatchImageFetch(IR::Block& block, IR::Inst& inst) {
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const auto info{inst.Flags<IR::TextureInstInfo>()};
    if (!IsTextureTypeRescalable(info.type)) {
        return;
    }
    const IR::U1 is_scaled{ir.IsTextureScaled(ir.Imm32(info.descriptor_index))};
    ScaleIntegerComposite(ir, inst, is_scaled, 1);
    // Scale ImageFetch offset
    ScaleIntegerOffsetComposite(ir, inst, is_scaled, 2);
}

void PatchImageRead(IR::Block& block, IR::Inst& inst) {
    IR::IREmitter ir{block, IR::Block::InstructionList::s_iterator_to(inst)};
    const auto info{inst.Flags<IR::TextureInstInfo>()};
    if (!IsTextureTypeRescalable(info.type)) {
        return;
    }
    const IR::U1 is_scaled{ir.IsImageScaled(ir.Imm32(info.descriptor_index))};
    ScaleIntegerComposite(ir, inst, is_scaled, 1);
}

void Visit(const IR::Program& program, IR::Block& block, IR::Inst& inst) {
    const bool is_fragment_shader{program.stage == Stage::Fragment};
    switch (inst.GetOpcode()) {
    case IR::Opcode::GetAttribute: {
        const IR::Attribute attr{inst.Arg(0).Attribute()};
        switch (attr) {
        case IR::Attribute::PositionX:
        case IR::Attribute::PositionY:
            if (is_fragment_shader && inst.Flags<u32>() != 0xDEADBEEF) {
                PatchFragCoord(block, inst);
            }
            break;
        default:
            break;
        }
        break;
    }
    case IR::Opcode::SetAttribute: {
        const IR::Attribute attr{inst.Arg(0).Attribute()};
        switch (attr) {
        case IR::Attribute::PointSize:
            if (inst.Flags<u32>() != 0xDEADBEEF) {
                PatchPointSize(block, inst);
            }
            break;
        default:
            break;
        }
        break;
    }
    case IR::Opcode::ImageQueryDimensions:
        PatchImageQueryDimensions(block, inst);
        break;
    case IR::Opcode::ImageFetch:
        if (is_fragment_shader) {
            SubScaleImageFetch(block, inst);
        } else {
            PatchImageFetch(block, inst);
        }
        break;
    case IR::Opcode::ImageRead:
        if (is_fragment_shader) {
            SubScaleImageRead(block, inst);
        } else {
            PatchImageRead(block, inst);
        }
        break;
    default:
        break;
    }
}
} // Anonymous namespace

void RescalingPass(IR::Program& program) {
    const bool is_fragment_shader{program.stage == Stage::Fragment};
    if (is_fragment_shader) {
        for (IR::Block* const block : program.post_order_blocks) {
            for (IR::Inst& inst : block->Instructions()) {
                VisitMark(*block, inst);
            }
        }
    }
    for (IR::Block* const block : program.post_order_blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            Visit(program, *block, inst);
        }
    }
}

} // namespace Shader::Optimization

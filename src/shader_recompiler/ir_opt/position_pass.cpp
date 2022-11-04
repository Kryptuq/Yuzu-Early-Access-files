// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boost/container/small_vector.hpp>

#include "shader_recompiler/frontend/ir/basic_block.h"
#include "shader_recompiler/frontend/ir/ir_emitter.h"
#include "shader_recompiler/frontend/ir/value.h"
#include "shader_recompiler/ir_opt/passes.h"

namespace Shader::Optimization {

namespace {
struct PositionInst {
    IR::Inst* inst;
    IR::Block* block;
    IR::Attribute attr;
};
using PositionInstVector = boost::container::small_vector<PositionInst, 24>;
} // Anonymous namespace

void PositionPass(Environment& env, IR::Program& program) {
    if (env.ShaderStage() != Stage::VertexB || env.ReadViewportTransformState()) {
        return;
    }

    Info& info{program.info};
    info.uses_render_area = true;

    PositionInstVector to_replace;
    for (IR::Block* const block : program.post_order_blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            switch (inst.GetOpcode()) {
            case IR::Opcode::SetAttribute: {
                const IR::Attribute attr{inst.Arg(0).Attribute()};
                switch (attr) {
                case IR::Attribute::PositionX:
                case IR::Attribute::PositionY: {
                    to_replace.push_back(PositionInst{.inst = &inst, .block = block, .attr = attr});
                    break;
                }
                default:
                    break;
                }
                break;
            }
            default:
                break;
            }
        }
    }

    for (PositionInst& position_inst : to_replace) {
        IR::IREmitter ir{*position_inst.block,
                         IR::Block::InstructionList::s_iterator_to(*position_inst.inst)};
        const IR::F32 value(position_inst.inst->Arg(1));
        const IR::F32F64 scale(ir.Imm32(2.f));
        const IR::F32 negative_one{ir.Imm32(-1.f)};
        switch (position_inst.attr) {
        case IR::Attribute::PositionX: {
            position_inst.inst->SetArg(
                1,
                ir.FPFma(value, ir.FPMul(ir.FPRecip(ir.RenderAreaWidth()), scale), negative_one));
            break;
        }
        case IR::Attribute::PositionY: {
            position_inst.inst->SetArg(
                1,
                ir.FPFma(value, ir.FPMul(ir.FPRecip(ir.RenderAreaHeight()), scale), negative_one));
            break;
        }
        default:
            break;
        }
    }
}
} // namespace Shader::Optimization

// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/ir/ir_emitter.h"
#include "shader_recompiler/ir_opt/passes.h"

namespace Shader::Optimization {

void VertexATransformPass(IR::Program& program) {
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::Epilogue) {
                return inst.Invalidate();
            }
        }
    }
}

void VertexBTransformPass(IR::Program& program) {
    for (IR::Block* const block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::Prologue) {
                return inst.Invalidate();
            }
        }
    }
}

} // namespace Shader::Optimization

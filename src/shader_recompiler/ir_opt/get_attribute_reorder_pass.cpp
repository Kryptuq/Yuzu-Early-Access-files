// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "shader_recompiler/frontend/ir/basic_block.h"
#include "shader_recompiler/frontend/ir/value.h"
#include "shader_recompiler/ir_opt/passes.h"

namespace Shader::Optimization {

static bool CanBeReordered(const IR::Inst& inst) {
    switch (inst.GetOpcode()) {
    case IR::Opcode::GetAttribute:
        return inst.AreAllArgsImmediates();
    default:
        return false;
    }
}

void GetAttributeReorderPass(IR::Program& program) {
    auto& first_block_list{program.blocks[0]->Instructions()};
    auto& prologue{first_block_list.front()};
    first_block_list.pop_front();
    for (IR::Block* const block : program.blocks) {
        auto& instrucions{block->Instructions()};
        auto it = instrucions.begin();
        while (it != instrucions.end()) {
            if (!CanBeReordered(*it)) {
                ++it;
                continue;
            }
            auto& removed_val{*it};
            it = instrucions.erase(it);
            first_block_list.push_front(removed_val);
        }
    }
    first_block_list.push_front(prologue);
}

} // namespace Shader::Optimization

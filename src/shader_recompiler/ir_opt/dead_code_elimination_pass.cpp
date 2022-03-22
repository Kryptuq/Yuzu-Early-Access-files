// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "shader_recompiler/frontend/ir/basic_block.h"
#include "shader_recompiler/frontend/ir/value.h"
#include "shader_recompiler/ir_opt/passes.h"

namespace Shader::Optimization {
namespace {
template <bool TEST_USES>
void DeadInstElimination(IR::Block* const block) {
    // We iterate over the instructions in reverse order.
    // This is because removing an instruction reduces the number of uses for earlier instructions.
    auto it{block->end()};
    while (it != block->begin()) {
        --it;
        if constexpr (TEST_USES) {
            if (it->HasUses() || it->MayHaveSideEffects()) {
                continue;
            }
        }
        it->Invalidate();
        it = block->Instructions().erase(it);
    }
}

void DeadBranchElimination(IR::Program& program) {
    const auto begin_it{program.syntax_list.begin()};
    for (auto node_it = begin_it; node_it != program.syntax_list.end(); ++node_it) {
        if (node_it->type != IR::AbstractSyntaxNode::Type::If) {
            continue;
        }
        IR::Inst* const cond_ref{node_it->data.if_node.cond.Inst()};
        const IR::U1 cond{cond_ref->Arg(0)};
        if (!cond.IsImmediate()) {
            continue;
        }
        if (cond.U1()) {
            continue;
        }
        // False immediate condition. Remove condition ref, erase the entire branch.
        cond_ref->Invalidate();
        // Account for nested if-statements within the if(false) branch
        u32 nested_ifs{1u};
        while (node_it->type != IR::AbstractSyntaxNode::Type::EndIf || nested_ifs > 0) {
            node_it = program.syntax_list.erase(node_it);
            switch (node_it->type) {
            case IR::AbstractSyntaxNode::Type::If:
                ++nested_ifs;
                break;
            case IR::AbstractSyntaxNode::Type::EndIf:
                --nested_ifs;
                break;
            case IR::AbstractSyntaxNode::Type::Block: {
                IR::Block* const block{node_it->data.block};
                DeadInstElimination<false>(block);
                break;
            }
            default:
                break;
            }
        }
        // Erase EndIf node of the if(false) branch
        node_it = program.syntax_list.erase(node_it);
    }
}
} // namespace

void DeadCodeEliminationPass(IR::Program& program) {
    DeadBranchElimination(program);
    for (IR::Block* const block : program.post_order_blocks) {
        DeadInstElimination<true>(block);
    }
}

} // namespace Shader::Optimization

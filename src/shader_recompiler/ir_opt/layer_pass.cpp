// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <bit>
#include <optional>

#include <boost/container/small_vector.hpp>

#include "shader_recompiler/environment.h"
#include "shader_recompiler/frontend/ir/basic_block.h"
#include "shader_recompiler/frontend/ir/breadth_first_search.h"
#include "shader_recompiler/frontend/ir/ir_emitter.h"
#include "shader_recompiler/host_translate_info.h"
#include "shader_recompiler/ir_opt/passes.h"
#include "shader_recompiler/shader_info.h"

namespace Shader::Optimization {

static IR::Attribute EmulatedLayerAttribute(VaryingState& stores) {
    for (u32 i = 0; i < 32; i++) {
        if (!stores.Generic(i)) {
            return IR::Attribute::Generic0X + (i * 4);
        }
    }
    return IR::Attribute::Layer;
}

static bool PermittedProgramStage(Stage stage) {
    switch (stage) {
    case Stage::VertexA:
    case Stage::VertexB:
    case Stage::TessellationControl:
    case Stage::TessellationEval:
        return true;
    default:
        return false;
    }
}

void LayerPass(IR::Program& program, const HostTranslateInfo& host_info) {
    if (host_info.support_viewport_index_layer || !PermittedProgramStage(program.stage)) {
        return;
    }

    const auto end{program.post_order_blocks.end()};
    const auto layer_attribute = EmulatedLayerAttribute(program.info.stores);
    bool requires_layer_emulation = false;

    for (auto block = program.post_order_blocks.begin(); block != end; ++block) {
        for (IR::Inst& inst : (*block)->Instructions()) {
            if (inst.GetOpcode() == IR::Opcode::SetAttribute &&
                inst.Arg(0).Attribute() == IR::Attribute::Layer) {
                requires_layer_emulation = true;
                inst.SetArg(0, IR::Value{layer_attribute});
            }
        }
    }

    if (requires_layer_emulation) {
        program.info.requires_layer_emulation = true;
        program.info.emulated_layer = layer_attribute;
        program.info.stores.Set(IR::Attribute::Layer, false);
        program.info.stores.Set(layer_attribute, true);
    }
}

} // namespace Shader::Optimization

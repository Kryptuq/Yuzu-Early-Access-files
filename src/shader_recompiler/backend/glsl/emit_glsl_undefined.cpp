// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/glsl/emit_glsl_instructions.h"
#include "shader_recompiler/backend/glsl/glsl_emit_context.h"

namespace Shader::Backend::GLSL {

void EmitUndefU1(EmitContext& ctx, IR::Inst& inst) {
    ctx.AddU1("{}=false;", inst);
}

void EmitUndefU8(EmitContext& ctx, IR::Inst& inst) {
    ctx.AddU32("{}=0u;", inst);
}

void EmitUndefU16(EmitContext& ctx, IR::Inst& inst) {
    ctx.AddU32("{}=0u;", inst);
}

void EmitUndefU32(EmitContext& ctx, IR::Inst& inst) {
    ctx.AddU32("{}=0u;", inst);
}

void EmitUndefU64(EmitContext& ctx, IR::Inst& inst) {
    ctx.AddU64("{}=0u;", inst);
}

} // namespace Shader::Backend::GLSL

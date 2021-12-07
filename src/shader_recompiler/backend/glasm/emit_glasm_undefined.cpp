// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "shader_recompiler/backend/glasm/emit_glasm_instructions.h"
#include "shader_recompiler/backend/glasm/glasm_emit_context.h"

namespace Shader::Backend::GLASM {

void EmitUndefU1(EmitContext& ctx, IR::Inst& inst) {
    ctx.Add("MOV.S {}.x,0;", inst);
}

void EmitUndefU8(EmitContext& ctx, IR::Inst& inst) {
    ctx.Add("MOV.S {}.x,0;", inst);
}

void EmitUndefU16(EmitContext& ctx, IR::Inst& inst) {
    ctx.Add("MOV.S {}.x,0;", inst);
}

void EmitUndefU32(EmitContext& ctx, IR::Inst& inst) {
    ctx.Add("MOV.S {}.x,0;", inst);
}

void EmitUndefU64(EmitContext& ctx, IR::Inst& inst) {
    ctx.LongAdd("MOV.S64 {}.x,0;", inst);
}

} // namespace Shader::Backend::GLASM

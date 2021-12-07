// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "shader_recompiler/backend/glasm/emit_glasm_instructions.h"
#include "shader_recompiler/backend/glasm/glasm_emit_context.h"

namespace Shader::Backend::GLASM {

void EmitBarrier(EmitContext& ctx) {
    ctx.Add("BAR;");
}

void EmitWorkgroupMemoryBarrier(EmitContext& ctx) {
    ctx.Add("MEMBAR.CTA;");
}

void EmitDeviceMemoryBarrier(EmitContext& ctx) {
    ctx.Add("MEMBAR;");
}

} // namespace Shader::Backend::GLASM

// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/glsl/emit_glsl_instructions.h"
#include "shader_recompiler/backend/glsl/glsl_emit_context.h"

namespace Shader::Backend::GLSL {
void EmitBarrier(EmitContext& ctx) {
    ctx.Add("barrier();");
}

void EmitWorkgroupMemoryBarrier(EmitContext& ctx) {
    ctx.Add("groupMemoryBarrier();");
}

void EmitDeviceMemoryBarrier(EmitContext& ctx) {
    ctx.Add("memoryBarrier();");
}
} // namespace Shader::Backend::GLSL

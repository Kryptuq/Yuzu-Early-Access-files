// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/backend/glsl/emit_glsl_instructions.h"
#include "shader_recompiler/backend/glsl/glsl_emit_context.h"
#include "shader_recompiler/exception.h"

namespace Shader::Backend::GLSL {

void EmitJoin(EmitContext&) {
    throw NotImplementedException("Join shouldn't be emitted");
}

void EmitDemoteToHelperInvocation(EmitContext& ctx) {
    ctx.Add("discard;");
}

} // namespace Shader::Backend::GLSL

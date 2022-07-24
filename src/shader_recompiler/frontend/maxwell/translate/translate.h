// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "shader_recompiler/environment.h"
#include "shader_recompiler/frontend/ir/basic_block.h"

namespace Shader::Maxwell {

void Translate(Environment& env, IR::Block* block, u32 location_begin, u32 location_end);

} // namespace Shader::Maxwell

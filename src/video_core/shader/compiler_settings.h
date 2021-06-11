// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "video_core/engines/shader_bytecode.h"

namespace VideoCommon::Shader {

enum class CompileDepth : u32 {
    BruteForce = 0,
    FlowStack = 1,
    NoFlowStack = 2,
    DecompileBackwards = 3,
    FullDecompile = 4,
};

std::string CompileDepthAsString(CompileDepth cd);

struct CompilerSettings {
    CompileDepth depth{CompileDepth::NoFlowStack};
    bool disable_else_derivation{true};
};

} // namespace VideoCommon::Shader

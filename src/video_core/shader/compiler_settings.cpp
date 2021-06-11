// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "video_core/shader/compiler_settings.h"

namespace VideoCommon::Shader {

std::string CompileDepthAsString(const CompileDepth cd) {
    switch (cd) {
    case CompileDepth::BruteForce:
        return "Brute Force Compile";
    case CompileDepth::FlowStack:
        return "Simple Flow Stack Mode";
    case CompileDepth::NoFlowStack:
        return "Remove Flow Stack";
    case CompileDepth::DecompileBackwards:
        return "Decompile Backward Jumps";
    case CompileDepth::FullDecompile:
        return "Full Decompilation";
    default:
        return "Unknown Compiler Process";
    }
}

} // namespace VideoCommon::Shader

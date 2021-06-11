// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <string_view>

#include "common/common_types.h"

namespace Tegra::Engines {
enum class ShaderType : u32;
}

namespace VideoCommon::Shader {
class ShaderIR;
class Registry;
} // namespace VideoCommon::Shader

namespace OpenGL {

class Device;

std::string DecompileAssemblyShader(const Device& device, const VideoCommon::Shader::ShaderIR& ir,
                                    const VideoCommon::Shader::Registry& registry,
                                    Tegra::Engines::ShaderType stage, std::string_view identifier);

} // namespace OpenGL

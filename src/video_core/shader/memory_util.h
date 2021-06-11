// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <vector>

#include "common/common_types.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/engines/shader_type.h"

namespace Tegra {
class MemoryManager;
}

namespace VideoCommon::Shader {

using ProgramCode = std::vector<u64>;

constexpr u32 STAGE_MAIN_OFFSET = 10;
constexpr u32 KERNEL_MAIN_OFFSET = 0;

/// Gets the address for the specified shader stage program
GPUVAddr GetShaderAddress(Tegra::Engines::Maxwell3D& maxwell3d,
                          Tegra::Engines::Maxwell3D::Regs::ShaderProgram program);

/// Gets if the current instruction offset is a scheduler instruction
bool IsSchedInstruction(std::size_t offset, std::size_t main_offset);

/// Calculates the size of a program stream
std::size_t CalculateProgramSize(const ProgramCode& program, bool is_compute);

/// Gets the shader program code from memory for the specified address
ProgramCode GetShaderCode(Tegra::MemoryManager& memory_manager, GPUVAddr gpu_addr,
                          const u8* host_ptr, bool is_compute);

/// Hashes one (or two) program streams
u64 GetUniqueIdentifier(Tegra::Engines::ShaderType shader_type, bool is_a, const ProgramCode& code,
                        const ProgramCode& code_b = {});

} // namespace VideoCommon::Shader

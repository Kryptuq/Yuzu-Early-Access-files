// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cstddef>

#include <boost/container_hash/hash.hpp>

#include "common/common_types.h"
#include "core/core.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/memory_manager.h"
#include "video_core/shader/memory_util.h"
#include "video_core/shader/shader_ir.h"

namespace VideoCommon::Shader {

GPUVAddr GetShaderAddress(Tegra::Engines::Maxwell3D& maxwell3d,
                          Tegra::Engines::Maxwell3D::Regs::ShaderProgram program) {
    const auto& shader_config{maxwell3d.regs.shader_config[static_cast<std::size_t>(program)]};
    return maxwell3d.regs.code_address.CodeAddress() + shader_config.offset;
}

bool IsSchedInstruction(std::size_t offset, std::size_t main_offset) {
    // Sched instructions appear once every 4 instructions.
    constexpr std::size_t SchedPeriod = 4;
    const std::size_t absolute_offset = offset - main_offset;
    return (absolute_offset % SchedPeriod) == 0;
}

std::size_t CalculateProgramSize(const ProgramCode& program, bool is_compute) {
    // This is the encoded version of BRA that jumps to itself. All Nvidia
    // shaders end with one.
    static constexpr u64 SELF_JUMPING_BRANCH = 0xE2400FFFFF07000FULL;
    static constexpr u64 MASK = 0xFFFFFFFFFF7FFFFFULL;

    const std::size_t start_offset = is_compute ? KERNEL_MAIN_OFFSET : STAGE_MAIN_OFFSET;
    std::size_t offset = start_offset;
    while (offset < program.size()) {
        const u64 instruction = program[offset];
        if (!IsSchedInstruction(offset, start_offset)) {
            if ((instruction & MASK) == SELF_JUMPING_BRANCH) {
                // End on Maxwell's "nop" instruction
                break;
            }
            if (instruction == 0) {
                break;
            }
        }
        ++offset;
    }
    // The last instruction is included in the program size
    return std::min(offset + 1, program.size());
}

ProgramCode GetShaderCode(Tegra::MemoryManager& memory_manager, GPUVAddr gpu_addr,
                          const u8* host_ptr, bool is_compute) {
    ProgramCode code(VideoCommon::Shader::MAX_PROGRAM_LENGTH);
    ASSERT_OR_EXECUTE(host_ptr != nullptr, { return code; });
    memory_manager.ReadBlockUnsafe(gpu_addr, code.data(), code.size() * sizeof(u64));
    code.resize(CalculateProgramSize(code, is_compute));
    return code;
}

u64 GetUniqueIdentifier(Tegra::Engines::ShaderType shader_type, bool is_a, const ProgramCode& code,
                        const ProgramCode& code_b) {
    size_t unique_identifier = boost::hash_value(code);
    if (is_a) {
        // VertexA programs include two programs
        boost::hash_combine(unique_identifier, boost::hash_value(code_b));
    }
    return static_cast<u64>(unique_identifier);
}

} // namespace VideoCommon::Shader

// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "common/common_types.h"
#include "shader_recompiler/program_header.h"
#include "shader_recompiler/shader_info.h"
#include "shader_recompiler/stage.h"

namespace Shader {

class Environment {
public:
    virtual ~Environment() = default;

    [[nodiscard]] virtual u64 ReadInstruction(u32 address) = 0;

    [[nodiscard]] virtual u32 ReadCbufValue(u32 cbuf_index, u32 cbuf_offset) = 0;

    [[nodiscard]] virtual TextureType ReadTextureType(u32 raw_handle) = 0;

    [[nodiscard]] virtual TexturePixelFormat ReadTexturePixelFormat(u32 raw_handle) = 0;

    [[nodiscard]] virtual u32 ReadViewportTransformState() = 0;

    [[nodiscard]] virtual u32 TextureBoundBuffer() const = 0;

    [[nodiscard]] virtual u32 LocalMemorySize() const = 0;

    [[nodiscard]] virtual u32 SharedMemorySize() const = 0;

    [[nodiscard]] virtual std::array<u32, 3> WorkgroupSize() const = 0;

    [[nodiscard]] virtual bool HasHLEMacroState() const = 0;

    [[nodiscard]] virtual std::optional<ReplaceConstant> GetReplaceConstBuffer(u32 bank,
                                                                               u32 offset) = 0;

    virtual void Dump(u64 hash) = 0;

    [[nodiscard]] const ProgramHeader& SPH() const noexcept {
        return sph;
    }

    [[nodiscard]] const std::array<u32, 8>& GpPassthroughMask() const noexcept {
        return gp_passthrough_mask;
    }

    [[nodiscard]] Stage ShaderStage() const noexcept {
        return stage;
    }

    [[nodiscard]] u32 StartAddress() const noexcept {
        return start_address;
    }

    [[nodiscard]] bool IsPropietaryDriver() const noexcept {
        return is_propietary_driver;
    }

protected:
    ProgramHeader sph{};
    std::array<u32, 8> gp_passthrough_mask{};
    Stage stage{};
    u32 start_address{};
    bool is_propietary_driver{};
};

} // namespace Shader

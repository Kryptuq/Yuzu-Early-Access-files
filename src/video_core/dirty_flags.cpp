// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cstddef>

#include "common/common_types.h"
#include "video_core/dirty_flags.h"

#define OFF(field_name) MAXWELL3D_REG_INDEX(field_name)
#define NUM(field_name) (sizeof(::Tegra::Engines::Maxwell3D::Regs::field_name) / (sizeof(u32)))

namespace VideoCommon::Dirty {
namespace {
using Tegra::Engines::Maxwell3D;

void SetupDirtyVertexBuffers(Maxwell3D::DirtyState::Tables& tables) {
    static constexpr std::size_t num_array = 3;
    for (std::size_t i = 0; i < Maxwell3D::Regs::NumVertexArrays; ++i) {
        const std::size_t array_offset = OFF(vertex_streams) + i * NUM(vertex_streams[0]);
        const std::size_t limit_offset =
            OFF(vertex_stream_limits) + i * NUM(vertex_stream_limits[0]);

        FillBlock(tables, array_offset, num_array, VertexBuffer0 + i, VertexBuffers);
        FillBlock(tables, limit_offset, NUM(vertex_stream_limits), VertexBuffer0 + i,
                  VertexBuffers);
    }
}

void SetupIndexBuffer(Maxwell3D::DirtyState::Tables& tables) {
    FillBlock(tables[0], OFF(index_buffer), NUM(index_buffer), IndexBuffer);
}

void SetupDirtyDescriptors(Maxwell3D::DirtyState::Tables& tables) {
    FillBlock(tables[0], OFF(tex_header), NUM(tex_header), Descriptors);
    FillBlock(tables[0], OFF(tex_sampler), NUM(tex_sampler), Descriptors);
}

void SetupDirtyRenderTargets(Maxwell3D::DirtyState::Tables& tables) {
    static constexpr std::size_t num_per_rt = NUM(rt[0]);
    static constexpr std::size_t begin = OFF(rt);
    static constexpr std::size_t num = num_per_rt * Maxwell3D::Regs::NumRenderTargets;
    for (std::size_t rt = 0; rt < Maxwell3D::Regs::NumRenderTargets; ++rt) {
        FillBlock(tables[0], begin + rt * num_per_rt, num_per_rt, ColorBuffer0 + rt);
    }
    FillBlock(tables[1], begin, num, RenderTargets);
    FillBlock(tables[0], OFF(surface_clip), NUM(surface_clip), RenderTargets);

    tables[0][OFF(rt_control)] = RenderTargets;
    tables[1][OFF(rt_control)] = RenderTargetControl;

    static constexpr std::array zeta_flags{ZetaBuffer, RenderTargets};
    for (std::size_t i = 0; i < std::size(zeta_flags); ++i) {
        const u8 flag = zeta_flags[i];
        auto& table = tables[i];
        table[OFF(zeta_enable)] = flag;
        table[OFF(zeta_size.width)] = flag;
        table[OFF(zeta_size.height)] = flag;
        FillBlock(table, OFF(zeta), NUM(zeta), flag);
    }
}

void SetupDirtyShaders(Maxwell3D::DirtyState::Tables& tables) {
    FillBlock(tables[0], OFF(pipelines), NUM(pipelines) * Maxwell3D::Regs::MaxShaderProgram,
              Shaders);
}
} // Anonymous namespace

void SetupDirtyFlags(Maxwell3D::DirtyState::Tables& tables) {
    SetupDirtyVertexBuffers(tables);
    SetupIndexBuffer(tables);
    SetupDirtyDescriptors(tables);
    SetupDirtyRenderTargets(tables);
    SetupDirtyShaders(tables);
}

} // namespace VideoCommon::Dirty

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "common/common_types.h"
#include "video_core/engines/maxwell_3d.h"

namespace VideoCore {
class RasterizerInterface;
}

namespace Tegra::Engines {
using PrimitiveTopologyControl = Maxwell3D::Regs::PrimitiveTopologyControl;
using PrimitiveTopology = Maxwell3D::Regs::PrimitiveTopology;
using PrimitiveTopologyOverride = Maxwell3D::Regs::PrimitiveTopologyOverride;
using IndexBuffer = Maxwell3D::Regs::IndexBuffer;
using VertexBuffer = Maxwell3D::Regs::VertexBuffer;
using IndexBufferSmall = Maxwell3D::Regs::IndexBufferSmall;

class DrawManager {
public:
    enum class DrawMode : u32 { General = 0, Instance, InlineIndex };
    struct State {
        PrimitiveTopology topology{};
        DrawMode draw_mode{};
        bool draw_indexed{};
        u32 base_index{};
        VertexBuffer vertex_buffer;
        IndexBuffer index_buffer;
        u32 base_instance{};
        u32 instance_count{};
        std::vector<u8> inline_index_draw_indexes;
    };

    struct IndirectParams {
        bool is_indexed;
        bool include_count;
        GPUVAddr count_start_address;
        GPUVAddr indirect_start_address;
        size_t buffer_size;
        size_t max_draw_counts;
        size_t stride;
    };

    explicit DrawManager(Maxwell3D* maxwell_3d);

    void ProcessMethodCall(u32 method, u32 argument);

    void Clear(u32 layer_count);

    void DrawDeferred();

    void DrawArray(PrimitiveTopology topology, u32 vertex_first, u32 vertex_count,
                   u32 base_instance, u32 num_instances);

    void DrawIndex(PrimitiveTopology topology, u32 index_first, u32 index_count, u32 base_index,
                   u32 base_instance, u32 num_instances);

    void DrawArrayIndirect(PrimitiveTopology topology);

    void DrawIndexedIndirect(PrimitiveTopology topology, u32 index_first, u32 index_count);

    const State& GetDrawState() const {
        return draw_state;
    }

    IndirectParams& GetIndirectParams() {
        return indirect_state;
    }

    const IndirectParams& GetIndirectParams() const {
        return indirect_state;
    }

private:
    void SetInlineIndexBuffer(u32 index);

    void DrawBegin();

    void DrawEnd(u32 instance_count = 1, bool force_draw = false);

    void DrawIndexSmall(u32 argument);

    void UpdateTopology();

    void ProcessDraw(bool draw_indexed, u32 instance_count);

    void ProcessDrawIndirect();

    Maxwell3D* maxwell3d{};
    State draw_state{};
    IndirectParams indirect_state{};
};
} // namespace Tegra::Engines

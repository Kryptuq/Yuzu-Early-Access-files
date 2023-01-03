// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cstddef>

#include "common/common_types.h"
#include "core/core.h"
#include "video_core/control/channel_state.h"
#include "video_core/dirty_flags.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/renderer_vulkan/vk_state_tracker.h"

#define OFF(field_name) MAXWELL3D_REG_INDEX(field_name)
#define NUM(field_name) (sizeof(Maxwell3D::Regs::field_name) / (sizeof(u32)))

namespace Vulkan {
namespace {
using namespace Dirty;
using namespace VideoCommon::Dirty;
using Tegra::Engines::Maxwell3D;
using Regs = Maxwell3D::Regs;
using Tables = Maxwell3D::DirtyState::Tables;
using Table = Maxwell3D::DirtyState::Table;
using Flags = Maxwell3D::DirtyState::Flags;

Flags MakeInvalidationFlags() {
    static constexpr int INVALIDATION_FLAGS[]{
        Viewports,
        Scissors,
        DepthBias,
        BlendConstants,
        DepthBounds,
        StencilProperties,
        StencilReference,
        StencilWriteMask,
        StencilCompare,
        LineWidth,
        CullMode,
        DepthBoundsEnable,
        DepthTestEnable,
        DepthWriteEnable,
        DepthCompareOp,
        FrontFace,
        StencilOp,
        StencilTestEnable,
        VertexBuffers,
        VertexInput,
        StateEnable,
        PrimitiveRestartEnable,
        RasterizerDiscardEnable,
        DepthBiasEnable,
        LogicOpEnable,
        DepthClampEnable,
        LogicOp,
        Blending,
        ColorMask,
        BlendEquations,
        BlendEnable,
    };
    Flags flags{};
    for (const int flag : INVALIDATION_FLAGS) {
        flags[flag] = true;
    }
    for (int index = VertexBuffer0; index <= VertexBuffer31; ++index) {
        flags[index] = true;
    }
    for (int index = VertexAttribute0; index <= VertexAttribute31; ++index) {
        flags[index] = true;
    }
    for (int index = VertexBinding0; index <= VertexBinding31; ++index) {
        flags[index] = true;
    }
    return flags;
}

void SetupDirtyViewports(Tables& tables) {
    FillBlock(tables[0], OFF(viewport_transform), NUM(viewport_transform), Viewports);
    FillBlock(tables[0], OFF(viewports), NUM(viewports), Viewports);
    tables[0][OFF(viewport_scale_offset_enabled)] = Viewports;
    tables[1][OFF(window_origin)] = Viewports;
}

void SetupDirtyScissors(Tables& tables) {
    FillBlock(tables[0], OFF(scissor_test), NUM(scissor_test), Scissors);
}

void SetupDirtyDepthBias(Tables& tables) {
    auto& table = tables[0];
    table[OFF(depth_bias)] = DepthBias;
    table[OFF(depth_bias_clamp)] = DepthBias;
    table[OFF(slope_scale_depth_bias)] = DepthBias;
}

void SetupDirtyBlendConstants(Tables& tables) {
    FillBlock(tables[0], OFF(blend_color), NUM(blend_color), BlendConstants);
}

void SetupDirtyDepthBounds(Tables& tables) {
    FillBlock(tables[0], OFF(depth_bounds), NUM(depth_bounds), DepthBounds);
}

void SetupDirtyStencilProperties(Tables& tables) {
    const auto setup = [&](size_t position, u8 flag) {
        tables[0][position] = flag;
        tables[1][position] = StencilProperties;
    };
    tables[0][OFF(stencil_two_side_enable)] = StencilProperties;
    setup(OFF(stencil_front_ref), StencilReference);
    setup(OFF(stencil_front_mask), StencilWriteMask);
    setup(OFF(stencil_front_func_mask), StencilCompare);
    setup(OFF(stencil_back_ref), StencilReference);
    setup(OFF(stencil_back_mask), StencilWriteMask);
    setup(OFF(stencil_back_func_mask), StencilCompare);
}

void SetupDirtyLineWidth(Tables& tables) {
    tables[0][OFF(line_width_smooth)] = LineWidth;
    tables[0][OFF(line_width_aliased)] = LineWidth;
}

void SetupDirtyCullMode(Tables& tables) {
    auto& table = tables[0];
    table[OFF(gl_cull_face)] = CullMode;
    table[OFF(gl_cull_test_enabled)] = CullMode;
}

void SetupDirtyStateEnable(Tables& tables) {
    const auto setup = [&](size_t position, u8 flag) {
        tables[0][position] = flag;
        tables[1][position] = StateEnable;
    };
    setup(OFF(depth_bounds_enable), DepthBoundsEnable);
    setup(OFF(depth_test_enable), DepthTestEnable);
    setup(OFF(depth_write_enabled), DepthWriteEnable);
    setup(OFF(stencil_enable), StencilTestEnable);
    setup(OFF(primitive_restart.enabled), PrimitiveRestartEnable);
    setup(OFF(rasterize_enable), RasterizerDiscardEnable);
    setup(OFF(polygon_offset_point_enable), DepthBiasEnable);
    setup(OFF(polygon_offset_line_enable), DepthBiasEnable);
    setup(OFF(polygon_offset_fill_enable), DepthBiasEnable);
    setup(OFF(logic_op.enable), LogicOpEnable);
    setup(OFF(viewport_clip_control.geometry_clip), DepthClampEnable);
}

void SetupDirtyDepthCompareOp(Tables& tables) {
    tables[0][OFF(depth_test_func)] = DepthCompareOp;
}

void SetupDirtyFrontFace(Tables& tables) {
    auto& table = tables[0];
    table[OFF(gl_front_face)] = FrontFace;
    table[OFF(window_origin)] = FrontFace;
}

void SetupDirtyStencilOp(Tables& tables) {
    auto& table = tables[0];
    table[OFF(stencil_front_op.fail)] = StencilOp;
    table[OFF(stencil_front_op.zfail)] = StencilOp;
    table[OFF(stencil_front_op.zpass)] = StencilOp;
    table[OFF(stencil_front_op.func)] = StencilOp;
    table[OFF(stencil_back_op.fail)] = StencilOp;
    table[OFF(stencil_back_op.zfail)] = StencilOp;
    table[OFF(stencil_back_op.zpass)] = StencilOp;
    table[OFF(stencil_back_op.func)] = StencilOp;

    // Table 0 is used by StencilProperties
    tables[1][OFF(stencil_two_side_enable)] = StencilOp;
}

void SetupDirtyBlending(Tables& tables) {
    tables[0][OFF(color_mask_common)] = Blending;
    tables[1][OFF(color_mask_common)] = ColorMask;
    tables[0][OFF(blend_per_target_enabled)] = Blending;
    tables[1][OFF(blend_per_target_enabled)] = BlendEquations;
    FillBlock(tables[0], OFF(color_mask), NUM(color_mask), Blending);
    FillBlock(tables[1], OFF(color_mask), NUM(color_mask), ColorMask);
    FillBlock(tables[0], OFF(blend), NUM(blend), Blending);
    FillBlock(tables[1], OFF(blend), NUM(blend), BlendEquations);
    FillBlock(tables[1], OFF(blend.enable), NUM(blend.enable), BlendEnable);
    FillBlock(tables[0], OFF(blend_per_target), NUM(blend_per_target), Blending);
    FillBlock(tables[1], OFF(blend_per_target), NUM(blend_per_target), BlendEquations);
}

void SetupDirtySpecialOps(Tables& tables) {
    tables[0][OFF(logic_op.op)] = LogicOp;
}

void SetupDirtyViewportSwizzles(Tables& tables) {
    static constexpr size_t swizzle_offset = 6;
    for (size_t index = 0; index < Regs::NumViewports; ++index) {
        tables[0][OFF(viewport_transform) + index * NUM(viewport_transform[0]) + swizzle_offset] =
            ViewportSwizzles;
    }
}

void SetupDirtyVertexAttributes(Tables& tables) {
    for (size_t i = 0; i < Regs::NumVertexAttributes; ++i) {
        const size_t offset = OFF(vertex_attrib_format) + i * NUM(vertex_attrib_format[0]);
        FillBlock(tables[0], offset, NUM(vertex_attrib_format[0]), VertexAttribute0 + i);
    }
    FillBlock(tables[1], OFF(vertex_attrib_format), Regs::NumVertexAttributes, VertexInput);
}

void SetupDirtyVertexBindings(Tables& tables) {
    // Do NOT include stride here, it's implicit in VertexBuffer
    static constexpr size_t divisor_offset = 3;
    for (size_t i = 0; i < Regs::NumVertexArrays; ++i) {
        const u8 flag = static_cast<u8>(VertexBinding0 + i);
        tables[0][OFF(vertex_stream_instances) + i] = VertexInput;
        tables[1][OFF(vertex_stream_instances) + i] = flag;
        tables[0][OFF(vertex_streams) + i * NUM(vertex_streams[0]) + divisor_offset] = VertexInput;
        tables[1][OFF(vertex_streams) + i * NUM(vertex_streams[0]) + divisor_offset] = flag;
    }
}
} // Anonymous namespace

void StateTracker::SetupTables(Tegra::Control::ChannelState& channel_state) {
    auto& tables{channel_state.maxwell_3d->dirty.tables};
    SetupDirtyFlags(tables);
    SetupDirtyViewports(tables);
    SetupDirtyScissors(tables);
    SetupDirtyDepthBias(tables);
    SetupDirtyBlendConstants(tables);
    SetupDirtyDepthBounds(tables);
    SetupDirtyStencilProperties(tables);
    SetupDirtyLineWidth(tables);
    SetupDirtyCullMode(tables);
    SetupDirtyStateEnable(tables);
    SetupDirtyDepthCompareOp(tables);
    SetupDirtyFrontFace(tables);
    SetupDirtyStencilOp(tables);
    SetupDirtyBlending(tables);
    SetupDirtyViewportSwizzles(tables);
    SetupDirtyVertexAttributes(tables);
    SetupDirtyVertexBindings(tables);
    SetupDirtySpecialOps(tables);
}

void StateTracker::ChangeChannel(Tegra::Control::ChannelState& channel_state) {
    flags = &channel_state.maxwell_3d->dirty.flags;
}

void StateTracker::InvalidateState() {
    flags->set();
}

StateTracker::StateTracker()
    : flags{&default_flags}, default_flags{}, invalidation_flags{MakeInvalidationFlags()} {}

} // namespace Vulkan

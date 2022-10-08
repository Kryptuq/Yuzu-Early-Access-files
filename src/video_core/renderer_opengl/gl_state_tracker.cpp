// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cstddef>

#include "common/common_types.h"
#include "core/core.h"
#include "video_core/control/channel_state.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/renderer_opengl/gl_state_tracker.h"

#define OFF(field_name) MAXWELL3D_REG_INDEX(field_name)
#define NUM(field_name) (sizeof(Maxwell3D::Regs::field_name) / (sizeof(u32)))

namespace OpenGL {

namespace {

using namespace Dirty;
using namespace VideoCommon::Dirty;
using Tegra::Engines::Maxwell3D;
using Regs = Maxwell3D::Regs;
using Tables = Maxwell3D::DirtyState::Tables;
using Table = Maxwell3D::DirtyState::Table;

void SetupDirtyColorMasks(Tables& tables) {
    tables[0][OFF(color_mask_common)] = ColorMaskCommon;
    for (std::size_t rt = 0; rt < Regs::NumRenderTargets; ++rt) {
        const std::size_t offset = OFF(color_mask) + rt * NUM(color_mask[0]);
        FillBlock(tables[0], offset, NUM(color_mask[0]), ColorMask0 + rt);
    }

    FillBlock(tables[1], OFF(color_mask), NUM(color_mask), ColorMasks);
}

void SetupDirtyVertexInstances(Tables& tables) {
    static constexpr std::size_t instance_base_offset = 3;
    for (std::size_t i = 0; i < Regs::NumVertexArrays; ++i) {
        const std::size_t array_offset = OFF(vertex_streams) + i * NUM(vertex_streams[0]);
        const std::size_t instance_array_offset = array_offset + instance_base_offset;
        tables[0][instance_array_offset] = static_cast<u8>(VertexInstance0 + i);
        tables[1][instance_array_offset] = VertexInstances;

        const std::size_t instance_offset = OFF(vertex_stream_instances) + i;
        tables[0][instance_offset] = static_cast<u8>(VertexInstance0 + i);
        tables[1][instance_offset] = VertexInstances;
    }
}

void SetupDirtyVertexFormat(Tables& tables) {
    for (std::size_t i = 0; i < Regs::NumVertexAttributes; ++i) {
        const std::size_t offset = OFF(vertex_attrib_format) + i * NUM(vertex_attrib_format[0]);
        FillBlock(tables[0], offset, NUM(vertex_attrib_format[0]), VertexFormat0 + i);
    }

    FillBlock(tables[1], OFF(vertex_attrib_format), Regs::NumVertexAttributes, VertexFormats);
}

void SetupDirtyViewports(Tables& tables) {
    for (std::size_t i = 0; i < Regs::NumViewports; ++i) {
        const std::size_t transf_offset = OFF(viewport_transform) + i * NUM(viewport_transform[0]);
        const std::size_t viewport_offset = OFF(viewports) + i * NUM(viewports[0]);

        FillBlock(tables[0], transf_offset, NUM(viewport_transform[0]), Viewport0 + i);
        FillBlock(tables[0], viewport_offset, NUM(viewports[0]), Viewport0 + i);
    }

    FillBlock(tables[1], OFF(viewport_transform), NUM(viewport_transform), Viewports);
    FillBlock(tables[1], OFF(viewports), NUM(viewports), Viewports);

    tables[0][OFF(viewport_scale_offset_enbled)] = ViewportTransform;
    tables[1][OFF(viewport_scale_offset_enbled)] = Viewports;
}

void SetupDirtyScissors(Tables& tables) {
    for (std::size_t i = 0; i < Regs::NumViewports; ++i) {
        const std::size_t offset = OFF(scissor_test) + i * NUM(scissor_test[0]);
        FillBlock(tables[0], offset, NUM(scissor_test[0]), Scissor0 + i);
    }
    FillBlock(tables[1], OFF(scissor_test), NUM(scissor_test), Scissors);
}

void SetupDirtyPolygonModes(Tables& tables) {
    tables[0][OFF(polygon_mode_front)] = PolygonModeFront;
    tables[0][OFF(polygon_mode_back)] = PolygonModeBack;

    tables[1][OFF(polygon_mode_front)] = PolygonModes;
    tables[1][OFF(polygon_mode_back)] = PolygonModes;
    tables[0][OFF(fill_via_triangle_mode)] = PolygonModes;
}

void SetupDirtyDepthTest(Tables& tables) {
    auto& table = tables[0];
    table[OFF(depth_test_enable)] = DepthTest;
    table[OFF(depth_write_enabled)] = DepthMask;
    table[OFF(depth_test_func)] = DepthTest;
}

void SetupDirtyStencilTest(Tables& tables) {
    static constexpr std::array offsets = {
        OFF(stencil_enable),          OFF(stencil_front_op.func),
        OFF(stencil_front_func.ref),  OFF(stencil_front_func.func_mask),
        OFF(stencil_front_op.fail),   OFF(stencil_front_op.zfail),
        OFF(stencil_front_op.zpass),  OFF(stencil_front_func.mask),
        OFF(stencil_two_side_enable), OFF(stencil_back_op.func),
        OFF(stencil_back_func.ref),   OFF(stencil_back_func.func_mask),
        OFF(stencil_back_op.fail),    OFF(stencil_back_op.zfail),
        OFF(stencil_back_op.zpass),   OFF(stencil_back_func.mask)};
    for (const auto offset : offsets) {
        tables[0][offset] = StencilTest;
    }
}

void SetupDirtyAlphaTest(Tables& tables) {
    auto& table = tables[0];
    table[OFF(alpha_test_ref)] = AlphaTest;
    table[OFF(alpha_test_func)] = AlphaTest;
    table[OFF(alpha_test_enabled)] = AlphaTest;
}

void SetupDirtyBlend(Tables& tables) {
    FillBlock(tables[0], OFF(blend_color), NUM(blend_color), BlendColor);

    tables[0][OFF(blend_per_target_enabled)] = BlendIndependentEnabled;

    for (std::size_t i = 0; i < Regs::NumRenderTargets; ++i) {
        const std::size_t offset = OFF(blend_per_target) + i * NUM(blend_per_target[0]);
        FillBlock(tables[0], offset, NUM(blend_per_target[0]), BlendState0 + i);

        tables[0][OFF(blend.enable) + i] = static_cast<u8>(BlendState0 + i);
    }
    FillBlock(tables[1], OFF(blend_per_target), NUM(blend_per_target), BlendStates);
    FillBlock(tables[1], OFF(blend), NUM(blend), BlendStates);
}

void SetupDirtyPrimitiveRestart(Tables& tables) {
    FillBlock(tables[0], OFF(primitive_restart), NUM(primitive_restart), PrimitiveRestart);
}

void SetupDirtyPolygonOffset(Tables& tables) {
    auto& table = tables[0];
    table[OFF(polygon_offset_fill_enable)] = PolygonOffset;
    table[OFF(polygon_offset_line_enable)] = PolygonOffset;
    table[OFF(polygon_offset_point_enable)] = PolygonOffset;
    table[OFF(slope_scale_depth_bias)] = PolygonOffset;
    table[OFF(depth_bias)] = PolygonOffset;
    table[OFF(depth_bias_clamp)] = PolygonOffset;
}

void SetupDirtyMultisampleControl(Tables& tables) {
    FillBlock(tables[0], OFF(anti_alias_alpha_control), NUM(anti_alias_alpha_control),
              MultisampleControl);
}

void SetupDirtyRasterizeEnable(Tables& tables) {
    tables[0][OFF(rasterize_enable)] = RasterizeEnable;
}

void SetupDirtyFramebufferSRGB(Tables& tables) {
    tables[0][OFF(framebuffer_srgb)] = FramebufferSRGB;
}

void SetupDirtyLogicOp(Tables& tables) {
    FillBlock(tables[0], OFF(logic_op), NUM(logic_op), LogicOp);
}

void SetupDirtyFragmentClampColor(Tables& tables) {
    tables[0][OFF(frag_color_clamp)] = FragmentClampColor;
}

void SetupDirtyPointSize(Tables& tables) {
    tables[0][OFF(point_size_attribute)] = PointSize;
    tables[0][OFF(point_size)] = PointSize;
    tables[0][OFF(point_sprite_enable)] = PointSize;
}

void SetupDirtyLineWidth(Tables& tables) {
    tables[0][OFF(line_width_smooth)] = LineWidth;
    tables[0][OFF(line_width_aliased)] = LineWidth;
    tables[0][OFF(line_anti_alias_enable)] = LineWidth;
}

void SetupDirtyClipControl(Tables& tables) {
    auto& table = tables[0];
    table[OFF(window_origin)] = ClipControl;
    table[OFF(depth_mode)] = ClipControl;
}

void SetupDirtyDepthClampEnabled(Tables& tables) {
    tables[0][OFF(viewport_clip_control)] = DepthClampEnabled;
}

void SetupDirtyMisc(Tables& tables) {
    auto& table = tables[0];

    table[OFF(user_clip_enable)] = ClipDistances;

    table[OFF(gl_front_face)] = FrontFace;

    table[OFF(gl_cull_test_enabled)] = CullTest;
    table[OFF(gl_cull_face)] = CullTest;
}

} // Anonymous namespace

void StateTracker::SetupTables(Tegra::Control::ChannelState& channel_state) {
    auto& tables{channel_state.maxwell_3d->dirty.tables};
    SetupDirtyFlags(tables);
    SetupDirtyColorMasks(tables);
    SetupDirtyViewports(tables);
    SetupDirtyScissors(tables);
    SetupDirtyVertexInstances(tables);
    SetupDirtyVertexFormat(tables);
    SetupDirtyPolygonModes(tables);
    SetupDirtyDepthTest(tables);
    SetupDirtyStencilTest(tables);
    SetupDirtyAlphaTest(tables);
    SetupDirtyBlend(tables);
    SetupDirtyPrimitiveRestart(tables);
    SetupDirtyPolygonOffset(tables);
    SetupDirtyMultisampleControl(tables);
    SetupDirtyRasterizeEnable(tables);
    SetupDirtyFramebufferSRGB(tables);
    SetupDirtyLogicOp(tables);
    SetupDirtyFragmentClampColor(tables);
    SetupDirtyPointSize(tables);
    SetupDirtyLineWidth(tables);
    SetupDirtyClipControl(tables);
    SetupDirtyDepthClampEnabled(tables);
    SetupDirtyMisc(tables);
}

void StateTracker::ChangeChannel(Tegra::Control::ChannelState& channel_state) {
    flags = &channel_state.maxwell_3d->dirty.flags;
}

void StateTracker::InvalidateState() {
    flags->set();
}

StateTracker::StateTracker() : flags{&default_flags} {}

} // namespace OpenGL

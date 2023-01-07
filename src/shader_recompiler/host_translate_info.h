// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Shader {

// Try to keep entries here to a minimum
// They can accidentally change the cached information in a shader

/// Misc information about the host
struct HostTranslateInfo {
    bool support_float16{};      ///< True when the device supports 16-bit floats
    bool support_int64{};        ///< True when the device supports 64-bit integers
    bool needs_demote_reorder{}; ///< True when the device needs DemoteToHelperInvocation reordered
    bool support_snorm_render_buffer{};  ///< True when the device supports SNORM render buffers
    bool support_viewport_index_layer{}; ///< True when the device supports gl_Layer in VS
    u32 min_ssbo_alignment{};            ///< Minimum alignment supported by the device for SSBOs
    bool support_geometry_shader_passthrough{}; ///< True when the device supports geometry
                                                ///< passthrough shaders
};

} // namespace Shader

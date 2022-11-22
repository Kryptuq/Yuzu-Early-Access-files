// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <vector>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/polyfill_ranges.h"
#include "shader_recompiler/shader_info.h"
#include "video_core/transform_feedback.h"

namespace VideoCommon {

std::vector<Shader::TransformFeedbackVarying> MakeTransformFeedbackVaryings(
    const TransformFeedbackState& state) {
    static constexpr std::array VECTORS{
        28U,  // gl_Position
        32U,  // Generic 0
        36U,  // Generic 1
        40U,  // Generic 2
        44U,  // Generic 3
        48U,  // Generic 4
        52U,  // Generic 5
        56U,  // Generic 6
        60U,  // Generic 7
        64U,  // Generic 8
        68U,  // Generic 9
        72U,  // Generic 10
        76U,  // Generic 11
        80U,  // Generic 12
        84U,  // Generic 13
        88U,  // Generic 14
        92U,  // Generic 15
        96U,  // Generic 16
        100U, // Generic 17
        104U, // Generic 18
        108U, // Generic 19
        112U, // Generic 20
        116U, // Generic 21
        120U, // Generic 22
        124U, // Generic 23
        128U, // Generic 24
        132U, // Generic 25
        136U, // Generic 26
        140U, // Generic 27
        144U, // Generic 28
        148U, // Generic 29
        152U, // Generic 30
        156U, // Generic 31
        160U, // gl_FrontColor
        164U, // gl_FrontSecondaryColor
        160U, // gl_BackColor
        164U, // gl_BackSecondaryColor
        192U, // gl_TexCoord[0]
        196U, // gl_TexCoord[1]
        200U, // gl_TexCoord[2]
        204U, // gl_TexCoord[3]
        208U, // gl_TexCoord[4]
        212U, // gl_TexCoord[5]
        216U, // gl_TexCoord[6]
        220U, // gl_TexCoord[7]
    };
    std::vector<Shader::TransformFeedbackVarying> xfb(256);
    for (size_t buffer = 0; buffer < state.layouts.size(); ++buffer) {
        const auto& locations = state.varyings[buffer];
        const auto& layout = state.layouts[buffer];
        const u32 varying_count = layout.varying_count;
        u32 highest = 0;
        for (u32 offset = 0; offset < varying_count; ++offset) {
            const auto get_attribute = [&locations](u32 index) -> u32 {
                switch (index % 4) {
                case 0:
                    return locations[index / 4].attribute0.Value();
                case 1:
                    return locations[index / 4].attribute1.Value();
                case 2:
                    return locations[index / 4].attribute2.Value();
                case 3:
                    return locations[index / 4].attribute3.Value();
                }
                UNREACHABLE();
                return 0;
            };

            UNIMPLEMENTED_IF_MSG(layout.stream != 0, "Stream is not zero: {}", layout.stream);
            Shader::TransformFeedbackVarying varying{
                .buffer = static_cast<u32>(buffer),
                .stride = layout.stride,
                .offset = offset * 4,
                .components = 1,
            };
            const u32 base_offset = offset;
            const auto attribute{get_attribute(offset)};
            if (std::ranges::find(VECTORS, Common::AlignDown(attribute, 4)) != VECTORS.end()) {
                UNIMPLEMENTED_IF_MSG(attribute % 4 != 0, "Unaligned TFB {}", attribute);

                const auto base_index = attribute / 4;
                while (offset + 1 < varying_count && base_index == get_attribute(offset + 1) / 4) {
                    ++offset;
                    ++varying.components;
                }
            }
            xfb[attribute] = varying;
            highest = std::max(highest, (base_offset + varying.components) * 4);
        }
        UNIMPLEMENTED_IF(highest != layout.stride);
    }
    return xfb;
}

} // namespace VideoCommon

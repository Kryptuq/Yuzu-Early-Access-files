// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <utility>

#include "common/assert.h"
#include "video_core/textures/texture.h"

namespace VideoCommon {

[[nodiscard]] inline std::pair<int, int> SamplesLog2(int num_samples) {
    switch (num_samples) {
    case 1:
        return {0, 0};
    case 2:
        return {1, 0};
    case 4:
        return {1, 1};
    case 8:
        return {2, 1};
    case 16:
        return {2, 2};
    }
    UNREACHABLE_MSG("Invalid number of samples={}", num_samples);
    return {1, 1};
}

[[nodiscard]] inline int NumSamples(Tegra::Texture::MsaaMode msaa_mode) {
    using Tegra::Texture::MsaaMode;
    switch (msaa_mode) {
    case MsaaMode::Msaa1x1:
        return 1;
    case MsaaMode::Msaa2x1:
    case MsaaMode::Msaa2x1_D3D:
        return 2;
    case MsaaMode::Msaa2x2:
    case MsaaMode::Msaa2x2_VC4:
    case MsaaMode::Msaa2x2_VC12:
        return 4;
    case MsaaMode::Msaa4x2:
    case MsaaMode::Msaa4x2_D3D:
    case MsaaMode::Msaa4x2_VC8:
    case MsaaMode::Msaa4x2_VC24:
        return 8;
    case MsaaMode::Msaa4x4:
        return 16;
    }
    UNREACHABLE_MSG("Invalid MSAA mode={}", static_cast<int>(msaa_mode));
    return 1;
}

} // namespace VideoCommon

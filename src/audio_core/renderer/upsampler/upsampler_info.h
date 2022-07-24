// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "audio_core/common/common.h"
#include "audio_core/renderer/upsampler/upsampler_state.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
class UpsamplerManager;

/**
 * Manages information needed to upsample a mix buffer.
 */
struct UpsamplerInfo {
    /// States used by the AudioRenderer across calls.
    std::array<UpsamplerState, MaxChannels> states{};
    /// Pointer to the manager
    UpsamplerManager* manager{};
    /// Pointer to the samples to be upsampled
    CpuAddr samples_pos{};
    /// Target number of samples to upsample to
    u32 sample_count{};
    /// Number of channels to upsample
    u32 input_count{};
    /// Is this upsampler enabled?
    bool enabled{};
    /// Mix buffer indexes to be upsampled
    std::array<s16, MaxChannels> inputs{};
};

} // namespace AudioCore::AudioRenderer

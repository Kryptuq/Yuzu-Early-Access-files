// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "audio_core/common/common.h"
#include "common/common_types.h"
#include "common/fixed_point.h"

namespace AudioCore::AudioRenderer {
/**
 * Holds a state for a voice. One is kept host-side, and one is used by the AudioRenderer,
 * host-side is updated on the next iteration.
 */
struct VoiceState {
    /**
     * State of the voice's biquad filter.
     */
    struct BiquadFilterState {
        Common::FixedPoint<50, 14> s0;
        Common::FixedPoint<50, 14> s1;
        Common::FixedPoint<50, 14> s2;
        Common::FixedPoint<50, 14> s3;
    };

    /**
     * Context for ADPCM decoding.
     */
    struct AdpcmContext {
        u16 header;
        s16 yn0;
        s16 yn1;
    };

    /// Number of samples played
    u64 played_sample_count;
    /// Current offset from the starting offset
    u32 offset;
    /// Currently active wavebuffer index
    u32 wave_buffer_index;
    /// Array of which wavebuffers are currently valid

    std::array<bool, MaxWaveBuffers> wave_buffer_valid;
    /// Number of wavebuffers consumed, given back to the game
    u32 wave_buffers_consumed;
    /// History of samples, used for rate conversion

    std::array<s16, MaxWaveBuffers * 2> sample_history;
    /// Current read fraction, used for resampling
    Common::FixedPoint<49, 15> fraction;
    /// Current adpcm context
    AdpcmContext adpcm_context;
    /// Current biquad states, used when filtering

    std::array<std::array<BiquadFilterState, MaxBiquadFilters>, MaxBiquadFilters> biquad_states;
    /// Previous samples
    std::array<s32, MaxMixBuffers> previous_samples;
    /// Unused
    u32 external_context_size;
    /// Unused
    bool external_context_enabled;
    /// Was this voice dropped?
    bool voice_dropped;
    /// Number of times the wavebuffer has looped
    s32 loop_count;
};

} // namespace AudioCore::AudioRenderer

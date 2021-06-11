// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <vector>

#include "common/common_types.h"

namespace AudioCore {

struct InterpolationState {
    static constexpr std::size_t taps{4};
    static constexpr std::size_t history_size{taps * 2 - 1};
    std::array<std::array<s16, 2>, history_size> history{};
    double position{};
    s32 fraction{};
};

/// Interpolates input signal to produce output signal.
/// @param input The signal to interpolate.
/// @param ratio Interpolation ratio.
///              ratio > 1.0 results in fewer output samples.
///              ratio < 1.0 results in more output samples.
/// @returns Output signal.
std::vector<s16> Interpolate(InterpolationState& state, std::vector<s16> input, double ratio);

/// Interpolates input signal to produce output signal.
/// @param input The signal to interpolate.
/// @param input_rate The sample rate of input.
/// @param output_rate The desired sample rate of the output.
/// @returns Output signal.
inline std::vector<s16> Interpolate(InterpolationState& state, std::vector<s16> input,
                                    u32 input_rate, u32 output_rate) {
    const double ratio = static_cast<double>(input_rate) / static_cast<double>(output_rate);
    return Interpolate(state, std::move(input), ratio);
}

/// Nintendo Switchs DSP resampling algorithm. Based on a single channel
void Resample(s32* output, const s32* input, s32 pitch, s32& fraction, std::size_t sample_count);

} // namespace AudioCore

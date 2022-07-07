// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "audio_core/renderer/command/icommand.h"
#include "audio_core/renderer/voice/voice_info.h"
#include "audio_core/renderer/voice/voice_state.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
namespace ADSP {
class CommandListProcessor;
}

/**
 * AudioRenderer command for applying a biquad filter to the input mix buffer, saving the results to
 * the output mix buffer.
 */
struct BiquadFilterCommand : ICommand {
    /**
     * Print this command's information to a string.
     *
     * @param processor - The CommandListProcessor processing this command.
     * @param string    - The string to print into.
     */
    void Dump(const ADSP::CommandListProcessor& processor, std::string& string) override;

    /**
     * Process this command.
     *
     * @param processor - The CommandListProcessor processing this command.
     */
    void Process(const ADSP::CommandListProcessor& processor) override;

    /**
     * Verify this command's data is valid.
     *
     * @param processor - The CommandListProcessor processing this command.
     * @return True if the command is valid, otherwise false.
     */
    bool Verify(const ADSP::CommandListProcessor& processor) override;

    /// Input mix buffer index
    s16 input;
    /// Output mix buffer index
    s16 output;
    /// Input parameters for biquad
    VoiceInfo::BiquadFilterParameter biquad;
    /// Biquad state, updated each call
    CpuAddr state;
    /// If true, reset the state
    bool needs_init;
    /// If true, use float processing rather than int
    bool use_float_processing;
};

/**
 * Biquad filter float implementation.
 *
 * @param output       - Output container for filtered samples.
 * @param input        - Input container for samples to be filtered.
 * @param b            - Feedforward coefficients.
 * @param a            - Feedback coefficients.
 * @param state        - State to track previous samples.
 * @param sample_count - Number of samples to process.
 */
void ApplyBiquadFilterFloat(std::span<s32> output, std::span<const s32> input,
                            std::array<s16, 3>& b, std::array<s16, 2>& a,
                            VoiceState::BiquadFilterState& state, const u32 sample_count);

} // namespace AudioCore::AudioRenderer

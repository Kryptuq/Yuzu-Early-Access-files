// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <string>

#include "audio_core/renderer/command/icommand.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
namespace ADSP {
class CommandListProcessor;
}

/**
 * AudioRenderer command for mixing an input mix buffer to an output mix buffer, with a volume
 * applied to the input, and volume ramping to smooth out the transition.
 */
struct MixRampCommand : ICommand {
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

    /// Fixed point precision
    u8 precision;
    /// Input mix buffer index
    s16 input_index;
    /// Output mix buffer index
    s16 output_index;
    /// Previous mix volume
    f32 prev_volume;
    /// Current mix volume
    f32 volume;
    /// Pointer to the previous sample buffer, used for depopping
    CpuAddr previous_sample;
};

/**
 * Mix input mix buffer into output mix buffer, with volume applied to the input.
 * @tparam Q           - Number of bits for fixed point operations.
 * @param output       - Output mix buffer.
 * @param input        - Input mix buffer.
 * @param volume_      - Volume applied to the input.
 * @param ramp_        - Ramp applied to volume every sample.
 * @param sample_count - Number of samples to process.
 * @return The final gained input sample, used for depopping.
 */
template <size_t Q>
s32 ApplyMixRamp(std::span<s32> output, std::span<const s32> input, f32 volume_, f32 ramp_,
                 u32 sample_count);

} // namespace AudioCore::AudioRenderer

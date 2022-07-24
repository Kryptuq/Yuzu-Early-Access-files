// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "audio_core/renderer/command/icommand.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
namespace ADSP {
class CommandListProcessor;
}

/**
 * AudioRenderer command for upsampling a mix buffer to 48Khz.
 * Input must be 8Khz, 16Khz or 32Khz, and output will be 48Khz.
 */
struct UpsampleCommand : ICommand {
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

    /// Pointer to the output samples buffer.
    CpuAddr samples_buffer;
    /// Pointer to input mix buffer indexes.
    CpuAddr inputs;
    /// Number of input mix buffers.
    u32 buffer_count;
    /// Unknown, unused.
    u32 unk_20;
    /// Source data sample count.
    u32 source_sample_count;
    /// Source data sample rate.
    u32 source_sample_rate;
    /// Pointer to the upsampler info for this command.
    CpuAddr upsampler_info;
};

} // namespace AudioCore::AudioRenderer

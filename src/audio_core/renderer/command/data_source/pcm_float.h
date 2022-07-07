// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "audio_core/common/wave_buffer.h"
#include "audio_core/renderer/command/icommand.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
namespace ADSP {
class CommandListProcessor;
}

/**
 * AudioRenderer command to decode PCM float-encoded version 1 wavebuffers
 * into the output_index mix buffer.
 */
struct PcmFloatDataSourceVersion1Command : ICommand {
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

    /// Quality used for sample rate conversion
    SrcQuality src_quality;
    /// Mix buffer index for decoded samples
    s16 output_index;
    /// Flags to control decoding (see AudioCore::AudioRenderer::VoiceInfo::Flags)
    u16 flags;
    /// Wavebuffer sample rate
    u32 sample_rate;
    /// Pitch used for sample rate conversion
    f32 pitch;
    /// Target channel to read within the wavebuffer
    s8 channel_index;
    /// Number of channels within the wavebuffer
    s8 channel_count;
    /// Wavebuffers containing the wavebuffer address, context address, looping information etc
    std::array<WaveBufferVersion2, MaxWaveBuffers> wave_buffers;
    /// Voice state, updated each call and written back to game
    CpuAddr voice_state;
};

/**
 * AudioRenderer command to decode PCM float-encoded version 2 wavebuffers
 * into the output_index mix buffer.
 */
struct PcmFloatDataSourceVersion2Command : ICommand {
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

    /// Quality used for sample rate conversion
    SrcQuality src_quality;
    /// Mix buffer index for decoded samples
    s16 output_index;
    /// Flags to control decoding (see AudioCore::AudioRenderer::VoiceInfo::Flags)
    u16 flags;
    /// Wavebuffer sample rate
    u32 sample_rate;
    /// Pitch used for sample rate conversion
    f32 pitch;
    /// Target channel to read within the wavebuffer
    s8 channel_index;
    /// Number of channels within the wavebuffer
    s8 channel_count;
    /// Wavebuffers containing the wavebuffer address, context address, looping information etc
    std::array<WaveBufferVersion2, MaxWaveBuffers> wave_buffers;
    /// Voice state, updated each call and written back to game
    CpuAddr voice_state;
};

} // namespace AudioCore::AudioRenderer

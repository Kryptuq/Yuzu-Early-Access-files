// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <span>

#include "audio_core/common/common.h"
#include "audio_core/common/wave_buffer.h"
#include "audio_core/renderer/voice/voice_state.h"
#include "common/common_types.h"

namespace Core::Memory {
class Memory;
}

namespace AudioCore::AudioRenderer {

struct DecodeFromWaveBuffersArgs {
    SampleFormat sample_format;
    std::span<s32> output;
    VoiceState* voice_state;
    std::span<WaveBufferVersion2> wave_buffers;
    s8 channel;
    s8 channel_count;
    SrcQuality src_quality;
    f32 pitch;
    u32 source_sample_rate;
    u32 target_sample_rate;
    u32 sample_count;
    CpuAddr data_address;
    u64 data_size;
    bool IsVoicePlayedSampleCountResetAtLoopPointSupported;
    bool IsVoicePitchAndSrcSkippedSupported;
};

struct DecodeArg {
    CpuAddr buffer;
    u64 buffer_size;
    u32 start_offset;
    u32 end_offset;
    s8 channel_count;
    std::array<s16, 16> coefficients;
    VoiceState::AdpcmContext* adpcm_context;
    s8 target_channel;
    u32 offset;
    u32 samples_to_read;
};

/**
 * Decode wavebuffers according to the given args.
 *
 * @param memory - Core memory to read data from.
 * @param args - The wavebuffer data, and information for how to decode it.
 */
void DecodeFromWaveBuffers(Core::Memory::Memory& memory, const DecodeFromWaveBuffersArgs& args);

} // namespace AudioCore::AudioRenderer

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/data_source/decode.h"
#include "audio_core/renderer/command/data_source/pcm_float.h"

namespace AudioCore::AudioRenderer {

void PcmFloatDataSourceVersion1Command::Dump(const ADSP::CommandListProcessor& processor,
                                             std::string& string) {
    string +=
        fmt::format("PcmFloatDataSourceVersion1Command\n\toutput_index {:02X} channel {} "
                    "channel count {} source sample rate {} target sample rate {} src quality {}\n",
                    output_index, channel_index, channel_count, sample_rate,
                    processor.target_sample_rate, src_quality);
}

void PcmFloatDataSourceVersion1Command::Process(const ADSP::CommandListProcessor& processor) {
    auto out_buffer = processor.mix_buffers.subspan(output_index * processor.sample_count,
                                                    processor.sample_count);

    DecodeFromWaveBuffersArgs args{
        .sample_format{SampleFormat::PcmFloat},
        .output{out_buffer},
        .voice_state{reinterpret_cast<VoiceState*>(voice_state)},
        .wave_buffers{wave_buffers},
        .channel{channel_index},
        .channel_count{channel_count},
        .src_quality{src_quality},
        .pitch{pitch},
        .source_sample_rate{sample_rate},
        .target_sample_rate{processor.target_sample_rate},
        .sample_count{processor.sample_count},
        .data_address{0},
        .data_size{0},
        .IsVoicePlayedSampleCountResetAtLoopPointSupported{(flags & 1) != 0},
        .IsVoicePitchAndSrcSkippedSupported{(flags & 2) != 0},
    };

    DecodeFromWaveBuffers(*processor.memory, args);
}

bool PcmFloatDataSourceVersion1Command::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

void PcmFloatDataSourceVersion2Command::Dump(const ADSP::CommandListProcessor& processor,
                                             std::string& string) {
    string +=
        fmt::format("PcmFloatDataSourceVersion2Command\n\toutput_index {:02X} channel {} "
                    "channel count {} source sample rate {} target sample rate {} src quality {}\n",
                    output_index, channel_index, channel_count, sample_rate,
                    processor.target_sample_rate, src_quality);
}

void PcmFloatDataSourceVersion2Command::Process(const ADSP::CommandListProcessor& processor) {
    auto out_buffer = processor.mix_buffers.subspan(output_index * processor.sample_count,
                                                    processor.sample_count);

    DecodeFromWaveBuffersArgs args{
        .sample_format{SampleFormat::PcmFloat},
        .output{out_buffer},
        .voice_state{reinterpret_cast<VoiceState*>(voice_state)},
        .wave_buffers{wave_buffers},
        .channel{channel_index},
        .channel_count{channel_count},
        .src_quality{src_quality},
        .pitch{pitch},
        .source_sample_rate{sample_rate},
        .target_sample_rate{processor.target_sample_rate},
        .sample_count{processor.sample_count},
        .data_address{0},
        .data_size{0},
        .IsVoicePlayedSampleCountResetAtLoopPointSupported{(flags & 1) != 0},
        .IsVoicePitchAndSrcSkippedSupported{(flags & 2) != 0},
    };

    DecodeFromWaveBuffers(*processor.memory, args);
}

bool PcmFloatDataSourceVersion2Command::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer

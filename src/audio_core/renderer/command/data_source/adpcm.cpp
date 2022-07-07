// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <span>

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/data_source/adpcm.h"
#include "audio_core/renderer/command/data_source/decode.h"

namespace AudioCore::AudioRenderer {

void AdpcmDataSourceVersion1Command::Dump(const ADSP::CommandListProcessor& processor,
                                          std::string& string) {
    string += fmt::format("AdpcmDataSourceVersion1Command\n\toutput_index {:02X} source sample "
                          "rate {} target sample rate {} src quality {}\n",
                          output_index, sample_rate, processor.target_sample_rate, src_quality);
}

void AdpcmDataSourceVersion1Command::Process(const ADSP::CommandListProcessor& processor) {
    auto out_buffer{processor.mix_buffers.subspan(output_index * processor.sample_count,
                                                  processor.sample_count)};

    DecodeFromWaveBuffersArgs args{
        .sample_format{SampleFormat::Adpcm},
        .output{out_buffer},
        .voice_state{reinterpret_cast<VoiceState*>(voice_state)},
        .wave_buffers{wave_buffers},
        .channel{0},
        .channel_count{1},
        .src_quality{src_quality},
        .pitch{pitch},
        .source_sample_rate{sample_rate},
        .target_sample_rate{processor.target_sample_rate},
        .sample_count{processor.sample_count},
        .data_address{data_address},
        .data_size{data_size},
        .IsVoicePlayedSampleCountResetAtLoopPointSupported{(flags & 1) != 0},
        .IsVoicePitchAndSrcSkippedSupported{(flags & 2) != 0},
    };

    DecodeFromWaveBuffers(*processor.memory, args);
}

bool AdpcmDataSourceVersion1Command::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

void AdpcmDataSourceVersion2Command::Dump(const ADSP::CommandListProcessor& processor,
                                          std::string& string) {
    string += fmt::format("AdpcmDataSourceVersion2Command\n\toutput_index {:02X} source sample "
                          "rate {} target sample rate {} src quality {}\n",
                          output_index, sample_rate, processor.target_sample_rate, src_quality);
}

void AdpcmDataSourceVersion2Command::Process(const ADSP::CommandListProcessor& processor) {
    auto out_buffer{processor.mix_buffers.subspan(output_index * processor.sample_count,
                                                  processor.sample_count)};

    DecodeFromWaveBuffersArgs args{
        .sample_format{SampleFormat::Adpcm},
        .output{out_buffer},
        .voice_state{reinterpret_cast<VoiceState*>(voice_state)},
        .wave_buffers{wave_buffers},
        .channel{0},
        .channel_count{1},
        .src_quality{src_quality},
        .pitch{pitch},
        .source_sample_rate{sample_rate},
        .target_sample_rate{processor.target_sample_rate},
        .sample_count{processor.sample_count},
        .data_address{data_address},
        .data_size{data_size},
        .IsVoicePlayedSampleCountResetAtLoopPointSupported{(flags & 1) != 0},
        .IsVoicePitchAndSrcSkippedSupported{(flags & 2) != 0},
    };

    DecodeFromWaveBuffers(*processor.memory, args);
}

bool AdpcmDataSourceVersion2Command::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer

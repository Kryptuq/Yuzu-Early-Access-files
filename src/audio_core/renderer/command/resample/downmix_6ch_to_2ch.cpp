// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/resample/downmix_6ch_to_2ch.h"

namespace AudioCore::AudioRenderer {

void DownMix6chTo2chCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                                  std::string& string) {
    string += fmt::format("DownMix6chTo2chCommand\n\tinputs:  ");
    for (u32 i = 0; i < MaxChannels; i++) {
        string += fmt::format("{:02X}, ", inputs[i]);
    }
    string += "\n\toutputs: ";
    for (u32 i = 0; i < MaxChannels; i++) {
        string += fmt::format("{:02X}, ", outputs[i]);
    }
    string += "\n";
}

void DownMix6chTo2chCommand::Process(const ADSP::CommandListProcessor& processor) {
    auto in_front_left{
        processor.mix_buffers.subspan(inputs[0] * processor.sample_count, processor.sample_count)};
    auto in_front_right{
        processor.mix_buffers.subspan(inputs[1] * processor.sample_count, processor.sample_count)};
    auto in_center{
        processor.mix_buffers.subspan(inputs[2] * processor.sample_count, processor.sample_count)};
    auto in_lfe{
        processor.mix_buffers.subspan(inputs[3] * processor.sample_count, processor.sample_count)};
    auto in_back_left{
        processor.mix_buffers.subspan(inputs[4] * processor.sample_count, processor.sample_count)};
    auto in_back_right{
        processor.mix_buffers.subspan(inputs[5] * processor.sample_count, processor.sample_count)};

    auto out_front_left{
        processor.mix_buffers.subspan(outputs[0] * processor.sample_count, processor.sample_count)};
    auto out_front_right{
        processor.mix_buffers.subspan(outputs[1] * processor.sample_count, processor.sample_count)};
    auto out_center{
        processor.mix_buffers.subspan(outputs[2] * processor.sample_count, processor.sample_count)};
    auto out_lfe{
        processor.mix_buffers.subspan(outputs[3] * processor.sample_count, processor.sample_count)};
    auto out_back_left{
        processor.mix_buffers.subspan(outputs[4] * processor.sample_count, processor.sample_count)};
    auto out_back_right{
        processor.mix_buffers.subspan(outputs[5] * processor.sample_count, processor.sample_count)};

    for (u32 i = 0; i < processor.sample_count; i++) {
        const auto left_sample{(in_front_left[i] * down_mix_coeff[0] +
                                in_center[i] * down_mix_coeff[1] + in_lfe[i] * down_mix_coeff[2] +
                                in_back_left[i] * down_mix_coeff[3])
                                   .to_int()};

        const auto right_sample{(in_front_right[i] * down_mix_coeff[0] +
                                 in_center[i] * down_mix_coeff[1] + in_lfe[i] * down_mix_coeff[2] +
                                 in_back_right[i] * down_mix_coeff[3])
                                    .to_int()};

        out_front_left[i] = left_sample;
        out_front_right[i] = right_sample;
    }

    std::memset(out_center.data(), 0, out_center.size_bytes());
    std::memset(out_lfe.data(), 0, out_lfe.size_bytes());
    std::memset(out_back_left.data(), 0, out_back_left.size_bytes());
    std::memset(out_back_right.data(), 0, out_back_right.size_bytes());
}

bool DownMix6chTo2chCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer

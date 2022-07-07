// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/mix/mix_ramp.h"
#include "audio_core/renderer/command/mix/mix_ramp_grouped.h"

namespace AudioCore::AudioRenderer {

void MixRampGroupedCommand::Dump(const ADSP::CommandListProcessor& processor, std::string& string) {
    string += "MixRampGroupedCommand";
    for (u32 i = 0; i < buffer_count; i++) {
        string += fmt::format("\n\t{}", i);
        const auto ramp{(volumes[i] - prev_volumes[i]) / static_cast<f32>(processor.sample_count)};
        string += fmt::format("\n\t\tinput {:02X}", inputs[i]);
        string += fmt::format("\n\t\toutput {:02X}", outputs[i]);
        string += fmt::format("\n\t\tvolume {:.8f}", volumes[i]);
        string += fmt::format("\n\t\tprev_volume {:.8f}", prev_volumes[i]);
        string += fmt::format("\n\t\tramp {:.8f}", ramp);
        string += "\n";
    }
}

void MixRampGroupedCommand::Process(const ADSP::CommandListProcessor& processor) {
    std::span<s32> prev_samples = {reinterpret_cast<s32*>(previous_samples), MaxMixBuffers};

    for (u32 i = 0; i < buffer_count; i++) {
        auto last_sample{0};
        if (prev_volumes[i] != 0.0f || volumes[i] != 0.0f) {
            const auto output{processor.mix_buffers.subspan(outputs[i] * processor.sample_count,
                                                            processor.sample_count)};
            const auto input{processor.mix_buffers.subspan(inputs[i] * processor.sample_count,
                                                           processor.sample_count)};
            const auto ramp{(volumes[i] - prev_volumes[i]) /
                            static_cast<f32>(processor.sample_count)};

            if (prev_volumes[i] == 0.0f && ramp == 0.0f) {
                prev_samples[i] = 0;
                continue;
            }

            switch (precision) {
            case 15:
                last_sample =
                    ApplyMixRamp<15>(output, input, prev_volumes[i], ramp, processor.sample_count);
                break;
            case 23:
                last_sample =
                    ApplyMixRamp<23>(output, input, prev_volumes[i], ramp, processor.sample_count);
                break;
            default:
                LOG_ERROR(Service_Audio, "Invalid precision {}", precision);
                break;
            }
        }

        prev_samples[i] = last_sample;
    }
}

bool MixRampGroupedCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer

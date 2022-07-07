// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/mix/volume_ramp.h"
#include "common/fixed_point.h"

namespace AudioCore::AudioRenderer {
/**
 * Apply volume with ramping to the input mix buffer, saving to the output buffer.
 *
 * @tparam Q           - Number of bits for fixed point operations.
 * @param output       - Output mix buffers.
 * @param input        - Input mix buffers.
 * @param volume       - Volume applied to the input.
 * @param ramp         - Ramp applied to volume every sample.
 * @param sample_count - Number of samples to process.
 */
template <size_t Q>
static void ApplyLinearEnvelopeGain(std::span<s32> output, std::span<const s32> input,
                                    const f32 volume, const f32 ramp_, const u32 sample_count) {
    if (volume == 0.0f && ramp_ == 0.0f) {
        std::memset(output.data(), 0, output.size_bytes());
    } else if (volume == 1.0f && ramp_ == 0.0f) {
        std::memcpy(output.data(), input.data(), output.size_bytes());
    } else if (ramp_ == 0.0f) {
        const Common::FixedPoint<64 - Q, Q> gain{volume};
        for (u32 i = 0; i < sample_count; i++) {
            output[i] = (input[i] * gain).to_int();
        }
    } else {
        Common::FixedPoint<64 - Q, Q> gain{volume};
        const Common::FixedPoint<64 - Q, Q> ramp{ramp_};
        for (u32 i = 0; i < sample_count; i++) {
            output[i] = (input[i] * gain).to_int();
            gain += ramp;
        }
    }
}

void VolumeRampCommand::Dump(const ADSP::CommandListProcessor& processor, std::string& string) {
    const auto ramp{(volume - prev_volume) / static_cast<f32>(processor.sample_count)};
    string += fmt::format("VolumeRampCommand");
    string += fmt::format("\n\tinput {:02X}", input_index);
    string += fmt::format("\n\toutput {:02X}", output_index);
    string += fmt::format("\n\tvolume {:.8f}", volume);
    string += fmt::format("\n\tprev_volume {:.8f}", prev_volume);
    string += fmt::format("\n\tramp {:.8f}", ramp);
    string += "\n";
}

void VolumeRampCommand::Process(const ADSP::CommandListProcessor& processor) {
    auto output{processor.mix_buffers.subspan(output_index * processor.sample_count,
                                              processor.sample_count)};
    auto input{processor.mix_buffers.subspan(input_index * processor.sample_count,
                                             processor.sample_count)};
    const auto ramp{(volume - prev_volume) / static_cast<f32>(processor.sample_count)};

    // If input and output buffers are the same, and the volume is 1.0f, and there's no ramping,
    // this won't do anything, so just skip.
    if (input_index == output_index && prev_volume == 1.0f && ramp == 0.0f) {
        return;
    }

    switch (precision) {
    case 15:
        ApplyLinearEnvelopeGain<15>(output, input, prev_volume, ramp, processor.sample_count);
        break;

    case 23:
        ApplyLinearEnvelopeGain<23>(output, input, prev_volume, ramp, processor.sample_count);
        break;

    default:
        LOG_ERROR(Service_Audio, "Invalid precision {}", precision);
        break;
    }
}

bool VolumeRampCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer

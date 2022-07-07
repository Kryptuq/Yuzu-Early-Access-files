// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/mix/mix_ramp.h"
#include "common/fixed_point.h"
#include "common/logging/log.h"

namespace AudioCore::AudioRenderer {
/**
 * Mix input mix buffer into output mix buffer, with volume applied to the input.
 *
 * @tparam Q           - Number of bits for fixed point operations.
 * @param output       - Output mix buffer.
 * @param input        - Input mix buffer.
 * @param volume       - Volume applied to the input.
 * @param ramp         - Ramp applied to volume every sample.
 * @param sample_count - Number of samples to process.
 * @return The final gained input sample, used for depopping.
 */
template <size_t Q>
s32 ApplyMixRamp(std::span<s32> output, std::span<const s32> input, const f32 volume_,
                 const f32 ramp_, const u32 sample_count) {
    Common::FixedPoint<64 - Q, Q> volume{volume_};
    Common::FixedPoint<64 - Q, Q> sample{0};

    if (ramp_ == 0.0f) {
        for (u32 i = 0; i < sample_count; i++) {
            sample = input[i] * volume;
            output[i] = (output[i] + sample).to_int();
        }
    } else {
        Common::FixedPoint<64 - Q, Q> ramp{ramp_};
        for (u32 i = 0; i < sample_count; i++) {
            sample = input[i] * volume;
            output[i] = (output[i] + sample).to_int();
            volume += ramp;
        }
    }
    return sample.to_int();
}

template s32 ApplyMixRamp<15>(std::span<s32>, std::span<const s32>, const f32, const f32,
                              const u32);
template s32 ApplyMixRamp<23>(std::span<s32>, std::span<const s32>, const f32, const f32,
                              const u32);

void MixRampCommand::Dump(const ADSP::CommandListProcessor& processor, std::string& string) {
    const auto ramp{(volume - prev_volume) / static_cast<f32>(processor.sample_count)};
    string += fmt::format("MixRampCommand");
    string += fmt::format("\n\tinput {:02X}", input_index);
    string += fmt::format("\n\toutput {:02X}", output_index);
    string += fmt::format("\n\tvolume {:.8f}", volume);
    string += fmt::format("\n\tprev_volume {:.8f}", prev_volume);
    string += fmt::format("\n\tramp {:.8f}", ramp);
    string += "\n";
}

void MixRampCommand::Process(const ADSP::CommandListProcessor& processor) {
    auto output{processor.mix_buffers.subspan(output_index * processor.sample_count,
                                              processor.sample_count)};
    auto input{processor.mix_buffers.subspan(input_index * processor.sample_count,
                                             processor.sample_count)};
    const auto ramp{(volume - prev_volume) / static_cast<f32>(processor.sample_count)};
    auto prev_sample_ptr{reinterpret_cast<s32*>(previous_sample)};

    // If previous volume and ramp are both 0, nothing will be added to the output, so just skip.
    if (prev_volume == 0.0f && ramp == 0.0f) {
        *prev_sample_ptr = 0;
        return;
    }

    switch (precision) {
    case 15:
        *prev_sample_ptr =
            ApplyMixRamp<15>(output, input, prev_volume, ramp, processor.sample_count);
        break;

    case 23:
        *prev_sample_ptr =
            ApplyMixRamp<23>(output, input, prev_volume, ramp, processor.sample_count);
        break;

    default:
        LOG_ERROR(Service_Audio, "Invalid precision {}", precision);
        break;
    }
}

bool MixRampCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer

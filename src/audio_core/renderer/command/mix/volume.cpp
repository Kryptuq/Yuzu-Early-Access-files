// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/mix/volume.h"
#include "common/fixed_point.h"
#include "common/logging/log.h"

namespace AudioCore::AudioRenderer {
/**
 * Apply volume to the input mix buffer, saving to the output buffer.
 *
 * @tparam Q           - Number of bits for fixed point operations.
 * @param output       - Output mix buffer.
 * @param input        - Input mix buffer.
 * @param volume       - Volume applied to the input.
 * @param sample_count - Number of samples to process.
 */
template <size_t Q>
static void ApplyUniformGain(std::span<s32> output, std::span<const s32> input, const f32 volume,
                             const u32 sample_count) {
    if (volume == 1.0f) {
        std::memcpy(output.data(), input.data(), input.size_bytes());
    } else {
        const Common::FixedPoint<64 - Q, Q> gain{volume};
        for (u32 i = 0; i < sample_count; i++) {
            output[i] = (input[i] * gain).to_int();
        }
    }
}

void VolumeCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                         std::string& string) {
    string += fmt::format("VolumeCommand");
    string += fmt::format("\n\tinput {:02X}", input_index);
    string += fmt::format("\n\toutput {:02X}", output_index);
    string += fmt::format("\n\tvolume {:.8f}", volume);
    string += "\n";
}

void VolumeCommand::Process(const ADSP::CommandListProcessor& processor) {
    // If input and output buffers are the same, and the volume is 1.0f, this won't do
    // anything, so just skip.
    if (input_index == output_index && volume == 1.0f) {
        return;
    }

    auto output{processor.mix_buffers.subspan(output_index * processor.sample_count,
                                              processor.sample_count)};
    auto input{processor.mix_buffers.subspan(input_index * processor.sample_count,
                                             processor.sample_count)};

    switch (precision) {
    case 15:
        ApplyUniformGain<15>(output, input, volume, processor.sample_count);
        break;

    case 23:
        ApplyUniformGain<23>(output, input, volume, processor.sample_count);
        break;

    default:
        LOG_ERROR(Service_Audio, "Invalid precision {}", precision);
        break;
    }
}

bool VolumeCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer

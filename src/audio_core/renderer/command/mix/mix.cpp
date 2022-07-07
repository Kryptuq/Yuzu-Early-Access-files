// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <limits>
#include <span>

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/mix/mix.h"
#include "common/fixed_point.h"

namespace AudioCore::AudioRenderer {
/**
 * Mix input mix buffer into output mix buffer, with volume applied to the input.
 *
 * @tparam Q           - Number of bits for fixed point operations.
 * @param output       - Output mix buffer.
 * @param input        - Input mix buffer.
 * @param volume       - Volume applied to the input.
 * @param sample_count - Number of samples to process.
 */
template <size_t Q>
static void ApplyMix(std::span<s32> output, std::span<const s32> input, const f32 volume_,
                     const u32 sample_count) {
    const Common::FixedPoint<64 - Q, Q> volume{volume_};
    for (u32 i = 0; i < sample_count; i++) {
        output[i] = (output[i] + input[i] * volume).to_int();
    }
}

void MixCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                      std::string& string) {
    string += fmt::format("MixCommand");
    string += fmt::format("\n\tinput {:02X}", input_index);
    string += fmt::format("\n\toutput {:02X}", output_index);
    string += fmt::format("\n\tvolume {:.8f}", volume);
    string += "\n";
}

void MixCommand::Process(const ADSP::CommandListProcessor& processor) {
    auto output{processor.mix_buffers.subspan(output_index * processor.sample_count,
                                              processor.sample_count)};
    auto input{processor.mix_buffers.subspan(input_index * processor.sample_count,
                                             processor.sample_count)};

    // If volume is 0, nothing will be added to the output, so just skip.
    if (volume == 0.0f) {
        return;
    }

    switch (precision) {
    case 15:
        ApplyMix<15>(output, input, volume, processor.sample_count);
        break;

    case 23:
        ApplyMix<23>(output, input, volume, processor.sample_count);
        break;

    default:
        LOG_ERROR(Service_Audio, "Invalid precision {}", precision);
        break;
    }
}

bool MixCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer

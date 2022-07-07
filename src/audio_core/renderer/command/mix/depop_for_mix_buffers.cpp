// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/common/common.h"
#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/mix/depop_for_mix_buffers.h"

namespace AudioCore::AudioRenderer {
/**
 * Apply depopping. Add the depopped sample to each incoming new sample, decaying it each time
 * according to decay.
 *
 * @param output - Output buffer to be depopped.
 * @param depop_sample - Depopped sample to apply to output samples.
 * @param decay_ - Amount to decay the depopped sample for every output sample.
 * @param sample_count - Samples to process.
 * @return Final decayed depop sample.
 */
static s32 ApplyDepopMix(std::span<s32> output, const s32 depop_sample,
                         Common::FixedPoint<49, 15>& decay_, const u32 sample_count) {
    auto sample{std::abs(depop_sample)};
    auto decay{decay_.to_raw()};

    if (depop_sample <= 0) {
        for (u32 i = 0; i < sample_count; i++) {
            sample = static_cast<s32>((static_cast<s64>(sample) * decay) >> 15);
            output[i] -= sample;
        }
        return -sample;
    } else {
        for (u32 i = 0; i < sample_count; i++) {
            sample = static_cast<s32>((static_cast<s64>(sample) * decay) >> 15);
            output[i] += sample;
        }
        return sample;
    }
}

void DepopForMixBuffersCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                                     std::string& string) {
    string += fmt::format("DepopForMixBuffersCommand\n\tinput {:02X} count {} decay {}\n", input,
                          count, decay.to_float());
}

void DepopForMixBuffersCommand::Process(const ADSP::CommandListProcessor& processor) {
    std::span<s32> depop_buff{reinterpret_cast<s32*>(depop_buffer), MaxMixBuffers};
    auto end_index{std::min(processor.buffer_count, input + count)};
    for (u32 index = input; index < end_index; index++) {
        const auto depop_sample{depop_buff[index]};
        if (depop_sample != 0) {
            auto input_buffer{processor.mix_buffers.subspan(index * processor.sample_count,
                                                            processor.sample_count)};
            depop_buff[index] =
                ApplyDepopMix(input_buffer, depop_sample, decay, processor.sample_count);
        }
    }
}

bool DepopForMixBuffersCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer

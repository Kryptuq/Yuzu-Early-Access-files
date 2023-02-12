// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/sink/circular_buffer.h"
#include "core/memory.h"

namespace AudioCore::AudioRenderer {

void CircularBufferSinkCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                                     std::string& string) {
    string += fmt::format(
        "CircularBufferSinkCommand\n\tinput_count {} ring size {:04X} ring pos {:04X}\n\tinputs: ",
        input_count, size, pos);
    for (u32 i = 0; i < input_count; i++) {
        string += fmt::format("{:02X}, ", inputs[i]);
    }
    string += "\n";
}

void CircularBufferSinkCommand::Process(const ADSP::CommandListProcessor& processor) {
    constexpr s32 min{std::numeric_limits<s16>::min()};
    constexpr s32 max{std::numeric_limits<s16>::max()};

    std::vector<s16> output(processor.sample_count);
    for (u32 channel = 0; channel < input_count; channel++) {
        auto input{processor.mix_buffers.subspan(inputs[channel] * processor.sample_count,
                                                 processor.sample_count)};
        for (u32 sample_index = 0; sample_index < processor.sample_count; sample_index++) {
            output[sample_index] = static_cast<s16>(std::clamp(input[sample_index], min, max));
        }

        processor.memory->WriteBlockUnsafe(address + pos, output.data(),
                                           output.size() * sizeof(s16));
        pos += static_cast<u32>(processor.sample_count * sizeof(s16));
        if (pos >= size) {
            pos = 0;
        }
    }
}

bool CircularBufferSinkCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer

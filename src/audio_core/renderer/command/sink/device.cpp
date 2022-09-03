// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/sink/device.h"
#include "audio_core/sink/sink.h"

namespace AudioCore::AudioRenderer {

void DeviceSinkCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                             std::string& string) {
    string += fmt::format("DeviceSinkCommand\n\t{} session {} input_count {}\n\tinputs: ",
                          std::string_view(name), session_id, input_count);
    for (u32 i = 0; i < input_count; i++) {
        string += fmt::format("{:02X}, ", inputs[i]);
    }
    string += "\n";
}

void DeviceSinkCommand::Process(const ADSP::CommandListProcessor& processor) {
    constexpr s32 min = std::numeric_limits<s16>::min();
    constexpr s32 max = std::numeric_limits<s16>::max();

    auto stream{processor.GetOutputSinkStream()};
    stream->SetSystemChannels(input_count);

    Sink::SinkBuffer out_buffer{
        .frames{TargetSampleCount},
        .frames_played{0},
        .tag{0},
        .consumed{false},
    };

    std::vector<s16> samples(out_buffer.frames * input_count);

    for (u32 channel = 0; channel < input_count; channel++) {
        const auto offset{inputs[channel] * out_buffer.frames};

        for (u32 index = 0; index < out_buffer.frames; index++) {
            samples[index * input_count + channel] =
                static_cast<s16>(std::clamp(sample_buffer[offset + index], min, max));
        }
    }

    out_buffer.tag = reinterpret_cast<u64>(samples.data());
    stream->AppendBuffer(out_buffer, samples);

    if (stream->IsPaused()) {
        stream->Start();
    }
}

bool DeviceSinkCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer

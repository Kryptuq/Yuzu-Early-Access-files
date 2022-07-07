// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string>

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/mix/clear_mix.h"

namespace AudioCore::AudioRenderer {

void ClearMixBufferCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                                 std::string& string) {
    string += fmt::format("ClearMixBufferCommand\n");
}

void ClearMixBufferCommand::Process(const ADSP::CommandListProcessor& processor) {
    memset(processor.mix_buffers.data(), 0, processor.mix_buffers.size_bytes());
}

bool ClearMixBufferCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer

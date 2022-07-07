// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "audio_core/renderer/command/icommand.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
namespace ADSP {
class CommandListProcessor;
}

/**
 * AudioRenderer command to read and write an auxiliary buffer, writing the input mix buffer to game
 * memory, and reading into the output buffer from game memory.
 */
struct AuxCommand : ICommand {
    /**
     * Print this command's information to a string.
     *
     * @param processor - The CommandListProcessor processing this command.
     * @param string    - The string to print into.
     */
    void Dump(const ADSP::CommandListProcessor& processor, std::string& string) override;

    /**
     * Process this command.
     *
     * @param processor - The CommandListProcessor processing this command.
     */
    void Process(const ADSP::CommandListProcessor& processor) override;

    /**
     * Verify this command's data is valid.
     *
     * @param processor - The CommandListProcessor processing this command.
     * @return True if the command is valid, otherwise false.
     */
    bool Verify(const ADSP::CommandListProcessor& processor) override;

    /// Input mix buffer index
    s16 input;
    /// Output mix buffer index
    s16 output;
    /// Meta info for writing
    CpuAddr send_buffer_info;
    /// Meta info for reading
    CpuAddr return_buffer_info;
    /// Game memory write buffer
    CpuAddr send_buffer;
    /// Game memory read buffer
    CpuAddr return_buffer;
    /// Max samples to read/write
    u32 count_max;
    /// Current read/write offset
    u32 write_offset;
    /// Number of samples to update per call
    u32 update_count;
    /// is this effect enabled?
    bool effect_enabled;
};

} // namespace AudioCore::AudioRenderer

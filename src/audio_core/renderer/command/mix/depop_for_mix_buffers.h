// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "audio_core/renderer/command/icommand.h"
#include "common/common_types.h"
#include "common/fixed_point.h"

namespace AudioCore::AudioRenderer {
namespace ADSP {
class CommandListProcessor;
}

/**
 * AudioRenderer command for depopping a mix buffer.
 * Adds a cumulation of previous samples to the current mix buffer with a decay.
 */
struct DepopForMixBuffersCommand : ICommand {
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

    /// Starting input mix buffer index
    u32 input;
    /// Number of mix buffers to depop
    u32 count;
    /// Amount to decay the depop sample for each new sample
    Common::FixedPoint<49, 15> decay;
    /// Address of the depop buffer, holding the last sample for every mix buffer
    CpuAddr depop_buffer;
};

} // namespace AudioCore::AudioRenderer

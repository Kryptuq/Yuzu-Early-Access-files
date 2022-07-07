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
 * AudioRenderer command for downmixing 6 channels to 2.
 * Channels:
 *     0 - front left
 *     1 - front right
 *     2 - center
 *     3 - lfe
 *     4 - back left
 *     5 - back right
 */
struct DownMix6chTo2chCommand : ICommand {
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

    /// Input mix buffer offsets for each channel
    std::array<s16, MaxChannels> inputs;
    /// Output mix buffer offsets for each channel
    std::array<s16, MaxChannels> outputs;
    /// Coefficients used for downmixing
    std::array<Common::FixedPoint<48, 16>, 4> down_mix_coeff;
};

} // namespace AudioCore::AudioRenderer

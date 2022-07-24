// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <string>

#include "audio_core/renderer/command/icommand.h"
#include "audio_core/renderer/effect/i3dl2.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
namespace ADSP {
class CommandListProcessor;
}

/**
 * AudioRenderer command for a I3DL2Reverb effect. Apply a reverb to inputs mix buffer according to
 * the I3DL2 spec, outputs receives the results.
 */
struct I3dl2ReverbCommand : ICommand {
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
    /// Input parameters
    I3dl2ReverbInfo::ParameterVersion1 parameter;
    /// State, updated each call
    CpuAddr state;
    /// Game-supplied workbuffer (Unused)
    CpuAddr workbuffer;
    /// Is this effect enabled?
    bool effect_enabled;
};

} // namespace AudioCore::AudioRenderer

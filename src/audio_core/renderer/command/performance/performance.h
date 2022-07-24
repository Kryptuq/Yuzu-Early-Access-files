// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "audio_core/renderer/command/icommand.h"
#include "audio_core/renderer/performance/performance_entry_addresses.h"
#include "audio_core/renderer/performance/performance_manager.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
namespace ADSP {
class CommandListProcessor;
}

/**
 * AudioRenderer command for writing AudioRenderer performance metrics back to the sysmodule.
 */
struct PerformanceCommand : ICommand {
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

    /// State of the performance
    PerformanceState state;
    /// Pointers to be written
    PerformanceEntryAddresses entry_address;
};

} // namespace AudioCore::AudioRenderer

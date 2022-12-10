// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "audio_core/renderer/performance/performance_entry_addresses.h"
#include "audio_core/renderer/performance/performance_manager.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
class CommandGenerator;

/**
 * Holds entry information about performance metrics, filled in by the AudioRenderer during
 * Performance commands.
 */
class EntryAspect {
public:
    EntryAspect(CommandGenerator& command_generator, PerformanceEntryType type, s32 node_id);

    /// Command generator the command will be generated into
    CommandGenerator& command_generator;
    /// Addresses to be filled by the AudioRenderer
    PerformanceEntryAddresses performance_entry_address{};
    /// Is this detail aspect initialized?
    bool initialized{};
    /// Node id of this aspect
    s32 node_id;
};

} // namespace AudioCore::AudioRenderer

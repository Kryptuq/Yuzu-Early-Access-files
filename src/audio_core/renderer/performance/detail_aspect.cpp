// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/command/command_buffer.h"
#include "audio_core/renderer/command/command_generator.h"
#include "audio_core/renderer/performance/detail_aspect.h"

namespace AudioCore::AudioRenderer {

DetailAspect::DetailAspect(CommandGenerator& command_generator_,
                           const PerformanceEntryType entry_type, const s32 node_id_,
                           const PerformanceDetailType detail_type)
    : command_generator{command_generator_}, node_id{node_id_} {
    auto perf_manager{command_generator.GetPerformanceManager()};
    if (perf_manager != nullptr && perf_manager->IsInitialized() &&
        perf_manager->IsDetailTarget(node_id) &&
        perf_manager->GetNextEntry(performance_entry_address, detail_type, entry_type, node_id)) {
        command_generator.GeneratePerformanceCommand(node_id, PerformanceState::Start,
                                                     performance_entry_address);

        initialized = true;
    }
}

} // namespace AudioCore::AudioRenderer

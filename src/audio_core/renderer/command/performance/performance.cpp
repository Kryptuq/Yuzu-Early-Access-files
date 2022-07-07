// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/performance/performance.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/core_timing_util.h"

namespace AudioCore::AudioRenderer {

void PerformanceCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                              std::string& string) {
    string += fmt::format("PerformanceCommand\n\tstate {}\n", static_cast<u32>(state));
}

void PerformanceCommand::Process(const ADSP::CommandListProcessor& processor) {
    auto base{entry_address.translated_address};
    if (state == PerformanceState::Start) {
        auto start_time_ptr{reinterpret_cast<u32*>(base + entry_address.entry_start_time_offset)};
        *start_time_ptr = static_cast<u32>(
            Core::Timing::CyclesToUs(processor.system->CoreTiming().GetClockTicks() -
                                     processor.start_time - processor.current_processing_time)
                .count());
    } else if (state == PerformanceState::Stop) {
        auto processed_time_ptr{
            reinterpret_cast<u32*>(base + entry_address.entry_processed_time_offset)};
        auto entry_count_ptr{
            reinterpret_cast<u32*>(base + entry_address.header_entry_count_offset)};

        *processed_time_ptr = static_cast<u32>(
            Core::Timing::CyclesToUs(processor.system->CoreTiming().GetClockTicks() -
                                     processor.start_time - processor.current_processing_time)
                .count());
        (*entry_count_ptr)++;
    }
}

bool PerformanceCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer

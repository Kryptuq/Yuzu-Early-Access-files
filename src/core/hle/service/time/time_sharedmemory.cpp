// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/core_timing.h"
#include "core/hardware_properties.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/time/clock_types.h"
#include "core/hle/service/time/steady_clock_core.h"
#include "core/hle/service/time/time_sharedmemory.h"

namespace Service::Time {

static constexpr std::size_t SHARED_MEMORY_SIZE{0x1000};

SharedMemory::SharedMemory(Core::System& system_) : system(system_) {
    std::memset(system.Kernel().GetTimeSharedMem().GetPointer(), 0, SHARED_MEMORY_SIZE);
}

SharedMemory::~SharedMemory() = default;

void SharedMemory::SetupStandardSteadyClock(const Common::UUID& clock_source_id,
                                            Clock::TimeSpanType current_time_point) {
    const Clock::TimeSpanType ticks_time_span{Clock::TimeSpanType::FromTicks(
        system.CoreTiming().GetClockTicks(), Core::Hardware::CNTFREQ)};
    const Clock::SteadyClockContext context{
        static_cast<u64>(current_time_point.nanoseconds - ticks_time_span.nanoseconds),
        clock_source_id};
    StoreToLockFreeAtomicType(&GetFormat()->standard_steady_clock_timepoint, context);
}

void SharedMemory::UpdateLocalSystemClockContext(const Clock::SystemClockContext& context) {
    StoreToLockFreeAtomicType(&GetFormat()->standard_local_system_clock_context, context);
}

void SharedMemory::UpdateNetworkSystemClockContext(const Clock::SystemClockContext& context) {
    StoreToLockFreeAtomicType(&GetFormat()->standard_network_system_clock_context, context);
}

void SharedMemory::SetAutomaticCorrectionEnabled(bool is_enabled) {
    StoreToLockFreeAtomicType(
        &GetFormat()->is_standard_user_system_clock_automatic_correction_enabled, is_enabled);
}

SharedMemory::Format* SharedMemory::GetFormat() {
    return reinterpret_cast<SharedMemory::Format*>(system.Kernel().GetTimeSharedMem().GetPointer());
}

} // namespace Service::Time

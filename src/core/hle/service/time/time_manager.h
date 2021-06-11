// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "common/time_zone.h"
#include "core/file_sys/vfs_types.h"
#include "core/hle/service/time/clock_types.h"
#include "core/hle/service/time/ephemeral_network_system_clock_core.h"
#include "core/hle/service/time/standard_local_system_clock_core.h"
#include "core/hle/service/time/standard_network_system_clock_core.h"
#include "core/hle/service/time/standard_steady_clock_core.h"
#include "core/hle/service/time/standard_user_system_clock_core.h"
#include "core/hle/service/time/tick_based_steady_clock_core.h"
#include "core/hle/service/time/time_sharedmemory.h"
#include "core/hle/service/time/time_zone_content_manager.h"

namespace Service::Time {

namespace Clock {
class EphemeralNetworkSystemClockContextWriter;
class LocalSystemClockContextWriter;
class NetworkSystemClockContextWriter;
} // namespace Clock

// Parts of this implementation were based on Ryujinx (https://github.com/Ryujinx/Ryujinx/pull/783).
// This code was released under public domain.

class TimeManager final {
public:
    explicit TimeManager(Core::System& system_);
    ~TimeManager();

    void Initialize();

    Clock::StandardSteadyClockCore& GetStandardSteadyClockCore();

    const Clock::StandardSteadyClockCore& GetStandardSteadyClockCore() const;

    Clock::StandardLocalSystemClockCore& GetStandardLocalSystemClockCore();

    const Clock::StandardLocalSystemClockCore& GetStandardLocalSystemClockCore() const;

    Clock::StandardNetworkSystemClockCore& GetStandardNetworkSystemClockCore();

    const Clock::StandardNetworkSystemClockCore& GetStandardNetworkSystemClockCore() const;

    Clock::StandardUserSystemClockCore& GetStandardUserSystemClockCore();

    const Clock::StandardUserSystemClockCore& GetStandardUserSystemClockCore() const;

    TimeZone::TimeZoneContentManager& GetTimeZoneContentManager();

    const TimeZone::TimeZoneContentManager& GetTimeZoneContentManager() const;

    void UpdateLocalSystemClockTime(s64 posix_time);

    SharedMemory& GetSharedMemory();

    const SharedMemory& GetSharedMemory() const;

    void Shutdown();

    void SetupTimeZoneManager(std::string location_name,
                              Clock::SteadyClockTimePoint time_zone_updated_time_point,
                              std::size_t total_location_name_count, u128 time_zone_rule_version,
                              FileSys::VirtualFile& vfs_file);

    static s64 GetExternalTimeZoneOffset();

private:
    Core::System& system;

    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Service::Time

// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <ctime>

#include "common/settings.h"
#include "common/time_zone.h"
#include "core/hle/service/time/ephemeral_network_system_clock_context_writer.h"
#include "core/hle/service/time/ephemeral_network_system_clock_core.h"
#include "core/hle/service/time/local_system_clock_context_writer.h"
#include "core/hle/service/time/network_system_clock_context_writer.h"
#include "core/hle/service/time/tick_based_steady_clock_core.h"
#include "core/hle/service/time/time_manager.h"

namespace Service::Time {
namespace {
constexpr Clock::TimeSpanType standard_network_clock_accuracy{0x0009356907420000ULL};

s64 GetSecondsSinceEpoch() {
    const auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(time_since_epoch).count() +
           Settings::values.custom_rtc_differential;
}

s64 GetExternalRtcValue() {
    return GetSecondsSinceEpoch() + TimeManager::GetExternalTimeZoneOffset();
}
} // Anonymous namespace

struct TimeManager::Impl final {
    explicit Impl(Core::System& system)
        : shared_memory{system}, standard_local_system_clock_core{standard_steady_clock_core},
          standard_network_system_clock_core{standard_steady_clock_core},
          standard_user_system_clock_core{standard_local_system_clock_core,
                                          standard_network_system_clock_core, system},
          ephemeral_network_system_clock_core{tick_based_steady_clock_core},
          local_system_clock_context_writer{
              std::make_shared<Clock::LocalSystemClockContextWriter>(shared_memory)},
          network_system_clock_context_writer{
              std::make_shared<Clock::NetworkSystemClockContextWriter>(shared_memory)},
          ephemeral_network_system_clock_context_writer{
              std::make_shared<Clock::EphemeralNetworkSystemClockContextWriter>()},
          time_zone_content_manager{system} {

        const auto system_time{Clock::TimeSpanType::FromSeconds(GetExternalRtcValue())};
        SetupStandardSteadyClock(system, Common::UUID::MakeRandom(), system_time, {}, {});
        SetupStandardLocalSystemClock(system, {}, system_time.ToSeconds());

        Clock::SystemClockContext clock_context{};
        standard_local_system_clock_core.GetClockContext(system, clock_context);

        SetupStandardNetworkSystemClock(clock_context, standard_network_clock_accuracy);
        SetupStandardUserSystemClock(system, {}, Clock::SteadyClockTimePoint::GetRandom());
        SetupEphemeralNetworkSystemClock();
    }

    ~Impl() = default;

    Clock::StandardSteadyClockCore& GetStandardSteadyClockCore() {
        return standard_steady_clock_core;
    }

    const Clock::StandardSteadyClockCore& GetStandardSteadyClockCore() const {
        return standard_steady_clock_core;
    }

    Clock::StandardLocalSystemClockCore& GetStandardLocalSystemClockCore() {
        return standard_local_system_clock_core;
    }

    const Clock::StandardLocalSystemClockCore& GetStandardLocalSystemClockCore() const {
        return standard_local_system_clock_core;
    }

    Clock::StandardNetworkSystemClockCore& GetStandardNetworkSystemClockCore() {
        return standard_network_system_clock_core;
    }

    const Clock::StandardNetworkSystemClockCore& GetStandardNetworkSystemClockCore() const {
        return standard_network_system_clock_core;
    }

    Clock::StandardUserSystemClockCore& GetStandardUserSystemClockCore() {
        return standard_user_system_clock_core;
    }

    const Clock::StandardUserSystemClockCore& GetStandardUserSystemClockCore() const {
        return standard_user_system_clock_core;
    }

    TimeZone::TimeZoneContentManager& GetTimeZoneContentManager() {
        return time_zone_content_manager;
    }

    const TimeZone::TimeZoneContentManager& GetTimeZoneContentManager() const {
        return time_zone_content_manager;
    }

    SharedMemory& GetSharedMemory() {
        return shared_memory;
    }

    const SharedMemory& GetSharedMemory() const {
        return shared_memory;
    }

    void SetupTimeZoneManager(std::string location_name,
                              Clock::SteadyClockTimePoint time_zone_updated_time_point,
                              std::size_t total_location_name_count, u128 time_zone_rule_version,
                              FileSys::VirtualFile& vfs_file) {
        if (time_zone_content_manager.GetTimeZoneManager().SetDeviceLocationNameWithTimeZoneRule(
                location_name, vfs_file) != ResultSuccess) {
            UNREACHABLE();
            return;
        }

        time_zone_content_manager.GetTimeZoneManager().SetUpdatedTime(time_zone_updated_time_point);
        time_zone_content_manager.GetTimeZoneManager().SetTotalLocationNameCount(
            total_location_name_count);
        time_zone_content_manager.GetTimeZoneManager().SetTimeZoneRuleVersion(
            time_zone_rule_version);
        time_zone_content_manager.GetTimeZoneManager().MarkAsInitialized();
    }

    static s64 GetExternalTimeZoneOffset() {
        // With "auto" timezone setting, we use the external system's timezone offset
        if (Settings::GetTimeZoneString() == "auto") {
            return Common::TimeZone::GetCurrentOffsetSeconds().count();
        }
        return 0;
    }

    void SetupStandardSteadyClock(Core::System& system_, Common::UUID clock_source_id,
                                  Clock::TimeSpanType setup_value,
                                  Clock::TimeSpanType internal_offset, bool is_rtc_reset_detected) {
        standard_steady_clock_core.SetClockSourceId(clock_source_id);
        standard_steady_clock_core.SetSetupValue(setup_value);
        standard_steady_clock_core.SetInternalOffset(internal_offset);
        standard_steady_clock_core.MarkAsInitialized();

        const auto current_time_point{standard_steady_clock_core.GetCurrentRawTimePoint(system_)};
        shared_memory.SetupStandardSteadyClock(clock_source_id, current_time_point);
    }

    void SetupStandardLocalSystemClock(Core::System& system_,
                                       Clock::SystemClockContext clock_context, s64 posix_time) {
        standard_local_system_clock_core.SetUpdateCallbackInstance(
            local_system_clock_context_writer);

        const auto current_time_point{
            standard_local_system_clock_core.GetSteadyClockCore().GetCurrentTimePoint(system_)};
        if (current_time_point.clock_source_id == clock_context.steady_time_point.clock_source_id) {
            standard_local_system_clock_core.SetSystemClockContext(clock_context);
        } else {
            if (standard_local_system_clock_core.SetCurrentTime(system_, posix_time) !=
                ResultSuccess) {
                UNREACHABLE();
                return;
            }
        }

        standard_local_system_clock_core.MarkAsInitialized();
    }

    void SetupStandardNetworkSystemClock(Clock::SystemClockContext clock_context,
                                         Clock::TimeSpanType sufficient_accuracy) {
        standard_network_system_clock_core.SetUpdateCallbackInstance(
            network_system_clock_context_writer);

        if (standard_network_system_clock_core.SetSystemClockContext(clock_context) !=
            ResultSuccess) {
            UNREACHABLE();
            return;
        }

        standard_network_system_clock_core.SetStandardNetworkClockSufficientAccuracy(
            sufficient_accuracy);
        standard_network_system_clock_core.MarkAsInitialized();
    }

    void SetupStandardUserSystemClock(Core::System& system_, bool is_automatic_correction_enabled,
                                      Clock::SteadyClockTimePoint steady_clock_time_point) {
        if (standard_user_system_clock_core.SetAutomaticCorrectionEnabled(
                system_, is_automatic_correction_enabled) != ResultSuccess) {
            UNREACHABLE();
            return;
        }

        standard_user_system_clock_core.SetAutomaticCorrectionUpdatedTime(steady_clock_time_point);
        standard_user_system_clock_core.MarkAsInitialized();
        shared_memory.SetAutomaticCorrectionEnabled(is_automatic_correction_enabled);
    }

    void SetupEphemeralNetworkSystemClock() {
        ephemeral_network_system_clock_core.SetUpdateCallbackInstance(
            ephemeral_network_system_clock_context_writer);
        ephemeral_network_system_clock_core.MarkAsInitialized();
    }

    void UpdateLocalSystemClockTime(Core::System& system_, s64 posix_time) {
        const auto timespan{Clock::TimeSpanType::FromSeconds(posix_time)};
        if (GetStandardLocalSystemClockCore()
                .SetCurrentTime(system_, timespan.ToSeconds())
                .IsError()) {
            UNREACHABLE();
            return;
        }
    }

    SharedMemory shared_memory;

    Clock::StandardSteadyClockCore standard_steady_clock_core;
    Clock::TickBasedSteadyClockCore tick_based_steady_clock_core;
    Clock::StandardLocalSystemClockCore standard_local_system_clock_core;
    Clock::StandardNetworkSystemClockCore standard_network_system_clock_core;
    Clock::StandardUserSystemClockCore standard_user_system_clock_core;
    Clock::EphemeralNetworkSystemClockCore ephemeral_network_system_clock_core;

    std::shared_ptr<Clock::LocalSystemClockContextWriter> local_system_clock_context_writer;
    std::shared_ptr<Clock::NetworkSystemClockContextWriter> network_system_clock_context_writer;
    std::shared_ptr<Clock::EphemeralNetworkSystemClockContextWriter>
        ephemeral_network_system_clock_context_writer;

    TimeZone::TimeZoneContentManager time_zone_content_manager;
};

TimeManager::TimeManager(Core::System& system_) : system{system_} {}

TimeManager::~TimeManager() = default;

void TimeManager::Initialize() {
    impl = std::make_unique<Impl>(system);

    // Time zones can only be initialized after impl is valid
    impl->time_zone_content_manager.Initialize(*this);
}

Clock::StandardSteadyClockCore& TimeManager::GetStandardSteadyClockCore() {
    return impl->standard_steady_clock_core;
}

const Clock::StandardSteadyClockCore& TimeManager::GetStandardSteadyClockCore() const {
    return impl->standard_steady_clock_core;
}

Clock::StandardLocalSystemClockCore& TimeManager::GetStandardLocalSystemClockCore() {
    return impl->standard_local_system_clock_core;
}

const Clock::StandardLocalSystemClockCore& TimeManager::GetStandardLocalSystemClockCore() const {
    return impl->standard_local_system_clock_core;
}

Clock::StandardNetworkSystemClockCore& TimeManager::GetStandardNetworkSystemClockCore() {
    return impl->standard_network_system_clock_core;
}

const Clock::StandardNetworkSystemClockCore& TimeManager::GetStandardNetworkSystemClockCore()
    const {
    return impl->standard_network_system_clock_core;
}

Clock::StandardUserSystemClockCore& TimeManager::GetStandardUserSystemClockCore() {
    return impl->standard_user_system_clock_core;
}

const Clock::StandardUserSystemClockCore& TimeManager::GetStandardUserSystemClockCore() const {
    return impl->standard_user_system_clock_core;
}

TimeZone::TimeZoneContentManager& TimeManager::GetTimeZoneContentManager() {
    return impl->time_zone_content_manager;
}

const TimeZone::TimeZoneContentManager& TimeManager::GetTimeZoneContentManager() const {
    return impl->time_zone_content_manager;
}

SharedMemory& TimeManager::GetSharedMemory() {
    return impl->shared_memory;
}

const SharedMemory& TimeManager::GetSharedMemory() const {
    return impl->shared_memory;
}

void TimeManager::Shutdown() {
    impl.reset();
}

void TimeManager::UpdateLocalSystemClockTime(s64 posix_time) {
    impl->UpdateLocalSystemClockTime(system, posix_time);
}

void TimeManager::SetupTimeZoneManager(std::string location_name,
                                       Clock::SteadyClockTimePoint time_zone_updated_time_point,
                                       std::size_t total_location_name_count,
                                       u128 time_zone_rule_version,
                                       FileSys::VirtualFile& vfs_file) {
    impl->SetupTimeZoneManager(location_name, time_zone_updated_time_point,
                               total_location_name_count, time_zone_rule_version, vfs_file);
}

/*static*/ s64 TimeManager::GetExternalTimeZoneOffset() {
    // With "auto" timezone setting, we use the external system's timezone offset
    if (Settings::GetTimeZoneString() == "auto") {
        return Common::TimeZone::GetCurrentOffsetSeconds().count();
    }
    return 0;
}

} // namespace Service::Time

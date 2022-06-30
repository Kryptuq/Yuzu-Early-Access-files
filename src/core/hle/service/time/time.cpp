// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hardware_properties.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/time/time.h"
#include "core/hle/service/time/time_interface.h"
#include "core/hle/service/time/time_manager.h"
#include "core/hle/service/time/time_sharedmemory.h"
#include "core/hle/service/time/time_zone_service.h"

namespace Service::Time {

class ISystemClock final : public ServiceFramework<ISystemClock> {
public:
    explicit ISystemClock(Clock::SystemClockCore& clock_core_, Core::System& system_)
        : ServiceFramework{system_, "ISystemClock"}, clock_core{clock_core_} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &ISystemClock::GetCurrentTime, "GetCurrentTime"},
            {1, nullptr, "SetCurrentTime"},
            {2,  &ISystemClock::GetSystemClockContext, "GetSystemClockContext"},
            {3, nullptr, "SetSystemClockContext"},
            {4, nullptr, "GetOperationEventReadableHandle"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void GetCurrentTime(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Time, "called");

        if (!clock_core.IsInitialized()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERROR_UNINITIALIZED_CLOCK);
            return;
        }

        s64 posix_time{};
        if (const Result result{clock_core.GetCurrentTime(system, posix_time)}; result.IsError()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(result);
            return;
        }

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push<s64>(posix_time);
    }

    void GetSystemClockContext(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Time, "called");

        if (!clock_core.IsInitialized()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERROR_UNINITIALIZED_CLOCK);
            return;
        }

        Clock::SystemClockContext system_clock_context{};
        if (const Result result{clock_core.GetClockContext(system, system_clock_context)};
            result.IsError()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(result);
            return;
        }

        IPC::ResponseBuilder rb{ctx, sizeof(Clock::SystemClockContext) / 4 + 2};
        rb.Push(ResultSuccess);
        rb.PushRaw(system_clock_context);
    }

    Clock::SystemClockCore& clock_core;
};

class ISteadyClock final : public ServiceFramework<ISteadyClock> {
public:
    explicit ISteadyClock(Clock::SteadyClockCore& clock_core_, Core::System& system_)
        : ServiceFramework{system_, "ISteadyClock"}, clock_core{clock_core_} {
        static const FunctionInfo functions[] = {
            {0, &ISteadyClock::GetCurrentTimePoint, "GetCurrentTimePoint"},
            {2, nullptr, "GetTestOffset"},
            {3, nullptr, "SetTestOffset"},
            {100, nullptr, "GetRtcValue"},
            {101, nullptr, "IsRtcResetDetected"},
            {102, nullptr, "GetSetupResultValue"},
            {200, nullptr, "GetInternalOffset"},
            {201, nullptr, "SetInternalOffset"},
        };
        RegisterHandlers(functions);
    }

private:
    void GetCurrentTimePoint(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Time, "called");

        if (!clock_core.IsInitialized()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERROR_UNINITIALIZED_CLOCK);
            return;
        }

        const Clock::SteadyClockTimePoint time_point{clock_core.GetCurrentTimePoint(system)};
        IPC::ResponseBuilder rb{ctx, (sizeof(Clock::SteadyClockTimePoint) / 4) + 2};
        rb.Push(ResultSuccess);
        rb.PushRaw(time_point);
    }

    Clock::SteadyClockCore& clock_core;
};

Result Module::Interface::GetClockSnapshotFromSystemClockContextInternal(
    Kernel::KThread* thread, Clock::SystemClockContext user_context,
    Clock::SystemClockContext network_context, Clock::TimeType type,
    Clock::ClockSnapshot& clock_snapshot) {

    auto& time_manager{system.GetTimeManager()};

    clock_snapshot.steady_clock_time_point =
        time_manager.GetStandardSteadyClockCore().GetCurrentTimePoint(system);
    clock_snapshot.is_automatic_correction_enabled =
        time_manager.GetStandardUserSystemClockCore().IsAutomaticCorrectionEnabled();
    clock_snapshot.type = type;

    if (const Result result{
            time_manager.GetTimeZoneContentManager().GetTimeZoneManager().GetDeviceLocationName(
                clock_snapshot.location_name)};
        result != ResultSuccess) {
        return result;
    }

    clock_snapshot.user_context = user_context;

    if (const Result result{Clock::ClockSnapshot::GetCurrentTime(
            clock_snapshot.user_time, clock_snapshot.steady_clock_time_point,
            clock_snapshot.user_context)};
        result != ResultSuccess) {
        return result;
    }

    TimeZone::CalendarInfo userCalendarInfo{};
    if (const Result result{
            time_manager.GetTimeZoneContentManager().GetTimeZoneManager().ToCalendarTimeWithMyRules(
                clock_snapshot.user_time, userCalendarInfo)};
        result != ResultSuccess) {
        return result;
    }

    clock_snapshot.user_calendar_time = userCalendarInfo.time;
    clock_snapshot.user_calendar_additional_time = userCalendarInfo.additional_info;

    clock_snapshot.network_context = network_context;

    if (Clock::ClockSnapshot::GetCurrentTime(clock_snapshot.network_time,
                                             clock_snapshot.steady_clock_time_point,
                                             clock_snapshot.network_context) != ResultSuccess) {
        clock_snapshot.network_time = 0;
    }

    TimeZone::CalendarInfo networkCalendarInfo{};
    if (const Result result{
            time_manager.GetTimeZoneContentManager().GetTimeZoneManager().ToCalendarTimeWithMyRules(
                clock_snapshot.network_time, networkCalendarInfo)};
        result != ResultSuccess) {
        return result;
    }

    clock_snapshot.network_calendar_time = networkCalendarInfo.time;
    clock_snapshot.network_calendar_additional_time = networkCalendarInfo.additional_info;

    return ResultSuccess;
}

void Module::Interface::GetStandardUserSystemClock(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_Time, "called");
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<ISystemClock>(system.GetTimeManager().GetStandardUserSystemClockCore(),
                                      system);
}

void Module::Interface::GetStandardNetworkSystemClock(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_Time, "called");
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<ISystemClock>(system.GetTimeManager().GetStandardNetworkSystemClockCore(),
                                      system);
}

void Module::Interface::GetStandardSteadyClock(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_Time, "called");
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<ISteadyClock>(system.GetTimeManager().GetStandardSteadyClockCore(), system);
}

void Module::Interface::GetTimeZoneService(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_Time, "called");
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<ITimeZoneService>(system,
                                          system.GetTimeManager().GetTimeZoneContentManager());
}

void Module::Interface::GetStandardLocalSystemClock(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_Time, "called");
    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<ISystemClock>(system.GetTimeManager().GetStandardLocalSystemClockCore(),
                                      system);
}

void Module::Interface::IsStandardNetworkSystemClockAccuracySufficient(
    Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_Time, "called");
    auto& clock_core{system.GetTimeManager().GetStandardNetworkSystemClockCore()};
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push<u32>(clock_core.IsStandardNetworkSystemClockAccuracySufficient(system));
}

void Module::Interface::CalculateMonotonicSystemClockBaseTimePoint(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_Time, "called");

    auto& steady_clock_core{system.GetTimeManager().GetStandardSteadyClockCore()};
    if (!steady_clock_core.IsInitialized()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ERROR_UNINITIALIZED_CLOCK);
        return;
    }

    IPC::RequestParser rp{ctx};
    const auto context{rp.PopRaw<Clock::SystemClockContext>()};
    const auto current_time_point{steady_clock_core.GetCurrentTimePoint(system)};

    if (current_time_point.clock_source_id == context.steady_time_point.clock_source_id) {
        const auto ticks{Clock::TimeSpanType::FromTicks(system.CoreTiming().GetClockTicks(),
                                                        Core::Hardware::CNTFREQ)};
        const s64 base_time_point{context.offset + current_time_point.time_point -
                                  ticks.ToSeconds()};
        IPC::ResponseBuilder rb{ctx, (sizeof(s64) / 4) + 2};
        rb.Push(ResultSuccess);
        rb.PushRaw(base_time_point);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ERROR_TIME_MISMATCH);
}

void Module::Interface::GetClockSnapshot(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto type{rp.PopEnum<Clock::TimeType>()};

    LOG_DEBUG(Service_Time, "called, type={}", type);

    Clock::SystemClockContext user_context{};
    if (const Result result{
            system.GetTimeManager().GetStandardUserSystemClockCore().GetClockContext(system,
                                                                                     user_context)};
        result.IsError()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    Clock::SystemClockContext network_context{};
    if (const Result result{
            system.GetTimeManager().GetStandardNetworkSystemClockCore().GetClockContext(
                system, network_context)};
        result.IsError()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    Clock::ClockSnapshot clock_snapshot{};
    if (const Result result{GetClockSnapshotFromSystemClockContextInternal(
            &ctx.GetThread(), user_context, network_context, type, clock_snapshot)};
        result.IsError()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    ctx.WriteBuffer(clock_snapshot);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Module::Interface::GetClockSnapshotFromSystemClockContext(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto type{rp.PopEnum<Clock::TimeType>()};

    rp.Skip(1, false);

    const Clock::SystemClockContext user_context{rp.PopRaw<Clock::SystemClockContext>()};
    const Clock::SystemClockContext network_context{rp.PopRaw<Clock::SystemClockContext>()};

    LOG_DEBUG(Service_Time, "called, type={}", type);

    Clock::ClockSnapshot clock_snapshot{};
    if (const Result result{GetClockSnapshotFromSystemClockContextInternal(
            &ctx.GetThread(), user_context, network_context, type, clock_snapshot)};
        result != ResultSuccess) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    ctx.WriteBuffer(clock_snapshot);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Module::Interface::CalculateStandardUserSystemClockDifferenceByUser(
    Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_Time, "called");

    Clock::ClockSnapshot snapshot_a;
    Clock::ClockSnapshot snapshot_b;

    const auto snapshot_a_data = ctx.ReadBuffer(0);
    const auto snapshot_b_data = ctx.ReadBuffer(1);

    std::memcpy(&snapshot_a, snapshot_a_data.data(), sizeof(Clock::ClockSnapshot));
    std::memcpy(&snapshot_b, snapshot_b_data.data(), sizeof(Clock::ClockSnapshot));

    auto time_span_type{Clock::TimeSpanType::FromSeconds(snapshot_b.user_context.offset -
                                                         snapshot_a.user_context.offset)};

    if ((snapshot_b.user_context.steady_time_point.clock_source_id !=
         snapshot_a.user_context.steady_time_point.clock_source_id) ||
        (snapshot_b.is_automatic_correction_enabled &&
         snapshot_a.is_automatic_correction_enabled)) {
        time_span_type.nanoseconds = 0;
    }

    IPC::ResponseBuilder rb{ctx, (sizeof(s64) / 4) + 2};
    rb.Push(ResultSuccess);
    rb.PushRaw(time_span_type.nanoseconds);
}

void Module::Interface::CalculateSpanBetween(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_Time, "called");

    Clock::ClockSnapshot snapshot_a;
    Clock::ClockSnapshot snapshot_b;

    const auto snapshot_a_data = ctx.ReadBuffer(0);
    const auto snapshot_b_data = ctx.ReadBuffer(1);

    std::memcpy(&snapshot_a, snapshot_a_data.data(), sizeof(Clock::ClockSnapshot));
    std::memcpy(&snapshot_b, snapshot_b_data.data(), sizeof(Clock::ClockSnapshot));

    Clock::TimeSpanType time_span_type{};
    s64 span{};

    if (const Result result{snapshot_a.steady_clock_time_point.GetSpanBetween(
            snapshot_b.steady_clock_time_point, span)};
        result != ResultSuccess) {
        if (snapshot_a.network_time && snapshot_b.network_time) {
            time_span_type =
                Clock::TimeSpanType::FromSeconds(snapshot_b.network_time - snapshot_a.network_time);
        } else {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERROR_TIME_NOT_FOUND);
            return;
        }
    } else {
        time_span_type = Clock::TimeSpanType::FromSeconds(span);
    }

    IPC::ResponseBuilder rb{ctx, (sizeof(s64) / 4) + 2};
    rb.Push(ResultSuccess);
    rb.PushRaw(time_span_type.nanoseconds);
}

void Module::Interface::GetSharedMemoryNativeHandle(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_Time, "called");
    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(&system.Kernel().GetTimeSharedMem());
}

Module::Interface::Interface(std::shared_ptr<Module> module_, Core::System& system_,
                             const char* name)
    : ServiceFramework{system_, name}, module{std::move(module_)} {}

Module::Interface::~Interface() = default;

void InstallInterfaces(Core::System& system) {
    auto module{std::make_shared<Module>()};
    std::make_shared<Time>(module, system, "time:a")->InstallAsService(system.ServiceManager());
    std::make_shared<Time>(module, system, "time:s")->InstallAsService(system.ServiceManager());
    std::make_shared<Time>(module, system, "time:u")->InstallAsService(system.ServiceManager());
}

} // namespace Service::Time

// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/time/time_zone_content_manager.h"
#include "core/hle/service/time/time_zone_service.h"
#include "core/hle/service/time/time_zone_types.h"

namespace Service::Time {

ITimeZoneService::ITimeZoneService(Core::System& system_,
                                   TimeZone::TimeZoneContentManager& time_zone_manager_)
    : ServiceFramework{system_, "ITimeZoneService"}, time_zone_content_manager{time_zone_manager_} {
    static const FunctionInfo functions[] = {
        {0, &ITimeZoneService::GetDeviceLocationName, "GetDeviceLocationName"},
        {1, nullptr, "SetDeviceLocationName"},
        {2, nullptr, "GetTotalLocationNameCount"},
        {3, nullptr, "LoadLocationNameList"},
        {4, &ITimeZoneService::LoadTimeZoneRule, "LoadTimeZoneRule"},
        {5, nullptr, "GetTimeZoneRuleVersion"},
        {6, nullptr, "GetDeviceLocationNameAndUpdatedTime"},
        {100, &ITimeZoneService::ToCalendarTime, "ToCalendarTime"},
        {101, &ITimeZoneService::ToCalendarTimeWithMyRule, "ToCalendarTimeWithMyRule"},
        {201, &ITimeZoneService::ToPosixTime, "ToPosixTime"},
        {202, &ITimeZoneService::ToPosixTimeWithMyRule, "ToPosixTimeWithMyRule"},
    };
    RegisterHandlers(functions);
}

void ITimeZoneService::GetDeviceLocationName(HLERequestContext& ctx) {
    LOG_DEBUG(Service_Time, "called");

    TimeZone::LocationName location_name{};
    if (const Result result{
            time_zone_content_manager.GetTimeZoneManager().GetDeviceLocationName(location_name)};
        result != ResultSuccess) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    IPC::ResponseBuilder rb{ctx, (sizeof(location_name) / 4) + 2};
    rb.Push(ResultSuccess);
    rb.PushRaw(location_name);
}

void ITimeZoneService::LoadTimeZoneRule(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto raw_location_name{rp.PopRaw<std::array<u8, 0x24>>()};

    std::string location_name;
    for (const auto& byte : raw_location_name) {
        // Strip extra bytes
        if (byte == '\0') {
            break;
        }
        location_name.push_back(byte);
    }

    LOG_DEBUG(Service_Time, "called, location_name={}", location_name);

    TimeZone::TimeZoneRule time_zone_rule{};
    if (const Result result{
            time_zone_content_manager.LoadTimeZoneRule(time_zone_rule, location_name)};
        result != ResultSuccess) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    std::vector<u8> time_zone_rule_outbuffer(sizeof(TimeZone::TimeZoneRule));
    std::memcpy(time_zone_rule_outbuffer.data(), &time_zone_rule, sizeof(TimeZone::TimeZoneRule));
    ctx.WriteBuffer(time_zone_rule_outbuffer);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void ITimeZoneService::ToCalendarTime(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto posix_time{rp.Pop<s64>()};

    LOG_DEBUG(Service_Time, "called, posix_time=0x{:016X}", posix_time);

    TimeZone::TimeZoneRule time_zone_rule{};
    const auto buffer{ctx.ReadBuffer()};
    std::memcpy(&time_zone_rule, buffer.data(), buffer.size());

    TimeZone::CalendarInfo calendar_info{};
    if (const Result result{time_zone_content_manager.GetTimeZoneManager().ToCalendarTime(
            time_zone_rule, posix_time, calendar_info)};
        result != ResultSuccess) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2 + (sizeof(TimeZone::CalendarInfo) / 4)};
    rb.Push(ResultSuccess);
    rb.PushRaw(calendar_info);
}

void ITimeZoneService::ToCalendarTimeWithMyRule(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto posix_time{rp.Pop<s64>()};

    LOG_DEBUG(Service_Time, "called, posix_time=0x{:016X}", posix_time);

    TimeZone::CalendarInfo calendar_info{};
    if (const Result result{
            time_zone_content_manager.GetTimeZoneManager().ToCalendarTimeWithMyRules(
                posix_time, calendar_info)};
        result != ResultSuccess) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2 + (sizeof(TimeZone::CalendarInfo) / 4)};
    rb.Push(ResultSuccess);
    rb.PushRaw(calendar_info);
}

void ITimeZoneService::ToPosixTime(HLERequestContext& ctx) {
    LOG_DEBUG(Service_Time, "called");

    IPC::RequestParser rp{ctx};
    const auto calendar_time{rp.PopRaw<TimeZone::CalendarTime>()};
    TimeZone::TimeZoneRule time_zone_rule{};
    std::memcpy(&time_zone_rule, ctx.ReadBuffer().data(), sizeof(TimeZone::TimeZoneRule));

    s64 posix_time{};
    if (const Result result{time_zone_content_manager.GetTimeZoneManager().ToPosixTime(
            time_zone_rule, calendar_time, posix_time)};
        result != ResultSuccess) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    ctx.WriteBuffer(posix_time);

    // TODO(bunnei): Handle multiple times
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushRaw<u32>(1); // Number of times we're returning
}

void ITimeZoneService::ToPosixTimeWithMyRule(HLERequestContext& ctx) {
    LOG_DEBUG(Service_Time, "called");

    IPC::RequestParser rp{ctx};
    const auto calendar_time{rp.PopRaw<TimeZone::CalendarTime>()};

    s64 posix_time{};
    if (const Result result{time_zone_content_manager.GetTimeZoneManager().ToPosixTimeWithMyRule(
            calendar_time, posix_time)};
        result != ResultSuccess) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    ctx.WriteBuffer(posix_time);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushRaw<u32>(1); // Number of times we're returning
}

} // namespace Service::Time

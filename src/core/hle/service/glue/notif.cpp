// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cstring>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/service/glue/notif.h"
#include "core/hle/service/ipc_helpers.h"

namespace Service::Glue {

NOTIF_A::NOTIF_A(Core::System& system_) : ServiceFramework{system_, "notif:a"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {500, &NOTIF_A::RegisterAlarmSetting, "RegisterAlarmSetting"},
        {510, &NOTIF_A::UpdateAlarmSetting, "UpdateAlarmSetting"},
        {520, &NOTIF_A::ListAlarmSettings, "ListAlarmSettings"},
        {530, &NOTIF_A::LoadApplicationParameter, "LoadApplicationParameter"},
        {540, &NOTIF_A::DeleteAlarmSetting, "DeleteAlarmSetting"},
        {1000, &NOTIF_A::Initialize, "Initialize"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

NOTIF_A::~NOTIF_A() = default;

void NOTIF_A::RegisterAlarmSetting(HLERequestContext& ctx) {
    const auto alarm_setting_buffer_size = ctx.GetReadBufferSize(0);
    const auto application_parameter_size = ctx.GetReadBufferSize(1);

    ASSERT_MSG(alarm_setting_buffer_size == sizeof(AlarmSetting),
               "alarm_setting_buffer_size is not 0x40 bytes");
    ASSERT_MSG(application_parameter_size <= sizeof(ApplicationParameter),
               "application_parameter_size is bigger than 0x400 bytes");

    AlarmSetting new_alarm{};
    memcpy(&new_alarm, ctx.ReadBuffer(0).data(), sizeof(AlarmSetting));

    // TODO: Count alarms per game id
    if (alarms.size() >= max_alarms) {
        LOG_ERROR(Service_NOTIF, "Alarm limit reached");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultUnknown);
        return;
    }

    new_alarm.alarm_setting_id = last_alarm_setting_id++;
    alarms.push_back(new_alarm);

    // TODO: Save application parameter data

    LOG_WARNING(Service_NOTIF,
                "(STUBBED) called, application_parameter_size={}, setting_id={}, kind={}, muted={}",
                application_parameter_size, new_alarm.alarm_setting_id, new_alarm.kind,
                new_alarm.muted);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
    rb.Push(new_alarm.alarm_setting_id);
}

void NOTIF_A::UpdateAlarmSetting(HLERequestContext& ctx) {
    const auto alarm_setting_buffer_size = ctx.GetReadBufferSize(0);
    const auto application_parameter_size = ctx.GetReadBufferSize(1);

    ASSERT_MSG(alarm_setting_buffer_size == sizeof(AlarmSetting),
               "alarm_setting_buffer_size is not 0x40 bytes");
    ASSERT_MSG(application_parameter_size <= sizeof(ApplicationParameter),
               "application_parameter_size is bigger than 0x400 bytes");

    AlarmSetting alarm_setting{};
    memcpy(&alarm_setting, ctx.ReadBuffer(0).data(), sizeof(AlarmSetting));

    const auto alarm_it = GetAlarmFromId(alarm_setting.alarm_setting_id);
    if (alarm_it != alarms.end()) {
        LOG_DEBUG(Service_NOTIF, "Alarm updated");
        *alarm_it = alarm_setting;
        // TODO: Save application parameter data
    }

    LOG_WARNING(Service_NOTIF,
                "(STUBBED) called, application_parameter_size={}, setting_id={}, kind={}, muted={}",
                application_parameter_size, alarm_setting.alarm_setting_id, alarm_setting.kind,
                alarm_setting.muted);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void NOTIF_A::ListAlarmSettings(HLERequestContext& ctx) {
    LOG_INFO(Service_NOTIF, "called, alarm_count={}", alarms.size());

    // TODO: Only return alarms of this game id
    ctx.WriteBuffer(alarms);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(static_cast<u32>(alarms.size()));
}

void NOTIF_A::LoadApplicationParameter(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto alarm_setting_id{rp.Pop<AlarmSettingId>()};

    const auto alarm_it = GetAlarmFromId(alarm_setting_id);
    if (alarm_it == alarms.end()) {
        LOG_ERROR(Service_NOTIF, "Invalid alarm setting id={}", alarm_setting_id);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultUnknown);
        return;
    }

    // TODO: Read application parameter related to this setting id
    ApplicationParameter application_parameter{};

    LOG_WARNING(Service_NOTIF, "(STUBBED) called, alarm_setting_id={}", alarm_setting_id);

    ctx.WriteBuffer(application_parameter);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
    rb.Push(static_cast<u32>(application_parameter.size()));
}

void NOTIF_A::DeleteAlarmSetting(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto alarm_setting_id{rp.Pop<AlarmSettingId>()};

    std::erase_if(alarms, [alarm_setting_id](const AlarmSetting& alarm) {
        return alarm.alarm_setting_id == alarm_setting_id;
    });

    LOG_INFO(Service_NOTIF, "called, alarm_setting_id={}", alarm_setting_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void NOTIF_A::Initialize(HLERequestContext& ctx) {
    // TODO: Load previous alarms from config

    LOG_WARNING(Service_NOTIF, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

std::vector<NOTIF_A::AlarmSetting>::iterator NOTIF_A::GetAlarmFromId(
    AlarmSettingId alarm_setting_id) {
    return std::find_if(alarms.begin(), alarms.end(),
                        [alarm_setting_id](const AlarmSetting& alarm) {
                            return alarm.alarm_setting_id == alarm_setting_id;
                        });
}

} // namespace Service::Glue

// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/ipc_helpers.h"
#include "core/hle/service/glue/notif.h"

namespace Service::Glue {

NOTIF_A::NOTIF_A(Core::System& system_) : ServiceFramework{system_, "notif:a"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {500, nullptr, "RegisterAlarmSetting"},
        {510, nullptr, "UpdateAlarmSetting"},
        {520, &NOTIF_A::ListAlarmSettings, "ListAlarmSettings"},
        {530, nullptr, "LoadApplicationParameter"},
        {540, nullptr, "DeleteAlarmSetting"},
        {1000, &NOTIF_A::Initialize, "Initialize"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

NOTIF_A::~NOTIF_A() = default;

void NOTIF_A::ListAlarmSettings(Kernel::HLERequestContext& ctx) {
    // Returns an array of AlarmSetting
    constexpr s32 alarm_count = 0;

    LOG_WARNING(Service_NOTIF, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(alarm_count);
}

void NOTIF_A::Initialize(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_NOTIF, "(STUBBED) called");
    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

} // namespace Service::Glue

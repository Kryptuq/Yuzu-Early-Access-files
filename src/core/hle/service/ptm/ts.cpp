// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <memory>

#include "core/core.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/ptm/ts.h"

namespace Service::PTM {

TS::TS(Core::System& system_) : ServiceFramework{system_, "ts"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, nullptr, "GetTemperatureRange"},
        {1, &TS::GetTemperature, "GetTemperature"},
        {2, nullptr, "SetMeasurementMode"},
        {3, &TS::GetTemperatureMilliC, "GetTemperatureMilliC"},
        {4, nullptr, "OpenSession"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

TS::~TS() = default;

void TS::GetTemperature(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto location{rp.PopEnum<Location>()};

    const s32 temperature = location == Location::Internal ? 35 : 20;

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(temperature);
}

void TS::GetTemperatureMilliC(HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto location{rp.PopEnum<Location>()};

    const s32 temperature = location == Location::Internal ? 35000 : 20000;

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(temperature);
}

} // namespace Service::PTM

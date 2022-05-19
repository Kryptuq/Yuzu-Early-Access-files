// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/hle/service/vi/vi.h"
#include "core/hle/service/vi/vi_s.h"

namespace Service::VI {

VI_S::VI_S(Core::System& system_, NVFlinger::NVFlinger& nv_flinger_)
    : ServiceFramework{system_, "vi:s"}, nv_flinger{nv_flinger_} {
    static const FunctionInfo functions[] = {
        {1, &VI_S::GetDisplayService, "GetDisplayService"},
        {3, nullptr, "GetDisplayServiceWithProxyNameExchange"},
    };
    RegisterHandlers(functions);
}

VI_S::~VI_S() = default;

void VI_S::GetDisplayService(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_VI, "called");

    detail::GetDisplayServiceImpl(ctx, system, nv_flinger, Permission::System);
}

} // namespace Service::VI

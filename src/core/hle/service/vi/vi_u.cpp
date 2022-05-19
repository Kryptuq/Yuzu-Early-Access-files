// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/hle/service/vi/vi.h"
#include "core/hle/service/vi/vi_u.h"

namespace Service::VI {

VI_U::VI_U(Core::System& system_, NVFlinger::NVFlinger& nv_flinger_)
    : ServiceFramework{system_, "vi:u"}, nv_flinger{nv_flinger_} {
    static const FunctionInfo functions[] = {
        {0, &VI_U::GetDisplayService, "GetDisplayService"},
        {1, nullptr, "GetDisplayServiceWithProxyNameExchange"},
    };
    RegisterHandlers(functions);
}

VI_U::~VI_U() = default;

void VI_U::GetDisplayService(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_VI, "called");

    detail::GetDisplayServiceImpl(ctx, system, nv_flinger, Permission::User);
}

} // namespace Service::VI

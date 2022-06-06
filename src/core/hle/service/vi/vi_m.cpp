// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/hle/service/vi/vi.h"
#include "core/hle/service/vi/vi_m.h"

namespace Service::VI {

VI_M::VI_M(Core::System& system_, NVFlinger::NVFlinger& nv_flinger_,
           NVFlinger::HosBinderDriverServer& hos_binder_driver_server_)
    : ServiceFramework{system_, "vi:m"}, nv_flinger{nv_flinger_}, hos_binder_driver_server{
                                                                      hos_binder_driver_server_} {
    static const FunctionInfo functions[] = {
        {2, &VI_M::GetDisplayService, "GetDisplayService"},
        {3, nullptr, "GetDisplayServiceWithProxyNameExchange"},
    };
    RegisterHandlers(functions);
}

VI_M::~VI_M() = default;

void VI_M::GetDisplayService(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_VI, "called");

    detail::GetDisplayServiceImpl(ctx, system, nv_flinger, hos_binder_driver_server,
                                  Permission::Manager);
}

} // namespace Service::VI

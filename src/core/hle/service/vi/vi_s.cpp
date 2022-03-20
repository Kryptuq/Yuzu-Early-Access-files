// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/service/vi/vi.h"
#include "core/hle/service/vi/vi_s.h"

namespace Service::VI {

VI_S::VI_S(Core::System& system_, NVFlinger::NVFlinger& nv_flinger_,
           NVFlinger::HosBinderDriverServer& hos_binder_driver_server_)
    : ServiceFramework{system_, "vi:s"}, nv_flinger{nv_flinger_}, hos_binder_driver_server{
                                                                      hos_binder_driver_server_} {
    static const FunctionInfo functions[] = {
        {1, &VI_S::GetDisplayService, "GetDisplayService"},
        {3, nullptr, "GetDisplayServiceWithProxyNameExchange"},
    };
    RegisterHandlers(functions);
}

VI_S::~VI_S() = default;

void VI_S::GetDisplayService(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_VI, "called");

    detail::GetDisplayServiceImpl(ctx, system, nv_flinger, hos_binder_driver_server,
                                  Permission::System);
}

} // namespace Service::VI

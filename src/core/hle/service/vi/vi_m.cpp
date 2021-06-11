// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/service/vi/vi.h"
#include "core/hle/service/vi/vi_m.h"

namespace Service::VI {

VI_M::VI_M(Core::System& system_, NVFlinger::NVFlinger& nv_flinger_)
    : ServiceFramework{system_, "vi:m"}, nv_flinger{nv_flinger_} {
    static const FunctionInfo functions[] = {
        {2, &VI_M::GetDisplayService, "GetDisplayService"},
        {3, nullptr, "GetDisplayServiceWithProxyNameExchange"},
    };
    RegisterHandlers(functions);
}

VI_M::~VI_M() = default;

void VI_M::GetDisplayService(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_VI, "called");

    detail::GetDisplayServiceImpl(ctx, system, nv_flinger, Permission::Manager);
}

} // namespace Service::VI

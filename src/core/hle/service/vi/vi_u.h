// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Kernel {
class HLERequestContext;
}

namespace Service::NVFlinger {
class HosBinderDriverServer;
class NVFlinger;
} // namespace Service::NVFlinger

namespace Service::VI {

class VI_U final : public ServiceFramework<VI_U> {
public:
    explicit VI_U(Core::System& system_, NVFlinger::NVFlinger& nv_flinger_,
                  NVFlinger::HosBinderDriverServer& hos_binder_driver_server_);
    ~VI_U() override;

private:
    void GetDisplayService(Kernel::HLERequestContext& ctx);

    NVFlinger::NVFlinger& nv_flinger;
    NVFlinger::HosBinderDriverServer& hos_binder_driver_server;
};

} // namespace Service::VI

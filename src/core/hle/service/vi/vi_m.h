// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Nvnflinger {
class HosBinderDriverServer;
class Nvnflinger;
} // namespace Service::Nvnflinger

namespace Service::VI {

class VI_M final : public ServiceFramework<VI_M> {
public:
    explicit VI_M(Core::System& system_, Nvnflinger::Nvnflinger& nv_flinger_,
                  Nvnflinger::HosBinderDriverServer& hos_binder_driver_server_);
    ~VI_M() override;

private:
    void GetDisplayService(HLERequestContext& ctx);

    Nvnflinger::Nvnflinger& nv_flinger;
    Nvnflinger::HosBinderDriverServer& hos_binder_driver_server;
};

} // namespace Service::VI

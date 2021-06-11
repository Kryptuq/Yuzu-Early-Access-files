// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include "core/core.h"
#include "core/hle/service/glue/arp.h"
#include "core/hle/service/glue/bgtc.h"
#include "core/hle/service/glue/ectx.h"
#include "core/hle/service/glue/glue.h"

namespace Service::Glue {

void InstallInterfaces(Core::System& system) {
    // ARP
    std::make_shared<ARP_R>(system, system.GetARPManager())
        ->InstallAsService(system.ServiceManager());
    std::make_shared<ARP_W>(system, system.GetARPManager())
        ->InstallAsService(system.ServiceManager());

    // BackGround Task Controller
    std::make_shared<BGTC_T>(system)->InstallAsService(system.ServiceManager());
    std::make_shared<BGTC_SC>(system)->InstallAsService(system.ServiceManager());

    // Error Context
    std::make_shared<ECTX_AW>(system)->InstallAsService(system.ServiceManager());
}

} // namespace Service::Glue

// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/audio/audctl.h"
#include "core/hle/service/audio/auddbg.h"
#include "core/hle/service/audio/audin_a.h"
#include "core/hle/service/audio/audin_u.h"
#include "core/hle/service/audio/audio.h"
#include "core/hle/service/audio/audout_a.h"
#include "core/hle/service/audio/audout_u.h"
#include "core/hle/service/audio/audrec_a.h"
#include "core/hle/service/audio/audrec_u.h"
#include "core/hle/service/audio/audren_a.h"
#include "core/hle/service/audio/audren_u.h"
#include "core/hle/service/audio/codecctl.h"
#include "core/hle/service/audio/hwopus.h"
#include "core/hle/service/service.h"

namespace Service::Audio {

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    std::make_shared<AudCtl>(system)->InstallAsService(service_manager);
    std::make_shared<AudOutA>(system)->InstallAsService(service_manager);
    std::make_shared<AudOutU>(system)->InstallAsService(service_manager);
    std::make_shared<AudInA>(system)->InstallAsService(service_manager);
    std::make_shared<AudInU>(system)->InstallAsService(service_manager);
    std::make_shared<AudRecA>(system)->InstallAsService(service_manager);
    std::make_shared<AudRecU>(system)->InstallAsService(service_manager);
    std::make_shared<AudRenA>(system)->InstallAsService(service_manager);
    std::make_shared<AudRenU>(system)->InstallAsService(service_manager);
    std::make_shared<CodecCtl>(system)->InstallAsService(service_manager);
    std::make_shared<HwOpus>(system)->InstallAsService(service_manager);

    std::make_shared<AudDbg>(system, "audin:d")->InstallAsService(service_manager);
    std::make_shared<AudDbg>(system, "audout:d")->InstallAsService(service_manager);
    std::make_shared<AudDbg>(system, "audrec:d")->InstallAsService(service_manager);
    std::make_shared<AudDbg>(system, "audren:d")->InstallAsService(service_manager);
}

} // namespace Service::Audio

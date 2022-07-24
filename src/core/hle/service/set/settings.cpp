// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/set/set.h"
#include "core/hle/service/set/set_cal.h"
#include "core/hle/service/set/set_fd.h"
#include "core/hle/service/set/set_sys.h"
#include "core/hle/service/set/settings.h"
#include "core/hle/service/sm/sm.h"

namespace Service::Set {

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    std::make_shared<SET>(system)->InstallAsService(service_manager);
    std::make_shared<SET_CAL>(system)->InstallAsService(service_manager);
    std::make_shared<SET_FD>(system)->InstallAsService(service_manager);
    std::make_shared<SET_SYS>(system)->InstallAsService(service_manager);
}

} // namespace Service::Set

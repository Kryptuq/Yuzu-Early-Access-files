// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <memory>

#include "core/core.h"
#include "core/hle/service/ptm/psm.h"
#include "core/hle/service/ptm/ptm.h"
#include "core/hle/service/ptm/ts.h"

namespace Service::PTM {

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<PSM>(system)->InstallAsService(sm);
    std::make_shared<TS>(system)->InstallAsService(sm);
}

} // namespace Service::PTM

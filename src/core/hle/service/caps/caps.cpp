// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/caps/caps.h"
#include "core/hle/service/caps/caps_a.h"
#include "core/hle/service/caps/caps_c.h"
#include "core/hle/service/caps/caps_sc.h"
#include "core/hle/service/caps/caps_ss.h"
#include "core/hle/service/caps/caps_su.h"
#include "core/hle/service/caps/caps_u.h"
#include "core/hle/service/service.h"

namespace Service::Capture {

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<CAPS_A>(system)->InstallAsService(sm);
    std::make_shared<CAPS_C>(system)->InstallAsService(sm);
    std::make_shared<CAPS_U>(system)->InstallAsService(sm);
    std::make_shared<CAPS_SC>(system)->InstallAsService(sm);
    std::make_shared<CAPS_SS>(system)->InstallAsService(sm);
    std::make_shared<CAPS_SU>(system)->InstallAsService(sm);
}

} // namespace Service::Capture

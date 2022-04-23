// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/hle/service/apm/apm.h"
#include "core/hle/service/apm/apm_interface.h"

namespace Service::APM {

Module::Module() = default;
Module::~Module() = default;

void InstallInterfaces(Core::System& system) {
    auto module_ = std::make_shared<Module>();
    std::make_shared<APM>(system, module_, system.GetAPMController(), "apm")
        ->InstallAsService(system.ServiceManager());
    std::make_shared<APM>(system, module_, system.GetAPMController(), "apm:p")
        ->InstallAsService(system.ServiceManager());
    std::make_shared<APM>(system, module_, system.GetAPMController(), "apm:am")
        ->InstallAsService(system.ServiceManager());
    std::make_shared<APM_Sys>(system, system.GetAPMController())
        ->InstallAsService(system.ServiceManager());
}

} // namespace Service::APM

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/hle/service/apm/apm.h"
#include "core/hle/service/apm/apm_interface.h"
#include "core/hle/service/server_manager.h"

namespace Service::APM {

Module::Module() = default;
Module::~Module() = default;

void LoopProcess(Core::System& system) {
    auto module = std::make_shared<Module>();
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService(
        "apm", std::make_shared<APM>(system, module, system.GetAPMController(), "apm"));
    server_manager->RegisterNamedService(
        "apm:am", std::make_shared<APM>(system, module, system.GetAPMController(), "apm:am"));
    server_manager->RegisterNamedService(
        "apm:sys", std::make_shared<APM_Sys>(system, system.GetAPMController()));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::APM

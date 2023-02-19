// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <memory>

#include "core/core.h"
#include "core/hle/service/ptm/psm.h"
#include "core/hle/service/ptm/ptm.h"
#include "core/hle/service/ptm/ts.h"
#include "core/hle/service/server_manager.h"

namespace Service::PTM {

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("psm", std::make_shared<PSM>(system));
    server_manager->RegisterNamedService("ts", std::make_shared<TS>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::PTM

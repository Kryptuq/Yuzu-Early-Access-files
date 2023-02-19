// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/service/caps/caps.h"
#include "core/hle/service/caps/caps_a.h"
#include "core/hle/service/caps/caps_c.h"
#include "core/hle/service/caps/caps_sc.h"
#include "core/hle/service/caps/caps_ss.h"
#include "core/hle/service/caps/caps_su.h"
#include "core/hle/service/caps/caps_u.h"
#include "core/hle/service/server_manager.h"
#include "core/hle/service/service.h"

namespace Service::Capture {

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("caps:a", std::make_shared<CAPS_A>(system));
    server_manager->RegisterNamedService("caps:c", std::make_shared<CAPS_C>(system));
    server_manager->RegisterNamedService("caps:u", std::make_shared<CAPS_U>(system));
    server_manager->RegisterNamedService("caps:sc", std::make_shared<CAPS_SC>(system));
    server_manager->RegisterNamedService("caps:ss", std::make_shared<CAPS_SS>(system));
    server_manager->RegisterNamedService("caps:su", std::make_shared<CAPS_SU>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::Capture

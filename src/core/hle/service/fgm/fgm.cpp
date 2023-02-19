// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "core/hle/ipc_helpers.h"
#include "core/hle/service/fgm/fgm.h"
#include "core/hle/service/server_manager.h"
#include "core/hle/service/service.h"
#include "core/hle/service/sm/sm.h"

namespace Service::FGM {

class IRequest final : public ServiceFramework<IRequest> {
public:
    explicit IRequest(Core::System& system_) : ServiceFramework{system_, "IRequest"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Initialize"},
            {1, nullptr, "Set"},
            {2, nullptr, "Get"},
            {3, nullptr, "Cancel"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class FGM final : public ServiceFramework<FGM> {
public:
    explicit FGM(Core::System& system_, const char* name) : ServiceFramework{system_, name} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &FGM::Initialize, "Initialize"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void Initialize(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_FGM, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IRequest>(system);
    }
};

class FGM_DBG final : public ServiceFramework<FGM_DBG> {
public:
    explicit FGM_DBG(Core::System& system_) : ServiceFramework{system_, "fgm:dbg"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Initialize"},
            {1, nullptr, "Read"},
            {2, nullptr, "Cancel"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("fgm", std::make_shared<FGM>(system, "fgm"));
    server_manager->RegisterNamedService("fgm:0", std::make_shared<FGM>(system, "fgm:0"));
    server_manager->RegisterNamedService("fgm:9", std::make_shared<FGM>(system, "fgm:9"));
    server_manager->RegisterNamedService("fgm:dbg", std::make_shared<FGM_DBG>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::FGM

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/hle/service/ipc_helpers.h"
#include "core/hle/service/mnpp/mnpp_app.h"
#include "core/hle/service/server_manager.h"
#include "core/hle/service/service.h"

namespace Service::MNPP {

class MNPP_APP final : public ServiceFramework<MNPP_APP> {
public:
    explicit MNPP_APP(Core::System& system_) : ServiceFramework{system_, "mnpp:app"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &MNPP_APP::Unknown0, "unknown0"},
            {1, &MNPP_APP::Unknown1, "unknown1"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void Unknown0(HLERequestContext& ctx) {
        LOG_WARNING(Service_MNPP, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void Unknown1(HLERequestContext& ctx) {
        LOG_WARNING(Service_MNPP, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }
};

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("mnpp:app", std::make_shared<MNPP_APP>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::MNPP

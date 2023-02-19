// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <memory>

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/psc/psc.h"
#include "core/hle/service/server_manager.h"
#include "core/hle/service/service.h"

namespace Service::PSC {

class PSC_C final : public ServiceFramework<PSC_C> {
public:
    explicit PSC_C(Core::System& system_) : ServiceFramework{system_, "psc:c"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Initialize"},
            {1, nullptr, "DispatchRequest"},
            {2, nullptr, "GetResult"},
            {3, nullptr, "GetState"},
            {4, nullptr, "Cancel"},
            {5, nullptr, "PrintModuleInformation"},
            {6, nullptr, "GetModuleInformation"},
            {10, nullptr, "Unknown10"},
            {11, nullptr, "Unknown11"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class IPmModule final : public ServiceFramework<IPmModule> {
public:
    explicit IPmModule(Core::System& system_) : ServiceFramework{system_, "IPmModule"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "Initialize"},
            {1, nullptr, "GetRequest"},
            {2, nullptr, "Acknowledge"},
            {3, nullptr, "Finalize"},
            {4, nullptr, "AcknowledgeEx"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class PSC_M final : public ServiceFramework<PSC_M> {
public:
    explicit PSC_M(Core::System& system_) : ServiceFramework{system_, "psc:m"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &PSC_M::GetPmModule, "GetPmModule"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void GetPmModule(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_PSC, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IPmModule>(system);
    }
};

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("psc:c", std::make_shared<PSC_C>(system));
    server_manager->RegisterNamedService("psc:m", std::make_shared<PSC_M>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::PSC

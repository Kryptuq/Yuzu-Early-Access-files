// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/nfp/nfp.h"
#include "core/hle/service/nfp/nfp_user.h"
#include "core/hle/service/server_manager.h"

namespace Service::NFP {

class IUserManager final : public ServiceFramework<IUserManager> {
public:
    explicit IUserManager(Core::System& system_) : ServiceFramework{system_, "nfp:user"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IUserManager::CreateUserInterface, "CreateUserInterface"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void CreateUserInterface(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_NFP, "called");

        if (user_interface == nullptr) {
            user_interface = std::make_shared<IUser>(system);
        }

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IUser>(user_interface);
    }

    std::shared_ptr<IUser> user_interface;
};

void LoopProcess(Core::System& system) {
    auto server_manager = std::make_unique<ServerManager>(system);

    server_manager->RegisterNamedService("nfp:user", std::make_shared<IUserManager>(system));
    ServerManager::RunServer(std::move(server_manager));
}

} // namespace Service::NFP

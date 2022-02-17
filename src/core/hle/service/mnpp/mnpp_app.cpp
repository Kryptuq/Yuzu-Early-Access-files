// Copyright 2022 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/mnpp/mnpp_app.h"
#include "core/hle/service/sm/sm.h"

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
    void Unknown0(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_MNPP, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void Unknown1(Kernel::HLERequestContext& ctx) {
        LOG_WARNING(Service_MNPP, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }
};

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    std::make_shared<MNPP_APP>(system)->InstallAsService(service_manager);
}

} // namespace Service::MNPP

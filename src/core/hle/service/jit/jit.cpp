// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/ipc_helpers.h"
#include "core/hle/result.h"
#include "core/hle/service/jit/jit.h"
#include "core/hle/service/service.h"

namespace Service::JIT {

class IJitEnvironment final : public ServiceFramework<IJitEnvironment> {
public:
    explicit IJitEnvironment(Core::System& system_) : ServiceFramework{system_, "IJitEnvironment"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, nullptr, "GenerateCode"},
            {1, nullptr, "Control"},
            {1000, nullptr, "LoadPlugin"},
            {1001, nullptr, "GetCodeAddress"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }
};

class JITU final : public ServiceFramework<JITU> {
public:
    explicit JITU(Core::System& system_) : ServiceFramework{system_, "jit:u"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &JITU::CreateJitEnvironment, "CreateJitEnvironment"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

    void CreateJitEnvironment(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_JIT, "called");

        IPC::ResponseBuilder rb{ctx, 2, 0, 1};
        rb.Push(ResultSuccess);
        rb.PushIpcInterface<IJitEnvironment>(system);
    }
};

void InstallInterfaces(SM::ServiceManager& sm, Core::System& system) {
    std::make_shared<JITU>(system)->InstallAsService(sm);
}

} // namespace Service::JIT

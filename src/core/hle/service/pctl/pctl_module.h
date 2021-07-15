// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_funcs.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::PCTL {

enum class Capability : u32 {
    None = 0,
    Application = 1 << 0,
    SnsPost = 1 << 1,
    Recovery = 1 << 6,
    Status = 1 << 8,
    StereoVision = 1 << 9,
    System = 1 << 15,
};
DECLARE_ENUM_FLAG_OPERATORS(Capability);

class Module final {
public:
    class Interface : public ServiceFramework<Interface> {
    public:
        explicit Interface(Core::System& system_, std::shared_ptr<Module> module_,
                           const char* name_, Capability capability_);
        ~Interface() override;

        void CreateService(Kernel::HLERequestContext& ctx);
        void CreateServiceWithoutInitialize(Kernel::HLERequestContext& ctx);

    protected:
        std::shared_ptr<Module> module;

    private:
        Capability capability{};
    };
};

/// Registers all PCTL services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system);

} // namespace Service::PCTL

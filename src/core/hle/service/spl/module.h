// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <random>
#include "core/hle/service/service.h"
#include "core/hle/service/spl/spl_results.h"
#include "core/hle/service/spl/spl_types.h"

namespace Core {
class System;
}

namespace Service::SPL {

class Module final {
public:
    class Interface : public ServiceFramework<Interface> {
    public:
        explicit Interface(Core::System& system_, std::shared_ptr<Module> module_,
                           const char* name);
        ~Interface() override;

        // General
        void GetConfig(Kernel::HLERequestContext& ctx);
        void ModularExponentiate(Kernel::HLERequestContext& ctx);
        void SetConfig(Kernel::HLERequestContext& ctx);
        void GenerateRandomBytes(Kernel::HLERequestContext& ctx);
        void IsDevelopment(Kernel::HLERequestContext& ctx);
        void SetBootReason(Kernel::HLERequestContext& ctx);
        void GetBootReason(Kernel::HLERequestContext& ctx);

    protected:
        std::shared_ptr<Module> module;

    private:
        ResultVal<u64> GetConfigImpl(ConfigItem config_item) const;

        std::mt19937 rng;
    };
};

/// Registers all SPL services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system);

} // namespace Service::SPL

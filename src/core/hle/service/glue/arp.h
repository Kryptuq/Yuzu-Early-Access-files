// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Service::Glue {

class ARPManager;
class IRegistrar;

class ARP_R final : public ServiceFramework<ARP_R> {
public:
    explicit ARP_R(Core::System& system_, const ARPManager& manager_);
    ~ARP_R() override;

private:
    void GetApplicationLaunchProperty(Kernel::HLERequestContext& ctx);
    void GetApplicationLaunchPropertyWithApplicationId(Kernel::HLERequestContext& ctx);
    void GetApplicationControlProperty(Kernel::HLERequestContext& ctx);
    void GetApplicationControlPropertyWithApplicationId(Kernel::HLERequestContext& ctx);

    const ARPManager& manager;
};

class ARP_W final : public ServiceFramework<ARP_W> {
public:
    explicit ARP_W(Core::System& system_, ARPManager& manager_);
    ~ARP_W() override;

private:
    void AcquireRegistrar(Kernel::HLERequestContext& ctx);
    void UnregisterApplicationInstance(Kernel::HLERequestContext& ctx);

    ARPManager& manager;
    std::shared_ptr<IRegistrar> registrar;
};

} // namespace Service::Glue

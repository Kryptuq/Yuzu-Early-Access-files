// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
    void GetApplicationLaunchProperty(HLERequestContext& ctx);
    void GetApplicationLaunchPropertyWithApplicationId(HLERequestContext& ctx);
    void GetApplicationControlProperty(HLERequestContext& ctx);
    void GetApplicationControlPropertyWithApplicationId(HLERequestContext& ctx);

    const ARPManager& manager;
};

class ARP_W final : public ServiceFramework<ARP_W> {
public:
    explicit ARP_W(Core::System& system_, ARPManager& manager_);
    ~ARP_W() override;

private:
    void AcquireRegistrar(HLERequestContext& ctx);
    void UnregisterApplicationInstance(HLERequestContext& ctx);

    ARPManager& manager;
    std::shared_ptr<IRegistrar> registrar;
};

} // namespace Service::Glue

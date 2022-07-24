// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Kernel {
class KEvent;
}

namespace Service::AOC {

class AOC_U final : public ServiceFramework<AOC_U> {
public:
    explicit AOC_U(Core::System& system);
    ~AOC_U() override;

private:
    void CountAddOnContent(Kernel::HLERequestContext& ctx);
    void ListAddOnContent(Kernel::HLERequestContext& ctx);
    void GetAddOnContentBaseId(Kernel::HLERequestContext& ctx);
    void PrepareAddOnContent(Kernel::HLERequestContext& ctx);
    void GetAddOnContentListChangedEvent(Kernel::HLERequestContext& ctx);
    void GetAddOnContentListChangedEventWithProcessId(Kernel::HLERequestContext& ctx);
    void NotifyMountAddOnContent(Kernel::HLERequestContext& ctx);
    void NotifyUnmountAddOnContent(Kernel::HLERequestContext& ctx);
    void CheckAddOnContentMountStatus(Kernel::HLERequestContext& ctx);
    void CreateEcPurchasedEventManager(Kernel::HLERequestContext& ctx);
    void CreatePermanentEcPurchasedEventManager(Kernel::HLERequestContext& ctx);

    std::vector<u64> add_on_content;
    KernelHelpers::ServiceContext service_context;

    Kernel::KEvent* aoc_change_event;
};

/// Registers all AOC services with the specified service manager.
void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system);

} // namespace Service::AOC

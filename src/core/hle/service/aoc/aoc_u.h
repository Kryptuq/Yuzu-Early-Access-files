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
    void CountAddOnContent(HLERequestContext& ctx);
    void ListAddOnContent(HLERequestContext& ctx);
    void GetAddOnContentBaseId(HLERequestContext& ctx);
    void PrepareAddOnContent(HLERequestContext& ctx);
    void GetAddOnContentListChangedEvent(HLERequestContext& ctx);
    void GetAddOnContentListChangedEventWithProcessId(HLERequestContext& ctx);
    void NotifyMountAddOnContent(HLERequestContext& ctx);
    void NotifyUnmountAddOnContent(HLERequestContext& ctx);
    void CheckAddOnContentMountStatus(HLERequestContext& ctx);
    void CreateEcPurchasedEventManager(HLERequestContext& ctx);
    void CreatePermanentEcPurchasedEventManager(HLERequestContext& ctx);

    std::vector<u64> add_on_content;
    KernelHelpers::ServiceContext service_context;

    Kernel::KEvent* aoc_change_event;
};

void LoopProcess(Core::System& system);

} // namespace Service::AOC

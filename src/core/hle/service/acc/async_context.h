// Copyright 2021 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::Account {

class IAsyncContext : public ServiceFramework<IAsyncContext> {
public:
    explicit IAsyncContext(Core::System& system_);

    void GetSystemEvent(Kernel::HLERequestContext& ctx);
    void Cancel(Kernel::HLERequestContext& ctx);
    void HasDone(Kernel::HLERequestContext& ctx);
    void GetResult(Kernel::HLERequestContext& ctx);

protected:
    virtual bool IsComplete() const = 0;
    virtual void Cancel() = 0;
    virtual ResultCode GetResult() const = 0;

    void MarkComplete();

    std::atomic<bool> is_complete{false};
    Kernel::KEvent compeletion_event;
};

} // namespace Service::Account

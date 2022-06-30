// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "core/hle/kernel/k_auto_object.h"
#include "core/hle/kernel/slab_helpers.h"
#include "core/hle/result.h"

union Result;

namespace Core::Memory {
class Memory;
}

namespace Core::Timing {
class CoreTiming;
}

namespace Kernel {

class KernelCore;
class KSession;
class KThread;

class KClientSession final
    : public KAutoObjectWithSlabHeapAndContainer<KClientSession, KAutoObjectWithList> {
    KERNEL_AUTOOBJECT_TRAITS(KClientSession, KAutoObject);

public:
    explicit KClientSession(KernelCore& kernel_);
    ~KClientSession() override;

    void Initialize(KSession* parent_session_, std::string&& name_) {
        // Set member variables.
        parent = parent_session_;
        name = std::move(name_);
    }

    void Destroy() override;
    static void PostDestroy([[maybe_unused]] uintptr_t arg) {}

    KSession* GetParent() const {
        return parent;
    }

    Result SendSyncRequest(KThread* thread, Core::Memory::Memory& memory,
                           Core::Timing::CoreTiming& core_timing);

    void OnServerClosed();

private:
    KSession* parent{};
};

} // namespace Kernel

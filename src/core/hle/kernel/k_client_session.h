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
    explicit KClientSession(KernelCore& kernel);
    ~KClientSession() override;

    void Initialize(KSession* parent) {
        // Set member variables.
        m_parent = parent;
    }

    void Destroy() override;
    static void PostDestroy(uintptr_t arg) {}

    KSession* GetParent() const {
        return m_parent;
    }

    Result SendSyncRequest();

    void OnServerClosed();

private:
    KSession* m_parent{};
};

} // namespace Kernel

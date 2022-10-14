// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/kernel/slab_helpers.h"

namespace Kernel {

class KernelCore;
class KReadableEvent;
class KProcess;

class KEvent final : public KAutoObjectWithSlabHeapAndContainer<KEvent, KAutoObjectWithList> {
    KERNEL_AUTOOBJECT_TRAITS(KEvent, KAutoObject);

public:
    explicit KEvent(KernelCore& kernel_);
    ~KEvent() override;

    void Initialize(KProcess* owner);

    void Finalize() override;

    bool IsInitialized() const override {
        return m_initialized;
    }

    uintptr_t GetPostDestroyArgument() const override {
        return reinterpret_cast<uintptr_t>(m_owner);
    }

    KProcess* GetOwner() const override {
        return m_owner;
    }

    KReadableEvent& GetReadableEvent() {
        return m_readable_event;
    }

    static void PostDestroy(uintptr_t arg);

    Result Signal();
    Result Clear();

    void OnReadableEventDestroyed() {
        m_readable_event_destroyed = true;
    }

private:
    KReadableEvent m_readable_event;
    KProcess* m_owner{};
    bool m_initialized{};
    bool m_readable_event_destroyed{};
};

} // namespace Kernel

// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/kernel/k_auto_object.h"
#include "core/hle/kernel/k_synchronization_object.h"
#include "core/hle/kernel/slab_helpers.h"
#include "core/hle/result.h"

namespace Kernel {

class KernelCore;
class KEvent;

class KReadableEvent : public KSynchronizationObject {
    KERNEL_AUTOOBJECT_TRAITS(KReadableEvent, KSynchronizationObject);

public:
    explicit KReadableEvent(KernelCore& kernel);
    ~KReadableEvent() override;

    void Initialize(KEvent* parent);

    KEvent* GetParent() const {
        return m_parent;
    }

    Result Signal();
    Result Clear();

    bool IsSignaled() const override;
    void Destroy() override;

    Result Reset();

private:
    bool m_is_signaled{};
    KEvent* m_parent{};
};

} // namespace Kernel

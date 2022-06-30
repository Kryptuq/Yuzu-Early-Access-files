// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/kernel/k_auto_object.h"
#include "core/hle/kernel/slab_helpers.h"
#include "core/hle/result.h"

namespace Kernel {

class KernelCore;
class KEvent;

class KWritableEvent final
    : public KAutoObjectWithSlabHeapAndContainer<KWritableEvent, KAutoObjectWithList> {
    KERNEL_AUTOOBJECT_TRAITS(KWritableEvent, KAutoObject);

public:
    explicit KWritableEvent(KernelCore& kernel_);
    ~KWritableEvent() override;

    void Destroy() override;

    static void PostDestroy([[maybe_unused]] uintptr_t arg) {}

    void Initialize(KEvent* parent_, std::string&& name_);
    Result Signal();
    Result Clear();

    KEvent* GetParent() const {
        return parent;
    }

private:
    KEvent* parent{};
};

} // namespace Kernel

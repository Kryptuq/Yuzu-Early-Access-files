// Copyright 2021 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
    explicit KReadableEvent(KernelCore& kernel_);
    ~KReadableEvent() override;

    void Initialize(KEvent* parent_event_, std::string&& name_) {
        is_signaled = false;
        parent = parent_event_;
        name = std::move(name_);
    }

    KEvent* GetParent() const {
        return parent;
    }

    bool IsSignaled() const override;
    void Destroy() override;

    ResultCode Signal();
    ResultCode Clear();
    ResultCode Reset();

private:
    bool is_signaled{};
    KEvent* parent{};
};

} // namespace Kernel

// Copyright 2021 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/kernel/k_writable_event.h"
#include "core/hle/kernel/slab_helpers.h"

namespace Kernel {

class KernelCore;
class KReadableEvent;
class KWritableEvent;
class KProcess;

class KEvent final : public KAutoObjectWithSlabHeapAndContainer<KEvent, KAutoObjectWithList> {
    KERNEL_AUTOOBJECT_TRAITS(KEvent, KAutoObject);

public:
    explicit KEvent(KernelCore& kernel_);
    ~KEvent() override;

    void Initialize(std::string&& name, KProcess* owner_);

    void Finalize() override;

    bool IsInitialized() const override {
        return initialized;
    }

    uintptr_t GetPostDestroyArgument() const override {
        return reinterpret_cast<uintptr_t>(owner);
    }

    KProcess* GetOwner() const override {
        return owner;
    }

    KReadableEvent& GetReadableEvent() {
        return readable_event;
    }

    KWritableEvent& GetWritableEvent() {
        return writable_event;
    }

    static void PostDestroy(uintptr_t arg);

private:
    KReadableEvent readable_event;
    KWritableEvent writable_event;
    KProcess* owner{};
    bool initialized{};
};

} // namespace Kernel

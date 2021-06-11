// Copyright 2021 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>

#include "core/hle/kernel/slab_helpers.h"
#include "core/hle/kernel/svc_types.h"
#include "core/hle/result.h"

union ResultCode;

namespace Core::Memory {
class Memory;
}

namespace Kernel {

class KernelCore;
class KProcess;

class KTransferMemory final
    : public KAutoObjectWithSlabHeapAndContainer<KTransferMemory, KAutoObjectWithList> {
    KERNEL_AUTOOBJECT_TRAITS(KTransferMemory, KAutoObject);

public:
    explicit KTransferMemory(KernelCore& kernel_);
    ~KTransferMemory() override;

    ResultCode Initialize(VAddr address_, std::size_t size_, Svc::MemoryPermission owner_perm_);

    void Finalize() override;

    bool IsInitialized() const override {
        return is_initialized;
    }

    uintptr_t GetPostDestroyArgument() const override {
        return reinterpret_cast<uintptr_t>(owner);
    }

    static void PostDestroy(uintptr_t arg);

    KProcess* GetOwner() const override {
        return owner;
    }

    VAddr GetSourceAddress() const {
        return address;
    }

    size_t GetSize() const {
        return is_initialized ? size : 0;
    }

private:
    KProcess* owner{};
    VAddr address{};
    Svc::MemoryPermission owner_perm{};
    size_t size{};
    bool is_initialized{};
};

} // namespace Kernel

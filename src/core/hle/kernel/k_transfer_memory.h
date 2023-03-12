// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/kernel/slab_helpers.h"
#include "core/hle/kernel/svc_types.h"
#include "core/hle/result.h"

union Result;

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
    explicit KTransferMemory(KernelCore& kernel);
    ~KTransferMemory() override;

    Result Initialize(VAddr address, std::size_t size, Svc::MemoryPermission owner_perm);

    void Finalize() override;

    bool IsInitialized() const override {
        return m_is_initialized;
    }

    uintptr_t GetPostDestroyArgument() const override {
        return reinterpret_cast<uintptr_t>(m_owner);
    }

    static void PostDestroy(uintptr_t arg);

    KProcess* GetOwner() const override {
        return m_owner;
    }

    VAddr GetSourceAddress() const {
        return m_address;
    }

    size_t GetSize() const {
        return m_is_initialized ? m_size : 0;
    }

private:
    KProcess* m_owner{};
    VAddr m_address{};
    Svc::MemoryPermission m_owner_perm{};
    size_t m_size{};
    bool m_is_initialized{};
};

} // namespace Kernel

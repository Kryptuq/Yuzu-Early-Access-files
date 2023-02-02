// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "common/common_types.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/slab_helpers.h"
#include "core/hle/result.h"

namespace Kernel {

class KDeviceAddressSpace final
    : public KAutoObjectWithSlabHeapAndContainer<KDeviceAddressSpace, KAutoObjectWithList> {
    KERNEL_AUTOOBJECT_TRAITS(KDeviceAddressSpace, KAutoObject);

public:
    explicit KDeviceAddressSpace(KernelCore& kernel);
    ~KDeviceAddressSpace();

    Result Initialize(u64 address, u64 size);
    void Finalize();

    bool IsInitialized() const {
        return m_is_initialized;
    }
    static void PostDestroy(uintptr_t arg) {}

    Result Attach(Svc::DeviceName device_name);
    Result Detach(Svc::DeviceName device_name);

    Result MapByForce(KPageTable* page_table, VAddr process_address, size_t size,
                      u64 device_address, u32 option) {
        R_RETURN(this->Map(page_table, process_address, size, device_address, option, false));
    }

    Result MapAligned(KPageTable* page_table, VAddr process_address, size_t size,
                      u64 device_address, u32 option) {
        R_RETURN(this->Map(page_table, process_address, size, device_address, option, true));
    }

    Result Unmap(KPageTable* page_table, VAddr process_address, size_t size, u64 device_address);

    static void Initialize();

private:
    Result Map(KPageTable* page_table, VAddr process_address, size_t size, u64 device_address,
               u32 option, bool is_aligned);

private:
    KLightLock m_lock;
    // KDevicePageTable m_table;
    u64 m_space_address{};
    u64 m_space_size{};
    bool m_is_initialized{};
};

} // namespace Kernel

// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/core.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_scoped_resource_reservation.h"
#include "core/hle/kernel/k_shared_memory.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_results.h"

namespace Kernel {

KSharedMemory::KSharedMemory(KernelCore& kernel_) : KAutoObjectWithSlabHeapAndContainer{kernel_} {}

KSharedMemory::~KSharedMemory() {
    kernel.GetSystemResourceLimit()->Release(LimitableResource::PhysicalMemory, size);
}

Result KSharedMemory::Initialize(Core::DeviceMemory& device_memory_, KProcess* owner_process_,
                                 KPageGroup&& page_list_, Svc::MemoryPermission owner_permission_,
                                 Svc::MemoryPermission user_permission_, PAddr physical_address_,
                                 std::size_t size_, std::string name_) {
    // Set members.
    owner_process = owner_process_;
    device_memory = &device_memory_;
    page_list = std::move(page_list_);
    owner_permission = owner_permission_;
    user_permission = user_permission_;
    physical_address = physical_address_;
    size = size_;
    name = std::move(name_);

    // Get the resource limit.
    KResourceLimit* reslimit = kernel.GetSystemResourceLimit();

    // Reserve memory for ourselves.
    KScopedResourceReservation memory_reservation(reslimit, LimitableResource::PhysicalMemory,
                                                  size_);
    R_UNLESS(memory_reservation.Succeeded(), ResultLimitReached);

    // Commit our reservation.
    memory_reservation.Commit();

    // Set our resource limit.
    resource_limit = reslimit;
    resource_limit->Open();

    // Mark initialized.
    is_initialized = true;

    // Clear all pages in the memory.
    std::memset(device_memory_.GetPointer<void>(physical_address_), 0, size_);

    return ResultSuccess;
}

void KSharedMemory::Finalize() {
    // Release the memory reservation.
    resource_limit->Release(LimitableResource::PhysicalMemory, size);
    resource_limit->Close();

    // Perform inherited finalization.
    KAutoObjectWithSlabHeapAndContainer<KSharedMemory, KAutoObjectWithList>::Finalize();
}

Result KSharedMemory::Map(KProcess& target_process, VAddr address, std::size_t map_size,
                          Svc::MemoryPermission permissions) {
    const u64 page_count{(map_size + PageSize - 1) / PageSize};

    if (page_list.GetNumPages() != page_count) {
        UNIMPLEMENTED_MSG("Page count does not match");
    }

    const Svc::MemoryPermission expected =
        &target_process == owner_process ? owner_permission : user_permission;

    if (permissions != expected) {
        UNIMPLEMENTED_MSG("Permission does not match");
    }

    return target_process.PageTable().MapPages(address, page_list, KMemoryState::Shared,
                                               ConvertToKMemoryPermission(permissions));
}

Result KSharedMemory::Unmap(KProcess& target_process, VAddr address, std::size_t unmap_size) {
    const u64 page_count{(unmap_size + PageSize - 1) / PageSize};

    if (page_list.GetNumPages() != page_count) {
        UNIMPLEMENTED_MSG("Page count does not match");
    }

    return target_process.PageTable().UnmapPages(address, page_list, KMemoryState::Shared);
}

} // namespace Kernel

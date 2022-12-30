// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/assert.h"
#include "core/core.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_scoped_resource_reservation.h"
#include "core/hle/kernel/k_shared_memory.h"
#include "core/hle/kernel/k_system_resource.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_results.h"

namespace Kernel {

KSharedMemory::KSharedMemory(KernelCore& kernel_) : KAutoObjectWithSlabHeapAndContainer{kernel_} {}

KSharedMemory::~KSharedMemory() {
    kernel.GetSystemResourceLimit()->Release(LimitableResource::PhysicalMemoryMax, size);
}

Result KSharedMemory::Initialize(Core::DeviceMemory& device_memory_, KProcess* owner_process_,
                                 Svc::MemoryPermission owner_permission_,
                                 Svc::MemoryPermission user_permission_, std::size_t size_,
                                 std::string name_) {
    // Set members.
    owner_process = owner_process_;
    device_memory = &device_memory_;
    owner_permission = owner_permission_;
    user_permission = user_permission_;
    size = Common::AlignUp(size_, PageSize);
    name = std::move(name_);

    const size_t num_pages = Common::DivideUp(size, PageSize);

    // Get the resource limit.
    KResourceLimit* reslimit = kernel.GetSystemResourceLimit();

    // Reserve memory for ourselves.
    KScopedResourceReservation memory_reservation(reslimit, LimitableResource::PhysicalMemoryMax,
                                                  size_);
    R_UNLESS(memory_reservation.Succeeded(), ResultLimitReached);

    // Allocate the memory.

    //! HACK: Open continuous mapping from sysmodule pool.
    auto option = KMemoryManager::EncodeOption(KMemoryManager::Pool::Secure,
                                               KMemoryManager::Direction::FromBack);
    physical_address = kernel.MemoryManager().AllocateAndOpenContinuous(num_pages, 1, option);
    R_UNLESS(physical_address != 0, ResultOutOfMemory);

    //! Insert the result into our page group.
    page_group.emplace(physical_address, num_pages);

    // Commit our reservation.
    memory_reservation.Commit();

    // Set our resource limit.
    resource_limit = reslimit;
    resource_limit->Open();

    // Mark initialized.
    is_initialized = true;

    // Clear all pages in the memory.
    for (const auto& block : page_group->Nodes()) {
        std::memset(device_memory_.GetPointer<void>(block.GetAddress()), 0, block.GetSize());
    }

    return ResultSuccess;
}

void KSharedMemory::Finalize() {
    // Close and finalize the page group.
    // page_group->Close();
    // page_group->Finalize();

    //! HACK: Manually close.
    for (const auto& block : page_group->Nodes()) {
        kernel.MemoryManager().Close(block.GetAddress(), block.GetNumPages());
    }

    // Release the memory reservation.
    resource_limit->Release(LimitableResource::PhysicalMemoryMax, size);
    resource_limit->Close();

    // Perform inherited finalization.
    KAutoObjectWithSlabHeapAndContainer<KSharedMemory, KAutoObjectWithList>::Finalize();
}

Result KSharedMemory::Map(KProcess& target_process, VAddr address, std::size_t map_size,
                          Svc::MemoryPermission map_perm) {
    // Validate the size.
    R_UNLESS(size == map_size, ResultInvalidSize);

    // Validate the permission.
    const Svc::MemoryPermission test_perm =
        &target_process == owner_process ? owner_permission : user_permission;
    if (test_perm == Svc::MemoryPermission::DontCare) {
        ASSERT(map_perm == Svc::MemoryPermission::Read || map_perm == Svc::MemoryPermission::Write);
    } else {
        R_UNLESS(map_perm == test_perm, ResultInvalidNewMemoryPermission);
    }

    return target_process.PageTable().MapPages(address, *page_group, KMemoryState::Shared,
                                               ConvertToKMemoryPermission(map_perm));
}

Result KSharedMemory::Unmap(KProcess& target_process, VAddr address, std::size_t unmap_size) {
    // Validate the size.
    R_UNLESS(size == unmap_size, ResultInvalidSize);

    return target_process.PageTable().UnmapPages(address, *page_group, KMemoryState::Shared);
}

} // namespace Kernel

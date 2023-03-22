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

KSharedMemory::KSharedMemory(KernelCore& kernel) : KAutoObjectWithSlabHeapAndContainer{kernel} {}
KSharedMemory::~KSharedMemory() = default;

Result KSharedMemory::Initialize(Core::DeviceMemory& device_memory, KProcess* owner_process,
                                 Svc::MemoryPermission owner_permission,
                                 Svc::MemoryPermission user_permission, std::size_t size) {
    // Set members.
    m_owner_process = owner_process;
    m_device_memory = std::addressof(device_memory);
    m_owner_permission = owner_permission;
    m_user_permission = user_permission;
    m_size = Common::AlignUp(size, PageSize);

    const size_t num_pages = Common::DivideUp(size, PageSize);

    // Get the resource limit.
    KResourceLimit* reslimit = m_kernel.GetSystemResourceLimit();

    // Reserve memory for ourselves.
    KScopedResourceReservation memory_reservation(reslimit, LimitableResource::PhysicalMemoryMax,
                                                  size);
    R_UNLESS(memory_reservation.Succeeded(), ResultLimitReached);

    // Allocate the memory.

    //! HACK: Open continuous mapping from sysmodule pool.
    auto option = KMemoryManager::EncodeOption(KMemoryManager::Pool::Secure,
                                               KMemoryManager::Direction::FromBack);
    m_physical_address = m_kernel.MemoryManager().AllocateAndOpenContinuous(num_pages, 1, option);
    R_UNLESS(m_physical_address != 0, ResultOutOfMemory);

    //! Insert the result into our page group.
    m_page_group.emplace(m_kernel,
                         std::addressof(m_kernel.GetSystemSystemResource().GetBlockInfoManager()));
    m_page_group->AddBlock(m_physical_address, num_pages);

    // Commit our reservation.
    memory_reservation.Commit();

    // Set our resource limit.
    m_resource_limit = reslimit;
    m_resource_limit->Open();

    // Mark initialized.
    m_is_initialized = true;

    // Clear all pages in the memory.
    for (const auto& block : *m_page_group) {
        std::memset(m_device_memory->GetPointer<void>(block.GetAddress()), 0, block.GetSize());
    }

    R_SUCCEED();
}

void KSharedMemory::Finalize() {
    // Close and finalize the page group.
    m_page_group->Close();
    m_page_group->Finalize();

    // Release the memory reservation.
    m_resource_limit->Release(LimitableResource::PhysicalMemoryMax, m_size);
    m_resource_limit->Close();
}

Result KSharedMemory::Map(KProcess& target_process, KProcessAddress address, std::size_t map_size,
                          Svc::MemoryPermission map_perm) {
    // Validate the size.
    R_UNLESS(m_size == map_size, ResultInvalidSize);

    // Validate the permission.
    const Svc::MemoryPermission test_perm =
        std::addressof(target_process) == m_owner_process ? m_owner_permission : m_user_permission;
    if (test_perm == Svc::MemoryPermission::DontCare) {
        ASSERT(map_perm == Svc::MemoryPermission::Read || map_perm == Svc::MemoryPermission::Write);
    } else {
        R_UNLESS(map_perm == test_perm, ResultInvalidNewMemoryPermission);
    }

    R_RETURN(target_process.PageTable().MapPageGroup(address, *m_page_group, KMemoryState::Shared,
                                                     ConvertToKMemoryPermission(map_perm)));
}

Result KSharedMemory::Unmap(KProcess& target_process, KProcessAddress address,
                            std::size_t unmap_size) {
    // Validate the size.
    R_UNLESS(m_size == unmap_size, ResultInvalidSize);

    R_RETURN(
        target_process.PageTable().UnmapPageGroup(address, *m_page_group, KMemoryState::Shared));
}

} // namespace Kernel

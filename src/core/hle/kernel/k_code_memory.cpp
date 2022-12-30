// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/alignment.h"
#include "common/common_types.h"
#include "core/device_memory.h"
#include "core/hle/kernel/k_code_memory.h"
#include "core/hle/kernel/k_light_lock.h"
#include "core/hle/kernel/k_memory_block.h"
#include "core/hle/kernel/k_page_group.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/slab_helpers.h"
#include "core/hle/kernel/svc_types.h"
#include "core/hle/result.h"

namespace Kernel {

KCodeMemory::KCodeMemory(KernelCore& kernel_)
    : KAutoObjectWithSlabHeapAndContainer{kernel_}, m_lock(kernel_) {}

Result KCodeMemory::Initialize(Core::DeviceMemory& device_memory, VAddr addr, size_t size) {
    // Set members.
    m_owner = kernel.CurrentProcess();

    // Get the owner page table.
    auto& page_table = m_owner->PageTable();

    // Construct the page group.
    m_page_group = {};

    // Lock the memory.
    R_TRY(page_table.LockForCodeMemory(&m_page_group, addr, size))

    // Clear the memory.
    for (const auto& block : m_page_group.Nodes()) {
        std::memset(device_memory.GetPointer<void>(block.GetAddress()), 0xFF, block.GetSize());
    }

    // Set remaining tracking members.
    m_owner->Open();
    m_address = addr;
    m_is_initialized = true;
    m_is_owner_mapped = false;
    m_is_mapped = false;

    // We succeeded.
    return ResultSuccess;
}

void KCodeMemory::Finalize() {
    // Unlock.
    if (!m_is_mapped && !m_is_owner_mapped) {
        const size_t size = m_page_group.GetNumPages() * PageSize;
        m_owner->PageTable().UnlockForCodeMemory(m_address, size, m_page_group);
    }

    // Close the page group.
    m_page_group = {};

    // Close our reference to our owner.
    m_owner->Close();
}

Result KCodeMemory::Map(VAddr address, size_t size) {
    // Validate the size.
    R_UNLESS(m_page_group.GetNumPages() == Common::DivideUp(size, PageSize), ResultInvalidSize);

    // Lock ourselves.
    KScopedLightLock lk(m_lock);

    // Ensure we're not already mapped.
    R_UNLESS(!m_is_mapped, ResultInvalidState);

    // Map the memory.
    R_TRY(kernel.CurrentProcess()->PageTable().MapPages(
        address, m_page_group, KMemoryState::CodeOut, KMemoryPermission::UserReadWrite));

    // Mark ourselves as mapped.
    m_is_mapped = true;

    return ResultSuccess;
}

Result KCodeMemory::Unmap(VAddr address, size_t size) {
    // Validate the size.
    R_UNLESS(m_page_group.GetNumPages() == Common::DivideUp(size, PageSize), ResultInvalidSize);

    // Lock ourselves.
    KScopedLightLock lk(m_lock);

    // Unmap the memory.
    R_TRY(kernel.CurrentProcess()->PageTable().UnmapPages(address, m_page_group,
                                                          KMemoryState::CodeOut));

    // Mark ourselves as unmapped.
    m_is_mapped = false;

    return ResultSuccess;
}

Result KCodeMemory::MapToOwner(VAddr address, size_t size, Svc::MemoryPermission perm) {
    // Validate the size.
    R_UNLESS(m_page_group.GetNumPages() == Common::DivideUp(size, PageSize), ResultInvalidSize);

    // Lock ourselves.
    KScopedLightLock lk(m_lock);

    // Ensure we're not already mapped.
    R_UNLESS(!m_is_owner_mapped, ResultInvalidState);

    // Convert the memory permission.
    KMemoryPermission k_perm{};
    switch (perm) {
    case Svc::MemoryPermission::Read:
        k_perm = KMemoryPermission::UserRead;
        break;
    case Svc::MemoryPermission::ReadExecute:
        k_perm = KMemoryPermission::UserReadExecute;
        break;
    default:
        // Already validated by ControlCodeMemory svc
        UNREACHABLE();
    }

    // Map the memory.
    R_TRY(
        m_owner->PageTable().MapPages(address, m_page_group, KMemoryState::GeneratedCode, k_perm));

    // Mark ourselves as mapped.
    m_is_owner_mapped = true;

    return ResultSuccess;
}

Result KCodeMemory::UnmapFromOwner(VAddr address, size_t size) {
    // Validate the size.
    R_UNLESS(m_page_group.GetNumPages() == Common::DivideUp(size, PageSize), ResultInvalidSize);

    // Lock ourselves.
    KScopedLightLock lk(m_lock);

    // Unmap the memory.
    R_TRY(m_owner->PageTable().UnmapPages(address, m_page_group, KMemoryState::GeneratedCode));

    // Mark ourselves as unmapped.
    m_is_owner_mapped = false;

    return ResultSuccess;
}

} // namespace Kernel

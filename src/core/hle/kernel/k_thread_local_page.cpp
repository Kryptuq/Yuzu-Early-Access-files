// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/scope_exit.h"
#include "core/core.h"

#include "core/hle/kernel/k_memory_block.h"
#include "core/hle/kernel/k_page_buffer.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_thread_local_page.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel {

Result KThreadLocalPage::Initialize(KernelCore& kernel, KProcess* process) {
    // Set that this process owns us.
    m_owner = process;
    m_kernel = std::addressof(kernel);

    // Allocate a new page.
    KPageBuffer* page_buf = KPageBuffer::Allocate(kernel);
    R_UNLESS(page_buf != nullptr, ResultOutOfMemory);
    auto page_buf_guard = SCOPE_GUARD({ KPageBuffer::Free(kernel, page_buf); });

    // Map the address in.
    const auto phys_addr = kernel.System().DeviceMemory().GetPhysicalAddr(page_buf);
    R_TRY(m_owner->PageTable().MapPages(std::addressof(m_virt_addr), 1, PageSize, phys_addr,
                                        KMemoryState::ThreadLocal,
                                        KMemoryPermission::UserReadWrite));

    // We succeeded.
    page_buf_guard.Cancel();

    return ResultSuccess;
}

Result KThreadLocalPage::Finalize() {
    // Get the physical address of the page.
    const KPhysicalAddress phys_addr = m_owner->PageTable().GetPhysicalAddr(m_virt_addr);
    ASSERT(phys_addr);

    // Unmap the page.
    R_TRY(m_owner->PageTable().UnmapPages(this->GetAddress(), 1, KMemoryState::ThreadLocal));

    // Free the page.
    KPageBuffer::Free(*m_kernel, KPageBuffer::FromPhysicalAddress(m_kernel->System(), phys_addr));

    return ResultSuccess;
}

KProcessAddress KThreadLocalPage::Reserve() {
    for (size_t i = 0; i < m_is_region_free.size(); i++) {
        if (m_is_region_free[i]) {
            m_is_region_free[i] = false;
            return this->GetRegionAddress(i);
        }
    }

    return 0;
}

void KThreadLocalPage::Release(KProcessAddress addr) {
    m_is_region_free[this->GetRegionIndex(addr)] = true;
}

} // namespace Kernel

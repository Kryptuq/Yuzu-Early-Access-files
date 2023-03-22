// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/k_transfer_memory.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel {

KTransferMemory::KTransferMemory(KernelCore& kernel)
    : KAutoObjectWithSlabHeapAndContainer{kernel} {}

KTransferMemory::~KTransferMemory() = default;

Result KTransferMemory::Initialize(KProcessAddress address, std::size_t size,
                                   Svc::MemoryPermission owner_perm) {
    // Set members.
    m_owner = GetCurrentProcessPointer(m_kernel);

    // TODO(bunnei): Lock for transfer memory

    // Set remaining tracking members.
    m_owner->Open();
    m_owner_perm = owner_perm;
    m_address = address;
    m_size = size;
    m_is_initialized = true;

    R_SUCCEED();
}

void KTransferMemory::Finalize() {}

void KTransferMemory::PostDestroy(uintptr_t arg) {
    KProcess* owner = reinterpret_cast<KProcess*>(arg);
    owner->GetResourceLimit()->Release(LimitableResource::TransferMemoryCountMax, 1);
    owner->Close();
}

} // namespace Kernel

// Copyright 2021 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/k_transfer_memory.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel {

KTransferMemory::KTransferMemory(KernelCore& kernel_)
    : KAutoObjectWithSlabHeapAndContainer{kernel_} {}

KTransferMemory::~KTransferMemory() = default;

ResultCode KTransferMemory::Initialize(VAddr address_, std::size_t size_,
                                       Svc::MemoryPermission owner_perm_) {
    // Set members.
    owner = kernel.CurrentProcess();

    // TODO(bunnei): Lock for transfer memory

    // Set remaining tracking members.
    owner->Open();
    owner_perm = owner_perm_;
    address = address_;
    size = size_;
    is_initialized = true;

    return ResultSuccess;
}

void KTransferMemory::Finalize() {
    // Perform inherited finalization.
    KAutoObjectWithSlabHeapAndContainer<KTransferMemory, KAutoObjectWithList>::Finalize();
}

void KTransferMemory::PostDestroy(uintptr_t arg) {
    KProcess* owner = reinterpret_cast<KProcess*>(arg);
    owner->GetResourceLimit()->Release(LimitableResource::TransferMemory, 1);
    owner->Close();
}

} // namespace Kernel

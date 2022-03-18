// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/alignment.h"
#include "common/assert.h"
#include "core/core.h"
#include "core/device_memory.h"
#include "core/hle/kernel/k_page_buffer.h"
#include "core/hle/kernel/memory_types.h"

namespace Kernel {

KPageBuffer* KPageBuffer::FromPhysicalAddress(Core::System& system, PAddr phys_addr) {
    ASSERT(Common::IsAligned(phys_addr, PageSize));
    return reinterpret_cast<KPageBuffer*>(system.DeviceMemory().GetPointer(phys_addr));
}

} // namespace Kernel

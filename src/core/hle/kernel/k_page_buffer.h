// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>

#include "common/alignment.h"
#include "common/assert.h"
#include "common/common_types.h"
#include "core/core.h"
#include "core/device_memory.h"
#include "core/hle/kernel/memory_types.h"

namespace Kernel {

class KPageBuffer final : public KSlabAllocated<KPageBuffer> {
private:
    alignas(PageSize) std::array<u8, PageSize> m_buffer;

public:
    KPageBuffer() {
        std::memset(&m_buffer, 0, m_buffer.size());
    }

    PAddr GetPhysicalAddress(Core::System& system) const {
        return system.DeviceMemory().GetPhysicalAddr(this);
    }

    static KPageBuffer* FromPhysicalAddress(Core::System& system, PAddr phys_addr) {
        ASSERT(Common::IsAligned(phys_addr, PageSize));
        return reinterpret_cast<KPageBuffer*>(system.DeviceMemory().GetPointer(phys_addr));
    }
};

static_assert(sizeof(KPageBuffer) == PageSize);
static_assert(alignof(KPageBuffer) == PageSize);

} // namespace Kernel

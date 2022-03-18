// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>

#include "common/common_types.h"
#include "core/device_memory.h"
#include "core/hle/kernel/memory_types.h"
#include "core/hle/kernel/slab_helpers.h"

namespace Kernel {

class KPageBuffer final : public KSlabAllocated<KPageBuffer> {
public:
    KPageBuffer() = default;

    static KPageBuffer* FromPhysicalAddress(Core::System& system, PAddr phys_addr);

private:
    [[maybe_unused]] alignas(PageSize) std::array<u8, PageSize> m_buffer{};
};

static_assert(sizeof(KPageBuffer) == PageSize);
static_assert(alignof(KPageBuffer) == PageSize);

} // namespace Kernel

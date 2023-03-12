// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "common/common_types.h"
#include "core/hle/kernel/memory_types.h"
#include "core/hle/kernel/slab_helpers.h"

namespace Kernel {

class KernelCore;

class KPageBufferSlabHeap : protected impl::KSlabHeapImpl {
public:
    static constexpr size_t BufferSize = PageSize;

public:
    void Initialize(Core::System& system);
};

class KPageBuffer final : public KSlabAllocated<KPageBuffer> {
public:
    explicit KPageBuffer(KernelCore&) {}
    KPageBuffer() = default;

    static KPageBuffer* FromPhysicalAddress(Core::System& system, PAddr phys_addr);

private:
    alignas(PageSize) std::array<u8, PageSize> m_buffer{};
};
static_assert(sizeof(KPageBuffer) == KPageBufferSlabHeap::BufferSize);

} // namespace Kernel

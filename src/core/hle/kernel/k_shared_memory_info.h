// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/intrusive/list.hpp>

#include "core/hle/kernel/slab_helpers.h"

namespace Kernel {

class KSharedMemory;

class KSharedMemoryInfo final : public KSlabAllocated<KSharedMemoryInfo>,
                                public boost::intrusive::list_base_hook<> {

public:
    explicit KSharedMemoryInfo(KernelCore&) {}
    KSharedMemoryInfo() = default;

    constexpr void Initialize(KSharedMemory* m) {
        m_shared_memory = m;
        m_reference_count = 0;
    }

    constexpr KSharedMemory* GetSharedMemory() const {
        return m_shared_memory;
    }

    constexpr void Open() {
        ++m_reference_count;
        ASSERT(m_reference_count > 0);
    }

    constexpr bool Close() {
        ASSERT(m_reference_count > 0);
        return (--m_reference_count) == 0;
    }

private:
    KSharedMemory* m_shared_memory{};
    size_t m_reference_count{};
};

} // namespace Kernel

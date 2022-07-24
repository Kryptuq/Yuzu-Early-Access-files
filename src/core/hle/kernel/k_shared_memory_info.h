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
    explicit KSharedMemoryInfo() = default;

    constexpr void Initialize(KSharedMemory* shmem) {
        shared_memory = shmem;
    }

    constexpr KSharedMemory* GetSharedMemory() const {
        return shared_memory;
    }

    constexpr void Open() {
        ++reference_count;
    }

    constexpr bool Close() {
        return (--reference_count) == 0;
    }

private:
    KSharedMemory* shared_memory{};
    size_t reference_count{};
};

} // namespace Kernel

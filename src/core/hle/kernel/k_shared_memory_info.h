// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>

#include <boost/intrusive/list.hpp>

#include "common/assert.h"
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

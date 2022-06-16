// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv3 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

#include "common/address_space.h"
#include "video_core/host1x/syncpoint_manager.h"
#include "video_core/memory_manager.h"

namespace Core {
class System;
} // namespace Core

namespace Tegra {

namespace Host1x {

class Host1x {
public:
    Host1x(Core::System& system);

    SyncpointManager& GetSyncpointManager() {
        return syncpoint_manager;
    }

    const SyncpointManager& GetSyncpointManager() const {
        return syncpoint_manager;
    }

    Tegra::MemoryManager& MemoryManager() {
        return memory_manager;
    }

    const Tegra::MemoryManager& MemoryManager() const {
        return memory_manager;
    }

    Common::FlatAllocator<u32, 0, 32>& Allocator() {
        return *allocator;
    }

    const Common::FlatAllocator<u32, 0, 32>& Allocator() const {
        return *allocator;
    }

private:
    Core::System& system;
    SyncpointManager syncpoint_manager;
    Tegra::MemoryManager memory_manager;
    std::unique_ptr<Common::FlatAllocator<u32, 0, 32>> allocator;
};

} // namespace Host1x

} // namespace Tegra

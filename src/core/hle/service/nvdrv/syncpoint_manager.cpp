// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "core/hle/service/nvdrv/syncpoint_manager.h"
#include "video_core/gpu.h"

namespace Service::Nvidia {

SyncpointManager::SyncpointManager(Tegra::GPU& gpu_) : gpu{gpu_} {}

SyncpointManager::~SyncpointManager() = default;

u32 SyncpointManager::RefreshSyncpoint(u32 syncpoint_id) {
    syncpoints[syncpoint_id].min = gpu.GetSyncpointValue(syncpoint_id);
    return GetSyncpointMin(syncpoint_id);
}

u32 SyncpointManager::AllocateSyncpoint() {
    for (u32 syncpoint_id = 1; syncpoint_id < MaxSyncPoints; syncpoint_id++) {
        if (!syncpoints[syncpoint_id].is_allocated) {
            syncpoints[syncpoint_id].is_allocated = true;
            return syncpoint_id;
        }
    }
    UNREACHABLE_MSG("No more available syncpoints!");
    return {};
}

u32 SyncpointManager::IncreaseSyncpoint(u32 syncpoint_id, u32 value) {
    for (u32 index = 0; index < value; ++index) {
        syncpoints[syncpoint_id].max.fetch_add(1, std::memory_order_relaxed);
    }

    return GetSyncpointMax(syncpoint_id);
}

} // namespace Service::Nvidia

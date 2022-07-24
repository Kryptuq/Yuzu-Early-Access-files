// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>

#include "common/common_types.h"
#include "core/hle/service/nvdrv/nvdata.h"

namespace Tegra {
class GPU;
}

namespace Service::Nvidia {

class SyncpointManager final {
public:
    explicit SyncpointManager(Tegra::GPU& gpu_);
    ~SyncpointManager();

    /**
     * Returns true if the specified syncpoint is expired for the given value.
     * @param syncpoint_id Syncpoint ID to check.
     * @param value Value to check against the specified syncpoint.
     * @returns True if the specified syncpoint is expired for the given value, otherwise False.
     */
    bool IsSyncpointExpired(u32 syncpoint_id, u32 value) const {
        return (GetSyncpointMax(syncpoint_id) - value) >= (GetSyncpointMin(syncpoint_id) - value);
    }

    /**
     * Gets the lower bound for the specified syncpoint.
     * @param syncpoint_id Syncpoint ID to get the lower bound for.
     * @returns The lower bound for the specified syncpoint.
     */
    u32 GetSyncpointMin(u32 syncpoint_id) const {
        return syncpoints.at(syncpoint_id).min.load(std::memory_order_relaxed);
    }

    /**
     * Gets the uper bound for the specified syncpoint.
     * @param syncpoint_id Syncpoint ID to get the upper bound for.
     * @returns The upper bound for the specified syncpoint.
     */
    u32 GetSyncpointMax(u32 syncpoint_id) const {
        return syncpoints.at(syncpoint_id).max.load(std::memory_order_relaxed);
    }

    /**
     * Refreshes the minimum value for the specified syncpoint.
     * @param syncpoint_id Syncpoint ID to be refreshed.
     * @returns The new syncpoint minimum value.
     */
    u32 RefreshSyncpoint(u32 syncpoint_id);

    /**
     * Allocates a new syncoint.
     * @returns The syncpoint ID for the newly allocated syncpoint.
     */
    u32 AllocateSyncpoint();

    /**
     * Increases the maximum value for the specified syncpoint.
     * @param syncpoint_id Syncpoint ID to be increased.
     * @param value Value to increase the specified syncpoint by.
     * @returns The new syncpoint maximum value.
     */
    u32 IncreaseSyncpoint(u32 syncpoint_id, u32 value);

private:
    struct Syncpoint {
        std::atomic<u32> min;
        std::atomic<u32> max;
        std::atomic<bool> is_allocated;
    };

    std::array<Syncpoint, MaxSyncPoints> syncpoints{};

    Tegra::GPU& gpu;
};

} // namespace Service::Nvidia

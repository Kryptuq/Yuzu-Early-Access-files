// SPDX-FileCopyrightText: 2022 yuzu Emulator Project
// SPDX-FileCopyrightText: 2022 Skyline Team and Contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/assert.h"
#include "core/hle/service/nvdrv/core/syncpoint_manager.h"
#include "video_core/host1x/host1x.h"

namespace Service::Nvidia::NvCore {

SyncpointManager::SyncpointManager(Tegra::Host1x::Host1x& host1x_) : host1x{host1x_} {
    constexpr u32 VBlank0SyncpointId{26};
    constexpr u32 VBlank1SyncpointId{27};

    // Reserve both vblank syncpoints as client managed as they use Continuous Mode
    // Refer to section 14.3.5.3 of the TRM for more information on Continuous Mode
    // https://github.com/Jetson-TX1-AndroidTV/android_kernel_jetson_tx1_hdmi_primary/blob/8f74a72394efb871cb3f886a3de2998cd7ff2990/drivers/gpu/host1x/drm/dc.c#L660
    ReserveSyncpoint(VBlank0SyncpointId, true);
    ReserveSyncpoint(VBlank1SyncpointId, true);

    for (u32 syncpointId : channel_syncpoints) {
        if (syncpointId) {
            ReserveSyncpoint(syncpointId, false);
        }
    }
}

SyncpointManager::~SyncpointManager() = default;

u32 SyncpointManager::ReserveSyncpoint(u32 id, bool clientManaged) {
    if (syncpoints.at(id).reserved) {
        UNREACHABLE_MSG("Requested syncpoint is in use");
        return 0;
    }

    syncpoints.at(id).reserved = true;
    syncpoints.at(id).interfaceManaged = clientManaged;

    return id;
}

u32 SyncpointManager::FindFreeSyncpoint() {
    for (u32 i{1}; i < syncpoints.size(); i++) {
        if (!syncpoints[i].reserved) {
            return i;
        }
    }
    UNREACHABLE_MSG("Failed to find a free syncpoint!");
    return 0;
}

u32 SyncpointManager::AllocateSyncpoint(bool clientManaged) {
    std::lock_guard lock(reservation_lock);
    return ReserveSyncpoint(FindFreeSyncpoint(), clientManaged);
}

void SyncpointManager::FreeSyncpoint(u32 id) {
    std::lock_guard lock(reservation_lock);
    ASSERT(syncpoints.at(id).reserved);
    syncpoints.at(id).reserved = false;
}

bool SyncpointManager::IsSyncpointAllocated(u32 id) {
    return (id <= SyncpointCount) && syncpoints[id].reserved;
}

bool SyncpointManager::HasSyncpointExpired(u32 id, u32 threshold) {
    const SyncpointInfo& syncpoint{syncpoints.at(id)};

    if (!syncpoint.reserved) {
        UNREACHABLE();
        return 0;
    }

    // If the interface manages counters then we don't keep track of the maximum value as it handles
    // sanity checking the values then
    if (syncpoint.interfaceManaged) {
        return static_cast<s32>(syncpoint.counterMin - threshold) >= 0;
    } else {
        return (syncpoint.counterMax - threshold) >= (syncpoint.counterMin - threshold);
    }
}

u32 SyncpointManager::IncrementSyncpointMaxExt(u32 id, u32 amount) {
    if (!syncpoints.at(id).reserved) {
        UNREACHABLE();
        return 0;
    }

    return syncpoints.at(id).counterMax += amount;
}

u32 SyncpointManager::ReadSyncpointMinValue(u32 id) {
    if (!syncpoints.at(id).reserved) {
        UNREACHABLE();
        return 0;
    }

    return syncpoints.at(id).counterMin;
}

u32 SyncpointManager::UpdateMin(u32 id) {
    if (!syncpoints.at(id).reserved) {
        UNREACHABLE();
        return 0;
    }

    syncpoints.at(id).counterMin = host1x.GetSyncpointManager().GetHostSyncpointValue(id);
    return syncpoints.at(id).counterMin;
}

NvFence SyncpointManager::GetSyncpointFence(u32 id) {
    if (!syncpoints.at(id).reserved) {
        UNREACHABLE();
        return NvFence{};
    }

    return {.id = static_cast<s32>(id), .value = syncpoints.at(id).counterMax};
}

} // namespace Service::Nvidia::NvCore

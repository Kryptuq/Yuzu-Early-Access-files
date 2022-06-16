// SPDX-FileCopyrightText: Ryujinx Team and Contributors
// SPDX-License-Identifier: MIT

#include <algorithm>
#include "sync_manager.h"
#include "video_core/host1x/host1x.h"
#include "video_core/host1x/syncpoint_manager.h"

namespace Tegra {
namespace Host1x {

SyncptIncrManager::SyncptIncrManager(Host1x& host1x_) : host1x(host1x_) {}
SyncptIncrManager::~SyncptIncrManager() = default;

void SyncptIncrManager::Increment(u32 id) {
    increments.emplace_back(0, 0, id, true);
    IncrementAllDone();
}

u32 SyncptIncrManager::IncrementWhenDone(u32 class_id, u32 id) {
    const u32 handle = current_id++;
    increments.emplace_back(handle, class_id, id);
    return handle;
}

void SyncptIncrManager::SignalDone(u32 handle) {
    const auto done_incr =
        std::find_if(increments.begin(), increments.end(),
                     [handle](const SyncptIncr& incr) { return incr.id == handle; });
    if (done_incr != increments.cend()) {
        done_incr->complete = true;
    }
    IncrementAllDone();
}

void SyncptIncrManager::IncrementAllDone() {
    std::size_t done_count = 0;
    for (; done_count < increments.size(); ++done_count) {
        if (!increments[done_count].complete) {
            break;
        }
        auto& syncpoint_manager = host1x.GetSyncpointManager();
        syncpoint_manager.IncrementGuest(increments[done_count].syncpt_id);
        syncpoint_manager.IncrementHost(increments[done_count].syncpt_id);
    }
    increments.erase(increments.begin(), increments.begin() + done_count);
}

} // namespace Host1x
} // namespace Tegra

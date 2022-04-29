// SPDX-FileCopyrightText: Ryujinx Team and Contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <mutex>
#include <vector>
#include "common/common_types.h"

namespace Tegra {
class GPU;
struct SyncptIncr {
    u32 id;
    u32 class_id;
    u32 syncpt_id;
    bool complete;

    SyncptIncr(u32 id_, u32 class_id_, u32 syncpt_id_, bool done = false)
        : id(id_), class_id(class_id_), syncpt_id(syncpt_id_), complete(done) {}
};

class SyncptIncrManager {
public:
    explicit SyncptIncrManager(GPU& gpu);
    ~SyncptIncrManager();

    /// Add syncpoint id and increment all
    void Increment(u32 id);

    /// Returns a handle to increment later
    u32 IncrementWhenDone(u32 class_id, u32 id);

    /// IncrememntAllDone, including handle
    void SignalDone(u32 handle);

    /// Increment all sequential pending increments that are already done.
    void IncrementAllDone();

private:
    std::vector<SyncptIncr> increments;
    std::mutex increment_lock;
    u32 current_id{};

    GPU& gpu;
};

} // namespace Tegra

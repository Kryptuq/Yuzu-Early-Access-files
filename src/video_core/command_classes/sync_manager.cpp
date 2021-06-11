// MIT License
//
// Copyright (c) Ryujinx Team and Contributors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include <algorithm>
#include "sync_manager.h"
#include "video_core/gpu.h"

namespace Tegra {
SyncptIncrManager::SyncptIncrManager(GPU& gpu_) : gpu(gpu_) {}
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
        gpu.IncrementSyncPoint(increments[done_count].syncpt_id);
    }
    increments.erase(increments.begin(), increments.begin() + done_count);
}
} // namespace Tegra

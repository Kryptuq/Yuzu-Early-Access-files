// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <limits>
#include <vector>

#include "common/common_types.h"
#include "video_core/guest_driver.h"

namespace VideoCore {

void GuestDriverProfile::DeduceTextureHandlerSize(std::vector<u32> bound_offsets) {
    if (texture_handler_size) {
        return;
    }
    const std::size_t size = bound_offsets.size();
    if (size < 2) {
        return;
    }
    std::sort(bound_offsets.begin(), bound_offsets.end(), std::less{});
    u32 min_val = std::numeric_limits<u32>::max();
    for (std::size_t i = 1; i < size; ++i) {
        if (bound_offsets[i] == bound_offsets[i - 1]) {
            continue;
        }
        const u32 new_min = bound_offsets[i] - bound_offsets[i - 1];
        min_val = std::min(min_val, new_min);
    }
    if (min_val > 2) {
        return;
    }
    texture_handler_size = min_texture_handler_size * min_val;
}

} // namespace VideoCore

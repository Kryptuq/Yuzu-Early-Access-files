// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021 yuzu Emulator Project
// Copyright 2014 The Android Open Source Project
// Parts of this implementation were base on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/include/gui/BufferItem.h

#pragma once

#include <memory>

#include "common/common_types.h"
#include "common/math_util.h"
#include "core/hle/service/nvflinger/ui/fence.h"
#include "core/hle/service/nvflinger/window.h"

namespace Service::android {

class GraphicBuffer;

class BufferItem final {
public:
    constexpr BufferItem() = default;

    std::shared_ptr<GraphicBuffer> graphic_buffer;
    Fence fence;
    Common::Rectangle<s32> crop;
    NativeWindowTransform transform{};
    u32 scaling_mode{};
    s64 timestamp{};
    bool is_auto_timestamp{};
    u64 frame_number{};

    // The default value for buf, used to indicate this doesn't correspond to a slot.
    static constexpr s32 INVALID_BUFFER_SLOT = -1;
    union {
        s32 slot{INVALID_BUFFER_SLOT};
        s32 buf;
    };

    bool is_droppable{};
    bool acquire_called{};
    bool transform_to_display_inverse{};
    s32 swap_interval{};
};

} // namespace Service::android

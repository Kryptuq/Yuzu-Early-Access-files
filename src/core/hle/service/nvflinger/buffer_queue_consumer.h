// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2014 The Android Open Source Project
// SPDX-License-Identifier: GPL-3.0-or-later
// Parts of this implementation were based on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/include/gui/BufferQueueConsumer.h

#pragma once

#include <chrono>
#include <memory>

#include "common/common_types.h"
#include "core/hle/service/nvflinger/buffer_queue_defs.h"
#include "core/hle/service/nvflinger/status.h"

namespace Service::Nvidia::NvCore {
class NvMap;
} // namespace Service::Nvidia::NvCore

namespace Service::android {

class BufferItem;
class BufferQueueCore;
class IConsumerListener;

class BufferQueueConsumer final {
public:
    explicit BufferQueueConsumer(std::shared_ptr<BufferQueueCore> core_,
                                 Service::Nvidia::NvCore::NvMap& nvmap_);
    ~BufferQueueConsumer();

    Status AcquireBuffer(BufferItem* out_buffer, std::chrono::nanoseconds expected_present);
    Status ReleaseBuffer(s32 slot, u64 frame_number, const Fence& release_fence);
    Status Connect(std::shared_ptr<IConsumerListener> consumer_listener, bool controlled_by_app);
    Status GetReleasedBuffers(u64* out_slot_mask);

private:
    std::shared_ptr<BufferQueueCore> core;
    BufferQueueDefs::SlotsType& slots;
    Service::Nvidia::NvCore::NvMap& nvmap;
};

} // namespace Service::android

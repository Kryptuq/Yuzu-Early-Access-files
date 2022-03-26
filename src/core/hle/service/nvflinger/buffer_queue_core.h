// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021 yuzu Emulator Project
// Copyright 2014 The Android Open Source Project
// Parts of this implementation were base on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/include/gui/BufferQueueCore.h

#pragma once

#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "core/hle/service/nvflinger/buffer_item.h"
#include "core/hle/service/nvflinger/buffer_queue_defs.h"
#include "core/hle/service/nvflinger/pixel_format.h"
#include "core/hle/service/nvflinger/status.h"
#include "core/hle/service/nvflinger/window.h"

namespace Service::android {

class IConsumerListener;
class IProducerListener;

class BufferQueueCore final {
    friend class BufferQueueProducer;
    friend class BufferQueueConsumer;

public:
    static constexpr s32 INVALID_BUFFER_SLOT = BufferItem::INVALID_BUFFER_SLOT;

    BufferQueueCore();
    ~BufferQueueCore();

    void NotifyShutdown();

private:
    void SignalDequeueCondition();
    bool WaitForDequeueCondition();

    s32 GetMinUndequeuedBufferCountLocked(bool async) const;
    s32 GetMinMaxBufferCountLocked(bool async) const;
    s32 GetMaxBufferCountLocked(bool async) const;
    s32 GetPreallocatedBufferCountLocked() const;
    void FreeBufferLocked(s32 slot);
    void FreeAllBuffersLocked();
    bool StillTracking(const BufferItem& item) const;
    void WaitWhileAllocatingLocked() const;

private:
    mutable std::mutex mutex;
    bool is_abandoned{};
    bool consumer_controlled_by_app{};
    std::shared_ptr<IConsumerListener> consumer_listener;
    u32 consumer_usage_bit{};
    NativeWindowApi connected_api{NativeWindowApi::NoConnectedApi};
    std::shared_ptr<IProducerListener> connected_producer_listener;
    BufferQueueDefs::SlotsType slots{};
    std::vector<BufferItem> queue;
    s32 override_max_buffer_count{};
    mutable std::condition_variable_any dequeue_condition;
    const bool use_async_buffer{}; // This is always disabled on HOS
    bool dequeue_buffer_cannot_block{};
    PixelFormat default_buffer_format{PixelFormat::Rgba8888};
    u32 default_width{1};
    u32 default_height{1};
    s32 default_max_buffer_count{2};
    const s32 max_acquired_buffer_count{}; // This is always zero on HOS
    bool buffer_has_been_queued{};
    u64 frame_counter{};
    u32 transform_hint{};
    bool is_allocating{};
    mutable std::condition_variable_any is_allocating_condition;
    bool allow_allocation{true};
    u64 buffer_age{};
    bool is_shutting_down{};
};

} // namespace Service::android

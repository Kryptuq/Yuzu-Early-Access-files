// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2010 The Android Open Source Project
// SPDX-License-Identifier: GPL-3.0-or-later
// Parts of this implementation were based on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/include/gui/ConsumerBase.h

#pragma once

#include <array>
#include <chrono>
#include <memory>
#include <mutex>

#include "common/common_types.h"
#include "core/hle/service/nvflinger/buffer_queue_defs.h"
#include "core/hle/service/nvflinger/consumer_listener.h"
#include "core/hle/service/nvflinger/status.h"

namespace Service::android {

class BufferItem;
class BufferQueueConsumer;

class ConsumerBase : public IConsumerListener, public std::enable_shared_from_this<ConsumerBase> {
public:
    void Connect(bool controlled_by_app);

protected:
    explicit ConsumerBase(std::unique_ptr<BufferQueueConsumer> consumer_);
    ~ConsumerBase() override;

    void OnFrameAvailable(const BufferItem& item) override;
    void OnFrameReplaced(const BufferItem& item) override;
    void OnBuffersReleased() override;
    void OnSidebandStreamChanged() override;

    void FreeBufferLocked(s32 slot_index);
    Status AcquireBufferLocked(BufferItem* item, std::chrono::nanoseconds present_when);
    Status ReleaseBufferLocked(s32 slot, const std::shared_ptr<GraphicBuffer>& graphic_buffer);
    bool StillTracking(s32 slot, const std::shared_ptr<GraphicBuffer>& graphic_buffer) const;
    Status AddReleaseFenceLocked(s32 slot, const std::shared_ptr<GraphicBuffer>& graphic_buffer,
                                 const Fence& fence);

    struct Slot final {
        std::shared_ptr<GraphicBuffer> graphic_buffer;
        Fence fence;
        u64 frame_number{};
    };

protected:
    std::array<Slot, BufferQueueDefs::NUM_BUFFER_SLOTS> slots;

    bool is_abandoned{};

    std::unique_ptr<BufferQueueConsumer> consumer;

    mutable std::mutex mutex;
};

} // namespace Service::android

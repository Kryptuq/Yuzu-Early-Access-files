// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021 yuzu Emulator Project
// Copyright 2010 The Android Open Source Project
// Parts of this implementation were base on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/libs/gui/ConsumerBase.cpp

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/service/nvflinger/buffer_item.h"
#include "core/hle/service/nvflinger/buffer_queue_consumer.h"
#include "core/hle/service/nvflinger/buffer_queue_core.h"
#include "core/hle/service/nvflinger/consumer_base.h"
#include "core/hle/service/nvflinger/ui/graphic_buffer.h"

namespace Service::android {

ConsumerBase::ConsumerBase(std::unique_ptr<BufferQueueConsumer> consumer_)
    : consumer{std::move(consumer_)} {}

ConsumerBase::~ConsumerBase() {
    std::scoped_lock lock(mutex);

    ASSERT_MSG(is_abandoned, "consumer is not abandoned!");
}

void ConsumerBase::Connect(bool controlled_by_app) {
    consumer->Connect(shared_from_this(), controlled_by_app);
}

void ConsumerBase::FreeBufferLocked(s32 slot_index) {
    LOG_DEBUG(Service_NVFlinger, "slot_index={}", slot_index);

    slots[slot_index].graphic_buffer = nullptr;
    slots[slot_index].fence = Fence::NoFence();
    slots[slot_index].frame_number = 0;
}

void ConsumerBase::OnFrameAvailable(const BufferItem& item) {
    std::scoped_lock lock(mutex);
    LOG_DEBUG(Service_NVFlinger, "called");
}

void ConsumerBase::OnFrameReplaced(const BufferItem& item) {
    std::scoped_lock lock(mutex);
    LOG_DEBUG(Service_NVFlinger, "called");
}

void ConsumerBase::OnBuffersReleased() {
    std::scoped_lock lock(mutex);
    LOG_DEBUG(Service_NVFlinger, "called");
}

void ConsumerBase::OnSidebandStreamChanged() {}

Status ConsumerBase::AcquireBufferLocked(BufferItem* item, std::chrono::nanoseconds present_when,
                                         u64 max_frame_number) {
    if (is_abandoned) {
        LOG_ERROR(Service_NVFlinger, "consumer is abandoned!");
        return Status::NoInit;
    }

    Status err = consumer->AcquireBuffer(item, present_when, max_frame_number);
    if (err != Status::NoError) {
        return err;
    }

    if (item->graphic_buffer != nullptr) {
        if (slots[item->slot].graphic_buffer != nullptr) {
            FreeBufferLocked(item->slot);
        }
        slots[item->slot].graphic_buffer = item->graphic_buffer;
    }

    slots[item->slot].frame_number = item->frame_number;
    slots[item->slot].fence = item->fence;

    LOG_DEBUG(Service_NVFlinger, "slot={}", item->slot);

    return Status::NoError;
}

Status ConsumerBase::AddReleaseFenceLocked(s32 slot,
                                           const std::shared_ptr<GraphicBuffer> graphic_buffer,
                                           const Fence& fence) {
    LOG_DEBUG(Service_NVFlinger, "slot={}", slot);

    // If consumer no longer tracks this graphic_buffer, we can safely
    // drop this fence, as it will never be received by the producer.

    if (!StillTracking(slot, graphic_buffer)) {
        return Status::NoError;
    }

    slots[slot].fence = fence;

    return Status::NoError;
}

Status ConsumerBase::ReleaseBufferLocked(s32 slot,
                                         const std::shared_ptr<GraphicBuffer> graphic_buffer) {
    // If consumer no longer tracks this graphic_buffer (we received a new
    // buffer on the same slot), the buffer producer is definitely no longer
    // tracking it.

    if (!StillTracking(slot, graphic_buffer)) {
        return Status::NoError;
    }

    LOG_DEBUG(Service_NVFlinger, "slot={}", slot);
    Status err = consumer->ReleaseBuffer(slot, slots[slot].frame_number, slots[slot].fence);
    if (err == Status::StaleBufferSlot) {
        FreeBufferLocked(slot);
    }

    slots[slot].fence = Fence::NoFence();

    return err;
}

bool ConsumerBase::StillTracking(s32 slot,
                                 const std::shared_ptr<GraphicBuffer> graphic_buffer) const {
    if (slot < 0 || slot >= BufferQueueDefs::NUM_BUFFER_SLOTS) {
        return false;
    }

    return (slots[slot].graphic_buffer != nullptr &&
            slots[slot].graphic_buffer->Handle() == graphic_buffer->Handle());
}

} // namespace Service::android

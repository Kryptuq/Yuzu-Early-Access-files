// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2012 The Android Open Source Project
// SPDX-License-Identifier: GPL-3.0-or-later
// Parts of this implementation were based on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/libs/gui/BufferItemConsumer.cpp

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/service/nvflinger/buffer_item.h"
#include "core/hle/service/nvflinger/buffer_item_consumer.h"
#include "core/hle/service/nvflinger/buffer_queue_consumer.h"

namespace Service::android {

BufferItemConsumer::BufferItemConsumer(std::unique_ptr<BufferQueueConsumer> consumer_)
    : ConsumerBase{std::move(consumer_)} {}

Status BufferItemConsumer::AcquireBuffer(BufferItem* item, std::chrono::nanoseconds present_when,
                                         bool wait_for_fence) {
    if (!item) {
        return Status::BadValue;
    }

    std::scoped_lock lock{mutex};

    if (const auto status = AcquireBufferLocked(item, present_when); status != Status::NoError) {
        if (status != Status::NoBufferAvailable) {
            LOG_ERROR(Service_NVFlinger, "Failed to acquire buffer: {}", status);
        }
        return status;
    }

    if (wait_for_fence) {
        UNIMPLEMENTED();
    }

    item->graphic_buffer = slots[item->slot].graphic_buffer;

    return Status::NoError;
}

Status BufferItemConsumer::ReleaseBuffer(const BufferItem& item, Fence& release_fence) {
    std::scoped_lock lock{mutex};

    if (const auto status = AddReleaseFenceLocked(item.buf, item.graphic_buffer, release_fence);
        status != Status::NoError) {
        LOG_ERROR(Service_NVFlinger, "Failed to add fence: {}", status);
    }

    if (const auto status = ReleaseBufferLocked(item.buf, item.graphic_buffer);
        status != Status::NoError) {
        LOG_WARNING(Service_NVFlinger, "Failed to release buffer: {}", status);
        return status;
    }

    return Status::NoError;
}

} // namespace Service::android

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021 yuzu Emulator Project
// Copyright 2014 The Android Open Source Project
// Parts of this implementation were base on:
// https://cs.android.com/android/platform/superproject/+/android-5.1.1_r38:frameworks/native/libs/gui/BufferQueueCore.cpp

#include "common/assert.h"

#include "core/hle/service/nvflinger/buffer_queue_core.h"

namespace Service::android {

BufferQueueCore::BufferQueueCore() : lock{mutex, std::defer_lock} {
    for (s32 slot = 0; slot < BufferQueueDefs::NUM_BUFFER_SLOTS; ++slot) {
        free_slots.insert(slot);
    }
}

BufferQueueCore::~BufferQueueCore() = default;

void BufferQueueCore::NotifyShutdown() {
    std::unique_lock lk(mutex);

    is_shutting_down = true;

    SignalDequeueCondition();
}

void BufferQueueCore::SignalDequeueCondition() {
    dequeue_condition.notify_all();
}

bool BufferQueueCore::WaitForDequeueCondition() {
    if (is_shutting_down) {
        return false;
    }

    dequeue_condition.wait(lock);

    return true;
}

s32 BufferQueueCore::GetMinUndequeuedBufferCountLocked(bool async) const {
    // If DequeueBuffer is allowed to error out, we don't have to add an extra buffer.
    if (!use_async_buffer) {
        return max_acquired_buffer_count;
    }

    if (dequeue_buffer_cannot_block || async) {
        return max_acquired_buffer_count + 1;
    }

    return max_acquired_buffer_count;
}

s32 BufferQueueCore::GetMinMaxBufferCountLocked(bool async) const {
    return GetMinUndequeuedBufferCountLocked(async) + 1;
}

s32 BufferQueueCore::GetMaxBufferCountLocked(bool async) const {
    const auto min_buffer_count = GetMinMaxBufferCountLocked(async);
    auto max_buffer_count = std::max(default_max_buffer_count, min_buffer_count);

    if (override_max_buffer_count != 0) {
        ASSERT(override_max_buffer_count >= min_buffer_count);
        max_buffer_count = override_max_buffer_count;
    }

    // Any buffers that are dequeued by the producer or sitting in the queue waiting to be consumed
    // need to have their slots preserved.
    for (s32 slot = max_buffer_count; slot < BufferQueueDefs::NUM_BUFFER_SLOTS; ++slot) {
        const auto state = slots[slot].buffer_state;
        if (state == BufferState::Queued || state == BufferState::Dequeued) {
            max_buffer_count = slot + 1;
        }
    }

    return max_buffer_count;
}

s32 BufferQueueCore::GetPreallocatedBufferCountLocked() const {
    return static_cast<s32>(std::count_if(slots.begin(), slots.end(),
                                          [](const auto& slot) { return slot.is_preallocated; }));
}

void BufferQueueCore::FreeBufferLocked(s32 slot) {
    LOG_DEBUG(Service_NVFlinger, "slot {}", slot);

    const auto had_buffer = slots[slot].graphic_buffer != nullptr;

    slots[slot].graphic_buffer.reset();

    if (slots[slot].buffer_state == BufferState::Acquired) {
        slots[slot].needs_cleanup_on_release = true;
    }

    if (slots[slot].buffer_state != BufferState::Free) {
        free_slots.insert(slot);
    } else if (had_buffer) {
        // If the slot was FREE, but we had a buffer, we need to move this slot from the free
        // buffers list to the the free slots list.
        free_buffers.remove(slot);
        free_slots.insert(slot);
    }

    slots[slot].buffer_state = BufferState::Free;
    slots[slot].acquire_called = false;
    slots[slot].frame_number = 0;
    slots[slot].fence = Fence::NoFence();
}

void BufferQueueCore::FreeAllBuffersLocked() {
    queue.clear();
    buffer_has_been_queued = false;

    for (s32 slot = 0; slot < BufferQueueDefs::NUM_BUFFER_SLOTS; ++slot) {
        FreeBufferLocked(slot);
    }
}

bool BufferQueueCore::StillTracking(const BufferItem& item) const {
    const BufferSlot& slot = slots[item.slot];

    return (slot.graphic_buffer != nullptr) && (item.graphic_buffer == slot.graphic_buffer);
}

void BufferQueueCore::WaitWhileAllocatingLocked() const {
    while (is_allocating) {
        std::unique_lock lk(mutex);
        is_allocating_condition.wait(lk);
    }
}

} // namespace Service::android

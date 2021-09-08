// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/kernel/k_writable_event.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/nvflinger/buffer_queue.h"

namespace Service::NVFlinger {

BufferQueue::BufferQueue(Kernel::KernelCore& kernel, u32 id_, u64 layer_id_,
                         KernelHelpers::ServiceContext& service_context_)
    : id(id_), layer_id(layer_id_), service_context{service_context_} {
    buffer_wait_event = service_context.CreateEvent("BufferQueue:WaitEvent");
}

BufferQueue::~BufferQueue() {
    service_context.CloseEvent(buffer_wait_event);
}

void BufferQueue::SetPreallocatedBuffer(u32 slot, const IGBPBuffer& igbp_buffer) {
    ASSERT(slot < buffer_slots);
    LOG_WARNING(Service, "Adding graphics buffer {}", slot);

    {
        std::unique_lock lock{free_buffers_mutex};
        free_buffers.push_back(slot);
    }
    free_buffers_condition.notify_one();

    buffers[slot] = {
        .slot = slot,
        .status = Buffer::Status::Free,
        .igbp_buffer = igbp_buffer,
        .transform = {},
        .crop_rect = {},
        .swap_interval = 0,
        .multi_fence = {},
    };

    buffer_wait_event->GetWritableEvent().Signal();
}

std::optional<std::pair<u32, Service::Nvidia::MultiFence*>> BufferQueue::DequeueBuffer(u32 width,
                                                                                       u32 height) {
    // Wait for first request before trying to dequeue
    {
        std::unique_lock lock{free_buffers_mutex};
        free_buffers_condition.wait(lock, [this] { return !free_buffers.empty() || !is_connect; });
    }

    if (!is_connect) {
        // Buffer was disconnected while the thread was blocked, this is most likely due to
        // emulation being stopped
        return std::nullopt;
    }

    std::unique_lock lock{free_buffers_mutex};

    auto f_itr = free_buffers.begin();
    auto slot = buffers.size();

    while (f_itr != free_buffers.end()) {
        const Buffer& buffer = buffers[*f_itr];
        if (buffer.status == Buffer::Status::Free && buffer.igbp_buffer.width == width &&
            buffer.igbp_buffer.height == height) {
            slot = *f_itr;
            free_buffers.erase(f_itr);
            break;
        }
        ++f_itr;
    }
    if (slot == buffers.size()) {
        return std::nullopt;
    }
    buffers[slot].status = Buffer::Status::Dequeued;
    return {{buffers[slot].slot, &buffers[slot].multi_fence}};
}

const IGBPBuffer& BufferQueue::RequestBuffer(u32 slot) const {
    ASSERT(slot < buffers.size());
    ASSERT(buffers[slot].status == Buffer::Status::Dequeued);
    ASSERT(buffers[slot].slot == slot);

    return buffers[slot].igbp_buffer;
}

void BufferQueue::QueueBuffer(u32 slot, BufferTransformFlags transform,
                              const Common::Rectangle<int>& crop_rect, u32 swap_interval,
                              Service::Nvidia::MultiFence& multi_fence) {
    ASSERT(slot < buffers.size());
    ASSERT(buffers[slot].status == Buffer::Status::Dequeued);
    ASSERT(buffers[slot].slot == slot);

    buffers[slot].status = Buffer::Status::Queued;
    buffers[slot].transform = transform;
    buffers[slot].crop_rect = crop_rect;
    buffers[slot].swap_interval = swap_interval;
    buffers[slot].multi_fence = multi_fence;
    std::unique_lock lock{queue_sequence_mutex};
    queue_sequence.push_back(slot);
}

void BufferQueue::CancelBuffer(u32 slot, const Service::Nvidia::MultiFence& multi_fence) {
    ASSERT(slot < buffers.size());
    ASSERT(buffers[slot].status != Buffer::Status::Free);
    ASSERT(buffers[slot].slot == slot);

    buffers[slot].status = Buffer::Status::Free;
    buffers[slot].multi_fence = multi_fence;
    buffers[slot].swap_interval = 0;

    {
        std::unique_lock lock{free_buffers_mutex};
        free_buffers.push_back(slot);
    }
    free_buffers_condition.notify_one();

    buffer_wait_event->GetWritableEvent().Signal();
}

std::optional<std::reference_wrapper<const BufferQueue::Buffer>> BufferQueue::AcquireBuffer() {
    std::unique_lock lock{queue_sequence_mutex};
    std::size_t buffer_slot = buffers.size();
    // Iterate to find a queued buffer matching the requested slot.
    while (buffer_slot == buffers.size() && !queue_sequence.empty()) {
        const auto slot = static_cast<std::size_t>(queue_sequence.front());
        ASSERT(slot < buffers.size());
        if (buffers[slot].status == Buffer::Status::Queued) {
            ASSERT(buffers[slot].slot == slot);
            buffer_slot = slot;
        }
        queue_sequence.pop_front();
    }
    if (buffer_slot == buffers.size()) {
        return std::nullopt;
    }
    buffers[buffer_slot].status = Buffer::Status::Acquired;
    return {{buffers[buffer_slot]}};
}

void BufferQueue::ReleaseBuffer(u32 slot) {
    ASSERT(slot < buffers.size());
    ASSERT(buffers[slot].status == Buffer::Status::Acquired);
    ASSERT(buffers[slot].slot == slot);

    buffers[slot].status = Buffer::Status::Free;
    {
        std::unique_lock lock{free_buffers_mutex};
        free_buffers.push_back(slot);
    }
    free_buffers_condition.notify_one();

    buffer_wait_event->GetWritableEvent().Signal();
}

void BufferQueue::Connect() {
    std::unique_lock lock{queue_sequence_mutex};
    queue_sequence.clear();
    is_connect = true;
}

void BufferQueue::Disconnect() {
    buffers.fill({});
    {
        std::unique_lock lock{queue_sequence_mutex};
        queue_sequence.clear();
    }
    buffer_wait_event->GetWritableEvent().Signal();
    is_connect = false;
    free_buffers_condition.notify_one();
}

u32 BufferQueue::Query(QueryType type) {
    LOG_WARNING(Service, "(STUBBED) called type={}", type);

    switch (type) {
    case QueryType::NativeWindowFormat:
        return static_cast<u32>(PixelFormat::RGBA8888);
    case QueryType::NativeWindowWidth:
    case QueryType::NativeWindowHeight:
        break;
    }
    UNIMPLEMENTED_MSG("Unimplemented query type={}", type);
    return 0;
}

Kernel::KWritableEvent& BufferQueue::GetWritableBufferWaitEvent() {
    return buffer_wait_event->GetWritableEvent();
}

Kernel::KReadableEvent& BufferQueue::GetBufferWaitEvent() {
    return buffer_wait_event->GetReadableEvent();
}

} // namespace Service::NVFlinger

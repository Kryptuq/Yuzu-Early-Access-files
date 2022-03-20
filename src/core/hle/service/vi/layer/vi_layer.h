// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>

#include "common/common_types.h"

namespace Service::android {
class BufferItemConsumer;
class BufferQueueCore;
class BufferQueueProducer;
} // namespace Service::android

namespace Service::VI {

/// Represents a single display layer.
class Layer {
public:
    /// Constructs a layer with a given ID and buffer queue.
    ///
    /// @param layer_id_ The ID to assign to this layer.
    /// @param binder_id_ The binder ID to assign to this layer.
    /// @param binder_ The buffer producer queue for this layer to use.
    ///
    Layer(u64 layer_id_, u32 binder_id_, android::BufferQueueCore& core_,
          android::BufferQueueProducer& binder_,
          std::shared_ptr<android::BufferItemConsumer>&& consumer_);
    ~Layer();

    Layer(const Layer&) = delete;
    Layer& operator=(const Layer&) = delete;

    Layer(Layer&&) = default;
    Layer& operator=(Layer&&) = delete;

    /// Gets the ID for this layer.
    u64 GetLayerId() const {
        return layer_id;
    }

    /// Gets the binder ID for this layer.
    u32 GetBinderId() const {
        return binder_id;
    }

    /// Gets a reference to the buffer queue this layer is using.
    android::BufferQueueProducer& GetBufferQueue() {
        return binder;
    }

    /// Gets a const reference to the buffer queue this layer is using.
    const android::BufferQueueProducer& GetBufferQueue() const {
        return binder;
    }

    android::BufferItemConsumer& GetConsumer() {
        return *consumer;
    }

    const android::BufferItemConsumer& GetConsumer() const {
        return *consumer;
    }

    android::BufferQueueCore& Core() {
        return core;
    }

    const android::BufferQueueCore& Core() const {
        return core;
    }

private:
    const u64 layer_id;
    const u32 binder_id;
    android::BufferQueueCore& core;
    android::BufferQueueProducer& binder;
    std::shared_ptr<android::BufferItemConsumer> consumer;
};

} // namespace Service::VI

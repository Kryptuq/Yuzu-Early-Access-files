// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <algorithm>
#include <queue>

#include "common/common_types.h"
#include "common/settings.h"
#include "core/core.h"
#include "video_core/delayed_destruction_ring.h"
#include "video_core/gpu.h"
#include "video_core/memory_manager.h"
#include "video_core/rasterizer_interface.h"

namespace VideoCommon {

class FenceBase {
public:
    explicit FenceBase(u32 payload_, bool is_stubbed_)
        : address{}, payload{payload_}, is_semaphore{false}, is_stubbed{is_stubbed_} {}

    explicit FenceBase(GPUVAddr address_, u32 payload_, bool is_stubbed_)
        : address{address_}, payload{payload_}, is_semaphore{true}, is_stubbed{is_stubbed_} {}

    GPUVAddr GetAddress() const {
        return address;
    }

    u32 GetPayload() const {
        return payload;
    }

    bool IsSemaphore() const {
        return is_semaphore;
    }

private:
    GPUVAddr address;
    u32 payload;
    bool is_semaphore;

protected:
    bool is_stubbed;
};

template <typename TFence, typename TTextureCache, typename TTBufferCache, typename TQueryCache>
class FenceManager {
public:
    /// Notify the fence manager about a new frame
    void TickFrame() {
        delayed_destruction_ring.Tick();
    }

    // Unlike other fences, this one doesn't
    void SignalOrdering() {
        std::scoped_lock lock{buffer_cache.mutex};
        buffer_cache.AccumulateFlushes();
    }

    void SignalSemaphore(GPUVAddr addr, u32 value) {
        TryReleasePendingFences();
        const bool should_flush = ShouldFlush();
        CommitAsyncFlushes();
        TFence new_fence = CreateFence(addr, value, !should_flush);
        fences.push(new_fence);
        QueueFence(new_fence);
        if (should_flush) {
            rasterizer.FlushCommands();
        }
        rasterizer.SyncGuestHost();
    }

    void SignalSyncPoint(u32 value) {
        TryReleasePendingFences();
        const bool should_flush = ShouldFlush();
        CommitAsyncFlushes();
        TFence new_fence = CreateFence(value, !should_flush);
        fences.push(new_fence);
        QueueFence(new_fence);
        if (should_flush) {
            rasterizer.FlushCommands();
        }
        rasterizer.SyncGuestHost();
    }

    void WaitPendingFences() {
        while (!fences.empty()) {
            TFence& current_fence = fences.front();
            if (ShouldWait()) {
                WaitFence(current_fence);
            }
            PopAsyncFlushes();
            if (current_fence->IsSemaphore()) {
                gpu_memory.template Write<u32>(current_fence->GetAddress(),
                                               current_fence->GetPayload());
            } else {
                gpu.IncrementSyncPoint(current_fence->GetPayload());
            }
            PopFence();
        }
    }

protected:
    explicit FenceManager(VideoCore::RasterizerInterface& rasterizer_, Tegra::GPU& gpu_,
                          TTextureCache& texture_cache_, TTBufferCache& buffer_cache_,
                          TQueryCache& query_cache_)
        : rasterizer{rasterizer_}, gpu{gpu_}, gpu_memory{gpu.MemoryManager()},
          texture_cache{texture_cache_}, buffer_cache{buffer_cache_}, query_cache{query_cache_} {}

    virtual ~FenceManager() = default;

    /// Creates a Sync Point Fence Interface, does not create a backend fence if 'is_stubbed' is
    /// true
    virtual TFence CreateFence(u32 value, bool is_stubbed) = 0;
    /// Creates a Semaphore Fence Interface, does not create a backend fence if 'is_stubbed' is true
    virtual TFence CreateFence(GPUVAddr addr, u32 value, bool is_stubbed) = 0;
    /// Queues a fence into the backend if the fence isn't stubbed.
    virtual void QueueFence(TFence& fence) = 0;
    /// Notifies that the backend fence has been signaled/reached in host GPU.
    virtual bool IsFenceSignaled(TFence& fence) const = 0;
    /// Waits until a fence has been signalled by the host GPU.
    virtual void WaitFence(TFence& fence) = 0;

    VideoCore::RasterizerInterface& rasterizer;
    Tegra::GPU& gpu;
    Tegra::MemoryManager& gpu_memory;
    TTextureCache& texture_cache;
    TTBufferCache& buffer_cache;
    TQueryCache& query_cache;

private:
    void TryReleasePendingFences() {
        while (!fences.empty()) {
            TFence& current_fence = fences.front();
            if (ShouldWait() && !IsFenceSignaled(current_fence)) {
                return;
            }
            PopAsyncFlushes();
            if (current_fence->IsSemaphore()) {
                gpu_memory.template Write<u32>(current_fence->GetAddress(),
                                               current_fence->GetPayload());
            } else {
                gpu.IncrementSyncPoint(current_fence->GetPayload());
            }
            PopFence();
        }
    }

    bool ShouldWait() const {
        std::scoped_lock lock{buffer_cache.mutex, texture_cache.mutex};
        return texture_cache.ShouldWaitAsyncFlushes() || buffer_cache.ShouldWaitAsyncFlushes() ||
               query_cache.ShouldWaitAsyncFlushes();
    }

    bool ShouldFlush() const {
        std::scoped_lock lock{buffer_cache.mutex, texture_cache.mutex};
        return texture_cache.HasUncommittedFlushes() || buffer_cache.HasUncommittedFlushes() ||
               query_cache.HasUncommittedFlushes();
    }

    void PopAsyncFlushes() {
        std::scoped_lock lock{buffer_cache.mutex, texture_cache.mutex};
        texture_cache.PopAsyncFlushes();
        buffer_cache.PopAsyncFlushes();
        query_cache.PopAsyncFlushes();
    }

    void CommitAsyncFlushes() {
        std::scoped_lock lock{buffer_cache.mutex, texture_cache.mutex};
        texture_cache.CommitAsyncFlushes();
        buffer_cache.CommitAsyncFlushes();
        query_cache.CommitAsyncFlushes();
    }

    void PopFence() {
        delayed_destruction_ring.Push(std::move(fences.front()));
        fences.pop();
    }

    std::queue<TFence> fences;

    DelayedDestructionRing<TFence, 6> delayed_destruction_ring;
};

} // namespace VideoCommon

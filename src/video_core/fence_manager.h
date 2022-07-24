// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <cstring>
#include <deque>
#include <functional>
#include <memory>
#include <queue>

#include "common/common_types.h"
#include "video_core/delayed_destruction_ring.h"
#include "video_core/gpu.h"
#include "video_core/host1x/host1x.h"
#include "video_core/host1x/syncpoint_manager.h"
#include "video_core/rasterizer_interface.h"

namespace VideoCommon {

class FenceBase {
public:
    explicit FenceBase(bool is_stubbed_) : is_stubbed{is_stubbed_} {}

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

    void SyncOperation(std::function<void()>&& func) {
        uncommitted_operations.emplace_back(std::move(func));
    }

    void SignalFence(std::function<void()>&& func) {
        TryReleasePendingFences();
        const bool should_flush = ShouldFlush();
        CommitAsyncFlushes();
        uncommitted_operations.emplace_back(std::move(func));
        CommitOperations();
        TFence new_fence = CreateFence(!should_flush);
        fences.push(new_fence);
        QueueFence(new_fence);
        if (should_flush) {
            rasterizer.FlushCommands();
        }
    }

    void SignalSyncPoint(u32 value) {
        syncpoint_manager.IncrementGuest(value);
        std::function<void()> func([this, value] { syncpoint_manager.IncrementHost(value); });
        SignalFence(std::move(func));
    }

    void WaitPendingFences() {
        while (!fences.empty()) {
            TFence& current_fence = fences.front();
            if (ShouldWait()) {
                WaitFence(current_fence);
            }
            PopAsyncFlushes();
            auto operations = std::move(pending_operations.front());
            pending_operations.pop_front();
            for (auto& operation : operations) {
                operation();
            }
            PopFence();
        }
    }

protected:
    explicit FenceManager(VideoCore::RasterizerInterface& rasterizer_, Tegra::GPU& gpu_,
                          TTextureCache& texture_cache_, TTBufferCache& buffer_cache_,
                          TQueryCache& query_cache_)
        : rasterizer{rasterizer_}, gpu{gpu_}, syncpoint_manager{gpu.Host1x().GetSyncpointManager()},
          texture_cache{texture_cache_}, buffer_cache{buffer_cache_}, query_cache{query_cache_} {}

    virtual ~FenceManager() = default;

    /// Creates a Fence Interface, does not create a backend fence if 'is_stubbed' is
    /// true
    virtual TFence CreateFence(bool is_stubbed) = 0;
    /// Queues a fence into the backend if the fence isn't stubbed.
    virtual void QueueFence(TFence& fence) = 0;
    /// Notifies that the backend fence has been signaled/reached in host GPU.
    virtual bool IsFenceSignaled(TFence& fence) const = 0;
    /// Waits until a fence has been signalled by the host GPU.
    virtual void WaitFence(TFence& fence) = 0;

    VideoCore::RasterizerInterface& rasterizer;
    Tegra::GPU& gpu;
    Tegra::Host1x::SyncpointManager& syncpoint_manager;
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
            auto operations = std::move(pending_operations.front());
            pending_operations.pop_front();
            for (auto& operation : operations) {
                operation();
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
        {
            std::scoped_lock lock{buffer_cache.mutex, texture_cache.mutex};
            texture_cache.PopAsyncFlushes();
            buffer_cache.PopAsyncFlushes();
        }
        query_cache.PopAsyncFlushes();
    }

    void CommitAsyncFlushes() {
        {
            std::scoped_lock lock{buffer_cache.mutex, texture_cache.mutex};
            texture_cache.CommitAsyncFlushes();
            buffer_cache.CommitAsyncFlushes();
        }
        query_cache.CommitAsyncFlushes();
    }

    void PopFence() {
        delayed_destruction_ring.Push(std::move(fences.front()));
        fences.pop();
    }

    void CommitOperations() {
        pending_operations.emplace_back(std::move(uncommitted_operations));
    }

    std::queue<TFence> fences;
    std::deque<std::function<void()>> uncommitted_operations;
    std::deque<std::deque<std::function<void()>>> pending_operations;

    DelayedDestructionRing<TFence, 6> delayed_destruction_ring;
};

} // namespace VideoCommon

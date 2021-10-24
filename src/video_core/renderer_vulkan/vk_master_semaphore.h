// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <thread>

#include "common/common_types.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

class Device;

class MasterSemaphore {
public:
    explicit MasterSemaphore(const Device& device);
    ~MasterSemaphore();

    /// Returns the current logical tick.
    [[nodiscard]] u64 CurrentTick() const noexcept {
        return current_tick.load(std::memory_order_acquire);
    }

    /// Returns the last known GPU tick.
    [[nodiscard]] u64 KnownGpuTick() const noexcept {
        return gpu_tick.load(std::memory_order_acquire);
    }

    /// Returns the timeline semaphore handle.
    [[nodiscard]] VkSemaphore Handle() const noexcept {
        return *semaphore;
    }

    /// Returns true when a tick has been hit by the GPU.
    [[nodiscard]] bool IsFree(u64 tick) const noexcept {
        return KnownGpuTick() >= tick;
    }

    /// Advance to the logical tick and return the old one
    [[nodiscard]] u64 NextTick() noexcept {
        return current_tick.fetch_add(1, std::memory_order_release);
    }

    /// Refresh the known GPU tick
    void Refresh() {
        u64 this_tick{};
        u64 counter{};
        do {
            this_tick = gpu_tick.load(std::memory_order_acquire);
            counter = semaphore.GetCounter();
            if (counter < this_tick) {
                return;
            }
        } while (!gpu_tick.compare_exchange_weak(this_tick, counter, std::memory_order_release,
                                                 std::memory_order_relaxed));
    }

    /// Waits for a tick to be hit on the GPU
    void Wait(u64 tick) {
        // No need to wait if the GPU is ahead of the tick
        if (IsFree(tick)) {
            return;
        }
        // Update the GPU tick and try again
        Refresh();
        if (IsFree(tick)) {
            return;
        }
        // If none of the above is hit, fallback to a regular wait
        while (!semaphore.Wait(tick)) {
        }
        Refresh();
    }

private:
    vk::Semaphore semaphore;          ///< Timeline semaphore.
    std::atomic<u64> gpu_tick{0};     ///< Current known GPU tick.
    std::atomic<u64> current_tick{1}; ///< Current logical tick.
    std::jthread debug_thread;        ///< Debug thread to workaround validation layer bugs.
};

} // namespace Vulkan

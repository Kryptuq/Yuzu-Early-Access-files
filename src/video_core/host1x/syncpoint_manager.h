// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>

#include "common/common_types.h"

namespace Tegra {

namespace Host1x {

class SyncpointManager {
public:
    u32 GetGuestSyncpointValue(u32 id) const {
        return syncpoints_guest[id].load(std::memory_order_acquire);
    }

    u32 GetHostSyncpointValue(u32 id) const {
        return syncpoints_host[id].load(std::memory_order_acquire);
    }

    struct RegisteredAction {
        explicit RegisteredAction(u32 expected_value_, std::function<void()>&& action_)
            : expected_value{expected_value_}, action{std::move(action_)} {}
        u32 expected_value;
        std::function<void()> action;
    };
    using ActionHandle = std::list<RegisteredAction>::iterator;

    template <typename Func>
    ActionHandle RegisterGuestAction(u32 syncpoint_id, u32 expected_value, Func&& action) {
        std::function<void()> func(action);
        return RegisterAction(syncpoints_guest[syncpoint_id], guest_action_storage[syncpoint_id],
                              expected_value, std::move(func));
    }

    template <typename Func>
    ActionHandle RegisterHostAction(u32 syncpoint_id, u32 expected_value, Func&& action) {
        std::function<void()> func(action);
        return RegisterAction(syncpoints_host[syncpoint_id], host_action_storage[syncpoint_id],
                              expected_value, std::move(func));
    }

    void DeregisterGuestAction(u32 syncpoint_id, ActionHandle& handle);

    void DeregisterHostAction(u32 syncpoint_id, ActionHandle& handle);

    void IncrementGuest(u32 syncpoint_id);

    void IncrementHost(u32 syncpoint_id);

    void WaitGuest(u32 syncpoint_id, u32 expected_value);

    void WaitHost(u32 syncpoint_id, u32 expected_value);

    bool IsReadyGuest(u32 syncpoint_id, u32 expected_value) const {
        return syncpoints_guest[syncpoint_id].load(std::memory_order_acquire) >= expected_value;
    }

    bool IsReadyHost(u32 syncpoint_id, u32 expected_value) const {
        return syncpoints_host[syncpoint_id].load(std::memory_order_acquire) >= expected_value;
    }

private:
    void Increment(std::atomic<u32>& syncpoint, std::condition_variable& wait_cv,
                   std::list<RegisteredAction>& action_storage);

    ActionHandle RegisterAction(std::atomic<u32>& syncpoint,
                                std::list<RegisteredAction>& action_storage, u32 expected_value,
                                std::function<void()>&& action);

    void DeregisterAction(std::list<RegisteredAction>& action_storage, ActionHandle& handle);

    void Wait(std::atomic<u32>& syncpoint, std::condition_variable& wait_cv, u32 expected_value);

    static constexpr size_t NUM_MAX_SYNCPOINTS = 192;

    std::array<std::atomic<u32>, NUM_MAX_SYNCPOINTS> syncpoints_guest{};
    std::array<std::atomic<u32>, NUM_MAX_SYNCPOINTS> syncpoints_host{};

    std::array<std::list<RegisteredAction>, NUM_MAX_SYNCPOINTS> guest_action_storage;
    std::array<std::list<RegisteredAction>, NUM_MAX_SYNCPOINTS> host_action_storage;

    std::mutex guard;
    std::condition_variable wait_guest_cv;
    std::condition_variable wait_host_cv;
};

} // namespace Host1x

} // namespace Tegra

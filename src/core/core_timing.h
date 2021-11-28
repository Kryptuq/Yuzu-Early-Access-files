// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "common/common_types.h"
#include "common/wall_clock.h"

namespace Core::Timing {

/// A callback that may be scheduled for a particular core timing event.
using TimedCallback =
    std::function<void(std::uintptr_t user_data, std::chrono::nanoseconds ns_late)>;

/// Contains the characteristics of a particular event.
struct EventType {
    explicit EventType(TimedCallback&& callback_, std::string&& name_)
        : callback{std::move(callback_)}, name{std::move(name_)} {}

    /// The event's callback function.
    TimedCallback callback;
    /// A pointer to the name of the event.
    const std::string name;
};

/**
 * This is a system to schedule events into the emulated machine's future. Time is measured
 * in main CPU clock cycles.
 *
 * To schedule an event, you first have to register its type. This is where you pass in the
 * callback. You then schedule events using the type ID you get back.
 *
 * The s64 ns_late that the callbacks get is how many ns late it was.
 * So to schedule a new event on a regular basis:
 * inside callback:
 *   ScheduleEvent(period_in_ns - ns_late, callback, "whatever")
 */
class CoreTiming {
public:
    CoreTiming();
    ~CoreTiming();

    CoreTiming(const CoreTiming&) = delete;
    CoreTiming(CoreTiming&&) = delete;

    CoreTiming& operator=(const CoreTiming&) = delete;
    CoreTiming& operator=(CoreTiming&&) = delete;

    /// CoreTiming begins at the boundary of timing slice -1. An initial call to Advance() is
    /// required to end slice - 1 and start slice 0 before the first cycle of code is executed.
    void Initialize(std::function<void()>&& on_thread_init_);

    /// Tears down all timing related functionality.
    void Shutdown();

    /// Sets if emulation is multicore or single core, must be set before Initialize
    void SetMulticore(bool is_multicore_) {
        is_multicore = is_multicore_;
    }

    /// Check if it's using host timing.
    bool IsHostTiming() const {
        return is_multicore;
    }

    /// Pauses/Unpauses the execution of the timer thread.
    void Pause(bool is_paused);

    /// Pauses/Unpauses the execution of the timer thread and waits until paused.
    void SyncPause(bool is_paused);

    /// Checks if core timing is running.
    bool IsRunning() const;

    /// Checks if the timer thread has started.
    bool HasStarted() const {
        return has_started;
    }

    /// Checks if there are any pending time events.
    bool HasPendingEvents() const;

    /// Schedules an event in core timing
    void ScheduleEvent(std::chrono::nanoseconds ns_into_future,
                       const std::shared_ptr<EventType>& event_type, std::uintptr_t user_data = 0);

    void UnscheduleEvent(const std::shared_ptr<EventType>& event_type, std::uintptr_t user_data);

    /// We only permit one event of each type in the queue at a time.
    void RemoveEvent(const std::shared_ptr<EventType>& event_type);

    void AddTicks(u64 ticks_to_add);

    void ResetTicks();

    void Idle();

    s64 GetDowncount() const {
        return downcount;
    }

    /// Returns current time in emulated CPU cycles
    u64 GetCPUTicks() const;

    /// Returns current time in emulated in Clock cycles
    u64 GetClockTicks() const;

    /// Returns current time in microseconds.
    std::chrono::microseconds GetGlobalTimeUs() const;

    /// Returns current time in nanoseconds.
    std::chrono::nanoseconds GetGlobalTimeNs() const;

    /// Checks for events manually and returns time in nanoseconds for next event, threadsafe.
    std::optional<s64> Advance();

private:
    struct Event;

    /// Clear all pending events. This should ONLY be done on exit.
    void ClearPendingEvents();

    static void ThreadEntry(CoreTiming& instance);
    void ThreadLoop();

    std::unique_ptr<Common::WallClock> clock;

    u64 global_timer = 0;

    // The queue is a min-heap using std::make_heap/push_heap/pop_heap.
    // We don't use std::priority_queue because we need to be able to serialize, unserialize and
    // erase arbitrary events (RemoveEvent()) regardless of the queue order. These aren't
    // accomodated by the standard adaptor class.
    std::vector<Event> event_queue;
    u64 event_fifo_id = 0;

    std::shared_ptr<EventType> ev_lost;
    std::atomic<bool> has_started{};
    std::function<void()> on_thread_init{};

    std::vector<std::thread> worker_threads;

    std::condition_variable event_cv;
    std::condition_variable wait_pause_cv;
    std::condition_variable wait_signal_cv;
    mutable std::mutex event_mutex;

    std::atomic<bool> paused_state{};
    bool is_paused{};
    bool shutting_down{};
    bool is_multicore{};
    size_t pause_count{};

    /// Cycle timing
    u64 ticks{};
    s64 downcount{};
};

/// Creates a core timing event with the given name and callback.
///
/// @param name     The name of the core timing event to create.
/// @param callback The callback to execute for the event.
///
/// @returns An EventType instance representing the created event.
///
std::shared_ptr<EventType> CreateEvent(std::string name, TimedCallback&& callback);

} // namespace Core::Timing

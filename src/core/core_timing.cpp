// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <mutex>
#include <string>
#include <tuple>

#include "common/logging/log.h"
#include "common/microprofile.h"
#include "common/thread.h"
#include "core/core_timing.h"
#include "core/core_timing_util.h"
#include "core/hardware_properties.h"

namespace Core::Timing {

constexpr s64 MAX_SLICE_LENGTH = 4000;

std::shared_ptr<EventType> CreateEvent(std::string name, TimedCallback&& callback) {
    return std::make_shared<EventType>(std::move(callback), std::move(name));
}

struct CoreTiming::Event {
    u64 time;
    u64 fifo_order;
    std::uintptr_t user_data;
    std::weak_ptr<EventType> type;

    // Sort by time, unless the times are the same, in which case sort by
    // the order added to the queue
    friend bool operator>(const Event& left, const Event& right) {
        return std::tie(left.time, left.fifo_order) > std::tie(right.time, right.fifo_order);
    }

    friend bool operator<(const Event& left, const Event& right) {
        return std::tie(left.time, left.fifo_order) < std::tie(right.time, right.fifo_order);
    }
};

CoreTiming::CoreTiming()
    : clock{Common::CreateBestMatchingClock(Hardware::BASE_CLOCK_RATE, Hardware::CNTFREQ)} {}

CoreTiming::~CoreTiming() = default;

void CoreTiming::ThreadEntry(CoreTiming& instance, size_t id) {
    const std::string name = "yuzu:HostTiming_" + std::to_string(id);
    MicroProfileOnThreadCreate(name.c_str());
    Common::SetCurrentThreadName(name.c_str());
    Common::SetCurrentThreadPriority(Common::ThreadPriority::Critical);
    instance.on_thread_init();
    instance.ThreadLoop();
    MicroProfileOnThreadExit();
}

void CoreTiming::Initialize(std::function<void()>&& on_thread_init_) {
    on_thread_init = std::move(on_thread_init_);
    event_fifo_id = 0;
    shutting_down = false;
    ticks = 0;
    const auto empty_timed_callback = [](std::uintptr_t, std::chrono::nanoseconds) {};
    ev_lost = CreateEvent("_lost_event", empty_timed_callback);
    if (is_multicore) {
        worker_threads.emplace_back(ThreadEntry, std::ref(*this), 0);
    }
}

void CoreTiming::Shutdown() {
    is_paused = true;
    shutting_down = true;
    std::atomic_thread_fence(std::memory_order_release);

    event_cv.notify_all();
    wait_pause_cv.notify_all();
    for (auto& thread : worker_threads) {
        thread.join();
    }
    worker_threads.clear();
    ClearPendingEvents();
    has_started = false;
}

void CoreTiming::Pause(bool is_paused_) {
    std::unique_lock main_lock(event_mutex);
    if (is_paused_ == paused_state.load(std::memory_order_relaxed)) {
        return;
    }
    if (is_multicore) {
        is_paused = is_paused_;
        event_cv.notify_all();
        if (!is_paused_) {
            wait_pause_cv.notify_all();
        }
    }
    paused_state.store(is_paused_, std::memory_order_relaxed);
}

void CoreTiming::SyncPause(bool is_paused_) {
    std::unique_lock main_lock(event_mutex);
    if (is_paused_ == paused_state.load(std::memory_order_relaxed)) {
        return;
    }

    if (is_multicore) {
        is_paused = is_paused_;
        event_cv.notify_all();
        if (!is_paused_) {
            wait_pause_cv.notify_all();
        }
    }
    paused_state.store(is_paused_, std::memory_order_relaxed);
    if (is_multicore) {
        if (is_paused_) {
            wait_signal_cv.wait(main_lock, [this] { return pause_count == worker_threads.size(); });
        } else {
            wait_signal_cv.wait(main_lock, [this] { return pause_count == 0; });
        }
    }
}

bool CoreTiming::IsRunning() const {
    return !paused_state.load(std::memory_order_acquire);
}

bool CoreTiming::HasPendingEvents() const {
    std::unique_lock main_lock(event_mutex);
    return !event_queue.empty() || pending_events.load(std::memory_order_relaxed) != 0;
}

void CoreTiming::ScheduleEvent(std::chrono::nanoseconds ns_into_future,
                               const std::shared_ptr<EventType>& event_type,
                               std::uintptr_t user_data) {

    std::unique_lock main_lock(event_mutex);
    const u64 timeout = static_cast<u64>((GetGlobalTimeNs() + ns_into_future).count());

    event_queue.emplace_back(Event{timeout, event_fifo_id++, user_data, event_type});
    pending_events.fetch_add(1, std::memory_order_relaxed);

    std::push_heap(event_queue.begin(), event_queue.end(), std::greater<>());

    if (is_multicore) {
        event_cv.notify_one();
    }
}

void CoreTiming::UnscheduleEvent(const std::shared_ptr<EventType>& event_type,
                                 std::uintptr_t user_data) {
    std::unique_lock main_lock(event_mutex);
    const auto itr = std::remove_if(event_queue.begin(), event_queue.end(), [&](const Event& e) {
        return e.type.lock().get() == event_type.get() && e.user_data == user_data;
    });

    // Removing random items breaks the invariant so we have to re-establish it.
    if (itr != event_queue.end()) {
        event_queue.erase(itr, event_queue.end());
        std::make_heap(event_queue.begin(), event_queue.end(), std::greater<>());
        pending_events.fetch_sub(1, std::memory_order_relaxed);
    }
}

void CoreTiming::AddTicks(u64 ticks_to_add) {
    ticks += ticks_to_add;
    downcount -= static_cast<s64>(ticks);
}

void CoreTiming::Idle() {
    if (!event_queue.empty()) {
        const u64 next_event_time = event_queue.front().time;
        const u64 next_ticks = nsToCycles(std::chrono::nanoseconds(next_event_time)) + 10U;
        if (next_ticks > ticks) {
            ticks = next_ticks;
        }
        return;
    }
    ticks += 1000U;
}

void CoreTiming::ResetTicks() {
    downcount = MAX_SLICE_LENGTH;
}

u64 CoreTiming::GetCPUTicks() const {
    if (is_multicore) {
        return clock->GetCPUCycles();
    }
    return ticks;
}

u64 CoreTiming::GetClockTicks() const {
    if (is_multicore) {
        return clock->GetClockCycles();
    }
    return CpuCyclesToClockCycles(ticks);
}

void CoreTiming::ClearPendingEvents() {
    std::unique_lock main_lock(event_mutex);
    event_queue.clear();
}

void CoreTiming::RemoveEvent(const std::shared_ptr<EventType>& event_type) {
    std::unique_lock main_lock(event_mutex);

    const auto itr = std::remove_if(event_queue.begin(), event_queue.end(), [&](const Event& e) {
        return e.type.lock().get() == event_type.get();
    });

    // Removing random items breaks the invariant so we have to re-establish it.
    if (itr != event_queue.end()) {
        event_queue.erase(itr, event_queue.end());
        std::make_heap(event_queue.begin(), event_queue.end(), std::greater<>());
    }
}

std::optional<s64> CoreTiming::Advance() {
    global_timer = GetGlobalTimeNs().count();

    std::unique_lock main_lock(event_mutex);
    while (!event_queue.empty() && event_queue.front().time <= global_timer) {
        Event evt = std::move(event_queue.front());
        std::pop_heap(event_queue.begin(), event_queue.end(), std::greater<>());
        event_queue.pop_back();

        if (const auto event_type{evt.type.lock()}) {

            event_mutex.unlock();

            const s64 delay = static_cast<s64>(GetGlobalTimeNs().count() - evt.time);
            event_type->callback(evt.user_data, std::chrono::nanoseconds{delay});

            event_mutex.lock();
            pending_events.fetch_sub(1, std::memory_order_relaxed);
        }

        global_timer = GetGlobalTimeNs().count();
    }

    if (!event_queue.empty()) {
        const s64 next_time = event_queue.front().time - global_timer;
        return next_time;
    } else {
        return std::nullopt;
    }
}

void CoreTiming::ThreadLoop() {
    const auto predicate = [this] { return !event_queue.empty() || is_paused; };
    has_started = true;
    while (!shutting_down) {
        while (!is_paused && !shutting_down) {
            const auto next_time = Advance();
            if (next_time) {
                if (*next_time > 0) {
                    std::chrono::nanoseconds next_time_ns = std::chrono::nanoseconds(*next_time);
                    std::unique_lock main_lock(event_mutex);
                    event_cv.wait_for(main_lock, next_time_ns, predicate);
                }
            } else {
                std::unique_lock main_lock(event_mutex);
                event_cv.wait(main_lock, predicate);
            }
        }
        std::unique_lock main_lock(event_mutex);
        pause_count++;
        if (pause_count == worker_threads.size()) {
            clock->Pause(true);
            wait_signal_cv.notify_all();
        }
        wait_pause_cv.wait(main_lock, [this] { return !is_paused || shutting_down; });
        pause_count--;
        if (pause_count == 0) {
            clock->Pause(false);
            wait_signal_cv.notify_all();
        }
    }
}

std::chrono::nanoseconds CoreTiming::GetGlobalTimeNs() const {
    if (is_multicore) {
        return clock->GetTimeNS();
    }
    return CyclesToNs(ticks);
}

std::chrono::microseconds CoreTiming::GetGlobalTimeUs() const {
    if (is_multicore) {
        return clock->GetTimeUS();
    }
    return CyclesToUs(ticks);
}

} // namespace Core::Timing

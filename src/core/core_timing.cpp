// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <mutex>
#include <string>
#include <tuple>

#include "common/microprofile.h"
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

void CoreTiming::ThreadEntry(CoreTiming& instance) {
    constexpr char name[] = "yuzu:HostTiming";
    MicroProfileOnThreadCreate(name);
    Common::SetCurrentThreadName(name);
    Common::SetCurrentThreadPriority(Common::ThreadPriority::VeryHigh);
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
        timer_thread = std::make_unique<std::thread>(ThreadEntry, std::ref(*this));
    }
}

void CoreTiming::Shutdown() {
    paused = true;
    shutting_down = true;
    pause_event.Set();
    event.Set();
    if (timer_thread) {
        timer_thread->join();
    }
    ClearPendingEvents();
    timer_thread.reset();
    has_started = false;
}

void CoreTiming::Pause(bool is_paused) {
    paused = is_paused;
    pause_event.Set();
}

void CoreTiming::SyncPause(bool is_paused) {
    if (is_paused == paused && paused_set == paused) {
        return;
    }
    Pause(is_paused);
    if (timer_thread) {
        if (!is_paused) {
            pause_event.Set();
        }
        event.Set();
        while (paused_set != is_paused)
            ;
    }
}

bool CoreTiming::IsRunning() const {
    return !paused_set;
}

bool CoreTiming::HasPendingEvents() const {
    return !(wait_set && event_queue.empty());
}

void CoreTiming::ScheduleEvent(std::chrono::nanoseconds ns_into_future,
                               const std::shared_ptr<EventType>& event_type,
                               std::uintptr_t user_data) {
    {
        std::scoped_lock scope{basic_lock};
        const u64 timeout = static_cast<u64>((GetGlobalTimeNs() + ns_into_future).count());

        event_queue.emplace_back(Event{timeout, event_fifo_id++, user_data, event_type});

        std::push_heap(event_queue.begin(), event_queue.end(), std::greater<>());
    }
    event.Set();
}

void CoreTiming::UnscheduleEvent(const std::shared_ptr<EventType>& event_type,
                                 std::uintptr_t user_data) {
    std::scoped_lock scope{basic_lock};
    const auto itr = std::remove_if(event_queue.begin(), event_queue.end(), [&](const Event& e) {
        return e.type.lock().get() == event_type.get() && e.user_data == user_data;
    });

    // Removing random items breaks the invariant so we have to re-establish it.
    if (itr != event_queue.end()) {
        event_queue.erase(itr, event_queue.end());
        std::make_heap(event_queue.begin(), event_queue.end(), std::greater<>());
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
    event_queue.clear();
}

void CoreTiming::RemoveEvent(const std::shared_ptr<EventType>& event_type) {
    std::scoped_lock lock{basic_lock};

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
    std::scoped_lock lock{advance_lock, basic_lock};
    global_timer = GetGlobalTimeNs().count();

    while (!event_queue.empty() && event_queue.front().time <= global_timer) {
        Event evt = std::move(event_queue.front());
        std::pop_heap(event_queue.begin(), event_queue.end(), std::greater<>());
        event_queue.pop_back();
        basic_lock.unlock();

        if (const auto event_type{evt.type.lock()}) {
            event_type->callback(
                evt.user_data, std::chrono::nanoseconds{static_cast<s64>(global_timer - evt.time)});
        }

        basic_lock.lock();
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
    has_started = true;
    while (!shutting_down) {
        while (!paused) {
            paused_set = false;
            const auto next_time = Advance();
            if (next_time) {
                if (*next_time > 0) {
                    std::chrono::nanoseconds next_time_ns = std::chrono::nanoseconds(*next_time);
                    event.WaitFor(next_time_ns);
                }
            } else {
                wait_set = true;
                event.Wait();
            }
            wait_set = false;
        }
        paused_set = true;
        clock->Pause(true);
        pause_event.Wait();
        clock->Pause(false);
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

// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <mutex>
#include <string>
#include <tuple>

#ifdef _WIN32
#include "common/windows/timer_resolution.h"
#endif

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
    s64 time;
    u64 fifo_order;
    std::uintptr_t user_data;
    std::weak_ptr<EventType> type;
    s64 reschedule_time;

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
    : cpu_clock{Common::CreateBestMatchingClock(Hardware::BASE_CLOCK_RATE, Hardware::CNTFREQ)},
      event_clock{Common::CreateStandardWallClock(Hardware::BASE_CLOCK_RATE, Hardware::CNTFREQ)} {}

CoreTiming::~CoreTiming() {
    Reset();
}

void CoreTiming::ThreadEntry(CoreTiming& instance) {
    static constexpr char name[] = "HostTiming";
    MicroProfileOnThreadCreate(name);
    Common::SetCurrentThreadName(name);
    Common::SetCurrentThreadPriority(Common::ThreadPriority::Critical);
    instance.on_thread_init();
    instance.ThreadLoop();
    MicroProfileOnThreadExit();
}

void CoreTiming::Initialize(std::function<void()>&& on_thread_init_) {
    Reset();
    on_thread_init = std::move(on_thread_init_);
    event_fifo_id = 0;
    shutting_down = false;
    ticks = 0;
    const auto empty_timed_callback = [](std::uintptr_t, u64, std::chrono::nanoseconds)
        -> std::optional<std::chrono::nanoseconds> { return std::nullopt; };
    ev_lost = CreateEvent("_lost_event", empty_timed_callback);
    if (is_multicore) {
        timer_thread = std::make_unique<std::thread>(ThreadEntry, std::ref(*this));
    }
}

void CoreTiming::ClearPendingEvents() {
    event_queue.clear();
}

void CoreTiming::Pause(bool is_paused) {
    paused = is_paused;
    pause_event.Set();

    if (!is_paused) {
        pause_end_time = GetGlobalTimeNs().count();
    }
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

    if (!is_paused) {
        pause_end_time = GetGlobalTimeNs().count();
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
                               std::uintptr_t user_data, bool absolute_time) {
    {
        std::scoped_lock scope{basic_lock};
        const auto next_time{absolute_time ? ns_into_future : GetGlobalTimeNs() + ns_into_future};

        event_queue.emplace_back(
            Event{next_time.count(), event_fifo_id++, user_data, event_type, 0});
        std::push_heap(event_queue.begin(), event_queue.end(), std::greater<>());
    }

    event.Set();
}

void CoreTiming::ScheduleLoopingEvent(std::chrono::nanoseconds start_time,
                                      std::chrono::nanoseconds resched_time,
                                      const std::shared_ptr<EventType>& event_type,
                                      std::uintptr_t user_data, bool absolute_time) {
    {
        std::scoped_lock scope{basic_lock};
        const auto next_time{absolute_time ? start_time : GetGlobalTimeNs() + start_time};

        event_queue.emplace_back(
            Event{next_time.count(), event_fifo_id++, user_data, event_type, resched_time.count()});

        std::push_heap(event_queue.begin(), event_queue.end(), std::greater<>());
    }

    event.Set();
}

void CoreTiming::UnscheduleEvent(const std::shared_ptr<EventType>& event_type,
                                 std::uintptr_t user_data, bool wait) {
    {
        std::scoped_lock lk{basic_lock};
        const auto itr =
            std::remove_if(event_queue.begin(), event_queue.end(), [&](const Event& e) {
                return e.type.lock().get() == event_type.get() && e.user_data == user_data;
            });

        // Removing random items breaks the invariant so we have to re-establish it.
        if (itr != event_queue.end()) {
            event_queue.erase(itr, event_queue.end());
            std::make_heap(event_queue.begin(), event_queue.end(), std::greater<>());
        }
    }

    // Force any in-progress events to finish
    if (wait) {
        std::scoped_lock lk{advance_lock};
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
    if (is_multicore) [[likely]] {
        return cpu_clock->GetCPUCycles();
    }
    return ticks;
}

u64 CoreTiming::GetClockTicks() const {
    if (is_multicore) [[likely]] {
        return cpu_clock->GetClockCycles();
    }
    return CpuCyclesToClockCycles(ticks);
}

std::optional<s64> CoreTiming::Advance() {
    std::scoped_lock lock{advance_lock, basic_lock};
    global_timer = GetGlobalTimeNs().count();

    while (!event_queue.empty() && event_queue.front().time <= global_timer) {
        Event evt = std::move(event_queue.front());
        std::pop_heap(event_queue.begin(), event_queue.end(), std::greater<>());
        event_queue.pop_back();

        if (const auto event_type{evt.type.lock()}) {
            basic_lock.unlock();

            const auto new_schedule_time{event_type->callback(
                evt.user_data, evt.time,
                std::chrono::nanoseconds{GetGlobalTimeNs().count() - evt.time})};

            basic_lock.lock();

            if (evt.reschedule_time != 0) {
                const auto next_schedule_time{new_schedule_time.has_value()
                                                  ? new_schedule_time.value().count()
                                                  : evt.reschedule_time};

                // If this event was scheduled into a pause, its time now is going to be way behind.
                // Re-set this event to continue from the end of the pause.
                auto next_time{evt.time + next_schedule_time};
                if (evt.time < pause_end_time) {
                    next_time = pause_end_time + next_schedule_time;
                }

                event_queue.emplace_back(
                    Event{next_time, event_fifo_id++, evt.user_data, evt.type, next_schedule_time});
                std::push_heap(event_queue.begin(), event_queue.end(), std::greater<>());
            }
        }

        global_timer = GetGlobalTimeNs().count();
    }

    if (!event_queue.empty()) {
        return event_queue.front().time;
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
                // There are more events left in the queue, wait until the next event.
                auto wait_time = *next_time - GetGlobalTimeNs().count();
                if (wait_time > 0) {
#ifdef _WIN32
                    const auto timer_resolution_ns =
                        Common::Windows::GetCurrentTimerResolution().count();

                    while (!paused && !event.IsSet() && wait_time > 0) {
                        wait_time = *next_time - GetGlobalTimeNs().count();

                        if (wait_time >= timer_resolution_ns) {
                            Common::Windows::SleepForOneTick();
                        } else {
                            std::this_thread::yield();
                        }
                    }

                    if (event.IsSet()) {
                        event.Reset();
                    }
#else
                    event.WaitFor(std::chrono::nanoseconds(wait_time));
#endif
                }
            } else {
                // Queue is empty, wait until another event is scheduled and signals us to continue.
                wait_set = true;
                event.Wait();
            }
            wait_set = false;
        }

        paused_set = true;
        event_clock->Pause(true);
        pause_event.Wait();
        event_clock->Pause(false);
    }
}

void CoreTiming::Reset() {
    paused = true;
    shutting_down = true;
    pause_event.Set();
    event.Set();
    if (timer_thread) {
        timer_thread->join();
    }
    timer_thread.reset();
    has_started = false;
}

std::chrono::nanoseconds CoreTiming::GetCPUTimeNs() const {
    if (is_multicore) [[likely]] {
        return cpu_clock->GetTimeNS();
    }
    return CyclesToNs(ticks);
}

std::chrono::nanoseconds CoreTiming::GetGlobalTimeNs() const {
    if (is_multicore) [[likely]] {
        return event_clock->GetTimeNS();
    }
    return CyclesToNs(ticks);
}

std::chrono::microseconds CoreTiming::GetGlobalTimeUs() const {
    if (is_multicore) [[likely]] {
        return event_clock->GetTimeUS();
    }
    return CyclesToUs(ticks);
}

} // namespace Core::Timing

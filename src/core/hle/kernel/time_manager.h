// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <mutex>

namespace Core {
class System;
} // namespace Core

namespace Core::Timing {
struct EventType;
} // namespace Core::Timing

namespace Kernel {

class KThread;

/**
 * The `TimeManager` takes care of scheduling time events on threads and executes their TimeUp
 * method when the event is triggered.
 */
class TimeManager {
public:
    explicit TimeManager(Core::System& system);

    /// Schedule a time event on `timetask` thread that will expire in 'nanoseconds'
    void ScheduleTimeEvent(KThread* time_task, s64 nanoseconds);

    /// Unschedule an existing time event
    void UnscheduleTimeEvent(KThread* thread);

private:
    Core::System& system;
    std::shared_ptr<Core::Timing::EventType> time_manager_event_type;
    std::mutex mutex;
};

} // namespace Kernel

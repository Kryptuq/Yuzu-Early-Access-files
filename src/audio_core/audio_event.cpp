// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/audio_event.h"
#include "common/assert.h"

namespace AudioCore {

size_t Event::GetManagerIndex(const Type type) const {
    switch (type) {
    case Type::AudioInManager:
        return 0;
    case Type::AudioOutManager:
        return 1;
    case Type::FinalOutputRecorderManager:
        return 2;
    case Type::Max:
        return 3;
    default:
        UNREACHABLE();
    }
    return 3;
}

void Event::SetAudioEvent(const Type type, const bool signalled) {
    events_signalled[GetManagerIndex(type)] = signalled;
    if (signalled) {
        manager_event.notify_one();
    }
}

bool Event::CheckAudioEventSet(const Type type) const {
    return events_signalled[GetManagerIndex(type)];
}

std::mutex& Event::GetAudioEventLock() {
    return event_lock;
}

std::condition_variable_any& Event::GetAudioEvent() {
    return manager_event;
}

bool Event::Wait(std::unique_lock<std::mutex>& l, const std::chrono::seconds timeout) {
    bool timed_out{false};
    if (!manager_event.wait_for(l, timeout, [&]() {
            return std::ranges::any_of(events_signalled, [](bool x) { return x; });
        })) {
        timed_out = true;
    }
    return timed_out;
}

void Event::ClearEvents() {
    events_signalled[0] = false;
    events_signalled[1] = false;
    events_signalled[2] = false;
    events_signalled[3] = false;
}

} // namespace AudioCore

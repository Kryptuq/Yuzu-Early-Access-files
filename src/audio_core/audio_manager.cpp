// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/audio_in_manager.h"
#include "audio_core/audio_manager.h"
#include "audio_core/audio_out_manager.h"
#include "core/core.h"

namespace AudioCore {

AudioManager::AudioManager(Core::System& system_) : system{system_} {
    thread = std::jthread([this]() { ThreadFunc(); });
}

void AudioManager::Shutdown() {
    running = false;
    events.SetAudioEvent(Event::Type::Max, true);
    thread.join();
}

Result AudioManager::SetOutManager(BufferEventFunc buffer_func) {
    if (!running) {
        return Service::Audio::ERR_OPERATION_FAILED;
    }

    std::scoped_lock l{lock};

    const auto index{events.GetManagerIndex(Event::Type::AudioOutManager)};
    if (buffer_events[index] == nullptr) {
        buffer_events[index] = buffer_func;
        needs_update = true;
        events.SetAudioEvent(Event::Type::AudioOutManager, true);
    }
    return ResultSuccess;
}

Result AudioManager::SetInManager(BufferEventFunc buffer_func) {
    if (!running) {
        return Service::Audio::ERR_OPERATION_FAILED;
    }

    std::scoped_lock l{lock};

    const auto index{events.GetManagerIndex(Event::Type::AudioInManager)};
    if (buffer_events[index] == nullptr) {
        buffer_events[index] = buffer_func;
        needs_update = true;
        events.SetAudioEvent(Event::Type::AudioInManager, true);
    }
    return ResultSuccess;
}

void AudioManager::SetEvent(const Event::Type type, const bool signalled) {
    events.SetAudioEvent(type, signalled);
}

void AudioManager::ThreadFunc() {
    std::unique_lock l{events.GetAudioEventLock()};
    events.ClearEvents();
    running = true;

    while (running) {
        auto timed_out{events.Wait(l, std::chrono::seconds(2))};

        if (events.CheckAudioEventSet(Event::Type::Max)) {
            break;
        }

        for (size_t i = 0; i < buffer_events.size(); i++) {
            if (events.CheckAudioEventSet(Event::Type(i)) || timed_out) {
                if (buffer_events[i]) {
                    buffer_events[i]();
                }
            }
            events.SetAudioEvent(Event::Type(i), false);
        }
    }
}

} // namespace AudioCore

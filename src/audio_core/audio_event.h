// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace AudioCore {
/**
 * Responsible for the input/output events, set by the stream backend when buffers are consumed, and
 * waited on by the audio manager. These callbacks signal the game's events to keep the audio buffer
 * recycling going.
 * In a real Switch this is not a separate class, and exists entirely within the audio manager.
 * On the Switch it's implemented more simply through a MultiWaitEventHolder, where it can
 * wait on multiple events at once, and the events are not needed by the backend.
 */
class Event {
public:
    enum class Type {
        AudioInManager,
        AudioOutManager,
        FinalOutputRecorderManager,
        Max,
    };

    /**
     * Convert a manager type to an index.
     *
     * @param type - The manager type to convert
     * @return The index of the type.
     */
    size_t GetManagerIndex(Type type) const;

    /**
     * Set an audio event to true or false.
     *
     * @param type      - The manager type to signal.
     * @param signalled - Its signal state.
     */
    void SetAudioEvent(Type type, bool signalled);

    /**
     * Check if the given manager type is signalled.
     *
     * @param type - The manager type to check.
     * @return True if the event is signalled, otherwise false.
     */
    bool CheckAudioEventSet(Type type) const;

    /**
     * Get the lock for audio events.
     *
     * @return Reference to the lock.
     */
    std::mutex& GetAudioEventLock();

    /**
     * Get the manager event, this signals the audio manager to release buffers and signal the game
     * for more.
     *
     * @return Reference to the condition variable.
     */
    std::condition_variable_any& GetAudioEvent();

    /**
     * Wait on the manager_event.
     *
     * @param l       - Lock held by the wait.
     * @param timeout - Timeout for the wait. This is 2 seconds by default.
     * @return True if the wait timed out, otherwise false if signalled.
     */
    bool Wait(std::unique_lock<std::mutex>& l, std::chrono::seconds timeout);

    /**
     * Reset all manager events.
     */
    void ClearEvents();

private:
    /// Lock, used by the audio manager
    std::mutex event_lock;
    /// Array of events, one per system type (see Type), last event is used to terminate
    std::array<std::atomic<bool>, 4> events_signalled;
    /// Event to signal the audio manager
    std::condition_variable_any manager_event;
};

} // namespace AudioCore

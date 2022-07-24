// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <mutex>
#include <thread>

#include "audio_core/audio_event.h"
#include "core/hle/service/audio/errors.h"

namespace Core {
class System;
}

namespace AudioCore {

namespace AudioOut {
class Manager;
}

namespace AudioIn {
class Manager;
}

/**
 * The AudioManager's main purpose is to wait for buffer events for the audio in and out managers,
 * and call an associated callback to release buffers.
 *
 * Execution pattern is:
 *     Buffers appended ->
 *     Buffers queued and played by the backend stream ->
 *     When consumed, set the corresponding manager event and signal the audio manager ->
 *     Consumed buffers are released, game is signalled ->
 *     Game appends more buffers.
 *
 * This is only used by audio in and audio out.
 */
class AudioManager {
    using BufferEventFunc = std::function<void()>;

public:
    explicit AudioManager(Core::System& system);

    /**
     * Shutdown the audio manager.
     */
    void Shutdown();

    /**
     * Register the out manager, keeping a function to be called when the out event is signalled.
     *
     * @param buffer_func - Function to be called on signal.
     * @return Result code.
     */
    Result SetOutManager(BufferEventFunc buffer_func);

    /**
     * Register the in manager, keeping a function to be called when the in event is signalled.
     *
     * @param buffer_func - Function to be called on signal.
     * @return Result code.
     */
    Result SetInManager(BufferEventFunc buffer_func);

    /**
     * Set an event to signalled, and signal the thread.
     *
     * @param type      - Manager type to set.
     * @param signalled - Set the event to true or false?
     */
    void SetEvent(Event::Type type, bool signalled);

private:
    /**
     * Main thread, waiting on a manager signal and calling the registered fucntion.
     */
    void ThreadFunc();

    /// Core system
    Core::System& system;
    /// Have sessions started palying?
    bool sessions_started{};
    /// Is the main thread running?
    std::atomic<bool> running{};
    /// Unused
    bool needs_update{};
    /// Events to be set and signalled
    Event events{};
    /// Callbacks for each manager
    std::array<BufferEventFunc, 3> buffer_events{};
    /// General lock
    std::mutex lock{};
    /// Main thread for waiting and callbacks
    std::jthread thread;
};

} // namespace AudioCore

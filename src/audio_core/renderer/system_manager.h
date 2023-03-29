// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

#include "audio_core/renderer/system.h"

namespace Core {
namespace Timing {
struct EventType;
}
class System;
} // namespace Core

namespace AudioCore::AudioRenderer {
namespace ADSP {
class ADSP;
class AudioRenderer_Mailbox;
} // namespace ADSP

/**
 * Manages all audio renderers, responsible for triggering command list generation and signalling
 * the ADSP.
 */
class SystemManager {
public:
    explicit SystemManager(Core::System& core);
    ~SystemManager();

    /**
     * Initialize the system manager, called when any system is registered.
     *
     * @return True if successfully initialized, otherwise false.
     */
    bool InitializeUnsafe();

    /**
     * Stop the system manager.
     */
    void Stop();

    /**
     * Add an audio render system to the manager.
     * The manager does not own the system, so do not free it without calling Remove.
     *
     * @param system - The system to add.
     * @return True if successfully added, otherwise false.
     */
    bool Add(System& system);

    /**
     * Remove an audio render system from the manager.
     *
     * @param system - The system to remove.
     * @return True if successfully removed, otherwise false.
     */
    bool Remove(System& system);

private:
    /**
     * Main thread responsible for command generation.
     */
    void ThreadFunc();

    enum class StreamState {
        Filling,
        Steady,
        Draining,
    };

    /// Core system
    Core::System& core;
    /// List of pointers to managed systems
    std::list<System*> systems{};
    /// Main worker thread for generating command lists
    std::jthread thread;
    /// Mutex for the systems
    std::mutex mutex1{};
    /// Mutex for adding/removing systems
    std::mutex mutex2{};
    /// Is the system manager thread active?
    std::atomic<bool> active{};
    /// Reference to the ADSP for communication
    ADSP::ADSP& adsp;
    /// AudioRenderer mailbox for communication
    ADSP::AudioRenderer_Mailbox* mailbox{};
    /// Atomic for main thread to wait on
    std::atomic<bool> update{};
};

} // namespace AudioCore::AudioRenderer

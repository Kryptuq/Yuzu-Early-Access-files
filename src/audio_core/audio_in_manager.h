// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <mutex>
#include <vector>

#include "audio_core/renderer/audio_device.h"

namespace Core {
class System;
}

namespace AudioCore::AudioIn {
class In;

constexpr size_t MaxInSessions = 4;
/**
 * Manages all audio in sessions.
 */
class Manager {
public:
    explicit Manager(Core::System& system);

    /**
     * Acquire a free session id for opening a new audio in.
     *
     * @param session_id - Output session_id.
     * @return Result code.
     */
    Result AcquireSessionId(size_t& session_id);

    /**
     * Release a session id on close.
     *
     * @param session_id - Session id to free.
     */
    void ReleaseSessionId(size_t session_id);

    /**
     * Link the audio in manager to the main audio manager.
     *
     * @return Result code.
     */
    Result LinkToManager();

    /**
     * Start the audio in manager.
     */
    void Start();

    /**
     * Callback function, called by the audio manager when the audio in event is signalled.
     */
    void BufferReleaseAndRegister();

    /**
     * Get a list of audio in device names.
     *
     * @param names     - Output container to write names to.
     * @param max_count - Maximum number of device names to write. Unused
     * @param filter    - Should the list be filtered? Unused.
     *
     * @return Number of names written.
     */
    u32 GetDeviceNames(std::vector<AudioRenderer::AudioDevice::AudioDeviceName>& names,
                       u32 max_count, bool filter);

    /// Core system
    Core::System& system;
    /// Array of session ids
    std::array<size_t, MaxInSessions> session_ids{};
    /// Array of resource user ids
    std::array<size_t, MaxInSessions> applet_resource_user_ids{};
    /// Pointer to each open session
    std::array<std::shared_ptr<In>, MaxInSessions> sessions{};
    /// The number of free sessions
    size_t num_free_sessions{};
    /// The next session id to be taken
    size_t next_session_id{};
    /// The next session id to be freed
    size_t free_session_id{};
    /// Whether this is linked to the audio manager
    bool linked_to_manager{};
    /// Whether the sessions have been started
    bool sessions_started{};
    /// Protect state due to audio manager callback
    std::recursive_mutex mutex{};
};

} // namespace AudioCore::AudioIn

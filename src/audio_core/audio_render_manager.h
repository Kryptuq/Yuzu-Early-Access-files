// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <memory>
#include <mutex>

#include "audio_core/common/common.h"
#include "audio_core/renderer/system_manager.h"
#include "core/hle/service/audio/errors.h"

namespace Core {
class System;
}

namespace AudioCore {
struct AudioRendererParameterInternal;

namespace AudioRenderer {
/**
 * Wrapper for the audio system manager, handles service calls.
 */
class Manager {
public:
    explicit Manager(Core::System& system);
    ~Manager();

    /**
     * Stop the manager.
     */
    void Stop();

    /**
     * Get the system manager.
     *
     * @return The system manager.
     */
    SystemManager& GetSystemManager();

    /**
     * Get required size for the audio renderer workbuffer.
     *
     * @param params    - Input parameters with the numbers of voices/mixes/sinks etc.
     * @param out_count - Output size of the required workbuffer.
     * @return Result code.
     */
    Result GetWorkBufferSize(const AudioRendererParameterInternal& params, u64& out_count) const;

    /**
     * Get a new session id.
     *
     * @return The new session id. -1 if invalid, otherwise 0-MaxRendererSessions.
     */
    s32 GetSessionId();

    /**
     * Get the number of currently active sessions.
     *
     * @return The number of active sessions.
     */
    u32 GetSessionCount() const;

    /**
     * Add a renderer system to the manager.
     * The system will be regularly called to generate commands for the AudioRenderer.
     *
     * @param system - The system to add.
     * @return True if the system was successfully added, otherwise false.
     */
    bool AddSystem(System& system);

    /**
     * Remove a renderer system from the manager.
     *
     * @param system - The system to remove.
     * @return True if the system was successfully removed, otherwise false.
     */
    bool RemoveSystem(System& system);

    /**
     * Free a session id when the system wants to shut down.
     *
     * @param session_id - The session id to free.
     */
    void ReleaseSessionId(s32 session_id);

private:
    /// Core system
    Core::System& system;
    /// Session ids, -1 when in use
    std::array<s32, MaxRendererSessions> session_ids{};
    /// Number of active renderers
    u32 session_count{};
    /// Lock for interacting with the sessions
    mutable std::mutex session_lock{};
    /// Regularly generates commands from the registered systems for the AudioRenderer
    std::unique_ptr<SystemManager> system_manager{};
};

} // namespace AudioRenderer
} // namespace AudioCore

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "audio_core/renderer/system.h"
#include "core/hle/service/audio/errors.h"

namespace Core {
class System;
}

namespace Kernel {
class KTransferMemory;
}

namespace AudioCore {
struct AudioRendererParameterInternal;

namespace AudioRenderer {
class Manager;

/**
 * Audio Renderer, wraps the main audio system and is mainly responsible for handling service calls.
 */
class Renderer {
public:
    explicit Renderer(Core::System& system, Manager& manager, Kernel::KEvent* rendered_event);

    /**
     * Initialize the renderer.
     * Registers the system with the AudioRenderer::Manager, allocates workbuffers and initializes
     * everything to a default state.
     *
     * @param params                  - Input parameters to initialize the system with.
     * @param transfer_memory         - Game-supplied memory for all workbuffers. Unused.
     * @param transfer_memory_size    - Size of the transfer memory. Unused.
     * @param process_handle          - Process handle, also used for memory. Unused.
     * @param applet_resource_user_id - Applet id for this renderer. Unused.
     * @param session_id              - Session id of this renderer.
     * @return Result code.
     */
    Result Initialize(const AudioRendererParameterInternal& params,
                      Kernel::KTransferMemory* transfer_memory, u64 transfer_memory_size,
                      u32 process_handle, u64 applet_resource_user_id, s32 session_id);

    /**
     * Finalize the renderer for shutdown.
     */
    void Finalize();

    /**
     * Get the renderer's system.
     *
     * @return Reference to the system.
     */
    System& GetSystem();

    /**
     * Start the renderer.
     */
    void Start();

    /**
     * Stop the renderer.
     */
    void Stop();

    /**
     * Update the audio renderer with new information.
     * Called via RequestUpdate from the AudRen:U service.
     *
     * @param input       - Input buffer containing the new data.
     * @param performance - Optional performance buffer for outputting performance metrics.
     * @param output      - Output data from the renderer.
     * @return Result code.
     */
    Result RequestUpdate(std::span<const u8> input, std::span<u8> performance,
                         std::span<u8> output);

private:
    /// System core
    Core::System& core;
    /// Manager this renderer is registered with
    Manager& manager;
    /// Is the audio renderer initialized?
    bool initialized{};
    /// Is the system registered with the manager?
    bool system_registered{};
    /// Audio render system, main driver of audio rendering
    System system;
};

} // namespace AudioRenderer
} // namespace AudioCore

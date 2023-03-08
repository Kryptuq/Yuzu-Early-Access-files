// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/audio_render_manager.h"
#include "audio_core/common/audio_renderer_parameter.h"
#include "audio_core/renderer/audio_renderer.h"
#include "audio_core/renderer/system_manager.h"
#include "core/core.h"
#include "core/hle/kernel/k_transfer_memory.h"
#include "core/hle/service/audio/errors.h"

namespace AudioCore::AudioRenderer {

Renderer::Renderer(Core::System& system_, Manager& manager_, Kernel::KEvent* rendered_event)
    : core{system_}, manager{manager_}, system{system_, rendered_event} {}

Result Renderer::Initialize(const AudioRendererParameterInternal& params,
                            Kernel::KTransferMemory* transfer_memory,
                            const u64 transfer_memory_size, const u32 process_handle,
                            const u64 applet_resource_user_id, const s32 session_id) {
    if (params.execution_mode == ExecutionMode::Auto) {
        if (!manager.AddSystem(system)) {
            LOG_ERROR(Service_Audio,
                      "Both Audio Render sessions are in use, cannot create any more");
            return Service::Audio::ResultOutOfSessions;
        }
        system_registered = true;
    }

    initialized = true;
    system.Initialize(params, transfer_memory, transfer_memory_size, process_handle,
                      applet_resource_user_id, session_id);

    return ResultSuccess;
}

void Renderer::Finalize() {
    auto session_id{system.GetSessionId()};

    system.Finalize();

    if (system_registered) {
        manager.RemoveSystem(system);
        system_registered = false;
    }

    manager.ReleaseSessionId(session_id);
}

System& Renderer::GetSystem() {
    return system;
}

void Renderer::Start() {
    system.Start();
}

void Renderer::Stop() {
    system.Stop();
}

Result Renderer::RequestUpdate(std::span<const u8> input, std::span<u8> performance,
                               std::span<u8> output) {
    return system.Update(input, performance, output);
}

} // namespace AudioCore::AudioRenderer

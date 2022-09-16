// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/audio_core.h"
#include "audio_core/audio_manager.h"
#include "audio_core/audio_out_manager.h"
#include "audio_core/out/audio_out.h"
#include "core/core.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/audio/errors.h"

namespace AudioCore::AudioOut {

Manager::Manager(Core::System& system_) : system{system_} {
    std::iota(session_ids.begin(), session_ids.end(), 0);
    num_free_sessions = MaxOutSessions;
}

Result Manager::AcquireSessionId(size_t& session_id) {
    if (num_free_sessions == 0) {
        LOG_ERROR(Service_Audio, "All 12 Audio Out sessions are in use, cannot create any more");
        return Service::Audio::ERR_MAXIMUM_SESSIONS_REACHED;
    }
    session_id = session_ids[next_session_id];
    next_session_id = (next_session_id + 1) % MaxOutSessions;
    num_free_sessions--;
    return ResultSuccess;
}

void Manager::ReleaseSessionId(const size_t session_id) {
    std::scoped_lock l{mutex};
    LOG_DEBUG(Service_Audio, "Freeing AudioOut session {}", session_id);
    session_ids[free_session_id] = session_id;
    num_free_sessions++;
    free_session_id = (free_session_id + 1) % MaxOutSessions;
    sessions[session_id].reset();
    applet_resource_user_ids[session_id] = 0;
}

Result Manager::LinkToManager() {
    std::scoped_lock l{mutex};
    if (!linked_to_manager) {
        AudioManager& manager{system.AudioCore().GetAudioManager()};
        manager.SetOutManager(std::bind(&Manager::BufferReleaseAndRegister, this));
        linked_to_manager = true;
    }

    return ResultSuccess;
}

void Manager::Start() {
    if (sessions_started) {
        return;
    }

    std::scoped_lock l{mutex};
    for (auto& session : sessions) {
        if (session) {
            session->StartSession();
        }
    }

    sessions_started = true;
}

void Manager::BufferReleaseAndRegister() {
    std::scoped_lock l{mutex};
    for (auto& session : sessions) {
        if (session != nullptr) {
            session->ReleaseAndRegisterBuffers();
        }
    }
}

u32 Manager::GetAudioOutDeviceNames(
    std::vector<AudioRenderer::AudioDevice::AudioDeviceName>& names) const {
    names.emplace_back("DeviceOut");
    return 1;
}

} // namespace AudioCore::AudioOut

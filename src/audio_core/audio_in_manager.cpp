// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/audio_core.h"
#include "audio_core/audio_in_manager.h"
#include "audio_core/audio_manager.h"
#include "audio_core/in/audio_in.h"
#include "audio_core/sink/sink_details.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/hle/service/audio/errors.h"

namespace AudioCore::AudioIn {

Manager::Manager(Core::System& system_) : system{system_} {
    std::iota(session_ids.begin(), session_ids.end(), 0);
    num_free_sessions = MaxInSessions;
}

Result Manager::AcquireSessionId(size_t& session_id) {
    if (num_free_sessions == 0) {
        LOG_ERROR(Service_Audio, "All 4 AudioIn sessions are in use, cannot create any more");
        return Service::Audio::ERR_MAXIMUM_SESSIONS_REACHED;
    }
    session_id = session_ids[next_session_id];
    next_session_id = (next_session_id + 1) % MaxInSessions;
    num_free_sessions--;
    return ResultSuccess;
}

void Manager::ReleaseSessionId(const size_t session_id) {
    std::scoped_lock l{mutex};
    LOG_DEBUG(Service_Audio, "Freeing AudioIn session {}", session_id);
    session_ids[free_session_id] = session_id;
    num_free_sessions++;
    free_session_id = (free_session_id + 1) % MaxInSessions;
    sessions[session_id].reset();
    applet_resource_user_ids[session_id] = 0;
}

Result Manager::LinkToManager() {
    std::scoped_lock l{mutex};
    if (!linked_to_manager) {
        AudioManager& manager{system.AudioCore().GetAudioManager()};
        manager.SetInManager(std::bind(&Manager::BufferReleaseAndRegister, this));
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

u32 Manager::GetDeviceNames(std::vector<AudioRenderer::AudioDevice::AudioDeviceName>& names,
                            [[maybe_unused]] const u32 max_count,
                            [[maybe_unused]] const bool filter) {
    std::scoped_lock l{mutex};

    LinkToManager();

    auto input_devices{Sink::GetDeviceListForSink(Settings::values.sink_id.GetValue(), true)};
    if (input_devices.size() > 1) {
        names.push_back(AudioRenderer::AudioDevice::AudioDeviceName("Uac"));
        return 1;
    }
    return 0;
}

} // namespace AudioCore::AudioIn

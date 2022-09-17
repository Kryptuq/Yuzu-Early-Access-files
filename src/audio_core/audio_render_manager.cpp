// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/audio_render_manager.h"
#include "audio_core/common/audio_renderer_parameter.h"
#include "audio_core/common/feature_support.h"
#include "core/core.h"

namespace AudioCore::AudioRenderer {

Manager::Manager(Core::System& system_)
    : system{system_}, system_manager{std::make_unique<SystemManager>(system)} {
    std::iota(session_ids.begin(), session_ids.end(), 0);
}

Manager::~Manager() {
    Stop();
}

void Manager::Stop() {
    system_manager->Stop();
}

SystemManager& Manager::GetSystemManager() {
    return *system_manager;
}

Result Manager::GetWorkBufferSize(const AudioRendererParameterInternal& params,
                                  u64& out_count) const {
    if (!CheckValidRevision(params.revision)) {
        return Service::Audio::ERR_INVALID_REVISION;
    }

    out_count = System::GetWorkBufferSize(params);

    return ResultSuccess;
}

s32 Manager::GetSessionId() {
    std::scoped_lock l{session_lock};
    auto session_id{session_ids[session_count]};

    if (session_id == -1) {
        return -1;
    }

    session_ids[session_count] = -1;
    session_count++;
    return session_id;
}

void Manager::ReleaseSessionId(const s32 session_id) {
    std::scoped_lock l{session_lock};
    session_ids[--session_count] = session_id;
}

u32 Manager::GetSessionCount() const {
    std::scoped_lock l{session_lock};
    return session_count;
}

bool Manager::AddSystem(System& system_) {
    return system_manager->Add(system_);
}

bool Manager::RemoveSystem(System& system_) {
    return system_manager->Remove(system_);
}

} // namespace AudioCore::AudioRenderer

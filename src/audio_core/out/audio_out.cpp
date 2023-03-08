// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/audio_out_manager.h"
#include "audio_core/out/audio_out.h"
#include "core/hle/kernel/k_event.h"

namespace AudioCore::AudioOut {

Out::Out(Core::System& system_, Manager& manager_, Kernel::KEvent* event_, size_t session_id_)
    : manager{manager_}, parent_mutex{manager.mutex}, event{event_}, system{system_, event,
                                                                            session_id_} {}

void Out::Free() {
    std::scoped_lock l{parent_mutex};
    manager.ReleaseSessionId(system.GetSessionId());
}

System& Out::GetSystem() {
    return system;
}

AudioOut::State Out::GetState() {
    std::scoped_lock l{parent_mutex};
    return system.GetState();
}

Result Out::StartSystem() {
    std::scoped_lock l{parent_mutex};
    return system.Start();
}

void Out::StartSession() {
    std::scoped_lock l{parent_mutex};
    system.StartSession();
}

Result Out::StopSystem() {
    std::scoped_lock l{parent_mutex};
    return system.Stop();
}

Result Out::AppendBuffer(const AudioOutBuffer& buffer, const u64 tag) {
    std::scoped_lock l{parent_mutex};

    if (system.AppendBuffer(buffer, tag)) {
        return ResultSuccess;
    }
    return Service::Audio::ResultBufferCountReached;
}

void Out::ReleaseAndRegisterBuffers() {
    std::scoped_lock l{parent_mutex};
    if (system.GetState() == State::Started) {
        system.ReleaseBuffers();
        system.RegisterBuffers();
    }
}

bool Out::FlushAudioOutBuffers() {
    std::scoped_lock l{parent_mutex};
    return system.FlushAudioOutBuffers();
}

u32 Out::GetReleasedBuffers(std::span<u64> tags) {
    std::scoped_lock l{parent_mutex};
    return system.GetReleasedBuffers(tags);
}

Kernel::KReadableEvent& Out::GetBufferEvent() {
    std::scoped_lock l{parent_mutex};
    return event->GetReadableEvent();
}

f32 Out::GetVolume() const {
    std::scoped_lock l{parent_mutex};
    return system.GetVolume();
}

void Out::SetVolume(const f32 volume) {
    std::scoped_lock l{parent_mutex};
    system.SetVolume(volume);
}

bool Out::ContainsAudioBuffer(const u64 tag) const {
    std::scoped_lock l{parent_mutex};
    return system.ContainsAudioBuffer(tag);
}

u32 Out::GetBufferCount() const {
    std::scoped_lock l{parent_mutex};
    return system.GetBufferCount();
}

u64 Out::GetPlayedSampleCount() const {
    std::scoped_lock l{parent_mutex};
    return system.GetPlayedSampleCount();
}

} // namespace AudioCore::AudioOut

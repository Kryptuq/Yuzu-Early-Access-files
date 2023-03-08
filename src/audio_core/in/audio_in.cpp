// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/audio_in_manager.h"
#include "audio_core/in/audio_in.h"
#include "core/hle/kernel/k_event.h"

namespace AudioCore::AudioIn {

In::In(Core::System& system_, Manager& manager_, Kernel::KEvent* event_, size_t session_id_)
    : manager{manager_}, parent_mutex{manager.mutex}, event{event_}, system{system_, event,
                                                                            session_id_} {}

void In::Free() {
    std::scoped_lock l{parent_mutex};
    manager.ReleaseSessionId(system.GetSessionId());
}

System& In::GetSystem() {
    return system;
}

AudioIn::State In::GetState() {
    std::scoped_lock l{parent_mutex};
    return system.GetState();
}

Result In::StartSystem() {
    std::scoped_lock l{parent_mutex};
    return system.Start();
}

void In::StartSession() {
    std::scoped_lock l{parent_mutex};
    system.StartSession();
}

Result In::StopSystem() {
    std::scoped_lock l{parent_mutex};
    return system.Stop();
}

Result In::AppendBuffer(const AudioInBuffer& buffer, u64 tag) {
    std::scoped_lock l{parent_mutex};

    if (system.AppendBuffer(buffer, tag)) {
        return ResultSuccess;
    }
    return Service::Audio::ResultBufferCountReached;
}

void In::ReleaseAndRegisterBuffers() {
    std::scoped_lock l{parent_mutex};
    if (system.GetState() == State::Started) {
        system.ReleaseBuffers();
        system.RegisterBuffers();
    }
}

bool In::FlushAudioInBuffers() {
    std::scoped_lock l{parent_mutex};
    return system.FlushAudioInBuffers();
}

u32 In::GetReleasedBuffers(std::span<u64> tags) {
    std::scoped_lock l{parent_mutex};
    return system.GetReleasedBuffers(tags);
}

Kernel::KReadableEvent& In::GetBufferEvent() {
    std::scoped_lock l{parent_mutex};
    return event->GetReadableEvent();
}

f32 In::GetVolume() const {
    std::scoped_lock l{parent_mutex};
    return system.GetVolume();
}

void In::SetVolume(f32 volume) {
    std::scoped_lock l{parent_mutex};
    system.SetVolume(volume);
}

bool In::ContainsAudioBuffer(u64 tag) const {
    std::scoped_lock l{parent_mutex};
    return system.ContainsAudioBuffer(tag);
}

u32 In::GetBufferCount() const {
    std::scoped_lock l{parent_mutex};
    return system.GetBufferCount();
}

u64 In::GetPlayedSampleCount() const {
    std::scoped_lock l{parent_mutex};
    return system.GetPlayedSampleCount();
}

} // namespace AudioCore::AudioIn

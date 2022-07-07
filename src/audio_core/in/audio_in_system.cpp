// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <mutex>
#include "audio_core/audio_event.h"
#include "audio_core/audio_manager.h"
#include "audio_core/in/audio_in_system.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/kernel/k_event.h"

namespace AudioCore::AudioIn {

System::System(Core::System& system_, Kernel::KEvent* event_, const size_t session_id_)
    : system{system_}, buffer_event{event_},
      session_id{session_id_}, session{std::make_unique<DeviceSession>(system_)} {}

System::~System() {
    Finalize();
}

void System::Finalize() {
    Stop();
    session->Finalize();
    buffer_event->GetWritableEvent().Signal();
}

void System::StartSession() {
    session->Start();
}

size_t System::GetSessionId() const {
    return session_id;
}

std::string_view System::GetDefaultDeviceName() {
    return "BuiltInHeadset";
}

std::string_view System::GetDefaultUacDeviceName() {
    return "Uac";
}

Result System::IsConfigValid(const std::string_view device_name,
                             const AudioInParameter& in_params) {
    if ((device_name.size() > 0) &&
        (device_name != GetDefaultDeviceName() && device_name != GetDefaultUacDeviceName())) {
        return Service::Audio::ERR_INVALID_DEVICE_NAME;
    }

    if (in_params.sample_rate != TargetSampleRate && in_params.sample_rate > 0) {
        return Service::Audio::ERR_INVALID_SAMPLE_RATE;
    }

    return ResultSuccess;
}

Result System::Initialize(std::string& device_name, const AudioInParameter& in_params,
                          const u32 handle_, const u64 applet_resource_user_id_) {
    auto result{IsConfigValid(device_name, in_params)};
    if (result.IsError()) {
        return result;
    }

    handle = handle_;
    applet_resource_user_id = applet_resource_user_id_;
    if (device_name.empty() || device_name[0] == '\0') {
        name = std::string(GetDefaultDeviceName());
    } else {
        name = std::move(device_name);
    }

    sample_rate = TargetSampleRate;
    sample_format = SampleFormat::PcmInt16;
    channel_count = in_params.channel_count <= 2 ? 2 : 6;
    volume = 1.0f;
    is_uac = name == "Uac";
    return ResultSuccess;
}

Result System::Start() {
    if (state != State::Stopped) {
        return Service::Audio::ERR_OPERATION_FAILED;
    }

    session->Initialize(name, sample_format, channel_count, session_id, handle,
                        applet_resource_user_id, Sink::StreamType::In);
    session->SetVolume(volume);
    session->Start();
    state = State::Started;

    std::vector<AudioBuffer> buffers_to_flush{};
    buffers.RegisterBuffers(buffers_to_flush);
    session->AppendBuffers(buffers_to_flush);

    return ResultSuccess;
}

Result System::Stop() {
    if (state == State::Started) {
        session->Stop();
        session->SetVolume(0.0f);
        state = State::Stopped;
    }

    return ResultSuccess;
}

bool System::AppendBuffer(const AudioInBuffer& buffer, const u64 tag) {
    if (buffers.GetTotalBufferCount() == BufferCount) {
        return false;
    }

    AudioBuffer new_buffer{
        .played_timestamp = 0, .samples = buffer.samples, .tag = tag, .size = buffer.size};

    buffers.AppendBuffer(new_buffer);
    RegisterBuffers();

    return true;
}

void System::RegisterBuffers() {
    if (state == State::Started) {
        std::vector<AudioBuffer> registered_buffers{};
        buffers.RegisterBuffers(registered_buffers);
        session->AppendBuffers(registered_buffers);
    }
}

void System::ReleaseBuffers() {
    bool signal{buffers.ReleaseBuffers(system.CoreTiming(), *session)};

    if (signal) {
        // Signal if any buffer was released, or if none are registered, we need more.
        buffer_event->GetWritableEvent().Signal();
    }
}

u32 System::GetReleasedBuffers(std::span<u64> tags) {
    return buffers.GetReleasedBuffers(tags);
}

bool System::FlushAudioInBuffers() {
    if (state != State::Started) {
        return false;
    }

    u32 buffers_released{};
    buffers.FlushBuffers(buffers_released);

    if (buffers_released > 0) {
        buffer_event->GetWritableEvent().Signal();
    }
    return true;
}

u16 System::GetChannelCount() const {
    return channel_count;
}

u32 System::GetSampleRate() const {
    return sample_rate;
}

SampleFormat System::GetSampleFormat() const {
    return sample_format;
}

State System::GetState() {
    switch (state) {
    case State::Started:
    case State::Stopped:
        return state;
    default:
        LOG_ERROR(Service_Audio, "AudioIn invalid state!");
        state = State::Stopped;
        break;
    }
    return state;
}

std::string System::GetName() const {
    return name;
}

f32 System::GetVolume() const {
    return volume;
}

void System::SetVolume(const f32 volume_) {
    volume = volume_;
    session->SetVolume(volume_);
}

bool System::ContainsAudioBuffer(const u64 tag) {
    return buffers.ContainsBuffer(tag);
}

u32 System::GetBufferCount() {
    return buffers.GetAppendedRegisteredCount();
}

u64 System::GetPlayedSampleCount() const {
    return session->GetPlayedSampleCount();
}

bool System::IsUac() const {
    return is_uac;
}

} // namespace AudioCore::AudioIn

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/audio_core.h"
#include "audio_core/audio_manager.h"
#include "audio_core/device/audio_buffer.h"
#include "audio_core/device/device_session.h"
#include "audio_core/sink/sink_stream.h"
#include "core/core.h"
#include "core/memory.h"

namespace AudioCore {

DeviceSession::DeviceSession(Core::System& system_) : system{system_} {}

DeviceSession::~DeviceSession() {
    Finalize();
}

Result DeviceSession::Initialize(std::string_view name_, SampleFormat sample_format_,
                                 u16 channel_count_, size_t session_id_, u32 handle_,
                                 u64 applet_resource_user_id_, Sink::StreamType type_) {
    if (stream) {
        Finalize();
    }
    name = fmt::format("{}-{}", name_, session_id_);
    type = type_;
    sample_format = sample_format_;
    channel_count = channel_count_;
    session_id = session_id_;
    handle = handle_;
    applet_resource_user_id = applet_resource_user_id_;

    if (type == Sink::StreamType::In) {
        sink = &system.AudioCore().GetInputSink();
    } else {
        sink = &system.AudioCore().GetOutputSink();
    }
    stream = sink->AcquireSinkStream(system, channel_count, name, type);
    return ResultSuccess;
}

void DeviceSession::Finalize() {
    Stop();
    sink->CloseStream(stream);
    stream = nullptr;
}

void DeviceSession::Start() {
    stream->SetPlayedSampleCount(played_sample_count);
    stream->Start();
}

void DeviceSession::Stop() {
    if (stream) {
        played_sample_count = stream->GetPlayedSampleCount();
        stream->Stop();
    }
}

void DeviceSession::AppendBuffers(std::span<AudioBuffer> buffers) const {
    auto& memory{system.Memory()};

    for (size_t i = 0; i < buffers.size(); i++) {
        Sink::SinkBuffer new_buffer{
            .frames = buffers[i].size / (channel_count * sizeof(s16)),
            .frames_played = 0,
            .tag = buffers[i].tag,
            .consumed = false,
        };

        if (type == Sink::StreamType::In) {
            std::vector<s16> samples{};
            stream->AppendBuffer(new_buffer, samples);
        } else {
            std::vector<s16> samples(buffers[i].size / sizeof(s16));
            memory.ReadBlockUnsafe(buffers[i].samples, samples.data(), buffers[i].size);
            stream->AppendBuffer(new_buffer, samples);
        }
    }
}

void DeviceSession::ReleaseBuffer(AudioBuffer& buffer) const {
    if (type == Sink::StreamType::In) {
        auto& memory{system.Memory()};
        auto samples{stream->ReleaseBuffer(buffer.size / sizeof(s16))};
        memory.WriteBlockUnsafe(buffer.samples, samples.data(), buffer.size);
    }
}

bool DeviceSession::IsBufferConsumed(u64 tag) const {
    if (stream) {
        return stream->IsBufferConsumed(tag);
    }
    return true;
}

void DeviceSession::SetVolume(f32 volume) const {
    if (stream) {
        stream->SetSystemVolume(volume);
    }
}

u64 DeviceSession::GetPlayedSampleCount() const {
    if (stream) {
        return stream->GetPlayedSampleCount();
    }
    return 0;
}

} // namespace AudioCore

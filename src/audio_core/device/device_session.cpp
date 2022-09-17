// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/audio_core.h"
#include "audio_core/audio_manager.h"
#include "audio_core/device/audio_buffer.h"
#include "audio_core/device/device_session.h"
#include "audio_core/sink/sink_stream.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/memory.h"

namespace AudioCore {

using namespace std::literals;
constexpr auto INCREMENT_TIME{5ms};

DeviceSession::DeviceSession(Core::System& system_)
    : system{system_}, thread_event{Core::Timing::CreateEvent(
                           "AudioOutSampleTick",
                           [this](std::uintptr_t, s64 time, std::chrono::nanoseconds) {
                               return ThreadFunc();
                           })} {}

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
    initialized = true;
    return ResultSuccess;
}

void DeviceSession::Finalize() {
    if (initialized) {
        Stop();
        sink->CloseStream(stream);
        stream = nullptr;
    }
}

void DeviceSession::Start() {
    if (stream) {
        stream->Start();
        system.CoreTiming().ScheduleLoopingEvent(std::chrono::nanoseconds::zero(), INCREMENT_TIME,
                                                 thread_event);
    }
}

void DeviceSession::Stop() {
    if (stream) {
        stream->Stop();
        system.CoreTiming().UnscheduleEvent(thread_event, {});
    }
}

void DeviceSession::AppendBuffers(std::span<const AudioBuffer> buffers) const {
    for (const auto& buffer : buffers) {
        Sink::SinkBuffer new_buffer{
            .frames = buffer.size / (channel_count * sizeof(s16)),
            .frames_played = 0,
            .tag = buffer.tag,
            .consumed = false,
        };

        if (type == Sink::StreamType::In) {
            std::vector<s16> samples{};
            stream->AppendBuffer(new_buffer, samples);
        } else {
            std::vector<s16> samples(buffer.size / sizeof(s16));
            system.Memory().ReadBlockUnsafe(buffer.samples, samples.data(), buffer.size);
            stream->AppendBuffer(new_buffer, samples);
        }
    }
}

void DeviceSession::ReleaseBuffer(const AudioBuffer& buffer) const {
    if (type == Sink::StreamType::In) {
        auto samples{stream->ReleaseBuffer(buffer.size / sizeof(s16))};
        system.Memory().WriteBlockUnsafe(buffer.samples, samples.data(), buffer.size);
    }
}

bool DeviceSession::IsBufferConsumed(const AudioBuffer& buffer) const {
    return played_sample_count >= buffer.end_timestamp;
}

void DeviceSession::SetVolume(f32 volume) const {
    if (stream) {
        stream->SetSystemVolume(volume);
    }
}

u64 DeviceSession::GetPlayedSampleCount() const {
    return played_sample_count;
}

std::optional<std::chrono::nanoseconds> DeviceSession::ThreadFunc() {
    // Add 5ms of samples at a 48K sample rate.
    played_sample_count += 48'000 * INCREMENT_TIME / 1s;
    if (type == Sink::StreamType::Out) {
        system.AudioCore().GetAudioManager().SetEvent(Event::Type::AudioOutManager, true);
    } else {
        system.AudioCore().GetAudioManager().SetEvent(Event::Type::AudioInManager, true);
    }
    return std::nullopt;
}

void DeviceSession::SetRingSize(u32 ring_size) {
    stream->SetRingSize(ring_size);
}

} // namespace AudioCore

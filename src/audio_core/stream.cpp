// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cmath>

#include "audio_core/sink.h"
#include "audio_core/sink_details.h"
#include "audio_core/sink_stream.h"
#include "audio_core/stream.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core_timing.h"

namespace AudioCore {

constexpr std::size_t MaxAudioBufferCount{32};

u32 Stream::GetNumChannels() const {
    switch (format) {
    case Format::Mono16:
        return 1;
    case Format::Stereo16:
        return 2;
    case Format::Multi51Channel16:
        return 6;
    }
    UNIMPLEMENTED_MSG("Unimplemented format={}", static_cast<u32>(format));
    return {};
}

Stream::Stream(Core::Timing::CoreTiming& core_timing_, u32 sample_rate_, Format format_,
               ReleaseCallback&& release_callback_, SinkStream& sink_stream_, std::string&& name_)
    : sample_rate{sample_rate_}, format{format_}, release_callback{std::move(release_callback_)},
      sink_stream{sink_stream_}, core_timing{core_timing_}, name{std::move(name_)} {
    release_event =
        Core::Timing::CreateEvent(name, [this](std::uintptr_t, std::chrono::nanoseconds ns_late) {
            ReleaseActiveBuffer(ns_late);
        });
}

void Stream::Play() {
    state = State::Playing;
    PlayNextBuffer();
}

void Stream::Stop() {
    state = State::Stopped;
    UNIMPLEMENTED();
}

bool Stream::Flush() {
    const bool had_buffers = !queued_buffers.empty();
    while (!queued_buffers.empty()) {
        queued_buffers.pop();
    }
    return had_buffers;
}

void Stream::SetVolume(float volume) {
    game_volume = volume;
}

Stream::State Stream::GetState() const {
    return state;
}

std::chrono::nanoseconds Stream::GetBufferReleaseNS(const Buffer& buffer) const {
    const std::size_t num_samples{buffer.GetSamples().size() / GetNumChannels()};
    return std::chrono::nanoseconds((static_cast<u64>(num_samples) * 1000000000ULL) / sample_rate);
}

static void VolumeAdjustSamples(std::vector<s16>& samples, float game_volume) {
    const float volume{std::clamp(Settings::Volume() - (1.0f - game_volume), 0.0f, 1.0f)};

    if (volume == 1.0f) {
        return;
    }

    // Perceived volume is not the same as the volume level
    const float volume_scale_factor = (0.85f * ((volume * volume) - volume)) + volume;
    for (auto& sample : samples) {
        sample = static_cast<s16>(sample * volume_scale_factor);
    }
}

void Stream::PlayNextBuffer(std::chrono::nanoseconds ns_late) {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);

    if (nanoseconds > expected_cb_time) {
        ns_late = nanoseconds - expected_cb_time;
    }

    if (!IsPlaying()) {
        // Ensure we are in playing state before playing the next buffer
        sink_stream.Flush();
        return;
    }

    if (active_buffer) {
        // Do not queue a new buffer if we are already playing a buffer
        return;
    }

    if (queued_buffers.empty()) {
        // No queued buffers - we are effectively paused
        sink_stream.Flush();
        return;
    }

    active_buffer = queued_buffers.front();
    queued_buffers.pop();

    auto& samples = active_buffer->GetSamples();

    VolumeAdjustSamples(samples, game_volume);

    sink_stream.EnqueueSamples(GetNumChannels(), samples);
    played_samples += samples.size();

    const auto buffer_release_ns = GetBufferReleaseNS(*active_buffer);

    // If ns_late is higher than the update rate ignore the delay
    if (ns_late > buffer_release_ns) {
        ns_late = {};
    }

    expected_cb_time = nanoseconds + (buffer_release_ns - ns_late);
    core_timing.ScheduleEvent(buffer_release_ns - ns_late, release_event, {});
}

void Stream::ReleaseActiveBuffer(std::chrono::nanoseconds ns_late) {
    ASSERT(active_buffer);
    released_buffers.push(std::move(active_buffer));
    release_callback();
    PlayNextBuffer(ns_late);
}

bool Stream::QueueBuffer(BufferPtr&& buffer) {
    if (queued_buffers.size() < MaxAudioBufferCount) {
        queued_buffers.push(std::move(buffer));
        PlayNextBuffer();
        return true;
    }
    return false;
}

bool Stream::ContainsBuffer([[maybe_unused]] Buffer::Tag tag) const {
    UNIMPLEMENTED();
    return {};
}

std::vector<Buffer::Tag> Stream::GetTagsAndReleaseBuffers(std::size_t max_count) {
    std::vector<Buffer::Tag> tags;
    for (std::size_t count = 0; count < max_count && !released_buffers.empty(); ++count) {
        if (released_buffers.front()) {
            tags.push_back(released_buffers.front()->GetTag());
        } else {
            ASSERT_MSG(false, "Invalid tag in released_buffers!");
        }
        released_buffers.pop();
    }
    return tags;
}

std::vector<Buffer::Tag> Stream::GetTagsAndReleaseBuffers() {
    std::vector<Buffer::Tag> tags;
    tags.reserve(released_buffers.size());
    while (!released_buffers.empty()) {
        if (released_buffers.front()) {
            tags.push_back(released_buffers.front()->GetTag());
        } else {
            ASSERT_MSG(false, "Invalid tag in released_buffers!");
        }
        released_buffers.pop();
    }
    return tags;
}

} // namespace AudioCore

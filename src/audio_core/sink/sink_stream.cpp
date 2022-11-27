// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <atomic>
#include <memory>
#include <span>
#include <vector>

#include "audio_core/audio_core.h"
#include "audio_core/common/common.h"
#include "audio_core/sink/sink_stream.h"
#include "common/common_types.h"
#include "common/fixed_point.h"
#include "common/settings.h"
#include "core/core.h"

namespace AudioCore::Sink {

void SinkStream::AppendBuffer(SinkBuffer& buffer, std::vector<s16>& samples) {
    if (type == StreamType::In) {
        queue.enqueue(buffer);
        queued_buffers++;
        return;
    }

    constexpr s32 min{std::numeric_limits<s16>::min()};
    constexpr s32 max{std::numeric_limits<s16>::max()};

    auto yuzu_volume{Settings::Volume()};
    if (yuzu_volume > 1.0f) {
        yuzu_volume = 0.6f + 20 * std::log10(yuzu_volume);
    }
    auto volume{system_volume * device_volume * yuzu_volume};

    if (system_channels == 6 && device_channels == 2) {
        // We're given 6 channels, but our device only outputs 2, so downmix.
        constexpr std::array<f32, 4> down_mix_coeff{1.0f, 0.707f, 0.251f, 0.707f};

        for (u32 read_index = 0, write_index = 0; read_index < samples.size();
             read_index += system_channels, write_index += device_channels) {
            const auto left_sample{
                ((Common::FixedPoint<49, 15>(
                      samples[read_index + static_cast<u32>(Channels::FrontLeft)]) *
                      down_mix_coeff[0] +
                  samples[read_index + static_cast<u32>(Channels::Center)] * down_mix_coeff[1] +
                  samples[read_index + static_cast<u32>(Channels::LFE)] * down_mix_coeff[2] +
                  samples[read_index + static_cast<u32>(Channels::BackLeft)] * down_mix_coeff[3]) *
                 volume)
                    .to_int()};

            const auto right_sample{
                ((Common::FixedPoint<49, 15>(
                      samples[read_index + static_cast<u32>(Channels::FrontRight)]) *
                      down_mix_coeff[0] +
                  samples[read_index + static_cast<u32>(Channels::Center)] * down_mix_coeff[1] +
                  samples[read_index + static_cast<u32>(Channels::LFE)] * down_mix_coeff[2] +
                  samples[read_index + static_cast<u32>(Channels::BackRight)] * down_mix_coeff[3]) *
                 volume)
                    .to_int()};

            samples[write_index + static_cast<u32>(Channels::FrontLeft)] =
                static_cast<s16>(std::clamp(left_sample, min, max));
            samples[write_index + static_cast<u32>(Channels::FrontRight)] =
                static_cast<s16>(std::clamp(right_sample, min, max));
        }

        samples.resize(samples.size() / system_channels * device_channels);

    } else if (system_channels == 2 && device_channels == 6) {
        // We need moar samples! Not all games will provide 6 channel audio.
        // TODO: Implement some upmixing here. Currently just passthrough, with other
        // channels left as silence.
        std::vector<s16> new_samples(samples.size() / system_channels * device_channels, 0);

        for (u32 read_index = 0, write_index = 0; read_index < samples.size();
             read_index += system_channels, write_index += device_channels) {
            const auto left_sample{static_cast<s16>(std::clamp(
                static_cast<s32>(
                    static_cast<f32>(samples[read_index + static_cast<u32>(Channels::FrontLeft)]) *
                    volume),
                min, max))};

            new_samples[write_index + static_cast<u32>(Channels::FrontLeft)] = left_sample;

            const auto right_sample{static_cast<s16>(std::clamp(
                static_cast<s32>(
                    static_cast<f32>(samples[read_index + static_cast<u32>(Channels::FrontRight)]) *
                    volume),
                min, max))};

            new_samples[write_index + static_cast<u32>(Channels::FrontRight)] = right_sample;
        }
        samples = std::move(new_samples);

    } else if (volume != 1.0f) {
        for (u32 i = 0; i < samples.size(); i++) {
            samples[i] = static_cast<s16>(
                std::clamp(static_cast<s32>(static_cast<f32>(samples[i]) * volume), min, max));
        }
    }

    samples_buffer.Push(samples);
    queue.enqueue(buffer);
    queued_buffers++;
}

std::vector<s16> SinkStream::ReleaseBuffer(u64 num_samples) {
    constexpr s32 min = std::numeric_limits<s16>::min();
    constexpr s32 max = std::numeric_limits<s16>::max();

    auto samples{samples_buffer.Pop(num_samples)};

    // TODO: Up-mix to 6 channels if the game expects it.
    // For audio input this is unlikely to ever be the case though.

    // Incoming mic volume seems to always be very quiet, so multiply by an additional 8 here.
    // TODO: Play with this and find something that works better.
    auto volume{system_volume * device_volume * 8};
    for (u32 i = 0; i < samples.size(); i++) {
        samples[i] = static_cast<s16>(
            std::clamp(static_cast<s32>(static_cast<f32>(samples[i]) * volume), min, max));
    }

    if (samples.size() < num_samples) {
        samples.resize(num_samples, 0);
    }
    return samples;
}

void SinkStream::ClearQueue() {
    samples_buffer.Pop();
    while (queue.pop()) {
    }
    queued_buffers = 0;
    playing_buffer = {};
    playing_buffer.consumed = true;
}

void SinkStream::ProcessAudioIn(std::span<const s16> input_buffer, std::size_t num_frames) {
    const std::size_t num_channels = GetDeviceChannels();
    const std::size_t frame_size = num_channels;
    const std::size_t frame_size_bytes = frame_size * sizeof(s16);
    size_t frames_written{0};

    // If we're paused or going to shut down, we don't want to consume buffers as coretiming is
    // paused and we'll desync, so just return.
    if (system.IsPaused() || system.IsShuttingDown()) {
        return;
    }

    if (queued_buffers > max_queue_size) {
        Stall();
    }

    while (frames_written < num_frames) {
        // If the playing buffer has been consumed or has no frames, we need a new one
        if (playing_buffer.consumed || playing_buffer.frames == 0) {
            if (!queue.try_dequeue(playing_buffer)) {
                // If no buffer was available we've underrun, just push the samples and
                // continue.
                samples_buffer.Push(&input_buffer[frames_written * frame_size],
                                    (num_frames - frames_written) * frame_size);
                frames_written = num_frames;
                continue;
            }
            // Successfully dequeued a new buffer.
            queued_buffers--;
        }

        // Get the minimum frames available between the currently playing buffer, and the
        // amount we have left to fill
        size_t frames_available{std::min<u64>(playing_buffer.frames - playing_buffer.frames_played,
                                              num_frames - frames_written)};

        samples_buffer.Push(&input_buffer[frames_written * frame_size],
                            frames_available * frame_size);

        frames_written += frames_available;
        playing_buffer.frames_played += frames_available;

        // If that's all the frames in the current buffer, add its samples and mark it as
        // consumed
        if (playing_buffer.frames_played >= playing_buffer.frames) {
            playing_buffer.consumed = true;
        }
    }

    std::memcpy(&last_frame[0], &input_buffer[(frames_written - 1) * frame_size], frame_size_bytes);

    if (queued_buffers <= max_queue_size) {
        Unstall();
    }
}

void SinkStream::ProcessAudioOutAndRender(std::span<s16> output_buffer, std::size_t num_frames) {
    const std::size_t num_channels = GetDeviceChannels();
    const std::size_t frame_size = num_channels;
    const std::size_t frame_size_bytes = frame_size * sizeof(s16);
    size_t frames_written{0};

    // If we're paused or going to shut down, we don't want to consume buffers as coretiming is
    // paused and we'll desync, so just play silence.
    if (system.IsPaused() || system.IsShuttingDown()) {
        constexpr std::array<s16, 6> silence{};
        for (size_t i = frames_written; i < num_frames; i++) {
            std::memcpy(&output_buffer[i * frame_size], &silence[0], frame_size_bytes);
        }
        return;
    }

    // Due to many frames being queued up with nvdec (5 frames or so?), a lot of buffers also get
    // queued up (30+) but not all at once, which causes constant stalling here, so just let the
    // video play out without attempting to stall.
    // Can hopefully remove this later with a more complete NVDEC implementation.
    const auto nvdec_active{system.AudioCore().IsNVDECActive()};

    // Core timing cannot be paused in single-core mode, so Stall ends up being called over and over
    // and never recovers to a normal state, so just skip attempting to sync things on single-core.
    if (system.IsMulticore() && !nvdec_active && queued_buffers > max_queue_size) {
        Stall();
    } else if (system.IsMulticore() && queued_buffers <= max_queue_size) {
        Unstall();
    }

    while (frames_written < num_frames) {
        // If the playing buffer has been consumed or has no frames, we need a new one
        if (playing_buffer.consumed || playing_buffer.frames == 0) {
            if (!queue.try_dequeue(playing_buffer)) {
                // If no buffer was available we've underrun, fill the remaining buffer with
                // the last written frame and continue.
                for (size_t i = frames_written; i < num_frames; i++) {
                    std::memcpy(&output_buffer[i * frame_size], &last_frame[0], frame_size_bytes);
                }
                frames_written = num_frames;
                continue;
            }
            // Successfully dequeued a new buffer.
            queued_buffers--;
        }

        // Get the minimum frames available between the currently playing buffer, and the
        // amount we have left to fill
        size_t frames_available{std::min<u64>(playing_buffer.frames - playing_buffer.frames_played,
                                              num_frames - frames_written)};

        samples_buffer.Pop(&output_buffer[frames_written * frame_size],
                           frames_available * frame_size);

        frames_written += frames_available;
        playing_buffer.frames_played += frames_available;

        // If that's all the frames in the current buffer, add its samples and mark it as
        // consumed
        if (playing_buffer.frames_played >= playing_buffer.frames) {
            playing_buffer.consumed = true;
        }
    }

    std::memcpy(&last_frame[0], &output_buffer[(frames_written - 1) * frame_size],
                frame_size_bytes);

    if (system.IsMulticore() && queued_buffers <= max_queue_size) {
        Unstall();
    }
}

void SinkStream::Stall() {
    if (IsStalled()) {
        return;
    }
    stalled_lock = system.StallProcesses();
}

void SinkStream::Unstall() {
    if (!IsStalled()) {
        return;
    }
    system.UnstallProcesses();
    stalled_lock.unlock();
}

} // namespace AudioCore::Sink

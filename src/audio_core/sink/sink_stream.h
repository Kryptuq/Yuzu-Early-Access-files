// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include "audio_core/common/common.h"
#include "common/common_types.h"

namespace AudioCore::Sink {

enum class StreamType {
    Render,
    Out,
    In,
};

struct SinkBuffer {
    u64 frames;
    u64 frames_played;
    u64 tag;
    bool consumed;
};

/**
 * Contains a real backend stream for outputting samples to hardware,
 * created only via a Sink (See Sink::AcquireSinkStream).
 *
 * Accepts a SinkBuffer and samples in PCM16 format to be output (see AppendBuffer).
 * Appended buffers act as a FIFO queue, and will be held until played.
 * You should regularly call IsBufferConsumed with the unique SinkBuffer tag to check if the buffer
 * has been consumed.
 *
 * Since these are a FIFO queue, always check IsBufferConsumed in the same order you appended the
 * buffers, skipping a buffer will result in all following buffers to never release.
 *
 * If the buffers appear to be stuck, you can stop and re-open an IAudioIn/IAudioOut service (this
 * is what games do), or call ClearQueue to flush all of the buffers without a full restart.
 */
class SinkStream {
public:
    virtual ~SinkStream() = default;

    /**
     * Finalize the sink stream.
     */
    virtual void Finalize() = 0;

    /**
     * Start the sink stream.
     *
     * @param resume - Set to true if this is resuming the stream a previously-active stream.
     *                 Default false.
     */
    virtual void Start(bool resume = false) = 0;

    /**
     * Stop the sink stream.
     */
    virtual void Stop() = 0;

    /**
     * Append a new buffer and its samples to a waiting queue to play.
     *
     * @param buffer  - Audio buffer information to be queued.
     * @param samples - The s16 samples to be queue for playback.
     */
    virtual void AppendBuffer(SinkBuffer& buffer, std::vector<s16>& samples) = 0;

    /**
     * Release a buffer. Audio In only, will fill a buffer with recorded samples.
     *
     * @param num_samples - Maximum number of samples to receive.
     * @return Vector of recorded samples. May have fewer than num_samples.
     */
    virtual std::vector<s16> ReleaseBuffer(u64 num_samples) = 0;

    /**
     * Check if a certain buffer has been consumed (fully played).
     *
     * @param tag - Unique tag of a buffer to check for.
     * @return True if the buffer has been played, otherwise false.
     */
    virtual bool IsBufferConsumed(u64 tag) = 0;

    /**
     * Empty out the buffer queue.
     */
    virtual void ClearQueue() = 0;

    /**
     * Check if the stream is paused.
     *
     * @return True if paused, otherwise false.
     */
    bool IsPaused() {
        return paused;
    }

    /**
     * Get the number of system channels in this stream.
     *
     * @return Number of system channels.
     */
    u32 GetSystemChannels() const {
        return system_channels;
    }

    /**
     * Set the number of channels the system expects.
     *
     * @param channels - New number of system channels.
     */
    void SetSystemChannels(u32 channels) {
        system_channels = channels;
    }

    /**
     * Get the number of channels the hardware supports.
     *
     * @return Number of channels supported.
     */
    u32 GetDeviceChannels() const {
        return device_channels;
    }

    /**
     * Get the total number of samples played by this stream.
     *
     * @return Number of samples played.
     */
    u64 GetPlayedSampleCount() const {
        return played_sample_count;
    }

    /**
     * Set the number of samples played.
     * This is started and stopped on system start/stop.
     *
     * @param played_sample_count_ - Number of samples to set.
     */
    void SetPlayedSampleCount(u64 played_sample_count_) {
        played_sample_count = played_sample_count_;
    }

    /**
     * Add to the played sample count.
     *
     * @param num_samples - Number of samples to add.
     */
    void AddPlayedSampleCount(u64 num_samples) {
        played_sample_count += num_samples;
    }

    /**
     * Get the system volume.
     *
     * @return The current system volume.
     */
    f32 GetSystemVolume() const {
        return system_volume;
    }

    /**
     * Get the device volume.
     *
     * @return The current device volume.
     */
    f32 GetDeviceVolume() const {
        return device_volume;
    }

    /**
     * Set the system volume.
     *
     * @param volume_ - The new system volume.
     */
    void SetSystemVolume(f32 volume_) {
        system_volume = volume_;
    }

    /**
     * Set the device volume.
     *
     * @param volume_ - The new device volume.
     */
    void SetDeviceVolume(f32 volume_) {
        device_volume = volume_;
    }

    /**
     * Get the number of queued audio buffers.
     *
     * @return The number of queued buffers.
     */
    u32 GetQueueSize() {
        return queued_buffers.load();
    }

protected:
    /// Number of buffers waiting to be played
    std::atomic<u32> queued_buffers{};
    /// Total samples played by this stream
    std::atomic<u64> played_sample_count{};
    /// Set by the audio render/in/out system which uses this stream
    f32 system_volume{1.0f};
    /// Set via IAudioDevice service calls
    f32 device_volume{1.0f};
    /// Set by the audio render/in/out systen which uses this stream
    u32 system_channels{2};
    /// Channels supported by hardware
    u32 device_channels{2};
    /// Is this stream currently paused?
    std::atomic<bool> paused{true};
    /// Was this stream previously playing?
    std::atomic<bool> was_playing{false};
};

using SinkStreamPtr = std::unique_ptr<SinkStream>;

} // namespace AudioCore::Sink

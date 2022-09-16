// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <span>
#include <vector>

#include "audio_core/common/common.h"
#include "common/common_types.h"
#include "common/reader_writer_queue.h"
#include "common/ring_buffer.h"

namespace Core {
class System;
} // namespace Core

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
 * Since these are a FIFO queue, IsBufferConsumed must be checked in the same order buffers were
 * appended, skipping a buffer will result in the queue getting stuck, and all following buffers to
 * never release.
 *
 * If the buffers appear to be stuck, you can stop and re-open an IAudioIn/IAudioOut service (this
 * is what games do), or call ClearQueue to flush all of the buffers without a full restart.
 */
class SinkStream {
public:
    explicit SinkStream(Core::System& system_, StreamType type_) : system{system_}, type{type_} {}
    virtual ~SinkStream() {
        Unstall();
    }

    /**
     * Finalize the sink stream.
     */
    virtual void Finalize() {}

    /**
     * Start the sink stream.
     *
     * @param resume - Set to true if this is resuming the stream a previously-active stream.
     *                 Default false.
     */
    virtual void Start(bool resume = false) {}

    /**
     * Stop the sink stream.
     */
    virtual void Stop() {}

    /**
     * Check if the stream is paused.
     *
     * @return True if paused, otherwise false.
     */
    bool IsPaused() const {
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

    /**
     * Set the maximum buffer queue size.
     */
    void SetRingSize(u32 ring_size) {
        max_queue_size = ring_size;
    }

    /**
     * Append a new buffer and its samples to a waiting queue to play.
     *
     * @param buffer  - Audio buffer information to be queued.
     * @param samples - The s16 samples to be queue for playback.
     */
    virtual void AppendBuffer(SinkBuffer& buffer, std::vector<s16>& samples);

    /**
     * Release a buffer. Audio In only, will fill a buffer with recorded samples.
     *
     * @param num_samples - Maximum number of samples to receive.
     * @return Vector of recorded samples. May have fewer than num_samples.
     */
    virtual std::vector<s16> ReleaseBuffer(u64 num_samples);

    /**
     * Empty out the buffer queue.
     */
    void ClearQueue();

    /**
     * Callback for AudioIn.
     *
     * @param input_buffer - Input buffer to be filled with samples.
     * @param num_frames - Number of frames to be filled.
     */
    void ProcessAudioIn(std::span<const s16> input_buffer, std::size_t num_frames);

    /**
     * Callback for AudioOut and AudioRenderer.
     *
     * @param output_buffer - Output buffer to be filled with samples.
     * @param num_frames - Number of frames to be filled.
     */
    void ProcessAudioOutAndRender(std::span<s16> output_buffer, std::size_t num_frames);

    /**
     * Stall core processes if the audio thread falls too far behind.
     */
    void Stall();

    /**
     * Unstall core processes.
     */
    void Unstall();

protected:
    /// Core system
    Core::System& system;
    /// Type of this stream
    StreamType type;
    /// Set by the audio render/in/out system which uses this stream
    u32 system_channels{2};
    /// Channels supported by hardware
    u32 device_channels{2};
    /// Is this stream currently paused?
    std::atomic<bool> paused{true};
    /// Name of this stream
    std::string name{};

private:
    /// Ring buffer of the samples waiting to be played or consumed
    Common::RingBuffer<s16, 0x10000> samples_buffer;
    /// Audio buffers queued and waiting to play
    Common::ReaderWriterQueue<SinkBuffer> queue;
    /// The currently-playing audio buffer
    SinkBuffer playing_buffer{};
    /// The last played (or received) frame of audio, used when the callback underruns
    std::array<s16, MaxChannels> last_frame{};
    /// Number of buffers waiting to be played
    std::atomic<u32> queued_buffers{};
    /// The ring size for audio out buffers (usually 4, rarely 2 or 8)
    u32 max_queue_size{};
    /// Set by the audio render/in/out system which uses this stream
    f32 system_volume{1.0f};
    /// Set via IAudioDevice service calls
    f32 device_volume{1.0f};
    /// True if coretiming has been stalled
    bool stalled{false};
};

using SinkStreamPtr = std::unique_ptr<SinkStream>;

} // namespace AudioCore::Sink

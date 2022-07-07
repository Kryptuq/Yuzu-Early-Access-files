// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "audio_core/common/common.h"
#include "audio_core/sink/sink.h"
#include "core/hle/service/audio/errors.h"

namespace Core {
class System;
}

namespace AudioCore {
namespace Sink {
class SinkStream;
struct SinkBuffer;
} // namespace Sink

struct AudioBuffer;

/**
 * Represents an input or output device stream for audio in and audio out (not used for render).
 **/
class DeviceSession {
public:
    explicit DeviceSession(Core::System& system);
    ~DeviceSession();

    /**
     * Initialize this device session.
     *
     * @param name                    - Name of this device.
     * @param sample_format           - Sample format for this device's output.
     * @param channel_count           - Number of channels for this device (2 or 6).
     * @param session_id              - This session's id.
     * @param handle                  - Handle for this device session (unused).
     * @param applet_resource_user_id - Applet resource user id for this device session (unused).
     * @param type                    - Type of this stream (Render, In, Out).
     * @return Result code for this call.
     */
    Result Initialize(std::string_view name, SampleFormat sample_format, u16 channel_count,
                      size_t session_id, u32 handle, u64 applet_resource_user_id,
                      Sink::StreamType type);

    /**
     * Finalize this device session.
     */
    void Finalize();

    /**
     * Append audio buffers to this device session to be played back.
     *
     * @param buffers - The buffers to play.
     */
    void AppendBuffers(std::span<AudioBuffer> buffers) const;

    /**
     * (Audio In only) Pop samples from the backend, and write them back to this buffer's address.
     *
     * @param buffer - The buffer to write to.
     */
    void ReleaseBuffer(AudioBuffer& buffer) const;

    /**
     * Check if the buffer for the given tag has been consumed by the backend.
     *
     * @param tag - Unqiue tag of the buffer to check.
     * @return true if the buffer has been consumed, otherwise false.
     */
    bool IsBufferConsumed(u64 tag) const;

    /**
     * Start this device session, starting the backend stream.
     */
    void Start();

    /**
     * Stop this device session, stopping the backend stream.
     */
    void Stop();

    /**
     * Set this device session's volume.
     *
     * @param volume - New volume for this session.
     */
    void SetVolume(f32 volume) const;

    /**
     * Get this device session's total played sample count.
     *
     * @return Samples played by this session.
     */
    u64 GetPlayedSampleCount() const;

private:
    /// System
    Core::System& system;
    /// Output sink this device will use
    Sink::Sink* sink;
    /// The backend stream for this device session to send samples to
    Sink::SinkStream* stream{};
    /// Name of this device session
    std::string name{};
    /// Type of this device session (render/in/out)
    Sink::StreamType type{};
    /// Sample format for this device.
    SampleFormat sample_format{SampleFormat::PcmInt16};
    /// Channel count for this device session
    u16 channel_count{};
    /// Session id of this device session
    size_t session_id{};
    /// Handle of this device session
    u32 handle{};
    /// Applet resource user id of this device session
    u64 applet_resource_user_id{};
    /// Total number of samples played by this device session
    u64 played_sample_count{};
};

} // namespace AudioCore

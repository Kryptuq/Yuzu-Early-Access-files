// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <memory>
#include <span>
#include <string>

#include "audio_core/common/common.h"
#include "audio_core/device/audio_buffers.h"
#include "audio_core/device/device_session.h"
#include "core/hle/service/audio/errors.h"

namespace Core {
class System;
}

namespace Kernel {
class KEvent;
}

namespace AudioCore::AudioOut {

constexpr SessionTypes SessionType = SessionTypes::AudioOut;

struct AudioOutParameter {
    /* 0x0 */ s32_le sample_rate;
    /* 0x4 */ u16_le channel_count;
    /* 0x6 */ u16_le reserved;
};
static_assert(sizeof(AudioOutParameter) == 0x8, "AudioOutParameter is an invalid size");

struct AudioOutParameterInternal {
    /* 0x0 */ u32_le sample_rate;
    /* 0x4 */ u32_le channel_count;
    /* 0x8 */ u32_le sample_format;
    /* 0xC */ u32_le state;
};
static_assert(sizeof(AudioOutParameterInternal) == 0x10,
              "AudioOutParameterInternal is an invalid size");

struct AudioOutBuffer {
    /* 0x00 */ AudioOutBuffer* next;
    /* 0x08 */ VAddr samples;
    /* 0x10 */ u64 capacity;
    /* 0x18 */ u64 size;
    /* 0x20 */ u64 offset;
};
static_assert(sizeof(AudioOutBuffer) == 0x28, "AudioOutBuffer is an invalid size");

enum class State {
    Started,
    Stopped,
};

/**
 * Controls and drives audio output.
 */
class System {
public:
    explicit System(Core::System& system, Kernel::KEvent* event, size_t session_id);
    ~System();

    /**
     * Get the default audio output device name.
     *
     * @return The default audio output device name.
     */
    std::string_view GetDefaultOutputDeviceName();

    /**
     * Is the given initialize config valid?
     *
     * @param device_name - The name of the requested output device.
     * @param in_params   - Input parameters, see AudioOutParameter.
     * @return Result code.
     */
    Result IsConfigValid(std::string_view device_name, const AudioOutParameter& in_params);

    /**
     * Initialize this system.
     *
     * @param device_name             - The name of the requested output device.
     * @param in_params               - Input parameters, see AudioOutParameter.
     * @param handle                  - Unused.
     * @param applet_resource_user_id - Unused.
     * @return Result code.
     */
    Result Initialize(std::string& device_name, const AudioOutParameter& in_params, u32 handle,
                      u64& applet_resource_user_id);

    /**
     * Start this system.
     *
     * @return Result code.
     */
    Result Start();

    /**
     * Stop this system.
     *
     * @return Result code.
     */
    Result Stop();

    /**
     * Finalize this system.
     */
    void Finalize();

    /**
     * Start this system's device session.
     */
    void StartSession();

    /**
     * Get this system's id.
     */
    size_t GetSessionId() const;

    /**
     * Append a new buffer to the device.
     *
     * @param buffer - New buffer to append.
     * @param tag    - Unique tag of the buffer.
     * @return True if the buffer was appended, otherwise false.
     */
    bool AppendBuffer(const AudioOutBuffer& buffer, u64 tag);

    /**
     * Register all appended buffers.
     */
    void RegisterBuffers();

    /**
     * Release all registered buffers.
     */
    void ReleaseBuffers();

    /**
     * Get all released buffers.
     *
     * @param tags - Container to be filled with the released buffers' tags.
     * @return The number of buffers released.
     */
    u32 GetReleasedBuffers(std::span<u64> tags);

    /**
     * Flush all appended and registered buffers.
     *
     * @return True if buffers were successfully flushed, otherwise false.
     */
    bool FlushAudioOutBuffers();

    /**
     * Get this system's current channel count.
     *
     * @return The channel count.
     */
    u16 GetChannelCount() const;

    /**
     * Get this system's current sample rate.
     *
     * @return The sample rate.
     */
    u32 GetSampleRate() const;

    /**
     * Get this system's current sample format.
     *
     * @return The sample format.
     */
    SampleFormat GetSampleFormat() const;

    /**
     * Get this system's current state.
     *
     * @return The current state.
     */
    State GetState();

    /**
     * Get this system's name.
     *
     * @return The system's name.
     */
    std::string GetName() const;

    /**
     * Get this system's current volume.
     *
     * @return The system's current volume.
     */
    f32 GetVolume() const;

    /**
     * Set this system's current volume.
     *
     * @param The new volume.
     */
    void SetVolume(f32 volume);

    /**
     * Does the system contain this buffer?
     *
     * @param tag - Unique tag to search for.
     * @return True if the buffer is in the system, otherwise false.
     */
    bool ContainsAudioBuffer(u64 tag);

    /**
     * Get the maximum number of usable buffers (default 32).
     *
     * @return The number of buffers.
     */
    u32 GetBufferCount();

    /**
     * Get the total number of samples played by this system.
     *
     * @return The number of samples.
     */
    u64 GetPlayedSampleCount() const;

private:
    /// Core system
    Core::System& system;
    /// (Unused)
    u32 handle{};
    /// (Unused)
    u64 applet_resource_user_id{};
    /// Buffer event, signalled when a buffer is ready
    Kernel::KEvent* buffer_event;
    /// Session id of this system
    size_t session_id{};
    /// Device session for this system
    std::unique_ptr<DeviceSession> session;
    /// Audio buffers in use by this system
    AudioBuffers<BufferCount> buffers{BufferCount};
    /// Sample rate of this system
    u32 sample_rate{};
    /// Sample format of this system
    SampleFormat sample_format{SampleFormat::PcmInt16};
    /// Channel count of this system
    u16 channel_count{};
    /// State of this system
    std::atomic<State> state{State::Stopped};
    /// Name of this system
    std::string name{};
    /// Volume of this system
    f32 volume{1.0f};
};

} // namespace AudioCore::AudioOut

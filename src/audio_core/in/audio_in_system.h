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

namespace AudioCore::AudioIn {

constexpr SessionTypes SessionType = SessionTypes::AudioIn;

struct AudioInParameter {
    /* 0x0 */ s32_le sample_rate;
    /* 0x4 */ u16_le channel_count;
    /* 0x6 */ u16_le reserved;
};
static_assert(sizeof(AudioInParameter) == 0x8, "AudioInParameter is an invalid size");

struct AudioInParameterInternal {
    /* 0x0 */ u32_le sample_rate;
    /* 0x4 */ u32_le channel_count;
    /* 0x8 */ u32_le sample_format;
    /* 0xC */ u32_le state;
};
static_assert(sizeof(AudioInParameterInternal) == 0x10,
              "AudioInParameterInternal is an invalid size");

struct AudioInBuffer {
    /* 0x00 */ AudioInBuffer* next;
    /* 0x08 */ VAddr samples;
    /* 0x10 */ u64 capacity;
    /* 0x18 */ u64 size;
    /* 0x20 */ u64 offset;
};
static_assert(sizeof(AudioInBuffer) == 0x28, "AudioInBuffer is an invalid size");

enum class State {
    Started,
    Stopped,
};

/**
 * Controls and drives audio input.
 */
class System {
public:
    explicit System(Core::System& system, Kernel::KEvent* event, size_t session_id);
    ~System();

    /**
     * Get the default audio input device name.
     *
     * @return The default audio input device name.
     */
    std::string_view GetDefaultDeviceName() const;

    /**
     * Get the default USB audio input device name.
     * This is preferred over non-USB as some games refuse to work with the BuiltInHeadset
     * (e.g Let's Sing).
     *
     * @return The default USB audio input device name.
     */
    std::string_view GetDefaultUacDeviceName() const;

    /**
     * Is the given initialize config valid?
     *
     * @param device_name - The name of the requested input device.
     * @param in_params   - Input parameters, see AudioInParameter.
     * @return Result code.
     */
    Result IsConfigValid(std::string_view device_name, const AudioInParameter& in_params) const;

    /**
     * Initialize this system.
     *
     * @param device_name             - The name of the requested input device.
     * @param in_params               - Input parameters, see AudioInParameter.
     * @param handle                  - Unused.
     * @param applet_resource_user_id - Unused.
     * @return Result code.
     */
    Result Initialize(std::string& device_name, const AudioInParameter& in_params, u32 handle,
                      u64 applet_resource_user_id);

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
    bool AppendBuffer(const AudioInBuffer& buffer, u64 tag);

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
    bool FlushAudioInBuffers();

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
     * @param volume The new volume.
     */
    void SetVolume(f32 volume);

    /**
     * Does the system contain this buffer?
     *
     * @param tag - Unique tag to search for.
     * @return True if the buffer is in the system, otherwise false.
     */
    bool ContainsAudioBuffer(u64 tag) const;

    /**
     * Get the maximum number of usable buffers (default 32).
     *
     * @return The number of buffers.
     */
    u32 GetBufferCount() const;

    /**
     * Get the total number of samples played by this system.
     *
     * @return The number of samples.
     */
    u64 GetPlayedSampleCount() const;

    /**
     * Is this system using a USB device?
     *
     * @return True if using a USB device, otherwise false.
     */
    bool IsUac() const;

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
    /// Is this system's device USB?
    bool is_uac{false};
};

} // namespace AudioCore::AudioIn

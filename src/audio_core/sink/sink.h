// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string>

#include "audio_core/sink/sink_stream.h"
#include "common/common_types.h"

namespace Common {
class Event;
}
namespace Core {
class System;
}

namespace AudioCore::Sink {

constexpr char auto_device_name[] = "auto";

/**
 * This class is an interface for an audio sink, holds multiple output streams and is responsible
 * for sinking samples to hardware. Used by Audio Render, Audio In and Audio Out.
 */
class Sink {
public:
    virtual ~Sink() = default;
    /**
     * Close a given stream.
     *
     * @param stream - The stream to close.
     */
    virtual void CloseStream(SinkStream* stream) = 0;

    /**
     * Close all streams.
     */
    virtual void CloseStreams() = 0;

    /**
     * Pause all streams.
     */
    virtual void PauseStreams() = 0;

    /**
     * Unpause all streams.
     */
    virtual void UnpauseStreams() = 0;

    /**
     * Create a new sink stream, kept within this sink, with a pointer returned for use.
     * Do not free the returned pointer. When done with the stream, call CloseStream on the sink.
     *
     * @param system          - Core system.
     * @param system_channels - Number of channels the audio system expects.
     *                          May differ from the device's channel count.
     * @param name            - Name of this stream.
     * @param type            - Type of this stream, render/in/out.
     * @param event           - Audio render only, a signal used to prevent the renderer running too
     *                          fast.
     * @return A pointer to the created SinkStream
     */
    virtual SinkStream* AcquireSinkStream(Core::System& system, u32 system_channels,
                                          const std::string& name, StreamType type) = 0;

    /**
     * Get the number of channels the hardware device supports.
     * Either 2 or 6.
     *
     * @return Number of device channels.
     */
    u32 GetDeviceChannels() const {
        return device_channels;
    }

    /**
     * Get the device volume. Set from calls to the IAudioDevice service.
     *
     * @return Volume of the device.
     */
    virtual f32 GetDeviceVolume() const = 0;

    /**
     * Set the device volume. Set from calls to the IAudioDevice service.
     *
     * @param volume - New volume of the device.
     */
    virtual void SetDeviceVolume(f32 volume) = 0;

    /**
     * Set the system volume. Comes from the audio system using this stream.
     *
     * @param volume - New volume of the system.
     */
    virtual void SetSystemVolume(f32 volume) = 0;

protected:
    /// Number of device channels supported by the hardware
    u32 device_channels{2};
};

using SinkPtr = std::unique_ptr<Sink>;

} // namespace AudioCore::Sink

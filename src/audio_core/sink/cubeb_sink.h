// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>

#include <cubeb/cubeb.h>

#include "audio_core/sink/sink.h"

namespace Core {
class System;
}

namespace AudioCore::Sink {
class SinkStream;

/**
 * Cubeb backend sink, holds multiple output streams and is responsible for sinking samples to
 * hardware. Used by Audio Render, Audio In and Audio Out.
 */
class CubebSink final : public Sink {
public:
    explicit CubebSink(std::string_view device_id);
    ~CubebSink() override;

    /**
     * Create a new sink stream.
     *
     * @param system          - Core system.
     * @param system_channels - Number of channels the audio system expects.
     *                          May differ from the device's channel count.
     * @param name            - Name of this stream.
     * @param type            - Type of this stream, render/in/out.
     *
     * @return A pointer to the created SinkStream
     */
    SinkStream* AcquireSinkStream(Core::System& system, u32 system_channels,
                                  const std::string& name, StreamType type) override;

    /**
     * Close a given stream.
     *
     * @param stream - The stream to close.
     */
    void CloseStream(SinkStream* stream) override;

    /**
     * Close all streams.
     */
    void CloseStreams() override;

    /**
     * Get the device volume. Set from calls to the IAudioDevice service.
     *
     * @return Volume of the device.
     */
    f32 GetDeviceVolume() const override;

    /**
     * Set the device volume. Set from calls to the IAudioDevice service.
     *
     * @param volume - New volume of the device.
     */
    void SetDeviceVolume(f32 volume) override;

    /**
     * Set the system volume. Comes from the audio system using this stream.
     *
     * @param volume - New volume of the system.
     */
    void SetSystemVolume(f32 volume) override;

private:
    /// Backend Cubeb context
    cubeb* ctx{};
    /// Cubeb id of the actual hardware output device
    cubeb_devid output_device{};
    /// Cubeb id of the actual hardware input device
    cubeb_devid input_device{};
    /// Vector of streams managed by this sink
    std::vector<SinkStreamPtr> sink_streams{};

#ifdef _WIN32
    /// Cubeb required COM to be initialized multi-threaded on Windows
    u32 com_init_result = 0;
#endif
};

/**
 * Get a list of connected devices from Cubeb.
 *
 * @param capture - Return input (capture) devices if true, otherwise output devices.
 */
std::vector<std::string> ListCubebSinkDevices(bool capture);

/**
 * Get the reported latency for this sink.
 *
 * @return Minimum latency for this sink.
 */
u32 GetCubebLatency();

} // namespace AudioCore::Sink

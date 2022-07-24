// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "audio_core/audio_render_manager.h"

namespace Core {
class System;
}

namespace AudioCore {
namespace Sink {
class Sink;
}

namespace AudioRenderer {
/**
 * An interface to an output audio device available to the Switch.
 */
class AudioDevice {
public:
    struct AudioDeviceName {
        std::array<char, 0x100> name;

        AudioDeviceName(const char* name_) {
            std::strncpy(name.data(), name_, name.size());
        }
    };

    std::array<AudioDeviceName, 4> usb_device_names{"AudioStereoJackOutput",
                                                    "AudioBuiltInSpeakerOutput", "AudioTvOutput",
                                                    "AudioUsbDeviceOutput"};
    std::array<AudioDeviceName, 3> device_names{"AudioStereoJackOutput",
                                                "AudioBuiltInSpeakerOutput", "AudioTvOutput"};
    std::array<AudioDeviceName, 3> output_device_names{"AudioBuiltInSpeakerOutput", "AudioTvOutput",
                                                       "AudioExternalOutput"};

    explicit AudioDevice(Core::System& system, u64 applet_resource_user_id, u32 revision);

    /**
     * Get a list of the available output devices.
     *
     * @param out_buffer - Output buffer to write the available device names.
     * @param max_count  - Maximum number of devices to write (count of out_buffer).
     * @return Number of device names written.
     */
    u32 ListAudioDeviceName(std::vector<AudioDeviceName>& out_buffer, size_t max_count);

    /**
     * Get a list of the available output devices.
     * Different to above somehow...
     *
     * @param out_buffer - Output buffer to write the available device names.
     * @param max_count  - Maximum number of devices to write (count of out_buffer).
     * @return Number of device names written.
     */
    u32 ListAudioOutputDeviceName(std::vector<AudioDeviceName>& out_buffer, size_t max_count);

    /**
     * Set the volume of all streams in the backend sink.
     *
     * @param volume - Volume to set.
     */
    void SetDeviceVolumes(f32 volume);

    /**
     * Get the volume for a given device name.
     * Note: This is not fully implemented, we only assume 1 device for all streams.
     *
     * @param name - Name of the device to check. Unused.
     * @return Volume of the device.
     */
    f32 GetDeviceVolume(std::string_view name);

private:
    /// Backend output sink for the device
    Sink::Sink& output_sink;
    /// Resource id this device is used for
    const u64 applet_resource_user_id;
    /// User audio renderer revision
    const u32 user_revision;
};

} // namespace AudioRenderer
} // namespace AudioCore

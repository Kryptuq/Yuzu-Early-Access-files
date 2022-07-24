// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/audio_core.h"
#include "audio_core/common/feature_support.h"
#include "audio_core/renderer/audio_device.h"
#include "audio_core/sink/sink.h"
#include "core/core.h"

namespace AudioCore::AudioRenderer {

AudioDevice::AudioDevice(Core::System& system, const u64 applet_resource_user_id_,
                         const u32 revision)
    : output_sink{system.AudioCore().GetOutputSink()},
      applet_resource_user_id{applet_resource_user_id_}, user_revision{revision} {}

u32 AudioDevice::ListAudioDeviceName(std::vector<AudioDeviceName>& out_buffer,
                                     const size_t max_count) {
    std::span<AudioDeviceName> names{};

    if (CheckFeatureSupported(SupportTags::AudioUsbDeviceOutput, user_revision)) {
        names = usb_device_names;
    } else {
        names = device_names;
    }

    u32 out_count{static_cast<u32>(std::min(max_count, names.size()))};
    for (u32 i = 0; i < out_count; i++) {
        out_buffer.push_back(names[i]);
    }
    return out_count;
}

u32 AudioDevice::ListAudioOutputDeviceName(std::vector<AudioDeviceName>& out_buffer,
                                           const size_t max_count) {
    u32 out_count{static_cast<u32>(std::min(max_count, output_device_names.size()))};

    for (u32 i = 0; i < out_count; i++) {
        out_buffer.push_back(output_device_names[i]);
    }
    return out_count;
}

void AudioDevice::SetDeviceVolumes(const f32 volume) {
    output_sink.SetDeviceVolume(volume);
}

f32 AudioDevice::GetDeviceVolume([[maybe_unused]] std::string_view name) {
    return output_sink.GetDeviceVolume();
}

} // namespace AudioCore::AudioRenderer

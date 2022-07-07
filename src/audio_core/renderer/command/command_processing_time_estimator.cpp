// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/command/command_processing_time_estimator.h"

namespace AudioCore::AudioRenderer {

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    const PcmInt16DataSourceVersion1Command& command) const {
    return static_cast<u32>(command.pitch * 0.25f * 1.2f);
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    const PcmInt16DataSourceVersion2Command& command) const {
    return static_cast<u32>(command.pitch * 0.25f * 1.2f);
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const PcmFloatDataSourceVersion1Command& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const PcmFloatDataSourceVersion2Command& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    const AdpcmDataSourceVersion1Command& command) const {
    return static_cast<u32>(command.pitch * 0.25f * 1.2f);
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    const AdpcmDataSourceVersion2Command& command) const {
    return static_cast<u32>(command.pitch * 0.25f * 1.2f);
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const VolumeCommand& command) const {
    return static_cast<u32>((static_cast<f32>(sample_count) * 8.8f) * 1.2f);
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const VolumeRampCommand& command) const {
    return static_cast<u32>((static_cast<f32>(sample_count) * 9.8f) * 1.2f);
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const BiquadFilterCommand& command) const {
    return static_cast<u32>((static_cast<f32>(sample_count) * 58.0f) * 1.2f);
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const MixCommand& command) const {
    return static_cast<u32>((static_cast<f32>(sample_count) * 10.0f) * 1.2f);
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const MixRampCommand& command) const {
    return static_cast<u32>((static_cast<f32>(sample_count) * 14.4f) * 1.2f);
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(const MixRampGroupedCommand& command) const {
    u32 count{0};
    for (u32 i = 0; i < command.buffer_count; i++) {
        if (command.volumes[i] != 0.0f || command.prev_volumes[i] != 0.0f) {
            count++;
        }
    }

    return static_cast<u32>(((static_cast<f32>(sample_count) * 14.4f) * 1.2f) *
                            static_cast<f32>(count));
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const DepopPrepareCommand& command) const {
    return 1080;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    const DepopForMixBuffersCommand& command) const {
    return static_cast<u32>((static_cast<f32>(sample_count) * 8.9f) *
                            static_cast<f32>(command.count));
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(const DelayCommand& command) const {
    return static_cast<u32>((static_cast<f32>(sample_count) * command.parameter.channel_count) *
                            202.5f);
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const UpsampleCommand& command) const {
    return 357915;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const DownMix6chTo2chCommand& command) const {
    return 16108;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(const AuxCommand& command) const {
    if (command.enabled) {
        return 15956;
    }
    return 3765;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const DeviceSinkCommand& command) const {
    return 10042;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const CircularBufferSinkCommand& command) const {
    return 55;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(const ReverbCommand& command) const {
    if (command.enabled) {
        return static_cast<u32>(
            (command.parameter.channel_count * static_cast<f32>(sample_count) * 750) * 1.2f);
    }
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(const I3dl2ReverbCommand& command) const {
    if (command.enabled) {
        return static_cast<u32>(
            (command.parameter.channel_count * static_cast<f32>(sample_count) * 530) * 1.2f);
    }
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const PerformanceCommand& command) const {
    return 1454;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const ClearMixBufferCommand& command) const {
    return static_cast<u32>(
        ((static_cast<f32>(sample_count) * 0.83f) * static_cast<f32>(buffer_count)) * 1.2f);
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const CopyMixBufferCommand& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const LightLimiterVersion1Command& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const LightLimiterVersion2Command& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const MultiTapBiquadFilterCommand& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion1::Estimate(
    [[maybe_unused]] const CaptureCommand& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    const PcmInt16DataSourceVersion1Command& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(
            (static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
                (command.pitch * 2.0f) * 749.269f +
            6138.94f);
    case 240:
        return static_cast<u32>(
            (static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
                (command.pitch * 2.0f) * 1195.456f +
            7797.047f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    const PcmInt16DataSourceVersion2Command& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(
            (static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
                (command.pitch * 2.0f) * 749.269f +
            6138.94f);
    case 240:
        return static_cast<u32>(
            (static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
                (command.pitch * 2.0f) * 1195.456f +
            7797.047f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    const PcmFloatDataSourceVersion1Command& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(
            (static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
                (command.pitch * 2.0f) * 749.269f +
            6138.94f);
    case 240:
        return static_cast<u32>(
            (static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
                (command.pitch * 2.0f) * 1195.456f +
            7797.047f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    const PcmFloatDataSourceVersion2Command& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(
            (static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
                (command.pitch * 2.0f) * 749.269f +
            6138.94f);
    case 240:
        return static_cast<u32>(
            (static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
                (command.pitch * 2.0f) * 1195.456f +
            7797.047f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    const AdpcmDataSourceVersion1Command& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(
            (static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
                (command.pitch * 2.0f) * 2125.588f +
            9039.47f);
    case 240:
        return static_cast<u32>(
            (static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
                (command.pitch * 2.0f) * 3564.088 +
            6225.471);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    const AdpcmDataSourceVersion2Command& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(
            (static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
                (command.pitch * 2.0f) * 2125.588f +
            9039.47f);
    case 240:
        return static_cast<u32>(
            (static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
                (command.pitch * 2.0f) * 3564.088 +
            6225.471);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const VolumeCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(1280.3f);
    case 240:
        return static_cast<u32>(1737.8f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const VolumeRampCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(1403.9f);
    case 240:
        return static_cast<u32>(1884.3f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const BiquadFilterCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(4813.2f);
    case 240:
        return static_cast<u32>(6915.4f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const MixCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(1342.2f);
    case 240:
        return static_cast<u32>(1833.2f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const MixRampCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(1859.0f);
    case 240:
        return static_cast<u32>(2286.1f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(const MixRampGroupedCommand& command) const {
    u32 count{0};
    for (u32 i = 0; i < command.buffer_count; i++) {
        if (command.volumes[i] != 0.0f || command.prev_volumes[i] != 0.0f) {
            count++;
        }
    }

    switch (sample_count) {
    case 160:
        return static_cast<u32>((static_cast<f32>(sample_count) * 7.245f) *
                                static_cast<f32>(count));
    case 240:
        return static_cast<u32>((static_cast<f32>(sample_count) * 7.245f) *
                                static_cast<f32>(count));
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const DepopPrepareCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(306.62f);
    case 240:
        return static_cast<u32>(293.22f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const DepopForMixBuffersCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(762.96f);
    case 240:
        return static_cast<u32>(726.96f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(const DelayCommand& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(41635.555f);
            case 2:
                return static_cast<u32>(97861.211f);
            case 4:
                return static_cast<u32>(192515.516f);
            case 6:
                return static_cast<u32>(301755.969f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(578.529f);
        case 2:
            return static_cast<u32>(663.064f);
        case 4:
            return static_cast<u32>(703.983f);
        case 6:
            return static_cast<u32>(760.032f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    case 240:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(8770.345f);
            case 2:
                return static_cast<u32>(25741.18f);
            case 4:
                return static_cast<u32>(47551.168f);
            case 6:
                return static_cast<u32>(81629.219f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(521.283f);
        case 2:
            return static_cast<u32>(585.396f);
        case 4:
            return static_cast<u32>(629.884f);
        case 6:
            return static_cast<u32>(713.57f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const UpsampleCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(292000.0f);
    case 240:
        return static_cast<u32>(0.0f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const DownMix6chTo2chCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(10009.0f);
    case 240:
        return static_cast<u32>(14577.0f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(const AuxCommand& command) const {
    // Is this function bugged, returning the wrong time?
    // Surely the larger time should be returned when enabled...
    // CMP W8, #0
    // MOV W8, #0x60;  // 489.163f
    // MOV W10, #0x64; // 7177.936f
    // CSEL X8, X10, X8, EQ

    switch (sample_count) {
    case 160:
        if (command.enabled) {
            return static_cast<u32>(489.163f);
        }
        return static_cast<u32>(7177.936f);
    case 240:
        if (command.enabled) {
            return static_cast<u32>(485.562f);
        }
        return static_cast<u32>(9499.822f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(const DeviceSinkCommand& command) const {
    switch (command.input_count) {
    case 2:
        switch (sample_count) {
        case 160:
            return static_cast<u32>(9261.545f);
        case 240:
            return static_cast<u32>(9336.054f);
        default:
            LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
            return 0;
        }
    case 6:
        switch (sample_count) {
        case 160:
            return static_cast<u32>(9336.054f);
        case 240:
            return static_cast<u32>(9566.728f);
        default:
            LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid input count {}", command.input_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    const CircularBufferSinkCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(static_cast<f32>(command.input_count) * 853.629f + 1284.517f);
    case 240:
        return static_cast<u32>(static_cast<f32>(command.input_count) * 1726.021f + 1369.683f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(const ReverbCommand& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(97192.227f);
            case 2:
                return static_cast<u32>(103278.555f);
            case 4:
                return static_cast<u32>(109579.039f);
            case 6:
                return static_cast<u32>(115065.438f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(492.009f);
        case 2:
            return static_cast<u32>(554.463f);
        case 4:
            return static_cast<u32>(595.864f);
        case 6:
            return static_cast<u32>(656.617f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    case 240:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(136463.641f);
            case 2:
                return static_cast<u32>(145749.047f);
            case 4:
                return static_cast<u32>(154796.938f);
            case 6:
                return static_cast<u32>(161968.406f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(495.789f);
        case 2:
            return static_cast<u32>(527.163f);
        case 4:
            return static_cast<u32>(598.752f);
        case 6:
            return static_cast<u32>(666.025f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(const I3dl2ReverbCommand& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(138836.484f);
            case 2:
                return static_cast<u32>(135428.172f);
            case 4:
                return static_cast<u32>(199181.844f);
            case 6:
                return static_cast<u32>(247345.906f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(718.704f);
        case 2:
            return static_cast<u32>(751.296f);
        case 4:
            return static_cast<u32>(797.464f);
        case 6:
            return static_cast<u32>(867.426f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    case 240:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(199952.734f);
            case 2:
                return static_cast<u32>(195199.5f);
            case 4:
                return static_cast<u32>(290575.875f);
            case 6:
                return static_cast<u32>(363494.531f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(534.24f);
        case 2:
            return static_cast<u32>(570.874f);
        case 4:
            return static_cast<u32>(660.933f);
        case 6:
            return static_cast<u32>(694.596f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const PerformanceCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(489.35f);
    case 240:
        return static_cast<u32>(491.18f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const ClearMixBufferCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(static_cast<f32>(buffer_count) * 260.4f + 139.65f);
    case 240:
        return static_cast<u32>(static_cast<f32>(buffer_count) * 668.85f + 193.2f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const CopyMixBufferCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(836.32f);
    case 240:
        return static_cast<u32>(1000.9f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const LightLimiterVersion1Command& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const LightLimiterVersion2Command& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const MultiTapBiquadFilterCommand& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion2::Estimate(
    [[maybe_unused]] const CaptureCommand& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    const PcmInt16DataSourceVersion1Command& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(
            ((static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
             (command.pitch * 0.000030518f)) *
                427.52f +
            6329.442f);
    case 240:
        return static_cast<u32>(
            ((static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
             (command.pitch * 0.000030518f)) *
                710.143f +
            7853.286f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    const PcmInt16DataSourceVersion2Command& command) const {
    switch (sample_count) {
    case 160:
        switch (command.src_quality) {
        case SrcQuality::Medium:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        427.52f +
                                    6329.442f);
        case SrcQuality::High:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        371.876f +
                                    8049.415f);
        case SrcQuality::Low:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        423.43f +
                                    5062.659f);
        default:
            LOG_ERROR(Service_Audio, "Invalid SRC quality {}",
                      static_cast<u32>(command.src_quality));
            return 0;
        }

    case 240:
        switch (command.src_quality) {
        case SrcQuality::Medium:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        710.143f +
                                    7853.286f);
        case SrcQuality::High:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        610.487f +
                                    10138.842f);
        case SrcQuality::Low:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        676.722f +
                                    5810.962f);
        default:
            LOG_ERROR(Service_Audio, "Invalid SRC quality {}",
                      static_cast<u32>(command.src_quality));
            return 0;
        }

    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    const PcmFloatDataSourceVersion1Command& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(
            ((static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
             (command.pitch * 0.000030518f)) *
                1672.026f +
            7681.211f);
    case 240:
        return static_cast<u32>(
            ((static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
             (command.pitch * 0.000030518f)) *
                2550.414f +
            9663.969f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    const PcmFloatDataSourceVersion2Command& command) const {
    switch (sample_count) {
    case 160:
        switch (command.src_quality) {
        case SrcQuality::Medium:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        1672.026f +
                                    7681.211f);
        case SrcQuality::High:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        1672.982f +
                                    9038.011f);
        case SrcQuality::Low:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        1673.216f +
                                    6027.577f);
        default:
            LOG_ERROR(Service_Audio, "Invalid SRC quality {}",
                      static_cast<u32>(command.src_quality));
            return 0;
        }

    case 240:
        switch (command.src_quality) {
        case SrcQuality::Medium:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        2550.414f +
                                    9663.969f);
        case SrcQuality::High:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        2522.303f +
                                    11758.571f);
        case SrcQuality::Low:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        2537.061f +
                                    7369.309f);
        default:
            LOG_ERROR(Service_Audio, "Invalid SRC quality {}",
                      static_cast<u32>(command.src_quality));
            return 0;
        }

    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    const AdpcmDataSourceVersion1Command& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(
            ((static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
             (command.pitch * 0.000030518f)) *
                1827.665f +
            7913.808f);
    case 240:
        return static_cast<u32>(
            ((static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
             (command.pitch * 0.000030518f)) *
                2756.372f +
            9736.702f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    const AdpcmDataSourceVersion2Command& command) const {
    switch (sample_count) {
    case 160:
        switch (command.src_quality) {
        case SrcQuality::Medium:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        1827.665f +
                                    7913.808f);
        case SrcQuality::High:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        1829.285f +
                                    9607.814f);
        case SrcQuality::Low:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        1824.609f +
                                    6517.476f);
        default:
            LOG_ERROR(Service_Audio, "Invalid SRC quality {}",
                      static_cast<u32>(command.src_quality));
            return 0;
        }

    case 240:
        switch (command.src_quality) {
        case SrcQuality::Medium:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        2756.372f +
                                    9736.702f);
        case SrcQuality::High:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        2731.308f +
                                    12154.379f);
        case SrcQuality::Low:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        2732.152f +
                                    7929.442f);
        default:
            LOG_ERROR(Service_Audio, "Invalid SRC quality {}",
                      static_cast<u32>(command.src_quality));
            return 0;
        }

    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    [[maybe_unused]] const VolumeCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(1311.1f);
    case 240:
        return static_cast<u32>(1713.6f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    [[maybe_unused]] const VolumeRampCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(1425.3f);
    case 240:
        return static_cast<u32>(1700.0f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    [[maybe_unused]] const BiquadFilterCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(4173.2f);
    case 240:
        return static_cast<u32>(5585.1f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    [[maybe_unused]] const MixCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(1402.8f);
    case 240:
        return static_cast<u32>(1853.2f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    [[maybe_unused]] const MixRampCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(1968.7f);
    case 240:
        return static_cast<u32>(2459.4f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(const MixRampGroupedCommand& command) const {
    u32 count{0};
    for (u32 i = 0; i < command.buffer_count; i++) {
        if (command.volumes[i] != 0.0f || command.prev_volumes[i] != 0.0f) {
            count++;
        }
    }

    switch (sample_count) {
    case 160:
        return static_cast<u32>((static_cast<f32>(sample_count) * 6.708f) *
                                static_cast<f32>(count));
    case 240:
        return static_cast<u32>((static_cast<f32>(sample_count) * 6.443f) *
                                static_cast<f32>(count));
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    [[maybe_unused]] const DepopPrepareCommand& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    [[maybe_unused]] const DepopForMixBuffersCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(739.64f);
    case 240:
        return static_cast<u32>(910.97f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(const DelayCommand& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(8929.042f);
            case 2:
                return static_cast<u32>(25500.75f);
            case 4:
                return static_cast<u32>(47759.617f);
            case 6:
                return static_cast<u32>(82203.07f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(1295.206f);
        case 2:
            return static_cast<u32>(1213.6f);
        case 4:
            return static_cast<u32>(942.028f);
        case 6:
            return static_cast<u32>(1001.553f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    case 240:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(11941.051f);
            case 2:
                return static_cast<u32>(37197.371f);
            case 4:
                return static_cast<u32>(69749.836f);
            case 6:
                return static_cast<u32>(120042.398f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(997.668f);
        case 2:
            return static_cast<u32>(977.634f);
        case 4:
            return static_cast<u32>(792.309f);
        case 6:
            return static_cast<u32>(875.427f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    [[maybe_unused]] const UpsampleCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(312990.0f);
    case 240:
        return static_cast<u32>(0.0f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    [[maybe_unused]] const DownMix6chTo2chCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(9949.7f);
    case 240:
        return static_cast<u32>(14679.0f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(const AuxCommand& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            return static_cast<u32>(7182.136f);
        }
        return static_cast<u32>(472.111f);
    case 240:
        if (command.enabled) {
            return static_cast<u32>(9435.961f);
        }
        return static_cast<u32>(462.619f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(const DeviceSinkCommand& command) const {
    switch (command.input_count) {
    case 2:
        switch (sample_count) {
        case 160:
            return static_cast<u32>(8979.956f);
        case 240:
            return static_cast<u32>(9221.907f);
        default:
            LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
            return 0;
        }
    case 6:
        switch (sample_count) {
        case 160:
            return static_cast<u32>(9177.903f);
        case 240:
            return static_cast<u32>(9725.897f);
        default:
            LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid input count {}", command.input_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    const CircularBufferSinkCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(static_cast<f32>(command.input_count) * 531.069f + 0.0f);
    case 240:
        return static_cast<u32>(static_cast<f32>(command.input_count) * 770.257f + 0.0f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(const ReverbCommand& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(81475.055f);
            case 2:
                return static_cast<u32>(84975.0f);
            case 4:
                return static_cast<u32>(91625.148f);
            case 6:
                return static_cast<u32>(95332.266f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(536.298f);
        case 2:
            return static_cast<u32>(588.798f);
        case 4:
            return static_cast<u32>(643.702f);
        case 6:
            return static_cast<u32>(705.999f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    case 240:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(120174.469f);
            case 2:
                return static_cast<u32>(125262.219f);
            case 4:
                return static_cast<u32>(135751.234f);
            case 6:
                return static_cast<u32>(141129.234f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(617.641f);
        case 2:
            return static_cast<u32>(659.536f);
        case 4:
            return static_cast<u32>(711.438f);
        case 6:
            return static_cast<u32>(778.071f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(const I3dl2ReverbCommand& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(116754.984f);
            case 2:
                return static_cast<u32>(125912.055f);
            case 4:
                return static_cast<u32>(146336.031f);
            case 6:
                return static_cast<u32>(165812.656f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(735.0f);
        case 2:
            return static_cast<u32>(766.615f);
        case 4:
            return static_cast<u32>(834.067f);
        case 6:
            return static_cast<u32>(875.437f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    case 240:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(170292.344f);
            case 2:
                return static_cast<u32>(183875.625f);
            case 4:
                return static_cast<u32>(214696.188f);
            case 6:
                return static_cast<u32>(243846.766f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(508.473f);
        case 2:
            return static_cast<u32>(582.445f);
        case 4:
            return static_cast<u32>(626.419f);
        case 6:
            return static_cast<u32>(682.468f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    [[maybe_unused]] const PerformanceCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(498.17f);
    case 240:
        return static_cast<u32>(489.42f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    [[maybe_unused]] const ClearMixBufferCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(static_cast<f32>(buffer_count - 1) * 266.645f + 0.0f);
    case 240:
        return static_cast<u32>(static_cast<f32>(buffer_count - 1) * 440.681f + 0.0f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    [[maybe_unused]] const CopyMixBufferCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(842.59f);
    case 240:
        return static_cast<u32>(986.72f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    const LightLimiterVersion1Command& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(21392.383f);
            case 2:
                return static_cast<u32>(26829.389f);
            case 4:
                return static_cast<u32>(32405.152f);
            case 6:
                return static_cast<u32>(52218.586f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(897.004f);
        case 2:
            return static_cast<u32>(931.549f);
        case 4:
            return static_cast<u32>(975.387f);
        case 6:
            return static_cast<u32>(1016.778f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    case 240:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(30555.504f);
            case 2:
                return static_cast<u32>(39010.785f);
            case 4:
                return static_cast<u32>(48270.18f);
            case 6:
                return static_cast<u32>(76711.875f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(874.429f);
        case 2:
            return static_cast<u32>(921.553f);
        case 4:
            return static_cast<u32>(945.262f);
        case 6:
            return static_cast<u32>(992.26f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    const LightLimiterVersion2Command& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            if (command.parameter.statistics_enabled) {
                switch (command.parameter.channel_count) {
                case 1:
                    return static_cast<u32>(23308.928f);
                case 2:
                    return static_cast<u32>(29954.062f);
                case 4:
                    return static_cast<u32>(35807.477f);
                case 6:
                    return static_cast<u32>(58339.773f);
                default:
                    LOG_ERROR(Service_Audio, "Invalid channel count {}",
                              command.parameter.channel_count);
                    return 0;
                }
            }
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(21392.383f);
            case 2:
                return static_cast<u32>(26829.389f);
            case 4:
                return static_cast<u32>(32405.152f);
            case 6:
                return static_cast<u32>(52218.586f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(897.004f);
        case 2:
            return static_cast<u32>(931.549f);
        case 4:
            return static_cast<u32>(975.387f);
        case 6:
            return static_cast<u32>(1016.778f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    case 240:
        if (command.enabled) {
            if (command.parameter.statistics_enabled) {
                switch (command.parameter.channel_count) {
                case 1:
                    return static_cast<u32>(33526.121f);
                case 2:
                    return static_cast<u32>(43549.355f);
                case 4:
                    return static_cast<u32>(52190.281f);
                case 6:
                    return static_cast<u32>(85526.516f);
                default:
                    LOG_ERROR(Service_Audio, "Invalid channel count {}",
                              command.parameter.channel_count);
                    return 0;
                }
            }
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(30555.504f);
            case 2:
                return static_cast<u32>(39010.785f);
            case 4:
                return static_cast<u32>(48270.18f);
            case 6:
                return static_cast<u32>(76711.875f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(874.429f);
        case 2:
            return static_cast<u32>(921.553f);
        case 4:
            return static_cast<u32>(945.262f);
        case 6:
            return static_cast<u32>(992.26f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    [[maybe_unused]] const MultiTapBiquadFilterCommand& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion3::Estimate(
    [[maybe_unused]] const CaptureCommand& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    const PcmInt16DataSourceVersion1Command& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(
            ((static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
             (command.pitch * 0.000030518f)) *
                427.52f +
            6329.442f);
    case 240:
        return static_cast<u32>(
            ((static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
             (command.pitch * 0.000030518f)) *
                710.143f +
            7853.286f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    const PcmInt16DataSourceVersion2Command& command) const {
    switch (sample_count) {
    case 160:
        switch (command.src_quality) {
        case SrcQuality::Medium:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        427.52f +
                                    6329.442f);
        case SrcQuality::High:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        371.876f +
                                    8049.415f);
        case SrcQuality::Low:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        423.43f +
                                    5062.659f);
        default:
            LOG_ERROR(Service_Audio, "Invalid SRC quality {}",
                      static_cast<u32>(command.src_quality));
            return 0;
        }

    case 240:
        switch (command.src_quality) {
        case SrcQuality::Medium:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        710.143f +
                                    7853.286f);
        case SrcQuality::High:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        610.487f +
                                    10138.842f);
        case SrcQuality::Low:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        676.722f +
                                    5810.962f);
        default:
            LOG_ERROR(Service_Audio, "Invalid SRC quality {}",
                      static_cast<u32>(command.src_quality));
            return 0;
        }

    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    const PcmFloatDataSourceVersion1Command& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(
            ((static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
             (command.pitch * 0.000030518f)) *
                1672.026f +
            7681.211f);
    case 240:
        return static_cast<u32>(
            ((static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
             (command.pitch * 0.000030518f)) *
                2550.414f +
            9663.969f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    const PcmFloatDataSourceVersion2Command& command) const {
    switch (sample_count) {
    case 160:
        switch (command.src_quality) {
        case SrcQuality::Medium:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        1672.026f +
                                    7681.211f);
        case SrcQuality::High:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        1672.982f +
                                    9038.011f);
        case SrcQuality::Low:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        1673.216f +
                                    6027.577f);
        default:
            LOG_ERROR(Service_Audio, "Invalid SRC quality {}",
                      static_cast<u32>(command.src_quality));
            return 0;
        }

    case 240:
        switch (command.src_quality) {
        case SrcQuality::Medium:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        2550.414f +
                                    9663.969f);
        case SrcQuality::High:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        2522.303f +
                                    11758.571f);
        case SrcQuality::Low:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        2537.061f +
                                    7369.309f);
        default:
            LOG_ERROR(Service_Audio, "Invalid SRC quality {}",
                      static_cast<u32>(command.src_quality));
            return 0;
        }

    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    const AdpcmDataSourceVersion1Command& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(
            ((static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
             (command.pitch * 0.000030518f)) *
                1827.665f +
            7913.808f);
    case 240:
        return static_cast<u32>(
            ((static_cast<f32>(command.sample_rate) / 200.0f / static_cast<f32>(sample_count)) *
             (command.pitch * 0.000030518f)) *
                2756.372f +
            9736.702f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    const AdpcmDataSourceVersion2Command& command) const {
    switch (sample_count) {
    case 160:
        switch (command.src_quality) {
        case SrcQuality::Medium:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        1827.665f +
                                    7913.808f);
        case SrcQuality::High:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        1829.285f +
                                    9607.814f);
        case SrcQuality::Low:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        1824.609f +
                                    6517.476f);
        default:
            LOG_ERROR(Service_Audio, "Invalid SRC quality {}",
                      static_cast<u32>(command.src_quality));
            return 0;
        }

    case 240:
        switch (command.src_quality) {
        case SrcQuality::Medium:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        2756.372f +
                                    9736.702f);
        case SrcQuality::High:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        2731.308f +
                                    12154.379f);
        case SrcQuality::Low:
            return static_cast<u32>((((static_cast<f32>(command.sample_rate) / 200.0f /
                                       static_cast<f32>(sample_count)) *
                                      (command.pitch * 0.000030518f)) -
                                     1.0f) *
                                        2732.152f +
                                    7929.442f);
        default:
            LOG_ERROR(Service_Audio, "Invalid SRC quality {}",
                      static_cast<u32>(command.src_quality));
            return 0;
        }

    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    [[maybe_unused]] const VolumeCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(1311.1f);
    case 240:
        return static_cast<u32>(1713.6f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    [[maybe_unused]] const VolumeRampCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(1425.3f);
    case 240:
        return static_cast<u32>(1700.0f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    [[maybe_unused]] const BiquadFilterCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(4173.2f);
    case 240:
        return static_cast<u32>(5585.1f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    [[maybe_unused]] const MixCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(1402.8f);
    case 240:
        return static_cast<u32>(1853.2f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    [[maybe_unused]] const MixRampCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(1968.7f);
    case 240:
        return static_cast<u32>(2459.4f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(const MixRampGroupedCommand& command) const {
    u32 count{0};
    for (u32 i = 0; i < command.buffer_count; i++) {
        if (command.volumes[i] != 0.0f || command.prev_volumes[i] != 0.0f) {
            count++;
        }
    }

    switch (sample_count) {
    case 160:
        return static_cast<u32>((static_cast<f32>(sample_count) * 6.708f) *
                                static_cast<f32>(count));
    case 240:
        return static_cast<u32>((static_cast<f32>(sample_count) * 6.443f) *
                                static_cast<f32>(count));
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    [[maybe_unused]] const DepopPrepareCommand& command) const {
    return 0;
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    [[maybe_unused]] const DepopForMixBuffersCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(739.64f);
    case 240:
        return static_cast<u32>(910.97f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(const DelayCommand& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(8929.042f);
            case 2:
                return static_cast<u32>(25500.75f);
            case 4:
                return static_cast<u32>(47759.617f);
            case 6:
                return static_cast<u32>(82203.07f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(1295.206f);
        case 2:
            return static_cast<u32>(1213.6f);
        case 4:
            return static_cast<u32>(942.028f);
        case 6:
            return static_cast<u32>(1001.553f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    case 240:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(11941.051f);
            case 2:
                return static_cast<u32>(37197.371f);
            case 4:
                return static_cast<u32>(69749.836f);
            case 6:
                return static_cast<u32>(120042.398f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(997.668f);
        case 2:
            return static_cast<u32>(977.634f);
        case 4:
            return static_cast<u32>(792.309f);
        case 6:
            return static_cast<u32>(875.427f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    [[maybe_unused]] const UpsampleCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(312990.0f);
    case 240:
        return static_cast<u32>(0.0f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    [[maybe_unused]] const DownMix6chTo2chCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(9949.7f);
    case 240:
        return static_cast<u32>(14679.0f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(const AuxCommand& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            return static_cast<u32>(7182.136f);
        }
        return static_cast<u32>(472.111f);
    case 240:
        if (command.enabled) {
            return static_cast<u32>(9435.961f);
        }
        return static_cast<u32>(462.619f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(const DeviceSinkCommand& command) const {
    switch (command.input_count) {
    case 2:
        switch (sample_count) {
        case 160:
            return static_cast<u32>(8979.956f);
        case 240:
            return static_cast<u32>(9221.907f);
        default:
            LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
            return 0;
        }
    case 6:
        switch (sample_count) {
        case 160:
            return static_cast<u32>(9177.903f);
        case 240:
            return static_cast<u32>(9725.897f);
        default:
            LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid input count {}", command.input_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    const CircularBufferSinkCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(static_cast<f32>(command.input_count) * 531.069f + 0.0f);
    case 240:
        return static_cast<u32>(static_cast<f32>(command.input_count) * 770.257f + 0.0f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(const ReverbCommand& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(81475.055f);
            case 2:
                return static_cast<u32>(84975.0f);
            case 4:
                return static_cast<u32>(91625.148f);
            case 6:
                return static_cast<u32>(95332.266f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(536.298f);
        case 2:
            return static_cast<u32>(588.798f);
        case 4:
            return static_cast<u32>(643.702f);
        case 6:
            return static_cast<u32>(705.999f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    case 240:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(120174.469f);
            case 2:
                return static_cast<u32>(125262.219f);
            case 4:
                return static_cast<u32>(135751.234f);
            case 6:
                return static_cast<u32>(141129.234f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(617.641f);
        case 2:
            return static_cast<u32>(659.536f);
        case 4:
            return static_cast<u32>(711.438f);
        case 6:
            return static_cast<u32>(778.071f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(const I3dl2ReverbCommand& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(116754.984f);
            case 2:
                return static_cast<u32>(125912.055f);
            case 4:
                return static_cast<u32>(146336.031f);
            case 6:
                return static_cast<u32>(165812.656f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(735.0f);
        case 2:
            return static_cast<u32>(766.615f);
        case 4:
            return static_cast<u32>(834.067f);
        case 6:
            return static_cast<u32>(875.437f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    case 240:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(170292.344f);
            case 2:
                return static_cast<u32>(183875.625f);
            case 4:
                return static_cast<u32>(214696.188f);
            case 6:
                return static_cast<u32>(243846.766f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(508.473f);
        case 2:
            return static_cast<u32>(582.445f);
        case 4:
            return static_cast<u32>(626.419f);
        case 6:
            return static_cast<u32>(682.468f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    [[maybe_unused]] const PerformanceCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(498.17f);
    case 240:
        return static_cast<u32>(489.42f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    [[maybe_unused]] const ClearMixBufferCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(static_cast<f32>(buffer_count - 1) * 266.645f + 0.0f);
    case 240:
        return static_cast<u32>(static_cast<f32>(buffer_count - 1) * 440.681f + 0.0f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    [[maybe_unused]] const CopyMixBufferCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(842.59f);
    case 240:
        return static_cast<u32>(986.72f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    const LightLimiterVersion1Command& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(21392.383f);
            case 2:
                return static_cast<u32>(26829.389f);
            case 4:
                return static_cast<u32>(32405.152f);
            case 6:
                return static_cast<u32>(52218.586f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(897.004f);
        case 2:
            return static_cast<u32>(931.549f);
        case 4:
            return static_cast<u32>(975.387f);
        case 6:
            return static_cast<u32>(1016.778f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    case 240:
        if (command.enabled) {
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(30555.504f);
            case 2:
                return static_cast<u32>(39010.785f);
            case 4:
                return static_cast<u32>(48270.18f);
            case 6:
                return static_cast<u32>(76711.875f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(874.429f);
        case 2:
            return static_cast<u32>(921.553f);
        case 4:
            return static_cast<u32>(945.262f);
        case 6:
            return static_cast<u32>(992.26f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    const LightLimiterVersion2Command& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            if (command.parameter.statistics_enabled) {
                switch (command.parameter.channel_count) {
                case 1:
                    return static_cast<u32>(23308.928f);
                case 2:
                    return static_cast<u32>(29954.062f);
                case 4:
                    return static_cast<u32>(35807.477f);
                case 6:
                    return static_cast<u32>(58339.773f);
                default:
                    LOG_ERROR(Service_Audio, "Invalid channel count {}",
                              command.parameter.channel_count);
                    return 0;
                }
            }
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(21392.383f);
            case 2:
                return static_cast<u32>(26829.389f);
            case 4:
                return static_cast<u32>(32405.152f);
            case 6:
                return static_cast<u32>(52218.586f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(897.004f);
        case 2:
            return static_cast<u32>(931.549f);
        case 4:
            return static_cast<u32>(975.387f);
        case 6:
            return static_cast<u32>(1016.778f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    case 240:
        if (command.enabled) {
            if (command.parameter.statistics_enabled) {
                switch (command.parameter.channel_count) {
                case 1:
                    return static_cast<u32>(33526.121f);
                case 2:
                    return static_cast<u32>(43549.355f);
                case 4:
                    return static_cast<u32>(52190.281f);
                case 6:
                    return static_cast<u32>(85526.516f);
                default:
                    LOG_ERROR(Service_Audio, "Invalid channel count {}",
                              command.parameter.channel_count);
                    return 0;
                }
            }
            switch (command.parameter.channel_count) {
            case 1:
                return static_cast<u32>(30555.504f);
            case 2:
                return static_cast<u32>(39010.785f);
            case 4:
                return static_cast<u32>(48270.18f);
            case 6:
                return static_cast<u32>(76711.875f);
            default:
                LOG_ERROR(Service_Audio, "Invalid channel count {}",
                          command.parameter.channel_count);
                return 0;
            }
        }
        switch (command.parameter.channel_count) {
        case 1:
            return static_cast<u32>(874.429f);
        case 2:
            return static_cast<u32>(921.553f);
        case 4:
            return static_cast<u32>(945.262f);
        case 6:
            return static_cast<u32>(992.26f);
        default:
            LOG_ERROR(Service_Audio, "Invalid channel count {}", command.parameter.channel_count);
            return 0;
        }
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(
    [[maybe_unused]] const MultiTapBiquadFilterCommand& command) const {
    switch (sample_count) {
    case 160:
        return static_cast<u32>(7424.5f);
    case 240:
        return static_cast<u32>(9730.4f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

u32 CommandProcessingTimeEstimatorVersion4::Estimate(const CaptureCommand& command) const {
    switch (sample_count) {
    case 160:
        if (command.enabled) {
            return static_cast<u32>(426.982f);
        }
        return static_cast<u32>(4261.005f);
    case 240:
        if (command.enabled) {
            return static_cast<u32>(435.204f);
        }
        return static_cast<u32>(5858.265f);
    default:
        LOG_ERROR(Service_Audio, "Invalid sample count {}", sample_count);
        return 0;
    }
}

} // namespace AudioCore::AudioRenderer

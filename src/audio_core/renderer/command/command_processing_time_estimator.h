// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "audio_core/renderer/command/commands.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
/**
 * Estimate the processing time required for all commands.
 */
class ICommandProcessingTimeEstimator {
public:
    virtual ~ICommandProcessingTimeEstimator() = default;

    virtual u32 Estimate(const PcmInt16DataSourceVersion1Command& command) const = 0;
    virtual u32 Estimate(const PcmInt16DataSourceVersion2Command& command) const = 0;
    virtual u32 Estimate(const PcmFloatDataSourceVersion1Command& command) const = 0;
    virtual u32 Estimate(const PcmFloatDataSourceVersion2Command& command) const = 0;
    virtual u32 Estimate(const AdpcmDataSourceVersion1Command& command) const = 0;
    virtual u32 Estimate(const AdpcmDataSourceVersion2Command& command) const = 0;
    virtual u32 Estimate(const VolumeCommand& command) const = 0;
    virtual u32 Estimate(const VolumeRampCommand& command) const = 0;
    virtual u32 Estimate(const BiquadFilterCommand& command) const = 0;
    virtual u32 Estimate(const MixCommand& command) const = 0;
    virtual u32 Estimate(const MixRampCommand& command) const = 0;
    virtual u32 Estimate(const MixRampGroupedCommand& command) const = 0;
    virtual u32 Estimate(const DepopPrepareCommand& command) const = 0;
    virtual u32 Estimate(const DepopForMixBuffersCommand& command) const = 0;
    virtual u32 Estimate(const DelayCommand& command) const = 0;
    virtual u32 Estimate(const UpsampleCommand& command) const = 0;
    virtual u32 Estimate(const DownMix6chTo2chCommand& command) const = 0;
    virtual u32 Estimate(const AuxCommand& command) const = 0;
    virtual u32 Estimate(const DeviceSinkCommand& command) const = 0;
    virtual u32 Estimate(const CircularBufferSinkCommand& command) const = 0;
    virtual u32 Estimate(const ReverbCommand& command) const = 0;
    virtual u32 Estimate(const I3dl2ReverbCommand& command) const = 0;
    virtual u32 Estimate(const PerformanceCommand& command) const = 0;
    virtual u32 Estimate(const ClearMixBufferCommand& command) const = 0;
    virtual u32 Estimate(const CopyMixBufferCommand& command) const = 0;
    virtual u32 Estimate(const LightLimiterVersion1Command& command) const = 0;
    virtual u32 Estimate(const LightLimiterVersion2Command& command) const = 0;
    virtual u32 Estimate(const MultiTapBiquadFilterCommand& command) const = 0;
    virtual u32 Estimate(const CaptureCommand& command) const = 0;
};

class CommandProcessingTimeEstimatorVersion1 final : public ICommandProcessingTimeEstimator {
public:
    CommandProcessingTimeEstimatorVersion1(u32 sample_count_, u32 buffer_count_)
        : sample_count{sample_count_}, buffer_count{buffer_count_} {}

    u32 Estimate(const PcmInt16DataSourceVersion1Command& command) const override;
    u32 Estimate(const PcmInt16DataSourceVersion2Command& command) const override;
    u32 Estimate(const PcmFloatDataSourceVersion1Command& command) const override;
    u32 Estimate(const PcmFloatDataSourceVersion2Command& command) const override;
    u32 Estimate(const AdpcmDataSourceVersion1Command& command) const override;
    u32 Estimate(const AdpcmDataSourceVersion2Command& command) const override;
    u32 Estimate(const VolumeCommand& command) const override;
    u32 Estimate(const VolumeRampCommand& command) const override;
    u32 Estimate(const BiquadFilterCommand& command) const override;
    u32 Estimate(const MixCommand& command) const override;
    u32 Estimate(const MixRampCommand& command) const override;
    u32 Estimate(const MixRampGroupedCommand& command) const override;
    u32 Estimate(const DepopPrepareCommand& command) const override;
    u32 Estimate(const DepopForMixBuffersCommand& command) const override;
    u32 Estimate(const DelayCommand& command) const override;
    u32 Estimate(const UpsampleCommand& command) const override;
    u32 Estimate(const DownMix6chTo2chCommand& command) const override;
    u32 Estimate(const AuxCommand& command) const override;
    u32 Estimate(const DeviceSinkCommand& command) const override;
    u32 Estimate(const CircularBufferSinkCommand& command) const override;
    u32 Estimate(const ReverbCommand& command) const override;
    u32 Estimate(const I3dl2ReverbCommand& command) const override;
    u32 Estimate(const PerformanceCommand& command) const override;
    u32 Estimate(const ClearMixBufferCommand& command) const override;
    u32 Estimate(const CopyMixBufferCommand& command) const override;
    u32 Estimate(const LightLimiterVersion1Command& command) const override;
    u32 Estimate(const LightLimiterVersion2Command& command) const override;
    u32 Estimate(const MultiTapBiquadFilterCommand& command) const override;
    u32 Estimate(const CaptureCommand& command) const override;

private:
    u32 sample_count{};
    u32 buffer_count{};
};

class CommandProcessingTimeEstimatorVersion2 final : public ICommandProcessingTimeEstimator {
public:
    CommandProcessingTimeEstimatorVersion2(u32 sample_count_, u32 buffer_count_)
        : sample_count{sample_count_}, buffer_count{buffer_count_} {}

    u32 Estimate(const PcmInt16DataSourceVersion1Command& command) const override;
    u32 Estimate(const PcmInt16DataSourceVersion2Command& command) const override;
    u32 Estimate(const PcmFloatDataSourceVersion1Command& command) const override;
    u32 Estimate(const PcmFloatDataSourceVersion2Command& command) const override;
    u32 Estimate(const AdpcmDataSourceVersion1Command& command) const override;
    u32 Estimate(const AdpcmDataSourceVersion2Command& command) const override;
    u32 Estimate(const VolumeCommand& command) const override;
    u32 Estimate(const VolumeRampCommand& command) const override;
    u32 Estimate(const BiquadFilterCommand& command) const override;
    u32 Estimate(const MixCommand& command) const override;
    u32 Estimate(const MixRampCommand& command) const override;
    u32 Estimate(const MixRampGroupedCommand& command) const override;
    u32 Estimate(const DepopPrepareCommand& command) const override;
    u32 Estimate(const DepopForMixBuffersCommand& command) const override;
    u32 Estimate(const DelayCommand& command) const override;
    u32 Estimate(const UpsampleCommand& command) const override;
    u32 Estimate(const DownMix6chTo2chCommand& command) const override;
    u32 Estimate(const AuxCommand& command) const override;
    u32 Estimate(const DeviceSinkCommand& command) const override;
    u32 Estimate(const CircularBufferSinkCommand& command) const override;
    u32 Estimate(const ReverbCommand& command) const override;
    u32 Estimate(const I3dl2ReverbCommand& command) const override;
    u32 Estimate(const PerformanceCommand& command) const override;
    u32 Estimate(const ClearMixBufferCommand& command) const override;
    u32 Estimate(const CopyMixBufferCommand& command) const override;
    u32 Estimate(const LightLimiterVersion1Command& command) const override;
    u32 Estimate(const LightLimiterVersion2Command& command) const override;
    u32 Estimate(const MultiTapBiquadFilterCommand& command) const override;
    u32 Estimate(const CaptureCommand& command) const override;

private:
    u32 sample_count{};
    u32 buffer_count{};
};

class CommandProcessingTimeEstimatorVersion3 final : public ICommandProcessingTimeEstimator {
public:
    CommandProcessingTimeEstimatorVersion3(u32 sample_count_, u32 buffer_count_)
        : sample_count{sample_count_}, buffer_count{buffer_count_} {}

    u32 Estimate(const PcmInt16DataSourceVersion1Command& command) const override;
    u32 Estimate(const PcmInt16DataSourceVersion2Command& command) const override;
    u32 Estimate(const PcmFloatDataSourceVersion1Command& command) const override;
    u32 Estimate(const PcmFloatDataSourceVersion2Command& command) const override;
    u32 Estimate(const AdpcmDataSourceVersion1Command& command) const override;
    u32 Estimate(const AdpcmDataSourceVersion2Command& command) const override;
    u32 Estimate(const VolumeCommand& command) const override;
    u32 Estimate(const VolumeRampCommand& command) const override;
    u32 Estimate(const BiquadFilterCommand& command) const override;
    u32 Estimate(const MixCommand& command) const override;
    u32 Estimate(const MixRampCommand& command) const override;
    u32 Estimate(const MixRampGroupedCommand& command) const override;
    u32 Estimate(const DepopPrepareCommand& command) const override;
    u32 Estimate(const DepopForMixBuffersCommand& command) const override;
    u32 Estimate(const DelayCommand& command) const override;
    u32 Estimate(const UpsampleCommand& command) const override;
    u32 Estimate(const DownMix6chTo2chCommand& command) const override;
    u32 Estimate(const AuxCommand& command) const override;
    u32 Estimate(const DeviceSinkCommand& command) const override;
    u32 Estimate(const CircularBufferSinkCommand& command) const override;
    u32 Estimate(const ReverbCommand& command) const override;
    u32 Estimate(const I3dl2ReverbCommand& command) const override;
    u32 Estimate(const PerformanceCommand& command) const override;
    u32 Estimate(const ClearMixBufferCommand& command) const override;
    u32 Estimate(const CopyMixBufferCommand& command) const override;
    u32 Estimate(const LightLimiterVersion1Command& command) const override;
    u32 Estimate(const LightLimiterVersion2Command& command) const override;
    u32 Estimate(const MultiTapBiquadFilterCommand& command) const override;
    u32 Estimate(const CaptureCommand& command) const override;

private:
    u32 sample_count{};
    u32 buffer_count{};
};

class CommandProcessingTimeEstimatorVersion4 final : public ICommandProcessingTimeEstimator {
public:
    CommandProcessingTimeEstimatorVersion4(u32 sample_count_, u32 buffer_count_)
        : sample_count{sample_count_}, buffer_count{buffer_count_} {}

    u32 Estimate(const PcmInt16DataSourceVersion1Command& command) const override;
    u32 Estimate(const PcmInt16DataSourceVersion2Command& command) const override;
    u32 Estimate(const PcmFloatDataSourceVersion1Command& command) const override;
    u32 Estimate(const PcmFloatDataSourceVersion2Command& command) const override;
    u32 Estimate(const AdpcmDataSourceVersion1Command& command) const override;
    u32 Estimate(const AdpcmDataSourceVersion2Command& command) const override;
    u32 Estimate(const VolumeCommand& command) const override;
    u32 Estimate(const VolumeRampCommand& command) const override;
    u32 Estimate(const BiquadFilterCommand& command) const override;
    u32 Estimate(const MixCommand& command) const override;
    u32 Estimate(const MixRampCommand& command) const override;
    u32 Estimate(const MixRampGroupedCommand& command) const override;
    u32 Estimate(const DepopPrepareCommand& command) const override;
    u32 Estimate(const DepopForMixBuffersCommand& command) const override;
    u32 Estimate(const DelayCommand& command) const override;
    u32 Estimate(const UpsampleCommand& command) const override;
    u32 Estimate(const DownMix6chTo2chCommand& command) const override;
    u32 Estimate(const AuxCommand& command) const override;
    u32 Estimate(const DeviceSinkCommand& command) const override;
    u32 Estimate(const CircularBufferSinkCommand& command) const override;
    u32 Estimate(const ReverbCommand& command) const override;
    u32 Estimate(const I3dl2ReverbCommand& command) const override;
    u32 Estimate(const PerformanceCommand& command) const override;
    u32 Estimate(const ClearMixBufferCommand& command) const override;
    u32 Estimate(const CopyMixBufferCommand& command) const override;
    u32 Estimate(const LightLimiterVersion1Command& command) const override;
    u32 Estimate(const LightLimiterVersion2Command& command) const override;
    u32 Estimate(const MultiTapBiquadFilterCommand& command) const override;
    u32 Estimate(const CaptureCommand& command) const override;

private:
    u32 sample_count{};
    u32 buffer_count{};
};

} // namespace AudioCore::AudioRenderer

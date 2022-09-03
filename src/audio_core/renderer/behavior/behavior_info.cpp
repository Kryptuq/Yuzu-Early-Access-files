// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/common/feature_support.h"
#include "audio_core/renderer/behavior/behavior_info.h"

namespace AudioCore::AudioRenderer {

BehaviorInfo::BehaviorInfo() : process_revision{CurrentRevision} {}

u32 BehaviorInfo::GetProcessRevisionNum() const {
    return process_revision;
}

u32 BehaviorInfo::GetProcessRevision() const {
    return Common::MakeMagic('R', 'E', 'V',
                             static_cast<char>(static_cast<u8>('0') + process_revision));
}

u32 BehaviorInfo::GetUserRevisionNum() const {
    return user_revision;
}

u32 BehaviorInfo::GetUserRevision() const {
    return Common::MakeMagic('R', 'E', 'V',
                             static_cast<char>(static_cast<u8>('0') + user_revision));
}

void BehaviorInfo::SetUserLibRevision(const u32 user_revision_) {
    user_revision = GetRevisionNum(user_revision_);
}

void BehaviorInfo::ClearError() {
    error_count = 0;
}

void BehaviorInfo::AppendError(ErrorInfo& error) {
    LOG_ERROR(Service_Audio, "Error during RequestUpdate, reporting code {:04X} address {:08X}",
              error.error_code.raw, error.address);
    if (error_count < MaxErrors) {
        errors[error_count++] = error;
    }
}

void BehaviorInfo::CopyErrorInfo(std::span<ErrorInfo> out_errors, u32& out_count) {
    out_count = std::min(error_count, MaxErrors);

    for (size_t i = 0; i < MaxErrors; i++) {
        if (i < out_count) {
            out_errors[i] = errors[i];
        } else {
            out_errors[i] = {};
        }
    }
}

void BehaviorInfo::UpdateFlags(const Flags flags_) {
    flags = flags_;
}

bool BehaviorInfo::IsMemoryForceMappingEnabled() const {
    return flags.IsMemoryForceMappingEnabled;
}

bool BehaviorInfo::IsAdpcmLoopContextBugFixed() const {
    return CheckFeatureSupported(SupportTags::AdpcmLoopContextBugFix, user_revision);
}

bool BehaviorInfo::IsSplitterSupported() const {
    return CheckFeatureSupported(SupportTags::Splitter, user_revision);
}

bool BehaviorInfo::IsSplitterBugFixed() const {
    return CheckFeatureSupported(SupportTags::SplitterBugFix, user_revision);
}

bool BehaviorInfo::IsEffectInfoVersion2Supported() const {
    return CheckFeatureSupported(SupportTags::EffectInfoVer2, user_revision);
}

bool BehaviorInfo::IsVariadicCommandBufferSizeSupported() const {
    return CheckFeatureSupported(SupportTags::AudioRendererVariadicCommandBufferSize,
                                 user_revision);
}

bool BehaviorInfo::IsWaveBufferVer2Supported() const {
    return CheckFeatureSupported(SupportTags::WaveBufferVer2, user_revision);
}

bool BehaviorInfo::IsLongSizePreDelaySupported() const {
    return CheckFeatureSupported(SupportTags::LongSizePreDelay, user_revision);
}

bool BehaviorInfo::IsCommandProcessingTimeEstimatorVersion2Supported() const {
    return CheckFeatureSupported(SupportTags::CommandProcessingTimeEstimatorVersion2,
                                 user_revision);
}

bool BehaviorInfo::IsCommandProcessingTimeEstimatorVersion3Supported() const {
    return CheckFeatureSupported(SupportTags::CommandProcessingTimeEstimatorVersion3,
                                 user_revision);
}

bool BehaviorInfo::IsCommandProcessingTimeEstimatorVersion4Supported() const {
    return CheckFeatureSupported(SupportTags::CommandProcessingTimeEstimatorVersion4,
                                 user_revision);
}

bool BehaviorInfo::IsCommandProcessingTimeEstimatorVersion5Supported() const {
    return CheckFeatureSupported(SupportTags::CommandProcessingTimeEstimatorVersion4,
                                 user_revision);
}

bool BehaviorInfo::IsAudioRendererProcessingTimeLimit70PercentSupported() const {
    return CheckFeatureSupported(SupportTags::AudioRendererProcessingTimeLimit70Percent,
                                 user_revision);
}

bool BehaviorInfo::IsAudioRendererProcessingTimeLimit75PercentSupported() const {
    return CheckFeatureSupported(SupportTags::AudioRendererProcessingTimeLimit75Percent,
                                 user_revision);
}

bool BehaviorInfo::IsAudioRendererProcessingTimeLimit80PercentSupported() const {
    return CheckFeatureSupported(SupportTags::AudioRendererProcessingTimeLimit80Percent,
                                 user_revision);
}

bool BehaviorInfo::IsFlushVoiceWaveBuffersSupported() const {
    return CheckFeatureSupported(SupportTags::FlushVoiceWaveBuffers, user_revision);
}

bool BehaviorInfo::IsElapsedFrameCountSupported() const {
    return CheckFeatureSupported(SupportTags::ElapsedFrameCount, user_revision);
}

bool BehaviorInfo::IsPerformanceMetricsDataFormatVersion2Supported() const {
    return CheckFeatureSupported(SupportTags::PerformanceMetricsDataFormatVersion2, user_revision);
}

size_t BehaviorInfo::GetPerformanceMetricsDataFormat() const {
    if (CheckFeatureSupported(SupportTags::PerformanceMetricsDataFormatVersion2, user_revision)) {
        return 2;
    }
    return 1;
}

bool BehaviorInfo::IsVoicePitchAndSrcSkippedSupported() const {
    return CheckFeatureSupported(SupportTags::VoicePitchAndSrcSkipped, user_revision);
}

bool BehaviorInfo::IsVoicePlayedSampleCountResetAtLoopPointSupported() const {
    return CheckFeatureSupported(SupportTags::VoicePlayedSampleCountResetAtLoopPoint,
                                 user_revision);
}

bool BehaviorInfo::IsBiquadFilterEffectStateClearBugFixed() const {
    return CheckFeatureSupported(SupportTags::BiquadFilterEffectStateClearBugFix, user_revision);
}

bool BehaviorInfo::IsVolumeMixParameterPrecisionQ23Supported() const {
    return CheckFeatureSupported(SupportTags::VolumeMixParameterPrecisionQ23, user_revision);
}

bool BehaviorInfo::UseBiquadFilterFloatProcessing() const {
    return CheckFeatureSupported(SupportTags::BiquadFilterFloatProcessing, user_revision);
}

bool BehaviorInfo::IsMixInParameterDirtyOnlyUpdateSupported() const {
    return CheckFeatureSupported(SupportTags::MixInParameterDirtyOnlyUpdate, user_revision);
}

bool BehaviorInfo::UseMultiTapBiquadFilterProcessing() const {
    return CheckFeatureSupported(SupportTags::MultiTapBiquadFilterProcessing, user_revision);
}

bool BehaviorInfo::IsDeviceApiVersion2Supported() const {
    return CheckFeatureSupported(SupportTags::DeviceApiVersion2, user_revision);
}

bool BehaviorInfo::IsDelayChannelMappingChanged() const {
    return CheckFeatureSupported(SupportTags::DelayChannelMappingChange, user_revision);
}

bool BehaviorInfo::IsReverbChannelMappingChanged() const {
    return CheckFeatureSupported(SupportTags::ReverbChannelMappingChange, user_revision);
}

bool BehaviorInfo::IsI3dl2ReverbChannelMappingChanged() const {
    return CheckFeatureSupported(SupportTags::I3dl2ReverbChannelMappingChange, user_revision);
}

} // namespace AudioCore::AudioRenderer

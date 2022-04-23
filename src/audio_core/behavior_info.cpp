// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include "audio_core/behavior_info.h"
#include "audio_core/common.h"
#include "common/logging/log.h"

namespace AudioCore {

BehaviorInfo::BehaviorInfo() : process_revision(AudioCommon::CURRENT_PROCESS_REVISION) {}
BehaviorInfo::~BehaviorInfo() = default;

bool BehaviorInfo::UpdateOutput(std::vector<u8>& buffer, std::size_t offset) {
    if (!AudioCommon::CanConsumeBuffer(buffer.size(), offset, sizeof(OutParams))) {
        LOG_ERROR(Audio, "Buffer is an invalid size!");
        return false;
    }

    OutParams params{};
    std::memcpy(params.errors.data(), errors.data(), sizeof(ErrorInfo) * errors.size());
    params.error_count = static_cast<u32_le>(error_count);
    std::memcpy(buffer.data() + offset, &params, sizeof(OutParams));
    return true;
}

void BehaviorInfo::ClearError() {
    error_count = 0;
}

void BehaviorInfo::UpdateFlags(u64_le dest_flags) {
    flags = dest_flags;
}

void BehaviorInfo::SetUserRevision(u32_le revision) {
    user_revision = revision;
}

u32_le BehaviorInfo::GetUserRevision() const {
    return user_revision;
}

u32_le BehaviorInfo::GetProcessRevision() const {
    return process_revision;
}

bool BehaviorInfo::IsAdpcmLoopContextBugFixed() const {
    return AudioCommon::IsRevisionSupported(2, user_revision);
}

bool BehaviorInfo::IsSplitterSupported() const {
    return AudioCommon::IsRevisionSupported(2, user_revision);
}

bool BehaviorInfo::IsLongSizePreDelaySupported() const {
    return AudioCommon::IsRevisionSupported(3, user_revision);
}

bool BehaviorInfo::IsAudioRendererProcessingTimeLimit80PercentSupported() const {
    return AudioCommon::IsRevisionSupported(5, user_revision);
}

bool BehaviorInfo::IsAudioRendererProcessingTimeLimit75PercentSupported() const {
    return AudioCommon::IsRevisionSupported(4, user_revision);
}

bool BehaviorInfo::IsAudioRendererProcessingTimeLimit70PercentSupported() const {
    return AudioCommon::IsRevisionSupported(1, user_revision);
}

bool BehaviorInfo::IsElapsedFrameCountSupported() const {
    return AudioCommon::IsRevisionSupported(5, user_revision);
}

bool BehaviorInfo::IsMemoryPoolForceMappingEnabled() const {
    return (flags & 1) != 0;
}

bool BehaviorInfo::IsFlushVoiceWaveBuffersSupported() const {
    return AudioCommon::IsRevisionSupported(5, user_revision);
}

bool BehaviorInfo::IsVoicePlayedSampleCountResetAtLoopPointSupported() const {
    return AudioCommon::IsRevisionSupported(5, user_revision);
}

bool BehaviorInfo::IsVoicePitchAndSrcSkippedSupported() const {
    return AudioCommon::IsRevisionSupported(5, user_revision);
}

bool BehaviorInfo::IsMixInParameterDirtyOnlyUpdateSupported() const {
    return AudioCommon::IsRevisionSupported(7, user_revision);
}

bool BehaviorInfo::IsSplitterBugFixed() const {
    return AudioCommon::IsRevisionSupported(5, user_revision);
}

void BehaviorInfo::CopyErrorInfo(BehaviorInfo::OutParams& dst) {
    dst.error_count = static_cast<u32>(error_count);
    std::copy(errors.begin(), errors.begin() + error_count, dst.errors.begin());
}

} // namespace AudioCore

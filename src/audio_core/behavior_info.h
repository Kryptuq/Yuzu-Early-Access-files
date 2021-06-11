// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>

#include <vector>
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"

namespace AudioCore {
class BehaviorInfo {
public:
    struct ErrorInfo {
        u32_le result{};
        INSERT_PADDING_WORDS(1);
        u64_le result_info{};
    };
    static_assert(sizeof(ErrorInfo) == 0x10, "ErrorInfo is an invalid size");

    struct InParams {
        u32_le revision{};
        u32_le padding{};
        u64_le flags{};
    };
    static_assert(sizeof(InParams) == 0x10, "InParams is an invalid size");

    struct OutParams {
        std::array<ErrorInfo, 10> errors{};
        u32_le error_count{};
        INSERT_PADDING_BYTES(12);
    };
    static_assert(sizeof(OutParams) == 0xb0, "OutParams is an invalid size");

    explicit BehaviorInfo();
    ~BehaviorInfo();

    bool UpdateOutput(std::vector<u8>& buffer, std::size_t offset);

    void ClearError();
    void UpdateFlags(u64_le dest_flags);
    void SetUserRevision(u32_le revision);
    [[nodiscard]] u32_le GetUserRevision() const;
    [[nodiscard]] u32_le GetProcessRevision() const;

    [[nodiscard]] bool IsAdpcmLoopContextBugFixed() const;
    [[nodiscard]] bool IsSplitterSupported() const;
    [[nodiscard]] bool IsLongSizePreDelaySupported() const;
    [[nodiscard]] bool IsAudioRendererProcessingTimeLimit80PercentSupported() const;
    [[nodiscard]] bool IsAudioRendererProcessingTimeLimit75PercentSupported() const;
    [[nodiscard]] bool IsAudioRendererProcessingTimeLimit70PercentSupported() const;
    [[nodiscard]] bool IsElapsedFrameCountSupported() const;
    [[nodiscard]] bool IsMemoryPoolForceMappingEnabled() const;
    [[nodiscard]] bool IsFlushVoiceWaveBuffersSupported() const;
    [[nodiscard]] bool IsVoicePlayedSampleCountResetAtLoopPointSupported() const;
    [[nodiscard]] bool IsVoicePitchAndSrcSkippedSupported() const;
    [[nodiscard]] bool IsMixInParameterDirtyOnlyUpdateSupported() const;
    [[nodiscard]] bool IsSplitterBugFixed() const;
    void CopyErrorInfo(OutParams& dst);

private:
    u32_le process_revision{};
    u32_le user_revision{};
    u64_le flags{};
    std::array<ErrorInfo, 10> errors{};
    std::size_t error_count{};
};

} // namespace AudioCore

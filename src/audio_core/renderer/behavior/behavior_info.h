// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <span>

#include "audio_core/common/common.h"
#include "common/common_types.h"
#include "core/hle/service/audio/errors.h"

namespace AudioCore::AudioRenderer {
/**
 * Holds host and user revisions, checks whether render features can be enabled, and reports errors.
 */
class BehaviorInfo {
    static constexpr u32 MaxErrors = 10;

public:
    struct ErrorInfo {
        /* 0x00 */ Result error_code{0};
        /* 0x04 */ u32 unk_04;
        /* 0x08 */ CpuAddr address;
    };
    static_assert(sizeof(ErrorInfo) == 0x10, "BehaviorInfo::ErrorInfo has the wrong size!");

    struct Flags {
        u64 IsMemoryForceMappingEnabled : 1;
    };

    struct InParameter {
        /* 0x00 */ u32 revision;
        /* 0x08 */ Flags flags;
    };
    static_assert(sizeof(InParameter) == 0x10, "BehaviorInfo::InParameter has the wrong size!");

    struct OutStatus {
        /* 0x00 */ std::array<ErrorInfo, MaxErrors> errors;
        /* 0xA0 */ u32 error_count;
        /* 0xA4 */ char unkA4[0xC];
    };
    static_assert(sizeof(OutStatus) == 0xB0, "BehaviorInfo::OutStatus has the wrong size!");

    BehaviorInfo();

    /**
     * Get the host revision as a number.
     *
     * @return The host revision.
     */
    u32 GetProcessRevisionNum() const;

    /**
     * Get the host revision in chars, e.g REV8.
     * Rev 10 and higher use the ascii characters above 9.
     * E.g:
     *     Rev 10 = REV:
     *     Rev 11 = REV;
     *
     * @return The host revision.
     */
    u32 GetProcessRevision() const;

    /**
     * Get the user revision as a number.
     *
     * @return The user revision.
     */
    u32 GetUserRevisionNum() const;

    /**
     * Get the user revision in chars, e.g REV8.
     * Rev 10 and higher use the ascii characters above 9. REV: REV; etc.
     *
     * @return The user revision.
     */
    u32 GetUserRevision() const;

    /**
     * Set the user revision.
     *
     * @param user_revision - The user's revision.
     */
    void SetUserLibRevision(u32 user_revision);

    /**
     * Clear the current error count.
     */
    void ClearError();

    /**
     * Append an error to the error list.
     *
     * @param error - The new error.
     */
    void AppendError(ErrorInfo& error);

    /**
     * Copy errors to the given output container.
     *
     * @param out_errors - Output container to receive the errors.
     * @param out_count  - The number of errors written.
     */
    void CopyErrorInfo(std::span<ErrorInfo> out_errors, u32& out_count);

    /**
     * Update the behaviour flags.
     *
     * @param flags - New flags to use.
     */
    void UpdateFlags(Flags flags);

    /**
     * Check if memory pools can be forcibly mapped.
     *
     * @return True if enabled, otherwise false.
     */
    bool IsMemoryForceMappingEnabled() const;

    /**
     * Check if the ADPCM context bug is fixed.
     * The ADPCM context was not being sent to the AudioRenderer, leading to incorrect scaling being
     * used.
     *
     * @return True if fixed, otherwise false.
     */
    bool IsAdpcmLoopContextBugFixed() const;

    /**
     * Check if the splitter is supported.
     *
     * @return True if supported, otherwise false.
     */
    bool IsSplitterSupported() const;

    /**
     * Check if the splitter bug is fixed.
     * Update is given the wrong number of splitter destinations, leading to invalid data
     * being processed.
     *
     * @return True if supported, otherwise false.
     */
    bool IsSplitterBugFixed() const;

    /**
     * Check if effects version 2 are supported.
     * This gives support for returning effect states from the AudioRenderer, currently only used
     * for Limiter statistics.
     *
     * @return True if supported, otherwise false.
     */
    bool IsEffectInfoVersion2Supported() const;

    /**
     * Check if a variadic command buffer is supported.
     * As of Rev 5 with the added optional performance metric logging, the command
     * buffer can be a variable size, so take that into account for calcualting its size.
     *
     * @return True if supported, otherwise false.
     */
    bool IsVariadicCommandBufferSizeSupported() const;

    /**
     * Check if wave buffers version 2 are supported.
     * See WaveBufferVersion1 and WaveBufferVersion2.
     *
     * @return True if supported, otherwise false.
     */
    bool IsWaveBufferVer2Supported() const;

    /**
     * Check if long size pre delay is supported.
     * This allows a longer initial delay time for the Reverb command.
     *
     * @return True if supported, otherwise false.
     */
    bool IsLongSizePreDelaySupported() const;

    /**
     * Check if the command time estimator version 2 is supported.
     *
     * @return True if supported, otherwise false.
     */
    bool IsCommandProcessingTimeEstimatorVersion2Supported() const;

    /**
     * Check if the command time estimator version 3 is supported.
     *
     * @return True if supported, otherwise false.
     */
    bool IsCommandProcessingTimeEstimatorVersion3Supported() const;

    /**
     * Check if the command time estimator version 4 is supported.
     *
     * @return True if supported, otherwise false.
     */
    bool IsCommandProcessingTimeEstimatorVersion4Supported() const;

    /**
     * Check if the AudioRenderer can use up to 70% of the allocated processing timeslice.
     * Note: Name is correct, Nintendo have the typo here
     *
     * @return True if supported, otherwise false.
     */
    bool IsAudioRenererProcessingTimeLimit70PercentSupported() const;

    /**
     * Check if the AudioRenderer can use up to 75% of the allocated processing timeslice.
     * Note: Name is correct, Nintendo have the typo here
     *
     * @return True if supported, otherwise false.
     */
    bool IsAudioRenererProcessingTimeLimit75PercentSupported() const;

    /**
     * Check if the AudioRenderer can use up to 80% of the allocated processing timeslice.
     * Note: Name is correct, Nintendo have the typo here
     *
     * @return True if supported, otherwise false.
     */
    bool IsAudioRenererProcessingTimeLimit80PercentSupported() const;

    /**
     * Check if voice flushing is supported
     * This allowws low-priority voices to be dropped if the AudioRenderer is running behind.
     *
     * @return True if supported, otherwise false.
     */
    bool IsFlushVoiceWaveBuffersSupported() const;

    /**
     * Check if counting the number of elapsed frames is supported.
     * This adds extra output to RequestUpdate, returning the number of times the AudioRenderer
     * processed a command list.
     *
     * @return True if supported, otherwise false.
     */
    bool IsElapsedFrameCountSupported() const;

    /**
     * Check if performance metrics version 2 are supported.
     * This adds extra output to RequestUpdate, returning the number of times the AudioRenderer
     * (Unused?).
     *
     * @return True if supported, otherwise false.
     */
    bool IsPerformanceMetricsDataFormatVersion2Supported() const;

    /**
     * Get the supported performance metrics version.
     * Version 2 logs some extra fields in output, such as number of voices dropped,
     * processing start time, if the AudioRenderer exceeded its time, etc.
     *
     * @return Version supported, either 1 or 2.
     */
    size_t GetPerformanceMetricsDataFormat() const;

    /**
     * Check if skipping voice pitch and sample rate conversion is supported.
     * This speeds up the data source commands by skipping resampling if unwanted.
     * See AudioCore::AudioRenderer::DecodeFromWaveBuffers
     *
     * @return True if supported, otherwise false.
     */
    bool IsVoicePitchAndSrcSkippedSupported() const;

    /**
     * Check if resetting played sample count at loop points is supported.
     * This resets the number of samples played in a voice state when a loop point is reached.
     * See AudioCore::AudioRenderer::DecodeFromWaveBuffers
     *
     * @return True if supported, otherwise false.
     */
    bool IsVoicePlayedSampleCountResetAtLoopPointSupported() const;

    /**
     * Check if the clear state bug for biquad filters is fixed.
     * The biquad state was not marked as needing re-initialisation when the effect was updated, it
     * was only initialized once with a new effect.
     * Note: Name is correct, Nintendo have the typo here
     *
     * @return True if fixed, otherwise false.
     */
    bool IsBiquadFilterEffectStateClaerBugFixed() const;

    /**
     * Check if Q23 precision is supported for fixed point.
     *
     * @return True if supported, otherwise false.
     */
    bool IsVolumeMixParameterPrecisionQ23Supported() const;

    /**
     * Check if float processing for biuad filters is supported.
     *
     * @return True if supported, otherwise false.
     */
    bool UseBiquadFilterFloatProcessing() const;

    /**
     * Check if dirty-only mix updates are supported.
     * This saves a lot of buffer size as mixes can be large and not change much.
     *
     * @return True if supported, otherwise false.
     */
    bool IsMixInParameterDirtyOnlyUpdateSupported() const;

    /**
     * Check if multi-tap biquad filters are supported.
     *
     * @return True if supported, otherwise false.
     */
    bool UseMultiTapBiquadFilterProcessing() const;

    /**
     * Check if device api version 2 is supported.
     * In the SDK but not in any sysmodule? Not sure, left here for completeness anyway.
     *
     * @return True if supported, otherwise false.
     */
    bool IsDeviceApiVersion2Supported() const;

    /// Host version
    u32 process_revision;
    /// User version
    u32 user_revision{};
    /// Behaviour flags
    Flags flags{};
    /// Errors generated and reported during Update
    std::array<ErrorInfo, MaxErrors> errors{};
    /// Error count
    u32 error_count{};
};

} // namespace AudioCore::AudioRenderer

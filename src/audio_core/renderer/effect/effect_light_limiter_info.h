// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <vector>

#include "audio_core/common/common.h"
#include "audio_core/renderer/effect/effect_info_base.h"
#include "common/common_types.h"
#include "common/fixed_point.h"

namespace AudioCore::AudioRenderer {

class LightLimiterInfo : public EffectInfoBase {
public:
    struct ParameterVersion1 {
        /* 0x00 */ std::array<s8, MaxChannels> inputs;
        /* 0x06 */ std::array<s8, MaxChannels> outputs;
        /* 0x0C */ u16 channel_count_max;
        /* 0x0E */ u16 channel_count;
        /* 0x0C */ u32 sample_rate;
        /* 0x14 */ s32 look_ahead_time_max;
        /* 0x18 */ s32 attack_time;
        /* 0x1C */ s32 release_time;
        /* 0x20 */ s32 look_ahead_time;
        /* 0x24 */ f32 attack_coeff;
        /* 0x28 */ f32 release_coeff;
        /* 0x2C */ f32 threshold;
        /* 0x30 */ f32 input_gain;
        /* 0x34 */ f32 output_gain;
        /* 0x38 */ s32 look_ahead_samples_min;
        /* 0x3C */ s32 look_ahead_samples_max;
        /* 0x40 */ ParameterState state;
        /* 0x41 */ bool statistics_enabled;
        /* 0x42 */ bool statistics_reset_required;
        /* 0x43 */ char unk43[0x1];
    };
    static_assert(sizeof(ParameterVersion1) <= sizeof(EffectInfoBase::InParameterVersion1),
                  "LightLimiterInfo::ParameterVersion1 has the wrong size!");

    struct ParameterVersion2 {
        /* 0x00 */ std::array<s8, MaxChannels> inputs;
        /* 0x06 */ std::array<s8, MaxChannels> outputs;
        /* 0x0C */ u16 channel_count_max;
        /* 0x0E */ u16 channel_count;
        /* 0x0C */ u32 sample_rate;
        /* 0x14 */ s32 look_ahead_time_max;
        /* 0x18 */ s32 attack_time;
        /* 0x1C */ s32 release_time;
        /* 0x20 */ s32 look_ahead_time;
        /* 0x24 */ f32 attack_coeff;
        /* 0x28 */ f32 release_coeff;
        /* 0x2C */ f32 threshold;
        /* 0x30 */ f32 input_gain;
        /* 0x34 */ f32 output_gain;
        /* 0x38 */ s32 look_ahead_samples_min;
        /* 0x3C */ s32 look_ahead_samples_max;
        /* 0x40 */ ParameterState state;
        /* 0x41 */ bool statistics_enabled;
        /* 0x42 */ bool statistics_reset_required;
        /* 0x43 */ char unk43[0x1];
    };
    static_assert(sizeof(ParameterVersion2) <= sizeof(EffectInfoBase::InParameterVersion2),
                  "LightLimiterInfo::ParameterVersion2 has the wrong size!");

    struct State {
        std::array<Common::FixedPoint<49, 15>, MaxChannels> samples_average;
        std::array<Common::FixedPoint<49, 15>, MaxChannels> compression_gain;
        std::array<s32, MaxChannels> look_ahead_sample_offsets;
        std::array<std::vector<Common::FixedPoint<49, 15>>, MaxChannels> look_ahead_sample_buffers;
    };
    static_assert(sizeof(State) <= sizeof(EffectInfoBase::State),
                  "LightLimiterInfo::State has the wrong size!");

    struct StatisticsInternal {
        /* 0x00 */ std::array<f32, MaxChannels> channel_max_sample;
        /* 0x18 */ std::array<f32, MaxChannels> channel_compression_gain_min;
    };
    static_assert(sizeof(StatisticsInternal) == 0x30,
                  "LightLimiterInfo::StatisticsInternal has the wrong size!");

    /**
     * Update the info with new parameters, version 1.
     *
     * @param error_info  - Used to write call result code.
     * @param in_params   - New parameters to update the info with.
     * @param pool_mapper - Pool for mapping buffers.
     */
    void Update(BehaviorInfo::ErrorInfo& error_info, const InParameterVersion1& in_params,
                const PoolMapper& pool_mapper) override;

    /**
     * Update the info with new parameters, version 2.
     *
     * @param error_info  - Used to write call result code.
     * @param in_params   - New parameters to update the info with.
     * @param pool_mapper - Pool for mapping buffers.
     */
    void Update(BehaviorInfo::ErrorInfo& error_info, const InParameterVersion2& in_params,
                const PoolMapper& pool_mapper) override;

    /**
     * Update the info after command generation. Usually only changes its state.
     */
    void UpdateForCommandGeneration() override;

    /**
     * Initialize a new limiter statistics result state. Version 2 only.
     *
     * @param result_state - Result state to initialize.
     */
    void InitializeResultState(EffectResultState& result_state) override;

    /**
     * Update the host-side limiter statistics with the ADSP-side one. Version 2 only.
     *
     * @param cpu_state - Host-side result state to update.
     * @param dsp_state - AudioRenderer-side result state to update from.
     */
    void UpdateResultState(EffectResultState& cpu_state, EffectResultState& dsp_state) override;

    /**
     * Get a workbuffer assigned to this effect with the given index.
     *
     * @param index - Workbuffer index.
     * @return Address of the buffer.
     */
    CpuAddr GetWorkbuffer(s32 index) override;
};

} // namespace AudioCore::AudioRenderer

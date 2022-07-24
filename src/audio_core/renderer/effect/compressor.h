// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "audio_core/common/common.h"
#include "audio_core/renderer/effect/effect_info_base.h"
#include "common/common_types.h"
#include "common/fixed_point.h"

namespace AudioCore::AudioRenderer {

class CompressorInfo : public EffectInfoBase {
public:
    struct ParameterVersion1 {
        /* 0x00 */ std::array<s8, MaxChannels> inputs;
        /* 0x06 */ std::array<s8, MaxChannels> outputs;
        /* 0x0C */ s16 channel_count_max;
        /* 0x0E */ s16 channel_count;
        /* 0x10 */ s32 sample_rate;
        /* 0x14 */ f32 threshold;
        /* 0x18 */ f32 compressor_ratio;
        /* 0x1C */ s32 attack_time;
        /* 0x20 */ s32 release_time;
        /* 0x24 */ f32 unk_24;
        /* 0x28 */ f32 unk_28;
        /* 0x2C */ f32 unk_2C;
        /* 0x30 */ f32 out_gain;
        /* 0x34 */ ParameterState state;
        /* 0x35 */ bool makeup_gain_enabled;
    };
    static_assert(sizeof(ParameterVersion1) <= sizeof(EffectInfoBase::InParameterVersion1),
                  "CompressorInfo::ParameterVersion1 has the wrong size!");

    struct ParameterVersion2 {
        /* 0x00 */ std::array<s8, MaxChannels> inputs;
        /* 0x06 */ std::array<s8, MaxChannels> outputs;
        /* 0x0C */ s16 channel_count_max;
        /* 0x0E */ s16 channel_count;
        /* 0x10 */ s32 sample_rate;
        /* 0x14 */ f32 threshold;
        /* 0x18 */ f32 compressor_ratio;
        /* 0x1C */ s32 attack_time;
        /* 0x20 */ s32 release_time;
        /* 0x24 */ f32 unk_24;
        /* 0x28 */ f32 unk_28;
        /* 0x2C */ f32 unk_2C;
        /* 0x30 */ f32 out_gain;
        /* 0x34 */ ParameterState state;
        /* 0x35 */ bool makeup_gain_enabled;
    };
    static_assert(sizeof(ParameterVersion2) <= sizeof(EffectInfoBase::InParameterVersion2),
                  "CompressorInfo::ParameterVersion2 has the wrong size!");

    struct State {
        f32 unk_00;
        f32 unk_04;
        f32 unk_08;
        f32 unk_0C;
        f32 unk_10;
        f32 unk_14;
        f32 unk_18;
        f32 makeup_gain;
        f32 unk_20;
        char unk_24[0x1C];
    };
    static_assert(sizeof(State) <= sizeof(EffectInfoBase::State),
                  "CompressorInfo::State has the wrong size!");

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
     * Get a workbuffer assigned to this effect with the given index.
     *
     * @param index - Workbuffer index.
     * @return Address of the buffer.
     */
    CpuAddr GetWorkbuffer(s32 index) override;
};

} // namespace AudioCore::AudioRenderer

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

class ReverbInfo : public EffectInfoBase {
public:
    struct ParameterVersion1 {
        /* 0x00 */ std::array<s8, MaxChannels> inputs;
        /* 0x06 */ std::array<s8, MaxChannels> outputs;
        /* 0x0C */ u16 channel_count_max;
        /* 0x0E */ u16 channel_count;
        /* 0x10 */ u32 sample_rate;
        /* 0x14 */ u32 early_mode;
        /* 0x18 */ s32 early_gain;
        /* 0x1C */ s32 pre_delay;
        /* 0x20 */ s32 late_mode;
        /* 0x24 */ s32 late_gain;
        /* 0x28 */ s32 decay_time;
        /* 0x2C */ s32 high_freq_Decay_ratio;
        /* 0x30 */ s32 colouration;
        /* 0x34 */ s32 base_gain;
        /* 0x38 */ s32 wet_gain;
        /* 0x3C */ s32 dry_gain;
        /* 0x40 */ ParameterState state;
    };
    static_assert(sizeof(ParameterVersion1) <= sizeof(EffectInfoBase::InParameterVersion1),
                  "ReverbInfo::ParameterVersion1 has the wrong size!");

    struct ParameterVersion2 {
        /* 0x00 */ std::array<s8, MaxChannels> inputs;
        /* 0x06 */ std::array<s8, MaxChannels> outputs;
        /* 0x0C */ u16 channel_count_max;
        /* 0x0E */ u16 channel_count;
        /* 0x10 */ u32 sample_rate;
        /* 0x14 */ u32 early_mode;
        /* 0x18 */ s32 early_gain;
        /* 0x1C */ s32 pre_delay;
        /* 0x20 */ s32 late_mode;
        /* 0x24 */ s32 late_gain;
        /* 0x28 */ s32 decay_time;
        /* 0x2C */ s32 high_freq_decay_ratio;
        /* 0x30 */ s32 colouration;
        /* 0x34 */ s32 base_gain;
        /* 0x38 */ s32 wet_gain;
        /* 0x3C */ s32 dry_gain;
        /* 0x40 */ ParameterState state;
    };
    static_assert(sizeof(ParameterVersion2) <= sizeof(EffectInfoBase::InParameterVersion2),
                  "ReverbInfo::ParameterVersion2 has the wrong size!");

    static constexpr u32 MaxDelayLines = 4;
    static constexpr u32 MaxDelayTaps = 10;
    static constexpr u32 NumEarlyModes = 5;
    static constexpr u32 NumLateModes = 5;

    struct ReverbDelayLine {
        void Initialize(const s32 delay_time, const f32 decay_rate) {
            buffer.resize(delay_time + 1, 0);
            buffer_end = &buffer[delay_time];
            output = &buffer[0];
            decay = decay_rate;
            sample_count_max = delay_time;
            SetDelay(delay_time);
        }

        void SetDelay(const s32 delay_time) {
            if (sample_count_max < delay_time) {
                return;
            }
            sample_count = delay_time;
            input = &buffer[(output - buffer.data() + sample_count) % (sample_count_max + 1)];
        }

        Common::FixedPoint<50, 14> Tick(const Common::FixedPoint<50, 14> sample) {
            Write(sample);

            auto out_sample{Read()};

            output++;
            if (output >= buffer_end) {
                output = buffer.data();
            }

            return out_sample;
        }

        Common::FixedPoint<50, 14> Read() {
            return *output;
        }

        void Write(const Common::FixedPoint<50, 14> sample) {
            *(input++) = sample;
            if (input >= buffer_end) {
                input = buffer.data();
            }
        }

        Common::FixedPoint<50, 14> TapOut(const s32 index) {
            auto out{input - (index + 1)};
            if (out < buffer.data()) {
                out += sample_count;
            }
            return *out;
        }

        s32 sample_count{};
        s32 sample_count_max{};
        std::vector<Common::FixedPoint<50, 14>> buffer{};
        Common::FixedPoint<50, 14>* buffer_end;
        Common::FixedPoint<50, 14>* input{};
        Common::FixedPoint<50, 14>* output{};
        Common::FixedPoint<50, 14> decay{};
    };

    struct State {
        ReverbDelayLine pre_delay_line;
        ReverbDelayLine center_delay_line;
        std::array<s32, MaxDelayTaps> early_delay_times;
        std::array<Common::FixedPoint<50, 14>, MaxDelayTaps> early_gains;
        s32 pre_delay_time;
        std::array<ReverbDelayLine, MaxDelayLines> decay_delay_lines;
        std::array<ReverbDelayLine, MaxDelayLines> fdn_delay_lines;
        std::array<Common::FixedPoint<50, 14>, MaxDelayLines> hf_decay_gain;
        std::array<Common::FixedPoint<50, 14>, MaxDelayLines> hf_decay_prev_gain;
        std::array<Common::FixedPoint<50, 14>, MaxDelayLines> prev_feedback_output;
    };
    static_assert(sizeof(State) <= sizeof(EffectInfoBase::State),
                  "ReverbInfo::State is too large!");

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
     * Initialize a new result state. Version 2 only, unused.
     *
     * @param result_state - Result state to initialize.
     */
    void InitializeResultState(EffectResultState& result_state) override;

    /**
     * Update the host-side state with the ADSP-side state. Version 2 only, unused.
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

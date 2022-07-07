// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "audio_core/common/common.h"
#include "audio_core/renderer/effect/effect_info_base.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
/**
 * Auxiliary Buffer used for Aux commands.
 * Send and return buffers are available (names from the game's perspective).
 * Send is read by the host, containing a buffer of samples to be used for whatever purpose.
 * Return is written by the host, writing a mix buffer back to the game.
 * This allows the game to use pre-processed samples skipping the other render processing,
 * and to examine or modify what the audio renderer has generated.
 */
class AuxInfo : public EffectInfoBase {
public:
    struct ParameterVersion1 {
        /* 0x00 */ std::array<s8, MaxMixBuffers> inputs;
        /* 0x18 */ std::array<s8, MaxMixBuffers> outputs;
        /* 0x30 */ u32 mix_buffer_count;
        /* 0x34 */ u32 sample_rate;
        /* 0x38 */ u32 count_max;
        /* 0x3C */ u32 mix_buffer_count_max;
        /* 0x40 */ CpuAddr send_buffer_info_address;
        /* 0x48 */ CpuAddr send_buffer_address;
        /* 0x50 */ CpuAddr return_buffer_info_address;
        /* 0x58 */ CpuAddr return_buffer_address;
        /* 0x60 */ u32 mix_buffer_sample_size;
        /* 0x64 */ u32 sample_count;
        /* 0x68 */ u32 mix_buffer_sample_count;
    };
    static_assert(sizeof(ParameterVersion1) <= sizeof(EffectInfoBase::InParameterVersion1),
                  "AuxInfo::ParameterVersion1 has the wrong size!");

    struct ParameterVersion2 {
        /* 0x00 */ std::array<s8, MaxMixBuffers> inputs;
        /* 0x18 */ std::array<s8, MaxMixBuffers> outputs;
        /* 0x30 */ u32 mix_buffer_count;
        /* 0x34 */ u32 sample_rate;
        /* 0x38 */ u32 count_max;
        /* 0x3C */ u32 mix_buffer_count_max;
        /* 0x40 */ CpuAddr send_buffer_info_address;
        /* 0x48 */ CpuAddr send_buffer_address;
        /* 0x50 */ CpuAddr return_buffer_info_address;
        /* 0x58 */ CpuAddr return_buffer_address;
        /* 0x60 */ u32 mix_buffer_sample_size;
        /* 0x64 */ u32 sample_count;
        /* 0x68 */ u32 mix_buffer_sample_count;
    };
    static_assert(sizeof(ParameterVersion2) <= sizeof(EffectInfoBase::InParameterVersion2),
                  "AuxInfo::ParameterVersion2 has the wrong size!");

    struct AuxInfoDsp {
        /* 0x00 */ u32 read_offset;
        /* 0x04 */ u32 write_offset;
        /* 0x08 */ u32 lost_sample_count;
        /* 0x0C */ u32 total_sample_count;
        /* 0x10 */ char unk10[0x30];
    };
    static_assert(sizeof(AuxInfoDsp) == 0x40, "AuxInfo::AuxInfoDsp has the wrong size!");

    struct AuxBufferInfo {
        /* 0x00 */ AuxInfoDsp cpu_info;
        /* 0x40 */ AuxInfoDsp dsp_info;
    };
    static_assert(sizeof(AuxBufferInfo) == 0x80, "AuxInfo::AuxBufferInfo has the wrong size!");

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

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/effect/light_limiter.h"

namespace AudioCore::AudioRenderer {

void LightLimiterInfo::Update(BehaviorInfo::ErrorInfo& error_info,
                              const InParameterVersion1& in_params, const PoolMapper& pool_mapper) {
    auto in_specific{reinterpret_cast<const ParameterVersion1*>(in_params.specific.data())};
    auto params{reinterpret_cast<ParameterVersion1*>(parameter.data())};

    std::memcpy(params, in_specific, sizeof(ParameterVersion1));
    mix_id = in_params.mix_id;
    process_order = in_params.process_order;
    enabled = in_params.enabled;

    if (buffer_unmapped || in_params.is_new) {
        usage_state = UsageState::New;
        params->state = ParameterState::Initialized;
        buffer_unmapped = !pool_mapper.TryAttachBuffer(
            error_info, workbuffers[0], in_params.workbuffer, in_params.workbuffer_size);
    } else {
        error_info.error_code = ResultSuccess;
        error_info.address = CpuAddr(0);
    }
}

void LightLimiterInfo::Update(BehaviorInfo::ErrorInfo& error_info,
                              const InParameterVersion2& in_params, const PoolMapper& pool_mapper) {
    auto in_specific{reinterpret_cast<const ParameterVersion1*>(in_params.specific.data())};
    auto params{reinterpret_cast<ParameterVersion1*>(parameter.data())};

    std::memcpy(params, in_specific, sizeof(ParameterVersion1));
    mix_id = in_params.mix_id;
    process_order = in_params.process_order;
    enabled = in_params.enabled;

    if (buffer_unmapped || in_params.is_new) {
        usage_state = UsageState::New;
        params->state = ParameterState::Initialized;
        buffer_unmapped = !pool_mapper.TryAttachBuffer(
            error_info, workbuffers[0], in_params.workbuffer, in_params.workbuffer_size);
    } else {
        error_info.error_code = ResultSuccess;
        error_info.address = CpuAddr(0);
    }
}

void LightLimiterInfo::UpdateForCommandGeneration() {
    if (enabled) {
        usage_state = UsageState::Enabled;
    } else {
        usage_state = UsageState::Disabled;
    }

    auto params{reinterpret_cast<ParameterVersion1*>(parameter.data())};
    params->state = ParameterState::Updated;
    params->statistics_reset_required = false;
}

void LightLimiterInfo::InitializeResultState(EffectResultState& result_state) {
    auto result_state_{reinterpret_cast<StatisticsInternal*>(result_state.state.data())};

    result_state_->channel_max_sample.fill(0);
    result_state_->channel_compression_gain_min.fill(1.0f);
}

void LightLimiterInfo::UpdateResultState(EffectResultState& cpu_state,
                                         EffectResultState& dsp_state) {
    auto cpu_statistics{reinterpret_cast<StatisticsInternal*>(cpu_state.state.data())};
    auto dsp_statistics{reinterpret_cast<StatisticsInternal*>(dsp_state.state.data())};

    *cpu_statistics = *dsp_statistics;
}

CpuAddr LightLimiterInfo::GetWorkbuffer(s32 index) {
    return GetSingleBuffer(index);
}

} // namespace AudioCore::AudioRenderer

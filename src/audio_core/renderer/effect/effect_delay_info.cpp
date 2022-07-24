// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/effect/effect_delay_info.h"

namespace AudioCore::AudioRenderer {

void DelayInfo::Update(BehaviorInfo::ErrorInfo& error_info, const InParameterVersion1& in_params,
                       const PoolMapper& pool_mapper) {
    auto in_specific{reinterpret_cast<const ParameterVersion1*>(in_params.specific.data())};
    auto params{reinterpret_cast<ParameterVersion1*>(parameter.data())};

    if (IsChannelCountValid(in_specific->channel_count_max)) {
        const auto old_state{params->state};
        std::memcpy(params, in_specific, sizeof(ParameterVersion1));
        mix_id = in_params.mix_id;
        process_order = in_params.process_order;
        enabled = in_params.enabled;

        if (!IsChannelCountValid(in_specific->channel_count)) {
            params->channel_count = params->channel_count_max;
        }

        if (!IsChannelCountValid(in_specific->channel_count) ||
            old_state != ParameterState::Updated) {
            params->state = old_state;
        }

        if (buffer_unmapped || in_params.is_new) {
            usage_state = UsageState::New;
            params->state = ParameterState::Initialized;
            buffer_unmapped = !pool_mapper.TryAttachBuffer(
                error_info, workbuffers[0], in_params.workbuffer, in_params.workbuffer_size);
            return;
        }
    }
    error_info.error_code = ResultSuccess;
    error_info.address = CpuAddr(0);
}

void DelayInfo::Update(BehaviorInfo::ErrorInfo& error_info, const InParameterVersion2& in_params,
                       const PoolMapper& pool_mapper) {
    auto in_specific{reinterpret_cast<const ParameterVersion1*>(in_params.specific.data())};
    auto params{reinterpret_cast<ParameterVersion1*>(parameter.data())};

    if (IsChannelCountValid(in_specific->channel_count_max)) {
        const auto old_state{params->state};
        std::memcpy(params, in_specific, sizeof(ParameterVersion1));
        mix_id = in_params.mix_id;
        process_order = in_params.process_order;
        enabled = in_params.enabled;

        if (!IsChannelCountValid(in_specific->channel_count)) {
            params->channel_count = params->channel_count_max;
        }

        if (!IsChannelCountValid(in_specific->channel_count) ||
            old_state != ParameterState::Updated) {
            params->state = old_state;
        }

        if (buffer_unmapped || in_params.is_new) {
            usage_state = UsageState::New;
            params->state = ParameterState::Initialized;
            buffer_unmapped = !pool_mapper.TryAttachBuffer(
                error_info, workbuffers[0], in_params.workbuffer, in_params.workbuffer_size);
            return;
        }
    }
    error_info.error_code = ResultSuccess;
    error_info.address = CpuAddr(0);
}

void DelayInfo::UpdateForCommandGeneration() {
    if (enabled) {
        usage_state = UsageState::Enabled;
    } else {
        usage_state = UsageState::Disabled;
    }

    auto params{reinterpret_cast<ParameterVersion1*>(parameter.data())};
    params->state = ParameterState::Updated;
}

void DelayInfo::InitializeResultState(EffectResultState& result_state) {}

void DelayInfo::UpdateResultState(EffectResultState& cpu_state, EffectResultState& dsp_state) {}

CpuAddr DelayInfo::GetWorkbuffer(s32 index) {
    return GetSingleBuffer(index);
}

} // namespace AudioCore::AudioRenderer

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/effect/buffer_mixer.h"

namespace AudioCore::AudioRenderer {

void BufferMixerInfo::Update(BehaviorInfo::ErrorInfo& error_info,
                             const InParameterVersion1& in_params, const PoolMapper& pool_mapper) {
    auto in_specific{reinterpret_cast<const ParameterVersion1*>(in_params.specific.data())};
    auto params{reinterpret_cast<ParameterVersion1*>(parameter.data())};

    std::memcpy(params, in_specific, sizeof(ParameterVersion1));
    mix_id = in_params.mix_id;
    process_order = in_params.process_order;
    enabled = in_params.enabled;

    error_info.error_code = ResultSuccess;
    error_info.address = CpuAddr(0);
}

void BufferMixerInfo::Update(BehaviorInfo::ErrorInfo& error_info,
                             const InParameterVersion2& in_params, const PoolMapper& pool_mapper) {
    auto in_specific{reinterpret_cast<const ParameterVersion2*>(in_params.specific.data())};
    auto params{reinterpret_cast<ParameterVersion2*>(parameter.data())};

    std::memcpy(params, in_specific, sizeof(ParameterVersion2));
    mix_id = in_params.mix_id;
    process_order = in_params.process_order;
    enabled = in_params.enabled;

    error_info.error_code = ResultSuccess;
    error_info.address = CpuAddr(0);
}

void BufferMixerInfo::UpdateForCommandGeneration() {
    if (enabled) {
        usage_state = UsageState::Enabled;
    } else {
        usage_state = UsageState::Disabled;
    }
}

void BufferMixerInfo::InitializeResultState(EffectResultState& result_state) {}

void BufferMixerInfo::UpdateResultState(EffectResultState& cpu_state,
                                        EffectResultState& dsp_state) {}

} // namespace AudioCore::AudioRenderer

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/effect/aux_.h"

namespace AudioCore::AudioRenderer {

void AuxInfo::Update(BehaviorInfo::ErrorInfo& error_info, const InParameterVersion1& in_params,
                     const PoolMapper& pool_mapper) {
    auto in_specific{reinterpret_cast<const ParameterVersion1*>(in_params.specific.data())};
    auto params{reinterpret_cast<ParameterVersion1*>(parameter.data())};

    std::memcpy(params, in_specific, sizeof(ParameterVersion1));
    mix_id = in_params.mix_id;
    process_order = in_params.process_order;
    enabled = in_params.enabled;
    if (buffer_unmapped || in_params.is_new) {
        const bool send_unmapped{!pool_mapper.TryAttachBuffer(
            error_info, workbuffers[0], in_specific->send_buffer_info_address,
            sizeof(AuxBufferInfo) + in_specific->count_max * sizeof(s32))};
        const bool return_unmapped{!pool_mapper.TryAttachBuffer(
            error_info, workbuffers[1], in_specific->return_buffer_info_address,
            sizeof(AuxBufferInfo) + in_specific->count_max * sizeof(s32))};

        buffer_unmapped = send_unmapped || return_unmapped;

        if (!buffer_unmapped) {
            auto send{workbuffers[0].GetReference(false)};
            send_buffer_info = send + sizeof(AuxInfoDsp);
            send_buffer = send + sizeof(AuxBufferInfo);

            auto ret{workbuffers[1].GetReference(false)};
            return_buffer_info = ret + sizeof(AuxInfoDsp);
            return_buffer = ret + sizeof(AuxBufferInfo);
        }
    } else {
        error_info.error_code = ResultSuccess;
        error_info.address = CpuAddr(0);
    }
}

void AuxInfo::Update(BehaviorInfo::ErrorInfo& error_info, const InParameterVersion2& in_params,
                     const PoolMapper& pool_mapper) {
    auto in_specific{reinterpret_cast<const ParameterVersion2*>(in_params.specific.data())};
    auto params{reinterpret_cast<ParameterVersion2*>(parameter.data())};

    std::memcpy(params, in_specific, sizeof(ParameterVersion2));
    mix_id = in_params.mix_id;
    process_order = in_params.process_order;
    enabled = in_params.enabled;

    if (buffer_unmapped || in_params.is_new) {
        const bool send_unmapped{!pool_mapper.TryAttachBuffer(
            error_info, workbuffers[0], params->send_buffer_info_address,
            sizeof(AuxBufferInfo) + params->count_max * sizeof(s32))};
        const bool return_unmapped{!pool_mapper.TryAttachBuffer(
            error_info, workbuffers[1], params->return_buffer_info_address,
            sizeof(AuxBufferInfo) + params->count_max * sizeof(s32))};

        buffer_unmapped = send_unmapped || return_unmapped;

        if (!buffer_unmapped) {
            auto send{workbuffers[0].GetReference(false)};
            send_buffer_info = send + sizeof(AuxInfoDsp);
            send_buffer = send + sizeof(AuxBufferInfo);

            auto ret{workbuffers[1].GetReference(false)};
            return_buffer_info = ret + sizeof(AuxInfoDsp);
            return_buffer = ret + sizeof(AuxBufferInfo);
        }
    } else {
        error_info.error_code = ResultSuccess;
        error_info.address = CpuAddr(0);
    }
}

void AuxInfo::UpdateForCommandGeneration() {
    if (enabled) {
        usage_state = UsageState::Enabled;
    } else {
        usage_state = UsageState::Disabled;
    }
}

void AuxInfo::InitializeResultState(EffectResultState& result_state) {}

void AuxInfo::UpdateResultState(EffectResultState& cpu_state, EffectResultState& dsp_state) {}

CpuAddr AuxInfo::GetWorkbuffer(s32 index) {
    return workbuffers[index].GetReference(true);
}

} // namespace AudioCore::AudioRenderer

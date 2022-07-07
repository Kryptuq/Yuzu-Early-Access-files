// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/effect/effect_aux_info.h"
#include "audio_core/renderer/effect/effect_capture_info.h"

namespace AudioCore::AudioRenderer {

void CaptureInfo::Update(BehaviorInfo::ErrorInfo& error_info, const InParameterVersion1& in_params,
                         const PoolMapper& pool_mapper) {
    auto in_specific{
        reinterpret_cast<const AuxInfo::ParameterVersion1*>(in_params.specific.data())};
    auto params{reinterpret_cast<AuxInfo::ParameterVersion1*>(parameter.data())};

    std::memcpy(params, in_specific, sizeof(AuxInfo::ParameterVersion1));
    mix_id = in_params.mix_id;
    process_order = in_params.process_order;
    enabled = in_params.enabled;
    if (buffer_unmapped || in_params.is_new) {
        buffer_unmapped = !pool_mapper.TryAttachBuffer(
            error_info, workbuffers[0], in_specific->send_buffer_info_address,
            in_specific->count_max * sizeof(s32) + sizeof(AuxInfo::AuxBufferInfo));

        if (!buffer_unmapped) {
            const auto send_address{workbuffers[0].GetReference(false)};
            send_buffer_info = send_address + sizeof(AuxInfo::AuxInfoDsp);
            send_buffer = send_address + sizeof(AuxInfo::AuxBufferInfo);
            return_buffer_info = 0;
            return_buffer = 0;
        }
    } else {
        error_info.error_code = ResultSuccess;
        error_info.address = CpuAddr(0);
    }
}

void CaptureInfo::Update(BehaviorInfo::ErrorInfo& error_info, const InParameterVersion2& in_params,
                         const PoolMapper& pool_mapper) {
    auto in_specific{
        reinterpret_cast<const AuxInfo::ParameterVersion2*>(in_params.specific.data())};
    auto params{reinterpret_cast<AuxInfo::ParameterVersion2*>(parameter.data())};

    std::memcpy(params, in_specific, sizeof(AuxInfo::ParameterVersion2));
    mix_id = in_params.mix_id;
    process_order = in_params.process_order;
    enabled = in_params.enabled;

    if (buffer_unmapped || in_params.is_new) {
        buffer_unmapped = !pool_mapper.TryAttachBuffer(
            error_info, workbuffers[0], params->send_buffer_info_address,
            params->count_max * sizeof(s32) + sizeof(AuxInfo::AuxBufferInfo));

        if (!buffer_unmapped) {
            const auto send_address{workbuffers[0].GetReference(false)};
            send_buffer_info = send_address + sizeof(AuxInfo::AuxInfoDsp);
            send_buffer = send_address + sizeof(AuxInfo::AuxBufferInfo);
            return_buffer_info = 0;
            return_buffer = 0;
        }
    } else {
        error_info.error_code = ResultSuccess;
        error_info.address = CpuAddr(0);
    }
}

void CaptureInfo::UpdateForCommandGeneration() {
    if (enabled) {
        usage_state = UsageState::Enabled;
    } else {
        usage_state = UsageState::Disabled;
    }
}

void CaptureInfo::InitializeResultState(EffectResultState& result_state) {}

void CaptureInfo::UpdateResultState(EffectResultState& cpu_state, EffectResultState& dsp_state) {}

CpuAddr CaptureInfo::GetWorkbuffer(s32 index) {
    return workbuffers[index].GetReference(true);
}

} // namespace AudioCore::AudioRenderer

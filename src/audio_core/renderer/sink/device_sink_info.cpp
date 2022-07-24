// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/sink/device_sink_info.h"
#include "audio_core/renderer/upsampler/upsampler_manager.h"

namespace AudioCore::AudioRenderer {

DeviceSinkInfo::DeviceSinkInfo() {
    state.fill(0);
    parameter.fill(0);
    type = Type::DeviceSink;
}

void DeviceSinkInfo::CleanUp() {
    auto state_{reinterpret_cast<DeviceState*>(state.data())};

    if (state_->upsampler_info) {
        state_->upsampler_info->manager->Free(state_->upsampler_info);
        state_->upsampler_info = nullptr;
    }

    parameter.fill(0);
    type = Type::Invalid;
}

void DeviceSinkInfo::Update(BehaviorInfo::ErrorInfo& error_info, OutStatus& out_status,
                            const InParameter& in_params,
                            [[maybe_unused]] const PoolMapper& pool_mapper) {

    const auto device_params{reinterpret_cast<const DeviceInParameter*>(&in_params.device)};
    auto current_params{reinterpret_cast<DeviceInParameter*>(parameter.data())};

    if (in_use == in_params.in_use) {
        current_params->downmix_enabled = device_params->downmix_enabled;
        current_params->downmix_coeff = device_params->downmix_coeff;
    } else {
        type = in_params.type;
        in_use = in_params.in_use;
        node_id = in_params.node_id;
        *current_params = *device_params;
    }

    auto current_state{reinterpret_cast<DeviceState*>(state.data())};

    for (size_t i = 0; i < current_state->downmix_coeff.size(); i++) {
        current_state->downmix_coeff[i] = current_params->downmix_coeff[i];
    }

    std::memset(&out_status, 0, sizeof(OutStatus));
    error_info.error_code = ResultSuccess;
    error_info.address = CpuAddr(0);
}

void DeviceSinkInfo::UpdateForCommandGeneration() {}

} // namespace AudioCore::AudioRenderer

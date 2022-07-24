// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/memory/pool_mapper.h"
#include "audio_core/renderer/sink/sink_info_base.h"

namespace AudioCore::AudioRenderer {

void SinkInfoBase::CleanUp() {
    type = Type::Invalid;
}

void SinkInfoBase::Update(BehaviorInfo::ErrorInfo& error_info, OutStatus& out_status,
                          [[maybe_unused]] const InParameter& in_params,
                          [[maybe_unused]] const PoolMapper& pool_mapper) {
    std::memset(&out_status, 0, sizeof(OutStatus));
    error_info.error_code = ResultSuccess;
    error_info.address = CpuAddr(0);
}

void SinkInfoBase::UpdateForCommandGeneration() {}

SinkInfoBase::DeviceState* SinkInfoBase::GetDeviceState() {
    return reinterpret_cast<DeviceState*>(state.data());
}

SinkInfoBase::Type SinkInfoBase::GetType() const {
    return type;
}

bool SinkInfoBase::IsUsed() const {
    return in_use;
}

bool SinkInfoBase::ShouldSkip() const {
    return buffer_unmapped;
}

u32 SinkInfoBase::GetNodeId() const {
    return node_id;
}

u8* SinkInfoBase::GetState() {
    return state.data();
}

u8* SinkInfoBase::GetParameter() {
    return parameter.data();
}

} // namespace AudioCore::AudioRenderer

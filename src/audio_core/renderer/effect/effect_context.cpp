// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/effect/effect_context.h"

namespace AudioCore::AudioRenderer {

void EffectContext::Initialize(std::span<EffectInfoBase> effect_infos_, const u32 effect_count_,
                               std::span<EffectResultState> result_states_cpu_,
                               std::span<EffectResultState> result_states_dsp_,
                               const size_t dsp_state_count_) {
    effect_infos = effect_infos_;
    effect_count = effect_count_;
    result_states_cpu = result_states_cpu_;
    result_states_dsp = result_states_dsp_;
    dsp_state_count = dsp_state_count_;
}

EffectInfoBase& EffectContext::GetInfo(const u32 index) {
    return effect_infos[index];
}

EffectResultState& EffectContext::GetResultState(const u32 index) {
    return result_states_cpu[index];
}

EffectResultState& EffectContext::GetDspSharedResultState(const u32 index) {
    return result_states_dsp[index];
}

u32 EffectContext::GetCount() const {
    return effect_count;
}

void EffectContext::UpdateStateByDspShared() {
    for (size_t i = 0; i < dsp_state_count; i++) {
        effect_infos[i].UpdateResultState(result_states_cpu[i], result_states_dsp[i]);
    }
}

} // namespace AudioCore::AudioRenderer

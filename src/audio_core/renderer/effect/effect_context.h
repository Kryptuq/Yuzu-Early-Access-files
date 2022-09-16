// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "audio_core/renderer/effect/effect_info_base.h"
#include "audio_core/renderer/effect/effect_result_state.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {

class EffectContext {
public:
    /**
     * Initialize the effect context
     * @param effect_infos_      - List of effect infos for this context
     * @param effect_count_      - The number of effects in the list
     * @param result_states_cpu_ - The workbuffer of result states for the CPU for this context
     * @param result_states_dsp_ - The workbuffer of result states for the DSP for this context
     * @param dsp_state_count    - The number of result states
     */
    void Initialize(std::span<EffectInfoBase> effect_infos_, u32 effect_count_,
                    std::span<EffectResultState> result_states_cpu_,
                    std::span<EffectResultState> result_states_dsp_, size_t dsp_state_count);

    /**
     * Get the EffectInfo for a given index
     * @param index Which effect to return
     * @return Pointer to the effect
     */
    EffectInfoBase& GetInfo(const u32 index);

    /**
     * Get the CPU result state for a given index
     * @param index Which result to return
     * @return Pointer to the effect result state
     */
    EffectResultState& GetResultState(const u32 index);

    /**
     * Get the DSP result state for a given index
     * @param index Which result to return
     * @return Pointer to the effect result state
     */
    EffectResultState& GetDspSharedResultState(const u32 index);

    /**
     * Get the number of effects in this context
     * @return The number of effects
     */
    u32 GetCount() const;

    /**
     * Update the CPU and DSP result states for all effects
     */
    void UpdateStateByDspShared();

private:
    /// Workbuffer for all of the effects
    std::span<EffectInfoBase> effect_infos{};
    /// Number of effects in the workbuffer
    u32 effect_count{};
    /// Workbuffer of states for all effects, kept host-side and not directly modified, dsp states
    /// are copied here on the next render frame
    std::span<EffectResultState> result_states_cpu{};
    /// Workbuffer of states for all effects, used by the AudioRenderer to track effect state
    /// between calls
    std::span<EffectResultState> result_states_dsp{};
    /// Number of result states in the workbuffers
    size_t dsp_state_count{};
};

} // namespace AudioCore::AudioRenderer

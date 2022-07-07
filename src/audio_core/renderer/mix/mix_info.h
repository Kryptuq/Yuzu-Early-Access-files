// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <span>

#include "audio_core/common/common.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
class EdgeMatrix;
class SplitterContext;
class EffectContext;
class BehaviorInfo;

/**
 * A single mix, which may feed through other mixes in a chain until reaching the final output mix.
 */
class MixInfo {
public:
    struct InParameter {
        /* 0x000 */ f32 volume;
        /* 0x004 */ u32 sample_rate;
        /* 0x008 */ u32 buffer_count;
        /* 0x00C */ bool in_use;
        /* 0x00D */ bool is_dirty;
        /* 0x010 */ s32 mix_id;
        /* 0x014 */ u32 effect_count;
        /* 0x018 */ s32 node_id;
        /* 0x01C */ char unk01C[0x8];
        /* 0x024 */ std::array<std::array<f32, MaxMixBuffers>, MaxMixBuffers> mix_volumes;
        /* 0x924 */ s32 dest_mix_id;
        /* 0x928 */ s32 dest_splitter_id;
        /* 0x92C */ char unk92C[0x4];
    };
    static_assert(sizeof(InParameter) == 0x930, "MixInfo::InParameter has the wrong size!");

    struct InDirtyParameter {
        /* 0x00 */ u32 magic;
        /* 0x04 */ s32 count;
        /* 0x08 */ char unk08[0x18];
    };
    static_assert(sizeof(InDirtyParameter) == 0x20,
                  "MixInfo::InDirtyParameter has the wrong size!");

    MixInfo(std::span<s32> effect_order_buffer, s32 effect_count, BehaviorInfo& behavior);

    /**
     * Clean up the mix, resetting it to a default state.
     */
    void Cleanup();

    /**
     * Clear the effect process order for all effects in this mix.
     */
    void ClearEffectProcessingOrder();

    /**
     * Update the mix according to the given parameters.
     *
     * @param edge_matrix      - Updated with new splitter node connections, if supported.
     * @param in_params        - Input parameters.
     * @param effect_context   - Used to update the effect orderings.
     * @param splitter_context - Used to update the mix graph if supported.
     * @param behavior        - Used for checking which features are supported.
     * @return True if the mix was updated and a sort is required, otherwise false.
     */
    bool Update(EdgeMatrix& edge_matrix, const InParameter& in_params,
                EffectContext& effect_context, SplitterContext& splitter_context,
                const BehaviorInfo& behavior);

    /**
     * Update the mix's connection in the node graph according to the given parameters.
     *
     * @param edge_matrix      - Updated with new splitter node connections, if supported.
     * @param in_params        - Input parameters.
     * @param splitter_context - Used to update the mix graph if supported.
     * @return True if the mix was updated and a sort is required, otherwise false.
     */
    bool UpdateConnection(EdgeMatrix& edge_matrix, const InParameter& in_params,
                          SplitterContext& splitter_context);

    /**
     * Check if this mix is connected to any other.
     *
     * @return True if the mix has a connection, otherwise false.
     */
    bool HasAnyConnection() const;

    /// Volume of this mix
    f32 volume{};
    /// Sample rate of this mix
    u32 sample_rate{};
    /// Number of buffers in this mix
    s16 buffer_count{};
    /// Is this mix in use?
    bool in_use{};
    /// Is this mix enabled?
    bool enabled{};
    /// Id of this mix
    s32 mix_id{UnusedMixId};
    /// Node id of this mix
    s32 node_id{};
    /// Buffer offset for this mix
    s16 buffer_offset{};
    /// Distance to the final mix
    s32 distance_from_final_mix{InvalidDistanceFromFinalMix};
    /// Array of effect orderings of all effects in this mix
    std::span<s32> effect_order_buffer;
    /// Number of effects in this mix
    const s32 effect_count;
    /// Id for next mix in the chain
    s32 dst_mix_id{UnusedMixId};
    /// Mixing volumes for this mix used when this mix is chained with another
    std::array<std::array<f32, MaxMixBuffers>, MaxMixBuffers> mix_volumes{};
    /// Id for next mix in the graph when splitter is used
    s32 dst_splitter_id{UnusedSplitterId};
    /// Is a longer pre-delay time supported for the reverb effect?
    const bool long_size_pre_delay_supported;
};

} // namespace AudioCore::AudioRenderer

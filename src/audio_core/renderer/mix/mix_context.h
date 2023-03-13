// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "audio_core/renderer/mix/mix_info.h"
#include "audio_core/renderer/nodes/edge_matrix.h"
#include "audio_core/renderer/nodes/node_states.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
class SplitterContext;

/*
 * Manages mixing states, sorting and building a node graph to describe a mix order.
 */
class MixContext {
public:
    /**
     * Initialize the mix context.
     *
     * @param sorted_mix_infos - Buffer for the sorted mix infos.
     * @param mix_infos - Buffer for the mix infos.
     * @param effect_process_order_buffer - Buffer for the effect process orders.
     * @param effect_count - Number of effects in the buffer.
     * @param node_states_workbuffer - Buffer for node states.
     * @param node_buffer_size - Size of the node states buffer.
     * @param edge_matrix_workbuffer - Buffer for edge matrix.
     * @param edge_matrix_size - Size of the edge matrix buffer.
     */
    void Initialize(std::span<MixInfo*> sorted_mix_infos, std::span<MixInfo> mix_infos, u32 count_,
                    std::span<s32> effect_process_order_buffer, u32 effect_count,
                    std::span<u8> node_states_workbuffer, u64 node_buffer_size,
                    std::span<u8> edge_matrix_workbuffer, u64 edge_matrix_size);

    /**
     * Get a sorted mix at the given index.
     *
     * @param index - Index of sorted mix.
     * @return The sorted mix.
     */
    MixInfo* GetSortedInfo(s32 index);

    /**
     * Set the sorted info at the given index.
     *
     * @param index    - Index of sorted mix.
     * @param mix_info - The new mix for this index.
     */
    void SetSortedInfo(s32 index, MixInfo& mix_info);

    /**
     * Get a mix at the given index.
     *
     * @param index - Index of mix.
     * @return The mix.
     */
    MixInfo* GetInfo(s32 index);

    /**
     * Get the final mix.
     *
     * @return The final mix.
     */
    MixInfo* GetFinalMixInfo();

    /**
     * Get the current number of mixes.
     *
     * @return The number of active mixes.
     */
    s32 GetCount() const;

    /**
     * Update all of the mixes' distance from the final mix.
     * Needs to be called after altering the mix graph.
     */
    void UpdateDistancesFromFinalMix();

    /**
     * Non-splitter sort, sorts the sorted mixes based on their distance from the final mix.
     */
    void SortInfo();

    /**
     * Re-calculate the mix buffer offsets for each mix after altering the mix.
     */
    void CalcMixBufferOffset();

    /**
     * Splitter sort, traverse the splitter node graph and sort the sorted mixes from results.
     *
     * @param splitter_context - Splitter context for the sort.
     * @return True if the sort was successful, otherwise false.
     */
    bool TSortInfo(const SplitterContext& splitter_context);

    /**
     * Get the edge matrix used for the mix graph.
     *
     * @return The edge matrix used.
     */
    EdgeMatrix& GetEdgeMatrix();

private:
    /// Array of sorted mixes
    std::span<MixInfo*> sorted_mix_infos{};
    /// Array of mixes
    std::span<MixInfo> mix_infos{};
    /// Number of active mixes
    s32 count{};
    /// Array of effect process orderings
    std::span<s32> effect_process_order_buffer{};
    /// Number of effects in the process ordering buffer
    u64 effect_count{};
    /// Node states used in splitter sort
    NodeStates node_states{};
    /// Edge matrix for connected nodes used in splitter sort
    EdgeMatrix edge_matrix{};
};

} // namespace AudioCore::AudioRenderer

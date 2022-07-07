// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/behavior/behavior_info.h"
#include "audio_core/renderer/effect/effect_context.h"
#include "audio_core/renderer/mix/mix_info.h"
#include "audio_core/renderer/nodes/edge_matrix.h"
#include "audio_core/renderer/splitter/splitter_context.h"

namespace AudioCore::AudioRenderer {

MixInfo::MixInfo(std::span<s32> effect_order_buffer_, s32 effect_count_, BehaviorInfo& behavior)
    : effect_order_buffer{effect_order_buffer_}, effect_count{effect_count_},
      long_size_pre_delay_supported{behavior.IsLongSizePreDelaySupported()} {
    ClearEffectProcessingOrder();
}

void MixInfo::Cleanup() {
    mix_id = UnusedMixId;
    dst_mix_id = UnusedMixId;
    dst_splitter_id = UnusedSplitterId;
}

void MixInfo::ClearEffectProcessingOrder() {
    for (s32 i = 0; i < effect_count; i++) {
        effect_order_buffer[i] = -1;
    }
}

bool MixInfo::Update(EdgeMatrix& edge_matrix, const InParameter& in_params,
                     EffectContext& effect_context, SplitterContext& splitter_context,
                     const BehaviorInfo& behavior) {
    volume = in_params.volume;
    sample_rate = in_params.sample_rate;
    buffer_count = static_cast<s16>(in_params.buffer_count);
    in_use = in_params.in_use;
    mix_id = in_params.mix_id;
    node_id = in_params.node_id;
    mix_volumes = in_params.mix_volumes;

    bool sort_required{false};
    if (behavior.IsSplitterSupported()) {
        sort_required = UpdateConnection(edge_matrix, in_params, splitter_context);
    } else {
        if (dst_mix_id != in_params.dest_mix_id) {
            dst_mix_id = in_params.dest_mix_id;
            sort_required = true;
        }
        dst_splitter_id = UnusedSplitterId;
    }

    ClearEffectProcessingOrder();

    // Check all effects, and set their order if they belong to this mix.
    const auto count{effect_context.GetCount()};
    for (u32 i = 0; i < count; i++) {
        const auto& info{effect_context.GetInfo(i)};
        if (mix_id == info.GetMixId()) {
            const auto processing_order{info.GetProcessingOrder()};
            if (processing_order > effect_count) {
                break;
            }
            effect_order_buffer[processing_order] = i;
        }
    }

    return sort_required;
}

bool MixInfo::UpdateConnection(EdgeMatrix& edge_matrix, const InParameter& in_params,
                               SplitterContext& splitter_context) {
    auto has_new_connection{false};
    if (dst_splitter_id != UnusedSplitterId) {
        auto& splitter_info{splitter_context.GetInfo(dst_splitter_id)};
        has_new_connection = splitter_info.HasNewConnection();
    }

    // Check if this mix matches the input parameters.
    // If everything is the same, don't bother updating.
    if (dst_mix_id == in_params.dest_mix_id && dst_splitter_id == in_params.dest_splitter_id &&
        !has_new_connection) {
        return false;
    }

    // Reset the mix in the graph, as we're about to update it.
    edge_matrix.RemoveEdges(mix_id);

    if (in_params.dest_mix_id == UnusedMixId) {
        if (in_params.dest_splitter_id != UnusedSplitterId) {
            // If the splitter is used, connect this mix to each active destination.
            auto& splitter_info{splitter_context.GetInfo(in_params.dest_splitter_id)};
            auto const destination_count{splitter_info.GetDestinationCount()};

            for (u32 i = 0; i < destination_count; i++) {
                auto destination{
                    splitter_context.GetDesintationData(in_params.dest_splitter_id, i)};

                if (destination) {
                    const auto destination_id{destination->GetMixId()};
                    if (destination_id != UnusedMixId) {
                        edge_matrix.Connect(mix_id, destination_id);
                    }
                }
            }
        }
    } else {
        // If the splitter is not used, only connect this mix to its destination.
        edge_matrix.Connect(mix_id, in_params.dest_mix_id);
    }

    dst_mix_id = in_params.dest_mix_id;
    dst_splitter_id = in_params.dest_splitter_id;
    return true;
}

bool MixInfo::HasAnyConnection() const {
    return dst_mix_id != UnusedMixId || dst_splitter_id != UnusedSplitterId;
}

} // namespace AudioCore::AudioRenderer

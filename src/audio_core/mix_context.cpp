// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include "audio_core/behavior_info.h"
#include "audio_core/common.h"
#include "audio_core/effect_context.h"
#include "audio_core/mix_context.h"
#include "audio_core/splitter_context.h"

namespace AudioCore {
MixContext::MixContext() = default;
MixContext::~MixContext() = default;

void MixContext::Initialize(const BehaviorInfo& behavior_info, std::size_t mix_count,
                            std::size_t effect_count) {
    info_count = mix_count;
    infos.resize(info_count);
    auto& final_mix = GetInfo(AudioCommon::FINAL_MIX);
    final_mix.GetInParams().mix_id = AudioCommon::FINAL_MIX;
    sorted_info.reserve(infos.size());
    for (auto& info : infos) {
        sorted_info.push_back(&info);
    }

    for (auto& info : infos) {
        info.SetEffectCount(effect_count);
    }

    // Only initialize our edge matrix and node states if splitters are supported
    if (behavior_info.IsSplitterSupported()) {
        node_states.Initialize(mix_count);
        edge_matrix.Initialize(mix_count);
    }
}

void MixContext::UpdateDistancesFromFinalMix() {
    // Set all distances to be invalid
    for (std::size_t i = 0; i < info_count; i++) {
        GetInfo(i).GetInParams().final_mix_distance = AudioCommon::NO_FINAL_MIX;
    }

    for (std::size_t i = 0; i < info_count; i++) {
        auto& info = GetInfo(i);
        auto& in_params = info.GetInParams();
        // Populate our sorted info
        sorted_info[i] = &info;

        if (!in_params.in_use) {
            continue;
        }

        auto mix_id = in_params.mix_id;
        // Needs to be referenced out of scope
        s32 distance_to_final_mix{AudioCommon::FINAL_MIX};
        for (; distance_to_final_mix < static_cast<s32>(info_count); distance_to_final_mix++) {
            if (mix_id == AudioCommon::FINAL_MIX) {
                // If we're at the final mix, we're done
                break;
            } else if (mix_id == AudioCommon::NO_MIX) {
                // If we have no more mix ids, we're done
                distance_to_final_mix = AudioCommon::NO_FINAL_MIX;
                break;
            } else {
                const auto& dest_mix = GetInfo(mix_id);
                const auto dest_mix_distance = dest_mix.GetInParams().final_mix_distance;

                if (dest_mix_distance == AudioCommon::NO_FINAL_MIX) {
                    // If our current mix isn't pointing to a final mix, follow through
                    mix_id = dest_mix.GetInParams().dest_mix_id;
                } else {
                    // Our current mix + 1 = final distance
                    distance_to_final_mix = dest_mix_distance + 1;
                    break;
                }
            }
        }

        // If we're out of range for our distance, mark it as no final mix
        if (distance_to_final_mix >= static_cast<s32>(info_count)) {
            distance_to_final_mix = AudioCommon::NO_FINAL_MIX;
        }

        in_params.final_mix_distance = distance_to_final_mix;
    }
}

void MixContext::CalcMixBufferOffset() {
    s32 offset{};
    for (std::size_t i = 0; i < info_count; i++) {
        auto& info = GetSortedInfo(i);
        auto& in_params = info.GetInParams();
        if (in_params.in_use) {
            // Only update if in use
            in_params.buffer_offset = offset;
            offset += in_params.buffer_count;
        }
    }
}

void MixContext::SortInfo() {
    // Get the distance to the final mix
    UpdateDistancesFromFinalMix();

    // Sort based on the distance to the final mix
    std::sort(sorted_info.begin(), sorted_info.end(),
              [](const ServerMixInfo* lhs, const ServerMixInfo* rhs) {
                  return lhs->GetInParams().final_mix_distance >
                         rhs->GetInParams().final_mix_distance;
              });

    // Calculate the mix buffer offset
    CalcMixBufferOffset();
}

bool MixContext::TsortInfo(SplitterContext& splitter_context) {
    // If we're not using mixes, just calculate the mix buffer offset
    if (!splitter_context.UsingSplitter()) {
        CalcMixBufferOffset();
        return true;
    }
    // Sort our node states
    if (!node_states.Tsort(edge_matrix)) {
        return false;
    }

    // Get our sorted list
    const auto sorted_list = node_states.GetIndexList();
    std::size_t info_id{};
    for (auto itr = sorted_list.rbegin(); itr != sorted_list.rend(); ++itr) {
        // Set our sorted info
        sorted_info[info_id++] = &GetInfo(*itr);
    }

    // Calculate the mix buffer offset
    CalcMixBufferOffset();
    return true;
}

std::size_t MixContext::GetCount() const {
    return info_count;
}

ServerMixInfo& MixContext::GetInfo(std::size_t i) {
    ASSERT(i < info_count);
    return infos.at(i);
}

const ServerMixInfo& MixContext::GetInfo(std::size_t i) const {
    ASSERT(i < info_count);
    return infos.at(i);
}

ServerMixInfo& MixContext::GetSortedInfo(std::size_t i) {
    ASSERT(i < info_count);
    return *sorted_info.at(i);
}

const ServerMixInfo& MixContext::GetSortedInfo(std::size_t i) const {
    ASSERT(i < info_count);
    return *sorted_info.at(i);
}

ServerMixInfo& MixContext::GetFinalMixInfo() {
    return infos.at(AudioCommon::FINAL_MIX);
}

const ServerMixInfo& MixContext::GetFinalMixInfo() const {
    return infos.at(AudioCommon::FINAL_MIX);
}

EdgeMatrix& MixContext::GetEdgeMatrix() {
    return edge_matrix;
}

const EdgeMatrix& MixContext::GetEdgeMatrix() const {
    return edge_matrix;
}

ServerMixInfo::ServerMixInfo() {
    Cleanup();
}
ServerMixInfo::~ServerMixInfo() = default;

const ServerMixInfo::InParams& ServerMixInfo::GetInParams() const {
    return in_params;
}

ServerMixInfo::InParams& ServerMixInfo::GetInParams() {
    return in_params;
}

bool ServerMixInfo::Update(EdgeMatrix& edge_matrix, const MixInfo::InParams& mix_in,
                           BehaviorInfo& behavior_info, SplitterContext& splitter_context,
                           EffectContext& effect_context) {
    in_params.volume = mix_in.volume;
    in_params.sample_rate = mix_in.sample_rate;
    in_params.buffer_count = mix_in.buffer_count;
    in_params.in_use = mix_in.in_use;
    in_params.mix_id = mix_in.mix_id;
    in_params.node_id = mix_in.node_id;
    for (std::size_t i = 0; i < mix_in.mix_volume.size(); i++) {
        std::copy(mix_in.mix_volume[i].begin(), mix_in.mix_volume[i].end(),
                  in_params.mix_volume[i].begin());
    }

    bool require_sort = false;

    if (behavior_info.IsSplitterSupported()) {
        require_sort = UpdateConnection(edge_matrix, mix_in, splitter_context);
    } else {
        in_params.dest_mix_id = mix_in.dest_mix_id;
        in_params.splitter_id = AudioCommon::NO_SPLITTER;
    }

    ResetEffectProcessingOrder();
    const auto effect_count = effect_context.GetCount();
    for (std::size_t i = 0; i < effect_count; i++) {
        auto* effect_info = effect_context.GetInfo(i);
        if (effect_info->GetMixID() == in_params.mix_id) {
            effect_processing_order[effect_info->GetProcessingOrder()] = static_cast<s32>(i);
        }
    }

    // TODO(ogniK): Update effect processing order
    return require_sort;
}

bool ServerMixInfo::HasAnyConnection() const {
    return in_params.splitter_id != AudioCommon::NO_SPLITTER ||
           in_params.mix_id != AudioCommon::NO_MIX;
}

void ServerMixInfo::Cleanup() {
    in_params.volume = 0.0f;
    in_params.sample_rate = 0;
    in_params.buffer_count = 0;
    in_params.in_use = false;
    in_params.mix_id = AudioCommon::NO_MIX;
    in_params.node_id = 0;
    in_params.buffer_offset = 0;
    in_params.dest_mix_id = AudioCommon::NO_MIX;
    in_params.splitter_id = AudioCommon::NO_SPLITTER;
    std::memset(in_params.mix_volume.data(), 0, sizeof(float) * in_params.mix_volume.size());
}

void ServerMixInfo::SetEffectCount(std::size_t count) {
    effect_processing_order.resize(count);
    ResetEffectProcessingOrder();
}

void ServerMixInfo::ResetEffectProcessingOrder() {
    for (auto& order : effect_processing_order) {
        order = AudioCommon::NO_EFFECT_ORDER;
    }
}

s32 ServerMixInfo::GetEffectOrder(std::size_t i) const {
    return effect_processing_order.at(i);
}

bool ServerMixInfo::UpdateConnection(EdgeMatrix& edge_matrix, const MixInfo::InParams& mix_in,
                                     SplitterContext& splitter_context) {
    // Mixes are identical
    if (in_params.dest_mix_id == mix_in.dest_mix_id &&
        in_params.splitter_id == mix_in.splitter_id &&
        ((in_params.splitter_id == AudioCommon::NO_SPLITTER) ||
         !splitter_context.GetInfo(in_params.splitter_id).HasNewConnection())) {
        return false;
    }
    // Remove current edges for mix id
    edge_matrix.RemoveEdges(in_params.mix_id);
    if (mix_in.dest_mix_id != AudioCommon::NO_MIX) {
        // If we have a valid destination mix id, set our edge matrix
        edge_matrix.Connect(in_params.mix_id, mix_in.dest_mix_id);
    } else if (mix_in.splitter_id != AudioCommon::NO_SPLITTER) {
        // Recurse our splitter linked and set our edges
        auto& splitter_info = splitter_context.GetInfo(mix_in.splitter_id);
        const auto length = splitter_info.GetLength();
        for (s32 i = 0; i < length; i++) {
            const auto* splitter_destination =
                splitter_context.GetDestinationData(mix_in.splitter_id, i);
            if (splitter_destination == nullptr) {
                continue;
            }
            if (splitter_destination->ValidMixId()) {
                edge_matrix.Connect(in_params.mix_id, splitter_destination->GetMixId());
            }
        }
    }
    in_params.dest_mix_id = mix_in.dest_mix_id;
    in_params.splitter_id = mix_in.splitter_id;
    return true;
}

} // namespace AudioCore

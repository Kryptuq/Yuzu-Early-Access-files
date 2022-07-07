// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/nodes/node_states.h"
#include "common/logging/log.h"

namespace AudioCore::AudioRenderer {

void NodeStates::Initialize(std::span<u8> buffer_, [[maybe_unused]] const u64 node_buffer_size,
                            const u32 count) {
    u64 num_blocks{Common::AlignUp(count, 0x40) / sizeof(u64)};
    u64 offset{0};

    node_count = count;

    nodes_found.buffer.resize(count);
    nodes_found.size = count;
    nodes_found.reset();

    offset += num_blocks;

    nodes_complete.buffer.resize(count);
    nodes_complete.size = count;
    nodes_complete.reset();

    offset += num_blocks;

    results = {reinterpret_cast<u32*>(&buffer_[offset]), count};

    offset += count * sizeof(u32);

    stack.stack = {reinterpret_cast<u32*>(&buffer_[offset]), count * count};
    stack.size = count * count;
    stack.unk_10 = count * count;

    offset += count * count * sizeof(u32);
}

bool NodeStates::Tsort(const EdgeMatrix& edge_matrix) {
    return DepthFirstSearch(edge_matrix, stack);
}

bool NodeStates::DepthFirstSearch(const EdgeMatrix& edge_matrix, Stack& stack_) {
    ResetState();

    for (u32 node_id = 0; node_id < node_count; node_id++) {
        if (GetState(node_id) == SearchState::Unknown) {
            stack_.push(node_id);
        }

        while (stack_.Count() > 0) {
            auto current_node{stack_.top()};
            switch (GetState(current_node)) {
            case SearchState::Unknown:
                SetState(current_node, SearchState::Found);
                break;
            case SearchState::Found:
                SetState(current_node, SearchState::Complete);
                PushTsortResult(current_node);
                stack_.pop();
                continue;
            case SearchState::Complete:
                stack_.pop();
                continue;
            }

            const auto edge_count{edge_matrix.GetNodeCount()};
            for (u32 edge_id = 0; edge_id < edge_count; edge_id++) {
                if (!edge_matrix.Connected(current_node, edge_id)) {
                    continue;
                }

                switch (GetState(edge_id)) {
                case SearchState::Unknown:
                    stack_.push(edge_id);
                    break;
                case SearchState::Found:
                    LOG_ERROR(Service_Audio,
                              "Cycle detected in the node graph, graph is not a DAG! "
                              "Bailing to avoid an infinite loop");
                    ResetState();
                    return false;
                case SearchState::Complete:
                    break;
                }
            }
        }
    }

    return true;
}

NodeStates::SearchState NodeStates::GetState(const u32 id) const {
    if (nodes_found.buffer[id]) {
        return SearchState::Found;
    } else if (nodes_complete.buffer[id]) {
        return SearchState::Complete;
    }
    return SearchState::Unknown;
}

void NodeStates::PushTsortResult(const u32 id) {
    results[result_pos++] = id;
}

void NodeStates::SetState(const u32 id, const SearchState state) {
    switch (state) {
    case SearchState::Complete:
        nodes_found.buffer[id] = false;
        nodes_complete.buffer[id] = true;
        break;
    case SearchState::Found:
        nodes_found.buffer[id] = true;
        nodes_complete.buffer[id] = false;
        break;
    case SearchState::Unknown:
        nodes_found.buffer[id] = false;
        nodes_complete.buffer[id] = false;
        break;
    default:
        LOG_ERROR(Service_Audio, "Unknown node SearchState {}", static_cast<u32>(state));
        break;
    }
}

void NodeStates::ResetState() {
    nodes_found.reset();
    nodes_complete.reset();
    std::fill(results.begin(), results.end(), -1);
    result_pos = 0;
}

u32 NodeStates::GetNodeCount() const {
    return node_count;
}

std::vector<s32> NodeStates::GetSortedResuls() const {
    return {results.rbegin(), results.rbegin() + result_pos};
}

} // namespace AudioCore::AudioRenderer

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <vector>

#include "audio_core/renderer/nodes/edge_matrix.h"
#include "common/alignment.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
/**
 * Graph utility functions for sorting and getting results from the DAG.
 */
class NodeStates {
    /**
     * State of a node in the depth first search.
     */
    enum class SearchState {
        Unknown,
        Found,
        Complete,
    };

    /**
     * Stack used for a depth first search.
     */
    struct Stack {
        /**
         * Calculate the workbuffer size required for this stack.
         *
         * @param count - Maximum number of nodes for the stack.
         * @return Required buffer size.
         */
        static u32 CalcBufferSize(u32 count) {
            return count * sizeof(u32);
        }

        /**
         * Reset the stack back to default.
         *
         * @param buffer_ - The new buffer to use.
         * @param size_   - The size of the new buffer.
         */
        void Reset(u32* buffer_, u32 size_) {
            stack = {buffer_, size_};
            size = size_;
            pos = 0;
            unk_10 = size_;
        }

        /**
         * Get the current stack position.
         *
         * @return The current stack position.
         */
        u32 Count() {
            return pos;
        }

        /**
         * Push a new node to the stack.
         *
         * @param data - The node to push.
         */
        void push(u32 data) {
            stack[pos++] = data;
        }

        /**
         * Pop a node from the stack.
         *
         * @return The node on the top of the stack.
         */
        u32 pop() {
            return stack[--pos];
        }

        /**
         * Get the top of the stack without popping.
         *
         * @return The node on the top of the stack.
         */
        u32 top() {
            return stack[pos - 1];
        }

        /// Buffer for the stack
        std::span<u32> stack{};
        /// Size of the stack buffer
        u32 size{};
        /// Current stack position
        u32 pos{};
        /// Unknown
        u32 unk_10{};
    };

public:
    /**
     * Calculate the workbuffer size required for the node states.
     *
     * @param count - The number of nodes.
     * @return The required workbuffer size.
     */
    static u64 GetWorkBufferSize(u32 count) {
        return (Common::AlignUp(count, 0x40) / sizeof(u64)) * 2 + count * sizeof(BitArray) +
               count * Stack::CalcBufferSize(count);
    }

    /**
     * Initialize the node states.
     *
     * @param buffer           - The workbuffer to use. Unused.
     * @param node_buffer_size - The size of the workbuffer. Unused.
     * @param count            - The number of nodes in the graph.
     */
    void Initialize(std::span<u8> nodes, u64 node_buffer_size, u32 count);

    /**
     * Sort the graph. Only calls DepthFirstSearch.
     *
     * @param edge_matrix - The edge matrix used to hold the connections between nodes.
     * @return True if the sort was successful, otherwise false.
     */
    bool Tsort(const EdgeMatrix& edge_matrix);

    /**
     * Sort the graph via depth first search.
     *
     * @param edge_matrix - The edge matrix used to hold the connections between nodes.
     * @param stack       - The stack used for pushing and popping nodes.
     * @return True if the sort was successful, otherwise false.
     */
    bool DepthFirstSearch(const EdgeMatrix& edge_matrix, Stack& stack);

    /**
     * Get the search state of a given node.
     *
     * @param id - The node id to check.
     * @return The node's search state. See SearchState
     */
    SearchState GetState(u32 id) const;

    /**
     * Push a node id to the results buffer when found in the DFS.
     *
     * @param id - The node id to push.
     */
    void PushTsortResult(u32 id);

    /**
     * Set the state of a node.
     *
     * @param id - The node id to alter.
     * @param state - The new search state.
     */
    void SetState(u32 id, SearchState state);

    /**
     * Reset the nodes found, complete and the results.
     */
    void ResetState();

    /**
     * Get the number of nodes in the graph.
     *
     * @return The number of nodes.
     */
    u32 GetNodeCount() const;

    /**
     * Get the sorted results from the DFS.
     *
     * @return Vector of nodes in reverse order.
     */
    std::vector<s32> GetSortedResuls() const;

private:
    /// Number of nodes in the graph
    u32 node_count{};
    /// Position in results buffer
    u32 result_pos{};
    /// List of nodes found
    BitArray nodes_found{};
    /// List of nodes completed
    BitArray nodes_complete{};
    /// List of results from the depth first search
    std::span<u32> results{};
    /// Stack used during the depth first search
    Stack stack{};
};

} // namespace AudioCore::AudioRenderer

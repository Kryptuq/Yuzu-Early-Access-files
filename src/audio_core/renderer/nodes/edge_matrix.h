// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "audio_core/renderer/nodes/bit_array.h"
#include "common/alignment.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
/**
 * An edge matrix, holding the connections for each node to every other node in the graph.
 */
class EdgeMatrix {
public:
    /**
     * Calculate the size required for its workbuffer.
     *
     * @param count - The number of nodes in the graph.
     * @return The required workbuffer size.
     */
    static u64 GetWorkBufferSize(u32 count) {
        return Common::AlignUp(count * count, 0x40) / sizeof(u64);
    }

    /**
     * Initialize this edge matrix.
     *
     * @param buffer           - The workbuffer to use. Unused.
     * @param node_buffer_size - The size of the workbuffer. Unused.
     * @param count            - The number of nodes in the graph.
     */
    void Initialize(std::span<u8> buffer, u64 node_buffer_size, u32 count);

    /**
     * Check if a node is connected to another.
     *
     * @param id             - The node id to check.
     * @param destination_id - Node id to check connection with.
     */
    bool Connected(u32 id, u32 destination_id) const;

    /**
     * Connect a node to another.
     *
     * @param id             - The node id to connect.
     * @param destination_id - Destination to connect it to.
     */
    void Connect(u32 id, u32 destination_id);

    /**
     * Disconnect a node from another.
     *
     * @param id             - The node id to disconnect.
     * @param destination_id - Destination to disconnect it from.
     */
    void Disconnect(u32 id, u32 destination_id);

    /**
     * Remove all connections for a given node.
     *
     * @param id - The node id to disconnect.
     */
    void RemoveEdges(u32 id);

    /**
     * Get the number of nodes in the graph.
     *
     * @return Number of nodes.
     */
    u32 GetNodeCount() const;

private:
    /// Edges for the current graph
    BitArray edges;
    /// Number of nodes (not edges) in the graph
    u32 count;
};

} // namespace AudioCore::AudioRenderer

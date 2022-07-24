// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/nodes/edge_matrix.h"

namespace AudioCore::AudioRenderer {

void EdgeMatrix::Initialize([[maybe_unused]] std::span<u8> buffer,
                            [[maybe_unused]] const u64 node_buffer_size, const u32 count_) {
    count = count_;
    edges.buffer.resize(count_ * count_);
    edges.size = count_ * count_;
    edges.reset();
}

bool EdgeMatrix::Connected(const u32 id, const u32 destination_id) const {
    return edges.buffer[count * id + destination_id];
}

void EdgeMatrix::Connect(const u32 id, const u32 destination_id) {
    edges.buffer[count * id + destination_id] = true;
}

void EdgeMatrix::Disconnect(const u32 id, const u32 destination_id) {
    edges.buffer[count * id + destination_id] = false;
}

void EdgeMatrix::RemoveEdges(const u32 id) {
    for (u32 dest = 0; dest < count; dest++) {
        Disconnect(id, dest);
    }
}

u32 EdgeMatrix::GetNodeCount() const {
    return count;
}

} // namespace AudioCore::AudioRenderer

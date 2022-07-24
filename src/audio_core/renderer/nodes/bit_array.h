// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>

#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
/**
 * Represents an array of bits used for nodes and edges for the mixing graph.
 */
struct BitArray {
    void reset() {
        buffer.assign(buffer.size(), false);
    }

    /// Bits
    std::vector<bool> buffer{};
    /// Size of the buffer
    u32 size{};
};

} // namespace AudioCore::AudioRenderer

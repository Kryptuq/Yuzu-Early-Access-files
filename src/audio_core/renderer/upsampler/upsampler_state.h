// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "common/common_types.h"
#include "common/fixed_point.h"

namespace AudioCore::AudioRenderer {
/**
 * Upsampling state used by the AudioRenderer across calls.
 */
struct UpsamplerState {
    static constexpr u16 HistorySize = 20;

    /// Source data to target data ratio. E.g 48'000/32'000 = 1.5
    Common::FixedPoint<16, 16> ratio;
    /// Sample history
    std::array<Common::FixedPoint<24, 8>, HistorySize> history;
    /// Size of the sinc coefficient window
    u16 window_size;
    /// Read index for the history
    u16 history_output_index;
    /// Write index for the history
    u16 history_input_index;
    /// Start offset within the history, fixed to 0
    u16 history_start_index;
    /// Ebd offset within the history, fixed to HistorySize
    u16 history_end_index;
    /// Is this state initialized?
    bool initialized;
    /// Index of the current sample.
    /// E.g 16K -> 48K has a ratio of 3, so this will be 0-2.
    /// See the Upsample command in the AudioRenderer for more information.
    u8 sample_index;
};

} // namespace AudioCore::AudioRenderer

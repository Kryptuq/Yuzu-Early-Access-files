// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <SoundTouch.h>
#include "common/common_types.h"

namespace AudioCore {

class TimeStretcher {
public:
    TimeStretcher(u32 sample_rate, u32 channel_count);

    /// @param in       Input sample buffer
    /// @param num_in   Number of input frames in `in`
    /// @param out      Output sample buffer
    /// @param num_out  Desired number of output frames in `out`
    /// @returns Actual number of frames written to `out`
    std::size_t Process(const s16* in, std::size_t num_in, s16* out, std::size_t num_out);

    void Clear();

    void Flush();

private:
    u32 m_sample_rate;
    soundtouch::SoundTouch m_sound_touch;
    double m_stretch_ratio = 1.0;
};

} // namespace AudioCore

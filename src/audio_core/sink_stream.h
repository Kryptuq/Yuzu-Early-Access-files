// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>

#include "common/common_types.h"

namespace AudioCore {

/**
 * Accepts samples in stereo signed PCM16 format to be output. Sinks *do not* handle resampling and
 * expect the correct sample rate. They are dumb outputs.
 */
class SinkStream {
public:
    virtual ~SinkStream() = default;

    /**
     * Feed stereo samples to sink.
     * @param num_channels Number of channels used.
     * @param samples Samples in interleaved stereo PCM16 format.
     */
    virtual void EnqueueSamples(u32 num_channels, const std::vector<s16>& samples) = 0;

    virtual std::size_t SamplesInQueue(u32 num_channels) const = 0;

    virtual void Flush() = 0;
};

using SinkStreamPtr = std::unique_ptr<SinkStream>;

} // namespace AudioCore

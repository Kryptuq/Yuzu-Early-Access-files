// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>

#include "common/common_types.h"

namespace AudioCore {

/**
 * Represents a buffer of audio samples to be played in an audio stream
 */
class Buffer {
public:
    using Tag = u64;

    Buffer(Tag tag_, std::vector<s16>&& samples_) : tag{tag_}, samples{std::move(samples_)} {}

    /// Returns the raw audio data for the buffer
    std::vector<s16>& GetSamples() {
        return samples;
    }

    /// Returns the raw audio data for the buffer
    const std::vector<s16>& GetSamples() const {
        return samples;
    }

    /// Returns the buffer tag, this is provided by the game to the audout service
    Tag GetTag() const {
        return tag;
    }

private:
    Tag tag;
    std::vector<s16> samples;
};

using BufferPtr = std::shared_ptr<Buffer>;

} // namespace AudioCore

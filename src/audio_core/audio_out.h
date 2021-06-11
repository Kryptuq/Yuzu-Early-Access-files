// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "audio_core/buffer.h"
#include "audio_core/sink.h"
#include "audio_core/stream.h"
#include "common/common_types.h"

namespace Core::Timing {
class CoreTiming;
}

namespace AudioCore {

/**
 * Represents an audio playback interface, used to open and play audio streams
 */
class AudioOut {
public:
    /// Opens a new audio stream
    StreamPtr OpenStream(Core::Timing::CoreTiming& core_timing, u32 sample_rate, u32 num_channels,
                         std::string&& name, Stream::ReleaseCallback&& release_callback);

    /// Returns a vector of recently released buffers specified by tag for the specified stream
    std::vector<Buffer::Tag> GetTagsAndReleaseBuffers(StreamPtr stream, std::size_t max_count);

    /// Returns a vector of all recently released buffers specified by tag for the specified stream
    std::vector<Buffer::Tag> GetTagsAndReleaseBuffers(StreamPtr stream);

    /// Starts an audio stream for playback
    void StartStream(StreamPtr stream);

    /// Stops an audio stream that is currently playing
    void StopStream(StreamPtr stream);

    /// Queues a buffer into the specified audio stream, returns true on success
    bool QueueBuffer(StreamPtr stream, Buffer::Tag tag, std::vector<s16>&& data);

private:
    SinkPtr sink;
};

} // namespace AudioCore

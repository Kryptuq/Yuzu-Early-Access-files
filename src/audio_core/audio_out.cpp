// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "audio_core/audio_out.h"
#include "audio_core/sink.h"
#include "audio_core/sink_details.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/settings.h"

namespace AudioCore {

/// Returns the stream format from the specified number of channels
static Stream::Format ChannelsToStreamFormat(u32 num_channels) {
    switch (num_channels) {
    case 1:
        return Stream::Format::Mono16;
    case 2:
        return Stream::Format::Stereo16;
    case 6:
        return Stream::Format::Multi51Channel16;
    }

    UNIMPLEMENTED_MSG("Unimplemented num_channels={}", num_channels);
    return {};
}

StreamPtr AudioOut::OpenStream(Core::Timing::CoreTiming& core_timing, u32 sample_rate,
                               u32 num_channels, std::string&& name,
                               Stream::ReleaseCallback&& release_callback) {
    if (!sink) {
        sink = CreateSinkFromID(Settings::values.sink_id.GetValue(),
                                Settings::values.audio_device_id.GetValue());
    }

    return std::make_shared<Stream>(
        core_timing, sample_rate, ChannelsToStreamFormat(num_channels), std::move(release_callback),
        sink->AcquireSinkStream(sample_rate, num_channels, name), std::move(name));
}

std::vector<Buffer::Tag> AudioOut::GetTagsAndReleaseBuffers(StreamPtr stream,
                                                            std::size_t max_count) {
    return stream->GetTagsAndReleaseBuffers(max_count);
}

std::vector<Buffer::Tag> AudioOut::GetTagsAndReleaseBuffers(StreamPtr stream) {
    return stream->GetTagsAndReleaseBuffers();
}

void AudioOut::StartStream(StreamPtr stream) {
    stream->Play();
}

void AudioOut::StopStream(StreamPtr stream) {
    stream->Stop();
}

bool AudioOut::QueueBuffer(StreamPtr stream, Buffer::Tag tag, std::vector<s16>&& data) {
    return stream->QueueBuffer(std::make_shared<Buffer>(tag, std::move(data)));
}

} // namespace AudioCore

// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <vector>

#include "audio_core/sink.h"

namespace AudioCore {

class SDLSink final : public Sink {
public:
    explicit SDLSink(std::string_view device_id);
    ~SDLSink() override;

    SinkStream& AcquireSinkStream(u32 sample_rate, u32 num_channels,
                                  const std::string& name) override;

private:
    std::string output_device;
    std::vector<SinkStreamPtr> sink_streams;
};

std::vector<std::string> ListSDLSinkDevices();

} // namespace AudioCore

// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <vector>

#include <cubeb/cubeb.h>

#include "audio_core/sink.h"

namespace AudioCore {

class CubebSink final : public Sink {
public:
    explicit CubebSink(std::string_view device_id);
    ~CubebSink() override;

    SinkStream& AcquireSinkStream(u32 sample_rate, u32 num_channels,
                                  const std::string& name) override;

private:
    cubeb* ctx{};
    cubeb_devid output_device{};
    std::vector<SinkStreamPtr> sink_streams;

#ifdef _WIN32
    u32 com_init_result = 0;
#endif
};

std::vector<std::string> ListCubebSinkDevices();

} // namespace AudioCore

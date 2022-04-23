// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "audio_core/sink/sink_details.h"
#ifdef HAVE_CUBEB
#include "audio_core/sink/cubeb_sink.h"
#endif
#ifdef HAVE_SDL2
#include "audio_core/sink/sdl2_sink.h"
#endif
#include "audio_core/sink/null_sink.h"
#include "common/logging/log.h"

namespace AudioCore::Sink {
namespace {
struct SinkDetails {
    using FactoryFn = std::unique_ptr<Sink> (*)(std::string_view);
    using ListDevicesFn = std::vector<std::string> (*)(bool);

    /// Name for this sink.
    const char* id;
    /// A method to call to construct an instance of this type of sink.
    FactoryFn factory;
    /// A method to call to list available devices.
    ListDevicesFn list_devices;
};

// sink_details is ordered in terms of desirability, with the best choice at the top.
constexpr SinkDetails sink_details[] = {
#ifdef HAVE_CUBEB
    SinkDetails{"cubeb",
                [](std::string_view device_id) -> std::unique_ptr<Sink> {
                    return std::make_unique<CubebSink>(device_id);
                },
                &ListCubebSinkDevices},
#endif
#ifdef HAVE_SDL2
    SinkDetails{"sdl2",
                [](std::string_view device_id) -> std::unique_ptr<Sink> {
                    return std::make_unique<SDLSink>(device_id);
                },
                &ListSDLSinkDevices},
#endif
    SinkDetails{"null",
                [](std::string_view device_id) -> std::unique_ptr<Sink> {
                    return std::make_unique<NullSink>(device_id);
                },
                [](bool capture) { return std::vector<std::string>{"null"}; }},
};

const SinkDetails& GetOutputSinkDetails(std::string_view sink_id) {
    auto iter =
        std::find_if(std::begin(sink_details), std::end(sink_details),
                     [sink_id](const auto& sink_detail) { return sink_detail.id == sink_id; });

    if (sink_id == "auto" || iter == std::end(sink_details)) {
        if (sink_id != "auto") {
            LOG_ERROR(Audio, "Invalid sink_id {}", sink_id);
        }
        // Auto-select.
        // sink_details is ordered in terms of desirability, with the best choice at the front.
        iter = std::begin(sink_details);
    }

    return *iter;
}
} // Anonymous namespace

std::vector<const char*> GetSinkIDs() {
    std::vector<const char*> sink_ids(std::size(sink_details));

    std::transform(std::begin(sink_details), std::end(sink_details), std::begin(sink_ids),
                   [](const auto& sink) { return sink.id; });

    return sink_ids;
}

std::vector<std::string> GetDeviceListForSink(std::string_view sink_id, bool capture) {
    return GetOutputSinkDetails(sink_id).list_devices(capture);
}

std::unique_ptr<Sink> CreateSinkFromID(std::string_view sink_id, std::string_view device_id) {
    return GetOutputSinkDetails(sink_id).factory(device_id);
}

} // namespace AudioCore::Sink

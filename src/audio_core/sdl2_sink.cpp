// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <atomic>
#include <cstring>
#include "audio_core/sdl2_sink.h"
#include "audio_core/stream.h"
#include "common/assert.h"
#include "common/logging/log.h"
//#include "common/settings.h"

// Ignore -Wimplicit-fallthrough due to https://github.com/libsdl-org/SDL/issues/4307
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#endif
#include <SDL.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace AudioCore {

class SDLSinkStream final : public SinkStream {
public:
    SDLSinkStream(u32 sample_rate, u32 num_channels_, const std::string& output_device)
        : num_channels{std::min(num_channels_, 6u)} {

        SDL_AudioSpec spec;
        spec.freq = sample_rate;
        spec.channels = static_cast<u8>(num_channels);
        spec.format = AUDIO_S16SYS;
        spec.samples = 4096;
        spec.callback = nullptr;

        SDL_AudioSpec obtained;
        if (output_device.empty()) {
            dev = SDL_OpenAudioDevice(nullptr, 0, &spec, &obtained, 0);
        } else {
            dev = SDL_OpenAudioDevice(output_device.c_str(), 0, &spec, &obtained, 0);
        }

        if (dev == 0) {
            LOG_CRITICAL(Audio_Sink, "Error opening sdl audio device: {}", SDL_GetError());
            return;
        }

        SDL_PauseAudioDevice(dev, 0);
    }

    ~SDLSinkStream() override {
        if (dev == 0) {
            return;
        }

        SDL_CloseAudioDevice(dev);
    }

    void EnqueueSamples(u32 source_num_channels, const std::vector<s16>& samples) override {
        if (source_num_channels > num_channels) {
            // Downsample 6 channels to 2
            ASSERT_MSG(source_num_channels == 6, "Channel count must be 6");

            std::vector<s16> buf;
            buf.reserve(samples.size() * num_channels / source_num_channels);
            for (std::size_t i = 0; i < samples.size(); i += source_num_channels) {
                // Downmixing implementation taken from the ATSC standard
                const s16 left{samples[i + 0]};
                const s16 right{samples[i + 1]};
                const s16 center{samples[i + 2]};
                const s16 surround_left{samples[i + 4]};
                const s16 surround_right{samples[i + 5]};
                // Not used in the ATSC reference implementation
                [[maybe_unused]] const s16 low_frequency_effects{samples[i + 3]};

                constexpr s32 clev{707}; // center mixing level coefficient
                constexpr s32 slev{707}; // surround mixing level coefficient

                buf.push_back(static_cast<s16>(left + (clev * center / 1000) +
                                               (slev * surround_left / 1000)));
                buf.push_back(static_cast<s16>(right + (clev * center / 1000) +
                                               (slev * surround_right / 1000)));
            }
            int ret = SDL_QueueAudio(dev, static_cast<const void*>(buf.data()),
                                     static_cast<u32>(buf.size() * sizeof(s16)));
            if (ret < 0)
                LOG_WARNING(Audio_Sink, "Could not queue audio buffer: {}", SDL_GetError());
            return;
        }

        int ret = SDL_QueueAudio(dev, static_cast<const void*>(samples.data()),
                                 static_cast<u32>(samples.size() * sizeof(s16)));
        if (ret < 0)
            LOG_WARNING(Audio_Sink, "Could not queue audio buffer: {}", SDL_GetError());
    }

    std::size_t SamplesInQueue(u32 channel_count) const override {
        if (dev == 0)
            return 0;

        return SDL_GetQueuedAudioSize(dev) / (channel_count * sizeof(s16));
    }

    void Flush() override {
        should_flush = true;
    }

    u32 GetNumChannels() const {
        return num_channels;
    }

private:
    SDL_AudioDeviceID dev = 0;
    u32 num_channels{};
    std::atomic<bool> should_flush{};
};

SDLSink::SDLSink(std::string_view target_device_name) {
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            LOG_CRITICAL(Audio_Sink, "SDL_InitSubSystem audio failed: {}", SDL_GetError());
            return;
        }
    }

    if (target_device_name != auto_device_name && !target_device_name.empty()) {
        output_device = target_device_name;
    } else {
        output_device.clear();
    }
}

SDLSink::~SDLSink() = default;

SinkStream& SDLSink::AcquireSinkStream(u32 sample_rate, u32 num_channels, const std::string&) {
    sink_streams.push_back(
        std::make_unique<SDLSinkStream>(sample_rate, num_channels, output_device));
    return *sink_streams.back();
}

std::vector<std::string> ListSDLSinkDevices() {
    std::vector<std::string> device_list;

    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            LOG_CRITICAL(Audio_Sink, "SDL_InitSubSystem audio failed: {}", SDL_GetError());
            return {};
        }
    }

    const int device_count = SDL_GetNumAudioDevices(0);
    for (int i = 0; i < device_count; ++i) {
        device_list.emplace_back(SDL_GetAudioDeviceName(i, 0));
    }

    return device_list;
}

} // namespace AudioCore

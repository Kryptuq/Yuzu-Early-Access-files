// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <atomic>
#include <cstring>
#include "audio_core/cubeb_sink.h"
#include "audio_core/stream.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/ring_buffer.h"
#include "common/settings.h"

#ifdef _WIN32
#include <objbase.h>
#endif

namespace AudioCore {

class CubebSinkStream final : public SinkStream {
public:
    CubebSinkStream(cubeb* ctx_, u32 sample_rate, u32 num_channels_, cubeb_devid output_device,
                    const std::string& name)
        : ctx{ctx_}, num_channels{std::min(num_channels_, 6u)} {

        cubeb_stream_params params{};
        params.rate = sample_rate;
        params.channels = num_channels;
        params.format = CUBEB_SAMPLE_S16NE;
        params.prefs = CUBEB_STREAM_PREF_PERSIST;
        switch (num_channels) {
        case 1:
            params.layout = CUBEB_LAYOUT_MONO;
            break;
        case 2:
            params.layout = CUBEB_LAYOUT_STEREO;
            break;
        case 6:
            params.layout = CUBEB_LAYOUT_3F2_LFE;
            break;
        }

        u32 minimum_latency{};
        if (cubeb_get_min_latency(ctx, &params, &minimum_latency) != CUBEB_OK) {
            LOG_CRITICAL(Audio_Sink, "Error getting minimum latency");
        }

        if (cubeb_stream_init(ctx, &stream_backend, name.c_str(), nullptr, nullptr, output_device,
                              &params, std::max(512u, minimum_latency),
                              &CubebSinkStream::DataCallback, &CubebSinkStream::StateCallback,
                              this) != CUBEB_OK) {
            LOG_CRITICAL(Audio_Sink, "Error initializing cubeb stream");
            return;
        }

        if (cubeb_stream_start(stream_backend) != CUBEB_OK) {
            LOG_CRITICAL(Audio_Sink, "Error starting cubeb stream");
            return;
        }
    }

    ~CubebSinkStream() override {
        if (!ctx) {
            return;
        }

        if (cubeb_stream_stop(stream_backend) != CUBEB_OK) {
            LOG_CRITICAL(Audio_Sink, "Error stopping cubeb stream");
        }

        cubeb_stream_destroy(stream_backend);
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
            queue.Push(buf);
            return;
        }

        queue.Push(samples);
    }

    std::size_t SamplesInQueue(u32 channel_count) const override {
        if (!ctx)
            return 0;

        return queue.Size() / channel_count;
    }

    void Flush() override {
        should_flush = true;
    }

    u32 GetNumChannels() const {
        return num_channels;
    }

private:
    std::vector<std::string> device_list;

    cubeb* ctx{};
    cubeb_stream* stream_backend{};
    u32 num_channels{};

    Common::RingBuffer<s16, 0x10000> queue;
    std::array<s16, 2> last_frame{};
    std::atomic<bool> should_flush{};

    static long DataCallback(cubeb_stream* stream, void* user_data, const void* input_buffer,
                             void* output_buffer, long num_frames);
    static void StateCallback(cubeb_stream* stream, void* user_data, cubeb_state state);
};

CubebSink::CubebSink(std::string_view target_device_name) {
    // Cubeb requires COM to be initialized on the thread calling cubeb_init on Windows
#ifdef _WIN32
    com_init_result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

    if (cubeb_init(&ctx, "yuzu", nullptr) != CUBEB_OK) {
        LOG_CRITICAL(Audio_Sink, "cubeb_init failed");
        return;
    }

    if (target_device_name != auto_device_name && !target_device_name.empty()) {
        cubeb_device_collection collection;
        if (cubeb_enumerate_devices(ctx, CUBEB_DEVICE_TYPE_OUTPUT, &collection) != CUBEB_OK) {
            LOG_WARNING(Audio_Sink, "Audio output device enumeration not supported");
        } else {
            const auto collection_end{collection.device + collection.count};
            const auto device{
                std::find_if(collection.device, collection_end, [&](const cubeb_device_info& info) {
                    return info.friendly_name != nullptr &&
                           target_device_name == info.friendly_name;
                })};
            if (device != collection_end) {
                output_device = device->devid;
            }
            cubeb_device_collection_destroy(ctx, &collection);
        }
    }
}

CubebSink::~CubebSink() {
    if (!ctx) {
        return;
    }

    for (auto& sink_stream : sink_streams) {
        sink_stream.reset();
    }

    cubeb_destroy(ctx);

#ifdef _WIN32
    if (SUCCEEDED(com_init_result)) {
        CoUninitialize();
    }
#endif
}

SinkStream& CubebSink::AcquireSinkStream(u32 sample_rate, u32 num_channels,
                                         const std::string& name) {
    sink_streams.push_back(
        std::make_unique<CubebSinkStream>(ctx, sample_rate, num_channels, output_device, name));
    return *sink_streams.back();
}

long CubebSinkStream::DataCallback([[maybe_unused]] cubeb_stream* stream, void* user_data,
                                   [[maybe_unused]] const void* input_buffer, void* output_buffer,
                                   long num_frames) {
    auto* impl = static_cast<CubebSinkStream*>(user_data);
    auto* buffer = static_cast<u8*>(output_buffer);

    if (!impl) {
        return {};
    }

    const std::size_t num_channels = impl->GetNumChannels();
    const std::size_t samples_to_write = num_channels * num_frames;
    const std::size_t samples_written = impl->queue.Pop(buffer, samples_to_write);

    if (samples_written >= num_channels) {
        std::memcpy(&impl->last_frame[0], buffer + (samples_written - num_channels) * sizeof(s16),
                    num_channels * sizeof(s16));
    }

    // Fill the rest of the frames with last_frame
    for (std::size_t i = samples_written; i < samples_to_write; i += num_channels) {
        std::memcpy(buffer + i * sizeof(s16), &impl->last_frame[0], num_channels * sizeof(s16));
    }

    return num_frames;
}

void CubebSinkStream::StateCallback([[maybe_unused]] cubeb_stream* stream,
                                    [[maybe_unused]] void* user_data,
                                    [[maybe_unused]] cubeb_state state) {}

std::vector<std::string> ListCubebSinkDevices() {
    std::vector<std::string> device_list;
    cubeb* ctx;

    if (cubeb_init(&ctx, "yuzu Device Enumerator", nullptr) != CUBEB_OK) {
        LOG_CRITICAL(Audio_Sink, "cubeb_init failed");
        return {};
    }

    cubeb_device_collection collection;
    if (cubeb_enumerate_devices(ctx, CUBEB_DEVICE_TYPE_OUTPUT, &collection) != CUBEB_OK) {
        LOG_WARNING(Audio_Sink, "Audio output device enumeration not supported");
    } else {
        for (std::size_t i = 0; i < collection.count; i++) {
            const cubeb_device_info& device = collection.device[i];
            if (device.friendly_name) {
                device_list.emplace_back(device.friendly_name);
            }
        }
        cubeb_device_collection_destroy(ctx, &collection);
    }

    cubeb_destroy(ctx);
    return device_list;
}

} // namespace AudioCore

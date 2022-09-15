// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <span>
#include <vector>

#include "audio_core/common/common.h"
#include "audio_core/sink/cubeb_sink.h"
#include "audio_core/sink/sink_stream.h"
#include "common/logging/log.h"
#include "core/core.h"

#ifdef _WIN32
#include <objbase.h>
#undef CreateEvent
#endif

namespace AudioCore::Sink {
/**
 * Cubeb sink stream, responsible for sinking samples to hardware.
 */
class CubebSinkStream final : public SinkStream {
public:
    /**
     * Create a new sink stream.
     *
     * @param ctx_             - Cubeb context to create this stream with.
     * @param device_channels_ - Number of channels supported by the hardware.
     * @param system_channels_ - Number of channels the audio systems expect.
     * @param output_device    - Cubeb output device id.
     * @param input_device     - Cubeb input device id.
     * @param name_            - Name of this stream.
     * @param type_            - Type of this stream.
     * @param system_          - Core system.
     * @param event            - Event used only for audio renderer, signalled on buffer consume.
     */
    CubebSinkStream(cubeb* ctx_, u32 device_channels_, u32 system_channels_,
                    cubeb_devid output_device, cubeb_devid input_device, const std::string& name_,
                    StreamType type_, Core::System& system_)
        : SinkStream(system_, type_), ctx{ctx_} {
#ifdef _WIN32
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif
        name = name_;
        device_channels = device_channels_;
        system_channels = system_channels_;

        cubeb_stream_params params{};
        params.rate = TargetSampleRate;
        params.channels = device_channels;
        params.format = CUBEB_SAMPLE_S16LE;
        params.prefs = CUBEB_STREAM_PREF_NONE;
        switch (params.channels) {
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

        u32 minimum_latency{0};
        const auto latency_error = cubeb_get_min_latency(ctx, &params, &minimum_latency);
        if (latency_error != CUBEB_OK) {
            LOG_CRITICAL(Audio_Sink, "Error getting minimum latency, error: {}", latency_error);
            minimum_latency = 256U;
        }

        minimum_latency = std::max(minimum_latency, 256u);

        LOG_INFO(Service_Audio,
                 "Opening cubeb stream {} type {} with: rate {} channels {} (system channels {}) "
                 "latency {}",
                 name, type, params.rate, params.channels, system_channels, minimum_latency);

        auto init_error{0};
        if (type == StreamType::In) {
            init_error = cubeb_stream_init(ctx, &stream_backend, name.c_str(), input_device,
                                           &params, output_device, nullptr, minimum_latency,
                                           &CubebSinkStream::DataCallback,
                                           &CubebSinkStream::StateCallback, this);
        } else {
            init_error = cubeb_stream_init(ctx, &stream_backend, name.c_str(), input_device,
                                           nullptr, output_device, &params, minimum_latency,
                                           &CubebSinkStream::DataCallback,
                                           &CubebSinkStream::StateCallback, this);
        }

        if (init_error != CUBEB_OK) {
            LOG_CRITICAL(Audio_Sink, "Error initializing cubeb stream, error: {}", init_error);
            return;
        }
    }

    /**
     * Destroy the sink stream.
     */
    ~CubebSinkStream() override {
        LOG_DEBUG(Service_Audio, "Destructing cubeb stream {}", name);

        Unstall();

        if (!ctx) {
            return;
        }

        Finalize();

#ifdef _WIN32
        CoUninitialize();
#endif
    }

    /**
     * Finalize the sink stream.
     */
    void Finalize() override {
        Stop();
        cubeb_stream_destroy(stream_backend);
    }

    /**
     * Start the sink stream.
     *
     * @param resume - Set to true if this is resuming the stream a previously-active stream.
     *                 Default false.
     */
    void Start(bool resume = false) override {
        if (!ctx || !paused) {
            return;
        }

        paused = false;
        if (cubeb_stream_start(stream_backend) != CUBEB_OK) {
            LOG_CRITICAL(Audio_Sink, "Error starting cubeb stream");
        }
    }

    /**
     * Stop the sink stream.
     */
    void Stop() override {
        Unstall();

        if (!ctx || paused) {
            return;
        }

        paused = true;
        if (cubeb_stream_stop(stream_backend) != CUBEB_OK) {
            LOG_CRITICAL(Audio_Sink, "Error stopping cubeb stream");
        }
    }

private:
    /**
     * Main callback from Cubeb. Either expects samples from us (audio render/audio out), or will
     * provide samples to be copied (audio in).
     *
     * @param stream      - Cubeb-specific data about the stream.
     * @param user_data   - Custom data pointer passed along, points to a CubebSinkStream.
     * @param in_buff     - Input buffer to be used if the stream is an input type.
     * @param out_buff    - Output buffer to be used if the stream is an output type.
     * @param num_frames_ - Number of frames of audio in the buffers. Note: Not number of samples.
     */
    static long DataCallback([[maybe_unused]] cubeb_stream* stream, void* user_data,
                             [[maybe_unused]] const void* in_buff, void* out_buff,
                             long num_frames_) {
        auto* impl = static_cast<CubebSinkStream*>(user_data);
        if (!impl) {
            return -1;
        }

        const std::size_t num_channels = impl->GetDeviceChannels();
        const std::size_t frame_size = num_channels;
        const std::size_t num_frames{static_cast<size_t>(num_frames_)};

        if (impl->type == StreamType::In) {
            std::span<const s16> input_buffer{reinterpret_cast<const s16*>(in_buff),
                                              num_frames * frame_size};
            impl->ProcessAudioIn(input_buffer, num_frames);
        } else {
            std::span<s16> output_buffer{reinterpret_cast<s16*>(out_buff), num_frames * frame_size};
            impl->ProcessAudioOutAndRender(output_buffer, num_frames);
        }

        return num_frames_;
    }

    /**
     * Cubeb callback for if a device state changes. Unused currently.
     *
     * @param stream      - Cubeb-specific data about the stream.
     * @param user_data   - Custom data pointer passed along, points to a CubebSinkStream.
     * @param state       - New state of the device.
     */
    static void StateCallback(cubeb_stream*, void*, cubeb_state) {}

    /// Main Cubeb context
    cubeb* ctx{};
    /// Cubeb stream backend
    cubeb_stream* stream_backend{};
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
                           target_device_name == std::string(info.friendly_name);
                })};
            if (device != collection_end) {
                output_device = device->devid;
            }
            cubeb_device_collection_destroy(ctx, &collection);
        }
    }

    cubeb_get_max_channel_count(ctx, &device_channels);
    device_channels = device_channels >= 6U ? 6U : 2U;
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

SinkStream* CubebSink::AcquireSinkStream(Core::System& system, u32 system_channels,
                                         const std::string& name, StreamType type) {
    SinkStreamPtr& stream = sink_streams.emplace_back(std::make_unique<CubebSinkStream>(
        ctx, device_channels, system_channels, output_device, input_device, name, type, system));

    return stream.get();
}

void CubebSink::CloseStream(SinkStream* stream) {
    for (size_t i = 0; i < sink_streams.size(); i++) {
        if (sink_streams[i].get() == stream) {
            sink_streams[i].reset();
            sink_streams.erase(sink_streams.begin() + i);
            break;
        }
    }
}

void CubebSink::CloseStreams() {
    sink_streams.clear();
}

f32 CubebSink::GetDeviceVolume() const {
    if (sink_streams.empty()) {
        return 1.0f;
    }

    return sink_streams[0]->GetDeviceVolume();
}

void CubebSink::SetDeviceVolume(f32 volume) {
    for (auto& stream : sink_streams) {
        stream->SetDeviceVolume(volume);
    }
}

void CubebSink::SetSystemVolume(f32 volume) {
    for (auto& stream : sink_streams) {
        stream->SetSystemVolume(volume);
    }
}

std::vector<std::string> ListCubebSinkDevices(bool capture) {
    std::vector<std::string> device_list;
    cubeb* ctx;

    if (cubeb_init(&ctx, "yuzu Device Enumerator", nullptr) != CUBEB_OK) {
        LOG_CRITICAL(Audio_Sink, "cubeb_init failed");
        return {};
    }

    auto type{capture ? CUBEB_DEVICE_TYPE_INPUT : CUBEB_DEVICE_TYPE_OUTPUT};
    cubeb_device_collection collection;
    if (cubeb_enumerate_devices(ctx, type, &collection) != CUBEB_OK) {
        LOG_WARNING(Audio_Sink, "Audio output device enumeration not supported");
    } else {
        for (std::size_t i = 0; i < collection.count; i++) {
            const cubeb_device_info& device = collection.device[i];
            if (device.friendly_name && device.friendly_name[0] != '\0' &&
                device.state == CUBEB_DEVICE_STATE_ENABLED) {
                device_list.emplace_back(device.friendly_name);
            }
        }
        cubeb_device_collection_destroy(ctx, &collection);
    }

    cubeb_destroy(ctx);
    return device_list;
}

} // namespace AudioCore::Sink

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>
#include <span>

#include "audio_core/audio_core.h"
#include "audio_core/audio_event.h"
#include "audio_core/audio_manager.h"
#include "audio_core/sink/cubeb_sink.h"
#include "audio_core/sink/sink_stream.h"
#include "common/assert.h"
#include "common/fixed_point.h"
#include "common/logging/log.h"
#include "common/reader_writer_queue.h"
#include "common/ring_buffer.h"
#include "common/settings.h"
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
    CubebSinkStream(cubeb* ctx_, const u32 device_channels_, const u32 system_channels_,
                    cubeb_devid output_device, cubeb_devid input_device, const std::string& name_,
                    const StreamType type_, Core::System& system_)
        : ctx{ctx_}, type{type_}, system{system_} {
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

        playing_buffer.consumed = true;

        LOG_DEBUG(Service_Audio,
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
    void Start(const bool resume = false) override {
        if (!ctx) {
            return;
        }

        if (resume && was_playing) {
            if (cubeb_stream_start(stream_backend) != CUBEB_OK) {
                LOG_CRITICAL(Audio_Sink, "Error starting cubeb stream");
            }
            paused = false;
        } else if (!resume) {
            if (cubeb_stream_start(stream_backend) != CUBEB_OK) {
                LOG_CRITICAL(Audio_Sink, "Error starting cubeb stream");
            }
            paused = false;
        }
    }

    /**
     * Stop the sink stream.
     */
    void Stop() override {
        if (!ctx) {
            return;
        }

        if (cubeb_stream_stop(stream_backend) != CUBEB_OK) {
            LOG_CRITICAL(Audio_Sink, "Error stopping cubeb stream");
        }

        was_playing.store(!paused);
        paused = true;
    }

    /**
     * Append a new buffer and its samples to a waiting queue to play.
     *
     * @param buffer  - Audio buffer information to be queued.
     * @param samples - The s16 samples to be queue for playback.
     */
    void AppendBuffer(::AudioCore::Sink::SinkBuffer& buffer, std::vector<s16>& samples) override {
        if (type == StreamType::In) {
            queue.enqueue(buffer);
            queued_buffers++;
        } else {
            constexpr s32 min{std::numeric_limits<s16>::min()};
            constexpr s32 max{std::numeric_limits<s16>::max()};

            auto yuzu_volume{Settings::Volume()};
            auto volume{system_volume * device_volume * yuzu_volume};

            if (system_channels == 6 && device_channels == 2) {
                // We're given 6 channels, but our device only outputs 2, so downmix.
                constexpr std::array<f32, 4> down_mix_coeff{1.0f, 0.707f, 0.251f, 0.707f};

                for (u32 read_index = 0, write_index = 0; read_index < samples.size();
                     read_index += system_channels, write_index += device_channels) {
                    const auto left_sample{
                        ((Common::FixedPoint<49, 15>(
                              samples[read_index + static_cast<u32>(Channels::FrontLeft)]) *
                              down_mix_coeff[0] +
                          samples[read_index + static_cast<u32>(Channels::Center)] *
                              down_mix_coeff[1] +
                          samples[read_index + static_cast<u32>(Channels::LFE)] *
                              down_mix_coeff[2] +
                          samples[read_index + static_cast<u32>(Channels::BackLeft)] *
                              down_mix_coeff[3]) *
                         volume)
                            .to_int()};

                    const auto right_sample{
                        ((Common::FixedPoint<49, 15>(
                              samples[read_index + static_cast<u32>(Channels::FrontRight)]) *
                              down_mix_coeff[0] +
                          samples[read_index + static_cast<u32>(Channels::Center)] *
                              down_mix_coeff[1] +
                          samples[read_index + static_cast<u32>(Channels::LFE)] *
                              down_mix_coeff[2] +
                          samples[read_index + static_cast<u32>(Channels::BackRight)] *
                              down_mix_coeff[3]) *
                         volume)
                            .to_int()};

                    samples[write_index + static_cast<u32>(Channels::FrontLeft)] =
                        static_cast<s16>(std::clamp(left_sample, min, max));
                    samples[write_index + static_cast<u32>(Channels::FrontRight)] =
                        static_cast<s16>(std::clamp(right_sample, min, max));
                }

                samples.resize(samples.size() / system_channels * device_channels);

            } else if (system_channels == 2 && device_channels == 6) {
                // We need moar samples! Not all games will provide 6 channel audio.
                // TODO: Implement some upmixing here. Currently just passthrough, with other
                // channels left as silence.
                std::vector<s16> new_samples(samples.size() / system_channels * device_channels, 0);

                for (u32 read_index = 0, write_index = 0; read_index < samples.size();
                     read_index += system_channels, write_index += device_channels) {
                    const auto left_sample{static_cast<s16>(std::clamp(
                        static_cast<s32>(
                            static_cast<f32>(
                                samples[read_index + static_cast<u32>(Channels::FrontLeft)]) *
                            volume),
                        min, max))};

                    new_samples[write_index + static_cast<u32>(Channels::FrontLeft)] = left_sample;

                    const auto right_sample{static_cast<s16>(std::clamp(
                        static_cast<s32>(
                            static_cast<f32>(
                                samples[read_index + static_cast<u32>(Channels::FrontRight)]) *
                            volume),
                        min, max))};

                    new_samples[write_index + static_cast<u32>(Channels::FrontRight)] =
                        right_sample;
                }
                samples = std::move(new_samples);

            } else if (volume != 1.0f) {
                for (u32 i = 0; i < samples.size(); i++) {
                    samples[i] = static_cast<s16>(std::clamp(
                        static_cast<s32>(static_cast<f32>(samples[i]) * volume), min, max));
                }
            }

            samples_buffer.Push(samples);
            queue.enqueue(buffer);
            queued_buffers++;
        }
    }

    /**
     * Release a buffer. Audio In only, will fill a buffer with recorded samples.
     *
     * @param num_samples - Maximum number of samples to receive.
     * @return Vector of recorded samples. May have fewer than num_samples.
     */
    std::vector<s16> ReleaseBuffer(const u64 num_samples) override {
        static constexpr s32 min = std::numeric_limits<s16>::min();
        static constexpr s32 max = std::numeric_limits<s16>::max();

        auto samples{samples_buffer.Pop(num_samples)};

        // TODO: Up-mix to 6 channels if the game expects it.
        // For audio input this is unlikely to ever be the case though.

        // Incoming mic volume seems to always be very quiet, so multiply by an additional 8 here.
        // TODO: Play with this and find something that works better.
        auto volume{system_volume * device_volume * 8};
        for (u32 i = 0; i < samples.size(); i++) {
            samples[i] = static_cast<s16>(
                std::clamp(static_cast<s32>(static_cast<f32>(samples[i]) * volume), min, max));
        }

        if (samples.size() < num_samples) {
            samples.resize(num_samples, 0);
        }
        return samples;
    }

    /**
     * Check if a certain buffer has been consumed (fully played).
     *
     * @param tag - Unique tag of a buffer to check for.
     * @return True if the buffer has been played, otherwise false.
     */
    bool IsBufferConsumed(const u64 tag) override {
        if (released_buffer.tag == 0) {
            if (!released_buffers.try_dequeue(released_buffer)) {
                return false;
            }
        }

        if (released_buffer.tag == tag) {
            released_buffer.tag = 0;
            return true;
        }
        return false;
    }

    /**
     * Empty out the buffer queue.
     */
    void ClearQueue() override {
        samples_buffer.Pop();
        while (queue.pop()) {
        }
        while (released_buffers.pop()) {
        }
        queued_buffers = 0;
        released_buffer = {};
        playing_buffer = {};
        playing_buffer.consumed = true;
    }

private:
    /**
     * Signal events back to the audio system that a buffer was played/can be filled.
     *
     * @param buffer - Consumed audio buffer to be released.
     */
    void SignalEvent(const ::AudioCore::Sink::SinkBuffer& buffer) {
        auto& manager{system.AudioCore().GetAudioManager()};
        switch (type) {
        case StreamType::Out:
            released_buffers.enqueue(buffer);
            manager.SetEvent(Event::Type::AudioOutManager, true);
            break;
        case StreamType::In:
            released_buffers.enqueue(buffer);
            manager.SetEvent(Event::Type::AudioInManager, true);
            break;
        case StreamType::Render:
            break;
        }
    }

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
        const std::size_t frame_size_bytes = frame_size * sizeof(s16);
        const std::size_t num_frames{static_cast<size_t>(num_frames_)};
        size_t frames_written{0};
        [[maybe_unused]] bool underrun{false};

        if (impl->type == StreamType::In) {
            // INPUT
            std::span<const s16> input_buffer{reinterpret_cast<const s16*>(in_buff),
                                              num_frames * frame_size};

            while (frames_written < num_frames) {
                auto& playing_buffer{impl->playing_buffer};

                // If the playing buffer has been consumed or has no frames, we need a new one
                if (playing_buffer.consumed || playing_buffer.frames == 0) {
                    if (!impl->queue.try_dequeue(impl->playing_buffer)) {
                        // If no buffer was available we've underrun, just push the samples and
                        // continue.
                        underrun = true;
                        impl->samples_buffer.Push(&input_buffer[frames_written * frame_size],
                                                  (num_frames - frames_written) * frame_size);
                        frames_written = num_frames;
                        continue;
                    } else {
                        // Successfully got a new buffer, mark the old one as consumed and signal.
                        impl->queued_buffers--;
                        impl->SignalEvent(impl->playing_buffer);
                    }
                }

                // Get the minimum frames available between the currently playing buffer, and the
                // amount we have left to fill
                size_t frames_available{
                    std::min(playing_buffer.frames - playing_buffer.frames_played,
                             num_frames - frames_written)};

                impl->samples_buffer.Push(&input_buffer[frames_written * frame_size],
                                          frames_available * frame_size);

                frames_written += frames_available;
                playing_buffer.frames_played += frames_available;

                // If that's all the frames in the current buffer, add its samples and mark it as
                // consumed
                if (playing_buffer.frames_played >= playing_buffer.frames) {
                    impl->AddPlayedSampleCount(playing_buffer.frames_played * num_channels);
                    impl->playing_buffer.consumed = true;
                }
            }

            std::memcpy(&impl->last_frame[0], &input_buffer[(frames_written - 1) * frame_size],
                        frame_size_bytes);
        } else {
            // OUTPUT
            std::span<s16> output_buffer{reinterpret_cast<s16*>(out_buff), num_frames * frame_size};

            while (frames_written < num_frames) {
                auto& playing_buffer{impl->playing_buffer};

                // If the playing buffer has been consumed or has no frames, we need a new one
                if (playing_buffer.consumed || playing_buffer.frames == 0) {
                    if (!impl->queue.try_dequeue(impl->playing_buffer)) {
                        // If no buffer was available we've underrun, fill the remaining buffer with
                        // the last written frame and continue.
                        underrun = true;
                        for (size_t i = frames_written; i < num_frames; i++) {
                            std::memcpy(&output_buffer[i * frame_size], &impl->last_frame[0],
                                        frame_size_bytes);
                        }
                        frames_written = num_frames;
                        continue;
                    } else {
                        // Successfully got a new buffer, mark the old one as consumed and signal.
                        impl->queued_buffers--;
                        impl->SignalEvent(impl->playing_buffer);
                    }
                }

                // Get the minimum frames available between the currently playing buffer, and the
                // amount we have left to fill
                size_t frames_available{
                    std::min(playing_buffer.frames - playing_buffer.frames_played,
                             num_frames - frames_written)};

                impl->samples_buffer.Pop(&output_buffer[frames_written * frame_size],
                                         frames_available * frame_size);

                frames_written += frames_available;
                playing_buffer.frames_played += frames_available;

                // If that's all the frames in the current buffer, add its samples and mark it as
                // consumed
                if (playing_buffer.frames_played >= playing_buffer.frames) {
                    impl->AddPlayedSampleCount(playing_buffer.frames_played * num_channels);
                    impl->playing_buffer.consumed = true;
                }
            }

            std::memcpy(&impl->last_frame[0], &output_buffer[(frames_written - 1) * frame_size],
                        frame_size_bytes);
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
    static void StateCallback([[maybe_unused]] cubeb_stream* stream,
                              [[maybe_unused]] void* user_data,
                              [[maybe_unused]] cubeb_state state) {}

    /// Main Cubeb context
    cubeb* ctx{};
    /// Cubeb stream backend
    cubeb_stream* stream_backend{};
    /// Name of this stream
    std::string name{};
    /// Type of this stream
    StreamType type;
    /// Core system
    Core::System& system;
    /// Ring buffer of the samples waiting to be played or consumed
    Common::RingBuffer<s16, 0x10000> samples_buffer;
    /// Audio buffers queued and waiting to play
    Common::ReaderWriterQueue<::AudioCore::Sink::SinkBuffer> queue;
    /// The currently-playing audio buffer
    ::AudioCore::Sink::SinkBuffer playing_buffer{};
    /// Audio buffers which have been played and are in queue to be released by the audio system
    Common::ReaderWriterQueue<::AudioCore::Sink::SinkBuffer> released_buffers{};
    /// Currently released buffer waiting to be taken by the audio system
    ::AudioCore::Sink::SinkBuffer released_buffer{};
    /// The last played (or received) frame of audio, used when the callback underruns
    std::array<s16, MaxChannels> last_frame{};
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

SinkStream* CubebSink::AcquireSinkStream(Core::System& system, const u32 system_channels,
                                         const std::string& name, const StreamType type) {
    SinkStreamPtr& stream = sink_streams.emplace_back(std::make_unique<CubebSinkStream>(
        ctx, device_channels, system_channels, output_device, input_device, name, type, system));

    return stream.get();
}

void CubebSink::CloseStream(const SinkStream* stream) {
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

void CubebSink::PauseStreams() {
    for (auto& stream : sink_streams) {
        stream->Stop();
    }
}

void CubebSink::UnpauseStreams() {
    for (auto& stream : sink_streams) {
        stream->Start(true);
    }
}

f32 CubebSink::GetDeviceVolume() const {
    if (sink_streams.empty()) {
        return 1.0f;
    }

    return sink_streams[0]->GetDeviceVolume();
}

void CubebSink::SetDeviceVolume(const f32 volume) {
    for (auto& stream : sink_streams) {
        stream->SetDeviceVolume(volume);
    }
}

void CubebSink::SetSystemVolume(const f32 volume) {
    for (auto& stream : sink_streams) {
        stream->SetSystemVolume(volume);
    }
}

std::vector<std::string> ListCubebSinkDevices(const bool capture) {
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

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <atomic>

#include "audio_core/audio_core.h"
#include "audio_core/audio_event.h"
#include "audio_core/audio_manager.h"
#include "audio_core/sink/sdl2_sink.h"
#include "audio_core/sink/sink_stream.h"
#include "common/assert.h"
#include "common/fixed_point.h"
#include "common/logging/log.h"
#include "common/reader_writer_queue.h"
#include "common/ring_buffer.h"
#include "common/settings.h"
#include "core/core.h"

// Ignore -Wimplicit-fallthrough due to https://github.com/libsdl-org/SDL/issues/4307
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#endif
#include <SDL.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace AudioCore::Sink {
/**
 * SDL sink stream, responsible for sinking samples to hardware.
 */
class SDLSinkStream final : public SinkStream {
public:
    /**
     * Create a new sink stream.
     *
     * @param device_channels_ - Number of channels supported by the hardware.
     * @param system_channels_ - Number of channels the audio systems expect.
     * @param output_device    - Name of the output device to use for this stream.
     * @param input_device     - Name of the input device to use for this stream.
     * @param type_            - Type of this stream.
     * @param system_          - Core system.
     * @param event            - Event used only for audio renderer, signalled on buffer consume.
     */
    SDLSinkStream(u32 device_channels_, const u32 system_channels_,
                  const std::string& output_device, const std::string& input_device,
                  const StreamType type_, Core::System& system_)
        : type{type_}, system{system_} {
        system_channels = system_channels_;
        device_channels = device_channels_;

        SDL_AudioSpec spec;
        spec.freq = TargetSampleRate;
        spec.channels = static_cast<u8>(device_channels);
        spec.format = AUDIO_S16SYS;
        if (type == StreamType::Render) {
            spec.samples = TargetSampleCount;
        } else {
            spec.samples = 1024;
        }
        spec.callback = &SDLSinkStream::DataCallback;
        spec.userdata = this;

        playing_buffer.consumed = true;

        std::string device_name{output_device};
        bool capture{false};
        if (type == StreamType::In) {
            device_name = input_device;
            capture = true;
        }

        SDL_AudioSpec obtained;
        if (device_name.empty()) {
            device = SDL_OpenAudioDevice(nullptr, capture, &spec, &obtained, false);
        } else {
            device = SDL_OpenAudioDevice(device_name.c_str(), capture, &spec, &obtained, false);
        }

        if (device == 0) {
            LOG_CRITICAL(Audio_Sink, "Error opening SDL audio device: {}", SDL_GetError());
            return;
        }

        LOG_DEBUG(Service_Audio,
                  "Opening sdl stream {} with: rate {} channels {} (system channels {}) "
                  " samples {}",
                  device, obtained.freq, obtained.channels, system_channels, obtained.samples);
    }

    /**
     * Destroy the sink stream.
     */
    ~SDLSinkStream() override {
        if (device == 0) {
            return;
        }

        SDL_CloseAudioDevice(device);
    }

    /**
     * Finalize the sink stream.
     */
    void Finalize() override {
        if (device == 0) {
            return;
        }

        SDL_CloseAudioDevice(device);
    }

    /**
     * Start the sink stream.
     *
     * @param resume - Set to true if this is resuming the stream a previously-active stream.
     *                 Default false.
     */
    void Start(const bool resume = false) override {
        if (device == 0) {
            return;
        }

        if (resume && was_playing) {
            SDL_PauseAudioDevice(device, 0);
            paused = false;
        } else if (!resume) {
            SDL_PauseAudioDevice(device, 0);
            paused = false;
        }
    }

    /**
     * Stop the sink stream.
     */
    void Stop() {
        if (device == 0) {
            return;
        }
        SDL_PauseAudioDevice(device, 1);
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
            constexpr s32 min = std::numeric_limits<s16>::min();
            constexpr s32 max = std::numeric_limits<s16>::max();

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
                    new_samples[write_index + static_cast<u32>(Channels::FrontLeft)] =
                        samples[read_index + static_cast<u32>(Channels::FrontLeft)];
                    new_samples[write_index + static_cast<u32>(Channels::FrontRight)] =
                        samples[read_index + static_cast<u32>(Channels::FrontRight)];
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
        released_buffer = {};
        playing_buffer = {};
        playing_buffer.consumed = true;
        queued_buffers = 0;
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
     * Main callback from SDL. Either expects samples from us (audio render/audio out), or will
     * provide samples to be copied (audio in).
     *
     * @param userdata - Custom data pointer passed along, points to a SDLSinkStream.
     * @param stream   - Buffer of samples to be filled or read.
     * @param len      - Length of the stream in bytes.
     */
    static void DataCallback(void* userdata, Uint8* stream, int len) {
        auto* impl = static_cast<SDLSinkStream*>(userdata);

        if (!impl) {
            return;
        }

        const std::size_t num_channels = impl->GetDeviceChannels();
        const std::size_t frame_size = num_channels;
        const std::size_t frame_size_bytes = frame_size * sizeof(s16);
        const std::size_t num_frames{len / num_channels / sizeof(s16)};
        size_t frames_written{0};
        [[maybe_unused]] bool underrun{false};

        if (impl->type == StreamType::In) {
            std::span<s16> input_buffer{reinterpret_cast<s16*>(stream), num_frames * frame_size};

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
            std::span<s16> output_buffer{reinterpret_cast<s16*>(stream), num_frames * frame_size};

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
    }

    /// SDL device id of the opened input/output device
    SDL_AudioDeviceID device{};
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

    device_channels = 2;
}

SDLSink::~SDLSink() = default;

SinkStream* SDLSink::AcquireSinkStream(Core::System& system, const u32 system_channels,
                                       const std::string&, const StreamType type) {
    SinkStreamPtr& stream = sink_streams.emplace_back(std::make_unique<SDLSinkStream>(
        device_channels, system_channels, output_device, input_device, type, system));
    return stream.get();
}

void SDLSink::CloseStream(const SinkStream* stream) {
    for (size_t i = 0; i < sink_streams.size(); i++) {
        if (sink_streams[i].get() == stream) {
            sink_streams[i].reset();
            sink_streams.erase(sink_streams.begin() + i);
            break;
        }
    }
}

void SDLSink::CloseStreams() {
    sink_streams.clear();
}

void SDLSink::PauseStreams() {
    for (auto& stream : sink_streams) {
        stream->Stop();
    }
}

void SDLSink::UnpauseStreams() {
    for (auto& stream : sink_streams) {
        stream->Start();
    }
}

f32 SDLSink::GetDeviceVolume() const {
    if (sink_streams.empty()) {
        return 1.0f;
    }

    return sink_streams[0]->GetDeviceVolume();
}

void SDLSink::SetDeviceVolume(const f32 volume) {
    for (auto& stream : sink_streams) {
        stream->SetDeviceVolume(volume);
    }
}

void SDLSink::SetSystemVolume(const f32 volume) {
    for (auto& stream : sink_streams) {
        stream->SetSystemVolume(volume);
    }
}

std::vector<std::string> ListSDLSinkDevices(const bool capture) {
    std::vector<std::string> device_list;

    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            LOG_CRITICAL(Audio_Sink, "SDL_InitSubSystem audio failed: {}", SDL_GetError());
            return {};
        }
    }

    const int device_count = SDL_GetNumAudioDevices(capture);
    for (int i = 0; i < device_count; ++i) {
        device_list.emplace_back(SDL_GetAudioDeviceName(i, 0));
    }

    return device_list;
}

} // namespace AudioCore::Sink

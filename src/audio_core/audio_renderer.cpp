// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <limits>
#include <vector>

#include "audio_core/audio_out.h"
#include "audio_core/audio_renderer.h"
#include "audio_core/common.h"
#include "audio_core/info_updater.h"
#include "audio_core/voice_context.h"
#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core_timing.h"
#include "core/memory.h"

namespace {
[[nodiscard]] static constexpr s16 ClampToS16(s32 value) {
    return static_cast<s16>(std::clamp(value, s32{std::numeric_limits<s16>::min()},
                                       s32{std::numeric_limits<s16>::max()}));
}

[[nodiscard]] static constexpr s16 Mix2To1(s16 l_channel, s16 r_channel) {
    // Mix 50% from left and 50% from right channel
    constexpr float l_mix_amount = 50.0f / 100.0f;
    constexpr float r_mix_amount = 50.0f / 100.0f;
    return ClampToS16(static_cast<s32>((static_cast<float>(l_channel) * l_mix_amount) +
                                       (static_cast<float>(r_channel) * r_mix_amount)));
}

[[maybe_unused, nodiscard]] static constexpr std::tuple<s16, s16> Mix6To2(
    s16 fl_channel, s16 fr_channel, s16 fc_channel, [[maybe_unused]] s16 lf_channel, s16 bl_channel,
    s16 br_channel) {
    // Front channels are mixed 36.94%, Center channels are mixed to be 26.12% & the back channels
    // are mixed to be 36.94%

    constexpr float front_mix_amount = 36.94f / 100.0f;
    constexpr float center_mix_amount = 26.12f / 100.0f;
    constexpr float back_mix_amount = 36.94f / 100.0f;

    // Mix 50% from left and 50% from right channel
    const auto left = front_mix_amount * static_cast<float>(fl_channel) +
                      center_mix_amount * static_cast<float>(fc_channel) +
                      back_mix_amount * static_cast<float>(bl_channel);

    const auto right = front_mix_amount * static_cast<float>(fr_channel) +
                       center_mix_amount * static_cast<float>(fc_channel) +
                       back_mix_amount * static_cast<float>(br_channel);

    return {ClampToS16(static_cast<s32>(left)), ClampToS16(static_cast<s32>(right))};
}

[[nodiscard]] static constexpr std::tuple<s16, s16> Mix6To2WithCoefficients(
    s16 fl_channel, s16 fr_channel, s16 fc_channel, s16 lf_channel, s16 bl_channel, s16 br_channel,
    const std::array<float_le, 4>& coeff) {
    const auto left =
        static_cast<float>(fl_channel) * coeff[0] + static_cast<float>(fc_channel) * coeff[1] +
        static_cast<float>(lf_channel) * coeff[2] + static_cast<float>(bl_channel) * coeff[3];

    const auto right =
        static_cast<float>(fr_channel) * coeff[0] + static_cast<float>(fc_channel) * coeff[1] +
        static_cast<float>(lf_channel) * coeff[2] + static_cast<float>(br_channel) * coeff[3];

    return {ClampToS16(static_cast<s32>(left)), ClampToS16(static_cast<s32>(right))};
}

} // namespace

namespace AudioCore {
constexpr s32 NUM_BUFFERS = 2;

AudioRenderer::AudioRenderer(Core::Timing::CoreTiming& core_timing_, Core::Memory::Memory& memory_,
                             AudioCommon::AudioRendererParameter params,
                             Stream::ReleaseCallback&& release_callback,
                             std::size_t instance_number)
    : worker_params{params}, memory_pool_info(params.effect_count + params.voice_count * 4),
      voice_context(params.voice_count), effect_context(params.effect_count), mix_context(),
      sink_context(params.sink_count), splitter_context(),
      voices(params.voice_count), memory{memory_},
      command_generator(worker_params, voice_context, mix_context, splitter_context, effect_context,
                        memory),
      core_timing{core_timing_} {
    behavior_info.SetUserRevision(params.revision);
    splitter_context.Initialize(behavior_info, params.splitter_count,
                                params.num_splitter_send_channels);
    mix_context.Initialize(behavior_info, params.submix_count + 1, params.effect_count);
    audio_out = std::make_unique<AudioCore::AudioOut>();
    stream = audio_out->OpenStream(
        core_timing, params.sample_rate, AudioCommon::STREAM_NUM_CHANNELS,
        fmt::format("AudioRenderer-Instance{}", instance_number), std::move(release_callback));
    process_event = Core::Timing::CreateEvent(
        fmt::format("AudioRenderer-Instance{}-Process", instance_number),
        [this](std::uintptr_t, std::chrono::nanoseconds) { ReleaseAndQueueBuffers(); });
    for (s32 i = 0; i < NUM_BUFFERS; ++i) {
        QueueMixedBuffer(i);
    }
}

AudioRenderer::~AudioRenderer() = default;

ResultCode AudioRenderer::Start() {
    audio_out->StartStream(stream);
    ReleaseAndQueueBuffers();
    return ResultSuccess;
}

ResultCode AudioRenderer::Stop() {
    audio_out->StopStream(stream);
    return ResultSuccess;
}

u32 AudioRenderer::GetSampleRate() const {
    return worker_params.sample_rate;
}

u32 AudioRenderer::GetSampleCount() const {
    return worker_params.sample_count;
}

u32 AudioRenderer::GetMixBufferCount() const {
    return worker_params.mix_buffer_count;
}

Stream::State AudioRenderer::GetStreamState() const {
    return stream->GetState();
}

ResultCode AudioRenderer::UpdateAudioRenderer(const std::vector<u8>& input_params,
                                              std::vector<u8>& output_params) {
    std::scoped_lock lock{mutex};
    InfoUpdater info_updater{input_params, output_params, behavior_info};

    if (!info_updater.UpdateBehaviorInfo(behavior_info)) {
        LOG_ERROR(Audio, "Failed to update behavior info input parameters");
        return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
    }

    if (!info_updater.UpdateMemoryPools(memory_pool_info)) {
        LOG_ERROR(Audio, "Failed to update memory pool parameters");
        return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
    }

    if (!info_updater.UpdateVoiceChannelResources(voice_context)) {
        LOG_ERROR(Audio, "Failed to update voice channel resource parameters");
        return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
    }

    if (!info_updater.UpdateVoices(voice_context, memory_pool_info, 0)) {
        LOG_ERROR(Audio, "Failed to update voice parameters");
        return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
    }

    // TODO(ogniK): Deal with stopped audio renderer but updates still taking place
    if (!info_updater.UpdateEffects(effect_context, true)) {
        LOG_ERROR(Audio, "Failed to update effect parameters");
        return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
    }

    if (behavior_info.IsSplitterSupported()) {
        if (!info_updater.UpdateSplitterInfo(splitter_context)) {
            LOG_ERROR(Audio, "Failed to update splitter parameters");
            return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
        }
    }

    const auto mix_result = info_updater.UpdateMixes(mix_context, worker_params.mix_buffer_count,
                                                     splitter_context, effect_context);

    if (mix_result.IsError()) {
        LOG_ERROR(Audio, "Failed to update mix parameters");
        return mix_result;
    }

    // TODO(ogniK): Sinks
    if (!info_updater.UpdateSinks(sink_context)) {
        LOG_ERROR(Audio, "Failed to update sink parameters");
        return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
    }

    // TODO(ogniK): Performance buffer
    if (!info_updater.UpdatePerformanceBuffer()) {
        LOG_ERROR(Audio, "Failed to update performance buffer parameters");
        return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
    }

    if (!info_updater.UpdateErrorInfo(behavior_info)) {
        LOG_ERROR(Audio, "Failed to update error info");
        return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
    }

    if (behavior_info.IsElapsedFrameCountSupported()) {
        if (!info_updater.UpdateRendererInfo(elapsed_frame_count)) {
            LOG_ERROR(Audio, "Failed to update renderer info");
            return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
        }
    }
    // TODO(ogniK): Statistics

    if (!info_updater.WriteOutputHeader()) {
        LOG_ERROR(Audio, "Failed to write output header");
        return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
    }

    // TODO(ogniK): Check when all sections are implemented

    if (!info_updater.CheckConsumedSize()) {
        LOG_ERROR(Audio, "Audio buffers were not consumed!");
        return AudioCommon::Audren::ERR_INVALID_PARAMETERS;
    }
    return ResultSuccess;
}

void AudioRenderer::QueueMixedBuffer(Buffer::Tag tag) {
    command_generator.PreCommand();
    // Clear mix buffers before our next operation
    command_generator.ClearMixBuffers();

    // If the splitter is not in use, sort our mixes
    if (!splitter_context.UsingSplitter()) {
        mix_context.SortInfo();
    }
    // Sort our voices
    voice_context.SortInfo();

    // Handle samples
    command_generator.GenerateVoiceCommands();
    command_generator.GenerateSubMixCommands();
    command_generator.GenerateFinalMixCommands();

    command_generator.PostCommand();
    // Base sample size
    std::size_t BUFFER_SIZE{worker_params.sample_count};
    // Samples, making sure to clear
    std::vector<s16> buffer(BUFFER_SIZE * stream->GetNumChannels(), 0);

    if (sink_context.InUse()) {
        const auto stream_channel_count = stream->GetNumChannels();
        const auto buffer_offsets = sink_context.OutputBuffers();
        const auto channel_count = buffer_offsets.size();
        const auto& final_mix = mix_context.GetFinalMixInfo();
        const auto& in_params = final_mix.GetInParams();
        std::vector<std::span<s32>> mix_buffers(channel_count);
        for (std::size_t i = 0; i < channel_count; i++) {
            mix_buffers[i] =
                command_generator.GetMixBuffer(in_params.buffer_offset + buffer_offsets[i]);
        }

        for (std::size_t i = 0; i < BUFFER_SIZE; i++) {
            if (channel_count == 1) {
                const auto sample = ClampToS16(mix_buffers[0][i]);

                // Place sample in all channels
                for (u32 channel = 0; channel < stream_channel_count; channel++) {
                    buffer[i * stream_channel_count + channel] = sample;
                }

                if (stream_channel_count == 6) {
                    // Output stream has a LF channel, mute it!
                    buffer[i * stream_channel_count + 3] = 0;
                }

            } else if (channel_count == 2) {
                const auto l_sample = ClampToS16(mix_buffers[0][i]);
                const auto r_sample = ClampToS16(mix_buffers[1][i]);
                if (stream_channel_count == 1) {
                    buffer[i * stream_channel_count + 0] = Mix2To1(l_sample, r_sample);
                } else if (stream_channel_count == 2) {
                    buffer[i * stream_channel_count + 0] = l_sample;
                    buffer[i * stream_channel_count + 1] = r_sample;
                } else if (stream_channel_count == 6) {
                    buffer[i * stream_channel_count + 0] = l_sample;
                    buffer[i * stream_channel_count + 1] = r_sample;

                    // Combine both left and right channels to the center channel
                    buffer[i * stream_channel_count + 2] = Mix2To1(l_sample, r_sample);

                    buffer[i * stream_channel_count + 4] = l_sample;
                    buffer[i * stream_channel_count + 5] = r_sample;
                }

            } else if (channel_count == 6) {
                const auto fl_sample = ClampToS16(mix_buffers[0][i]);
                const auto fr_sample = ClampToS16(mix_buffers[1][i]);
                const auto fc_sample = ClampToS16(mix_buffers[2][i]);
                const auto lf_sample = ClampToS16(mix_buffers[3][i]);
                const auto bl_sample = ClampToS16(mix_buffers[4][i]);
                const auto br_sample = ClampToS16(mix_buffers[5][i]);

                if (stream_channel_count == 1) {
                    // Games seem to ignore the center channel half the time, we use the front left
                    // and right channel for mixing as that's where majority of the audio goes
                    buffer[i * stream_channel_count + 0] = Mix2To1(fl_sample, fr_sample);
                } else if (stream_channel_count == 2) {
                    // Mix all channels into 2 channels
                    const auto [left, right] = Mix6To2WithCoefficients(
                        fl_sample, fr_sample, fc_sample, lf_sample, bl_sample, br_sample,
                        sink_context.GetDownmixCoefficients());
                    buffer[i * stream_channel_count + 0] = left;
                    buffer[i * stream_channel_count + 1] = right;
                } else if (stream_channel_count == 6) {
                    // Pass through
                    buffer[i * stream_channel_count + 0] = fl_sample;
                    buffer[i * stream_channel_count + 1] = fr_sample;
                    buffer[i * stream_channel_count + 2] = fc_sample;
                    buffer[i * stream_channel_count + 3] = lf_sample;
                    buffer[i * stream_channel_count + 4] = bl_sample;
                    buffer[i * stream_channel_count + 5] = br_sample;
                }
            }
        }
    }

    audio_out->QueueBuffer(stream, tag, std::move(buffer));
    elapsed_frame_count++;
    voice_context.UpdateStateByDspShared();
}

void AudioRenderer::ReleaseAndQueueBuffers() {
    if (!stream->IsPlaying()) {
        return;
    }

    {
        std::scoped_lock lock{mutex};
        const auto released_buffers{audio_out->GetTagsAndReleaseBuffers(stream)};
        for (const auto& tag : released_buffers) {
            QueueMixedBuffer(tag);
        }
    }

    const f32 sample_rate = static_cast<f32>(GetSampleRate());
    const f32 sample_count = static_cast<f32>(GetSampleCount());
    const f32 consume_rate = sample_rate / (sample_count * (sample_count / 240));
    const s32 ms = (1000 / static_cast<s32>(consume_rate)) - 1;
    const std::chrono::milliseconds next_event_time(std::max(ms / NUM_BUFFERS, 1));
    core_timing.ScheduleEvent(next_event_time, process_event, {});
}

} // namespace AudioCore

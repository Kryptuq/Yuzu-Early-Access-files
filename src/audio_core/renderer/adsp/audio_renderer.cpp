// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <chrono>

#include "audio_core/audio_core.h"
#include "audio_core/common/common.h"
#include "audio_core/renderer/adsp/audio_renderer.h"
#include "audio_core/sink/sink.h"
#include "common/logging/log.h"
#include "common/microprofile.h"
#include "common/thread.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/core_timing_util.h"

MICROPROFILE_DEFINE(Audio_Renderer, "Audio", "DSP", MP_RGB(60, 19, 97));

namespace AudioCore::AudioRenderer::ADSP {

void AudioRenderer_Mailbox::HostSendMessage(RenderMessage message_) {
    adsp_messages.enqueue(message_);
    adsp_event.Set();
}

RenderMessage AudioRenderer_Mailbox::HostWaitMessage() {
    host_event.Wait();
    RenderMessage msg{RenderMessage::Invalid};
    if (!host_messages.try_dequeue(msg)) {
        LOG_ERROR(Service_Audio, "Failed to dequeue host message!");
    }
    return msg;
}

void AudioRenderer_Mailbox::ADSPSendMessage(const RenderMessage message_) {
    host_messages.enqueue(message_);
    host_event.Set();
}

RenderMessage AudioRenderer_Mailbox::ADSPWaitMessage() {
    adsp_event.Wait();
    RenderMessage msg{RenderMessage::Invalid};
    if (!adsp_messages.try_dequeue(msg)) {
        LOG_ERROR(Service_Audio, "Failed to dequeue ADSP message!");
    }
    return msg;
}

CommandBuffer& AudioRenderer_Mailbox::GetCommandBuffer(const s32 session_id) {
    return command_buffers[session_id];
}

void AudioRenderer_Mailbox::SetCommandBuffer(const u32 session_id, const CommandBuffer& buffer) {
    command_buffers[session_id] = buffer;
}

u64 AudioRenderer_Mailbox::GetRenderTimeTaken() const {
    return command_buffers[0].render_time_taken + command_buffers[1].render_time_taken;
}

u64 AudioRenderer_Mailbox::GetSignalledTick() const {
    return signalled_tick;
}

void AudioRenderer_Mailbox::SetSignalledTick(const u64 tick) {
    signalled_tick = tick;
}

void AudioRenderer_Mailbox::ClearRemainCount(const u32 session_id) {
    command_buffers[session_id].remaining_command_count = 0;
}

u32 AudioRenderer_Mailbox::GetRemainCommandCount(const u32 session_id) const {
    return command_buffers[session_id].remaining_command_count;
}

void AudioRenderer_Mailbox::ClearCommandBuffers() {
    command_buffers[0].buffer = 0;
    command_buffers[0].size = 0;
    command_buffers[0].reset_buffers = false;
    command_buffers[1].buffer = 0;
    command_buffers[1].size = 0;
    command_buffers[1].reset_buffers = false;
}

AudioRenderer::AudioRenderer(Core::System& system_)
    : system{system_}, sink{system.AudioCore().GetOutputSink()} {
    CreateSinkStreams();
}

AudioRenderer::~AudioRenderer() {
    Stop();
    for (auto& stream : streams) {
        if (stream) {
            sink.CloseStream(stream);
        }
        stream = nullptr;
    }
}

void AudioRenderer::Start(AudioRenderer_Mailbox* mailbox_) {
    if (running) {
        return;
    }

    mailbox = mailbox_;
    thread = std::thread(&AudioRenderer::ThreadFunc, this);
    running = true;
}

void AudioRenderer::Stop() {
    if (!running) {
        return;
    }

    for (auto& stream : streams) {
        stream->Stop();
    }
    thread.join();
    running = false;
}

void AudioRenderer::CreateSinkStreams() {
    u32 channels{sink.GetDeviceChannels()};
    for (u32 i = 0; i < MaxRendererSessions; i++) {
        std::string name{fmt::format("ADSP_RenderStream-{}", i)};
        streams[i] =
            sink.AcquireSinkStream(system, channels, name, ::AudioCore::Sink::StreamType::Render);
        streams[i]->SetRingSize(4);
    }
}

void AudioRenderer::ThreadFunc() {
    constexpr char name[]{"yuzu:AudioRenderer"};
    MicroProfileOnThreadCreate(name);
    Common::SetCurrentThreadName(name);
    Common::SetCurrentThreadPriority(Common::ThreadPriority::Critical);
    if (mailbox->ADSPWaitMessage() != RenderMessage::AudioRenderer_InitializeOK) {
        LOG_ERROR(Service_Audio,
                  "ADSP Audio Renderer -- Failed to receive initialize message from host!");
        return;
    }

    mailbox->ADSPSendMessage(RenderMessage::AudioRenderer_InitializeOK);

    constexpr u64 max_process_time{2'304'000ULL};

    while (true) {
        auto message{mailbox->ADSPWaitMessage()};
        switch (message) {
        case RenderMessage::AudioRenderer_Shutdown:
            mailbox->ADSPSendMessage(RenderMessage::AudioRenderer_Shutdown);
            return;

        case RenderMessage::AudioRenderer_Render: {
            std::array<bool, MaxRendererSessions> buffers_reset{};
            std::array<u64, MaxRendererSessions> render_times_taken{};
            const auto start_time{system.CoreTiming().GetClockTicks()};

            for (u32 index = 0; index < 2; index++) {
                auto& command_buffer{mailbox->GetCommandBuffer(index)};
                auto& command_list_processor{command_list_processors[index]};

                // Check this buffer is valid, as it may not be used.
                if (command_buffer.buffer != 0) {
                    // If there are no remaining commands (from the previous list),
                    // this is a new command list, initalize it.
                    if (command_buffer.remaining_command_count == 0) {
                        command_list_processor.Initialize(system, command_buffer.buffer,
                                                          command_buffer.size, streams[index]);
                    }

                    if (command_buffer.reset_buffers && !buffers_reset[index]) {
                        streams[index]->ClearQueue();
                        buffers_reset[index] = true;
                    }

                    u64 max_time{max_process_time};
                    if (index == 1 && command_buffer.applet_resource_user_id ==
                                          mailbox->GetCommandBuffer(0).applet_resource_user_id) {
                        max_time = max_process_time -
                                   Core::Timing::CyclesToNs(render_times_taken[0]).count();
                        if (render_times_taken[0] > max_process_time) {
                            max_time = 0;
                        }
                    }

                    max_time = std::min(command_buffer.time_limit, max_time);
                    command_list_processor.SetProcessTimeMax(max_time);

                    // Process the command list
                    {
                        MICROPROFILE_SCOPE(Audio_Renderer);
                        render_times_taken[index] =
                            command_list_processor.Process(index) - start_time;
                    }

                    const auto end_time{system.CoreTiming().GetClockTicks()};

                    command_buffer.remaining_command_count =
                        command_list_processor.GetRemainingCommandCount();
                    command_buffer.render_time_taken = end_time - start_time;
                }
            }

            mailbox->ADSPSendMessage(RenderMessage::AudioRenderer_RenderResponse);
        } break;

        default:
            LOG_WARNING(Service_Audio,
                        "ADSP AudioRenderer received an invalid message, msg={:02X}!",
                        static_cast<u32>(message));
            break;
        }
    }
}

} // namespace AudioCore::AudioRenderer::ADSP

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/adsp/adsp.h"
#include "audio_core/renderer/adsp/command_buffer.h"
#include "audio_core/sink/sink.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/core_timing_util.h"
#include "core/memory.h"

namespace AudioCore::AudioRenderer::ADSP {

ADSP::ADSP(Core::System& system_, Sink::Sink& sink_)
    : system{system_}, memory{system.Memory()}, sink{sink_} {}

ADSP::~ADSP() {
    ClearCommandBuffers();
}

State ADSP::GetState() const {
    if (running) {
        return State::Started;
    }
    return State::Stopped;
}

AudioRenderer_Mailbox* ADSP::GetRenderMailbox() {
    return &render_mailbox;
}

void ADSP::ClearRemainCount(const u32 session_id) {
    render_mailbox.ClearRemainCount(session_id);
}

u64 ADSP::GetSignalledTick() const {
    return render_mailbox.GetSignalledTick();
}

u64 ADSP::GetTimeTaken() const {
    return render_mailbox.GetRenderTimeTaken();
}

u64 ADSP::GetRenderTimeTaken(const u32 session_id) {
    return render_mailbox.GetCommandBuffer(session_id).render_time_taken;
}

u32 ADSP::GetRemainCommandCount(const u32 session_id) const {
    return render_mailbox.GetRemainCommandCount(session_id);
}

void ADSP::SendCommandBuffer(const u32 session_id, CommandBuffer& command_buffer) {
    render_mailbox.SetCommandBuffer(session_id, command_buffer);
}

u64 ADSP::GetRenderingStartTick(const u32 session_id) {
    return render_mailbox.GetSignalledTick() +
           render_mailbox.GetCommandBuffer(session_id).render_time_taken;
}

bool ADSP::Start() {
    if (running) {
        return running;
    }

    running = true;
    systems_active++;
    audio_renderer = std::make_unique<AudioRenderer>(system);
    audio_renderer->Start(&render_mailbox);
    render_mailbox.HostSendMessage(RenderMessage::AudioRenderer_InitializeOK);
    if (render_mailbox.HostWaitMessage() != RenderMessage::AudioRenderer_InitializeOK) {
        LOG_ERROR(
            Service_Audio,
            "Host Audio Renderer -- Failed to receive initialize message response from ADSP!");
    }
    return running;
}

void ADSP::Stop() {
    systems_active--;
    if (running && systems_active == 0) {
        {
            std::scoped_lock l{mailbox_lock};
            render_mailbox.HostSendMessage(RenderMessage::AudioRenderer_Shutdown);
            if (render_mailbox.HostWaitMessage() != RenderMessage::AudioRenderer_Shutdown) {
                LOG_ERROR(Service_Audio, "Host Audio Renderer -- Failed to receive shutdown "
                                         "message response from ADSP!");
            }
        }
        audio_renderer->Stop();
        running = false;
    }
}

void ADSP::Signal() {
    const auto signalled_tick{system.CoreTiming().GetClockTicks()};
    render_mailbox.SetSignalledTick(signalled_tick);
    render_mailbox.HostSendMessage(RenderMessage::AudioRenderer_Render);
}

void ADSP::Wait() {
    std::scoped_lock l{mailbox_lock};
    auto response{render_mailbox.HostWaitMessage()};
    if (response != RenderMessage::AudioRenderer_RenderResponse) {
        LOG_ERROR(Service_Audio, "Invalid ADSP response message, expected 0x{:02X}, got 0x{:02X}",
                  static_cast<u32>(RenderMessage::AudioRenderer_RenderResponse),
                  static_cast<u32>(response));
    }

    ClearCommandBuffers();
}

void ADSP::ClearCommandBuffers() {
    render_mailbox.ClearCommandBuffers();
}

} // namespace AudioCore::AudioRenderer::ADSP

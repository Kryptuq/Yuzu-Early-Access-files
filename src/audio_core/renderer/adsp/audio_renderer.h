// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <memory>
#include <thread>

#include "audio_core/renderer/adsp/command_buffer.h"
#include "audio_core/renderer/adsp/command_list_processor.h"
#include "common/common_types.h"
#include "common/reader_writer_queue.h"
#include "common/thread.h"

namespace Core {
namespace Timing {
struct EventType;
}
class System;
} // namespace Core

namespace AudioCore {
namespace Sink {
class Sink;
}

namespace AudioRenderer::ADSP {

enum class RenderMessage {
    /* 0x00 */ Invalid,
    /* 0x01 */ AudioRenderer_MapUnmap_Map,
    /* 0x02 */ AudioRenderer_MapUnmap_MapResponse,
    /* 0x03 */ AudioRenderer_MapUnmap_Unmap,
    /* 0x04 */ AudioRenderer_MapUnmap_UnmapResponse,
    /* 0x05 */ AudioRenderer_MapUnmap_InvalidateCache,
    /* 0x06 */ AudioRenderer_MapUnmap_InvalidateCacheResponse,
    /* 0x07 */ AudioRenderer_MapUnmap_Shutdown,
    /* 0x08 */ AudioRenderer_MapUnmap_ShutdownResponse,
    /* 0x16 */ AudioRenderer_InitializeOK = 0x16,
    /* 0x20 */ AudioRenderer_RenderResponse = 0x20,
    /* 0x2A */ AudioRenderer_Render = 0x2A,
    /* 0x34 */ AudioRenderer_Shutdown = 0x34,
};

/**
 * A mailbox for the AudioRenderer, allowing communication between the host and the AudioRenderer
 * running on the ADSP.
 */
class AudioRenderer_Mailbox {
public:
    /**
     * Send a message from the host to the AudioRenderer.
     *
     * @param message_ - The message to send to the AudioRenderer.
     */
    void HostSendMessage(RenderMessage message);

    /**
     * Host wait for a message from the AudioRenderer.
     *
     * @return The message returned from the AudioRenderer.
     */
    RenderMessage HostWaitMessage();

    /**
     * Send a message from the AudioRenderer to the host.
     *
     * @param message_ - The message to send to the host.
     */
    void ADSPSendMessage(RenderMessage message);

    /**
     * AudioRenderer wait for a message from the host.
     *
     * @return The message returned from the AudioRenderer.
     */
    RenderMessage ADSPWaitMessage();

    /**
     * Get the command buffer with the given session id (0 or 1).
     *
     * @param session_id - The session id to get (0 or 1).
     * @return The command buffer.
     */
    CommandBuffer& GetCommandBuffer(s32 session_id);

    /**
     * Set the command buffer with the given session id (0 or 1).
     *
     * @param session_id - The session id to get (0 or 1).
     * @param buffer     - The command buffer to set.
     */
    void SetCommandBuffer(u32 session_id, CommandBuffer& buffer);

    /**
     * Get the total render time taken for the last command lists sent.
     *
     * @return Total render time taken for the last command lists.
     */
    u64 GetRenderTimeTaken() const;

    /**
     * Get the tick the AudioRenderer was signalled.
     *
     * @return The tick the AudioRenderer was signalled.
     */
    u64 GetSignalledTick() const;

    /**
     * Set the tick the AudioRenderer was signalled.
     *
     * @param tick - The tick the AudioRenderer was signalled.
     */
    void SetSignalledTick(u64 tick);

    /**
     * Clear the remaining command count.
     *
     * @param session_id - Index for which command list to clear (0 or 1).
     */
    void ClearRemainCount(u32 session_id);

    /**
     * Get the remaining command count for a given command list.
     *
     * @param session_id - Index for which command list to clear (0 or 1).
     * @return The remaining command count.
     */
    u32 GetRemainCommandCount(u32 session_id) const;

    /**
     * Clear the command buffers (does not clear the time taken or the remaining command count).
     */
    void ClearCommandBuffers();

private:
    /// Host signalling event
    Common::Event host_event{};
    /// AudioRenderer signalling event
    Common::Event adsp_event{};
    /// Host message queue

    Common::ReaderWriterQueue<RenderMessage> host_messages{};
    /// AudioRenderer message queue

    Common::ReaderWriterQueue<RenderMessage> adsp_messages{};
    /// Command buffers

    std::array<CommandBuffer, MaxRendererSessions> command_buffers{};
    /// Tick the AudioRnederer was signalled
    u64 signalled_tick{};
};

/**
 * The AudioRenderer application running on the ADSP.
 */
class AudioRenderer {
public:
    explicit AudioRenderer(Core::System& system);
    ~AudioRenderer();

    /**
     * Start the AudioRenderer.
     *
     * @param The mailbox to use for this session.
     */
    void Start(AudioRenderer_Mailbox* mailbox);

    /**
     * Stop the AudioRenderer.
     */
    void Stop();

private:
    /**
     * Main AudioRenderer thread, responsible for processing the command lists.
     */
    void ThreadFunc();

    /**
     * Creates the streams which will receive the processed samples.
     */
    void CreateSinkStreams();

    /// Core system
    Core::System& system;
    /// Main thread
    std::thread thread{};
    /// The current state
    std::atomic<bool> running{};
    /// The active mailbox
    AudioRenderer_Mailbox* mailbox{};
    /// The command lists to process
    std::array<CommandListProcessor, MaxRendererSessions> command_list_processors{};
    /// The output sink the AudioRenderer will use
    Sink::Sink& sink;
    /// The streams which will receive the processed samples
    std::array<Sink::SinkStream*, MaxRendererSessions> streams;
};

} // namespace AudioRenderer::ADSP
} // namespace AudioCore

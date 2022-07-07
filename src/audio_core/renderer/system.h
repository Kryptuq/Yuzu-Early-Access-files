// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <mutex>
#include <span>

#include "audio_core/renderer/behavior/behavior_info.h"
#include "audio_core/renderer/command/command_processing_time_estimator.h"
#include "audio_core/renderer/effect/effect_context.h"
#include "audio_core/renderer/memory/memory_pool_info.h"
#include "audio_core/renderer/mix/mix_context.h"
#include "audio_core/renderer/performance/performance_manager.h"
#include "audio_core/renderer/sink/sink_context.h"
#include "audio_core/renderer/splitter/splitter_context.h"
#include "audio_core/renderer/upsampler/upsampler_manager.h"
#include "audio_core/renderer/voice/voice_context.h"
#include "common/thread.h"
#include "core/hle/service/audio/errors.h"

namespace Core {
namespace Memory {
class Memory;
}
class System;
} // namespace Core

namespace Kernel {
class KEvent;
class KTransferMemory;
} // namespace Kernel

namespace AudioCore {
struct AudioRendererParameterInternal;

namespace AudioRenderer {
class CommandBuffer;
namespace ADSP {
class ADSP;
}

/**
 * Audio Renderer System, the main worker for audio rendering.
 */
class System {
    enum class State {
        Started = 0,
        Stopped = 2,
    };

public:
    explicit System(Core::System& core, Kernel::KEvent* adsp_rendered_event);

    /**
     * Calculate the total size required for all audio render workbuffers.
     *
     * @param params - Input parameters with the numbers of voices/mixes/sinks/etc.
     * @return Size (in bytes) required for the audio renderer.
     */
    static u64 GetWorkBufferSize(const AudioRendererParameterInternal& params);

    /**
     * Initialize the renderer system.
     * Allocates workbuffers and initializes everything to a default state, ready to receive a
     * RequestUpdate.
     *
     * @param params                  - Input parameters to initialize the system with.
     * @param transfer_memory         - Game-supplied memory for all workbuffers. Unused.
     * @param transfer_memory_size    - Size of the transfer memory. Unused.
     * @param process_handle          - Process handle, also used for memory. Unused.
     * @param applet_resource_user_id - Applet id for this renderer. Unused.
     * @param session_id              - Session id of this renderer.
     * @return Result code.
     */
    Result Initialize(const AudioRendererParameterInternal& params,
                      Kernel::KTransferMemory* transfer_memory, u64 transfer_memory_size,
                      u32 process_handle, u64 applet_resource_user_id, s32 session_id);

    /**
     * Finalize the system.
     */
    void Finalize();

    /**
     * Start the system.
     */
    void Start();

    /**
     * Stop the system.
     */
    void Stop();

    /**
     * Update the system.
     *
     * @param input       - Inout buffer containing the update data.
     * @param performance - Optional buffer for writing back performance metrics.
     * @param output      - Output information from rendering.
     * @return Result code.
     */
    Result Update(std::span<const u8> input, std::span<u8> performance, std::span<u8> output);

    /**
     * Get the time limit (percent) for rendering
     *
     * @return Time limit as a percent.
     */
    u32 GetRenderingTimeLimit() const;

    /**
     * Set the time limit (percent) for rendering
     *
     * @param limit - New time limit.
     */
    void SetRenderingTimeLimit(u32 limit);

    /**
     * Get the session id for this system.
     *
     * @return Session id of this system.
     */
    u32 GetSessionId() const;

    /**
     * Get the sample rate of this system.
     *
     * @return Sample rate of this system.
     */
    u32 GetSampleRate() const;

    /**
     * Get the sample count of this system.
     *
     * @return Sample count of this system.
     */
    u32 GetSampleCount() const;

    /**
     * Get the number of mix buffers for this system.
     *
     * @return Number of mix buffers in the system.
     */
    u32 GetMixBufferCount() const;

    /**
     * Get the execution mode of this system.
     * Note: Only Auto is implemented.
     *
     * @return Execution mode for this system.
     */
    ExecutionMode GetExecutionMode() const;

    /**
     * Get the rendering deivce for this system.
     * This is unused.
     *
     * @return Rendering device for this system.
     */
    u32 GetRenderingDevice() const;

    /**
     * Check if this system is currently active.
     *
     * @return True if active, otherwise false.
     */
    bool IsActive() const;

    /**
     * Prepare and generate a list of commands for the AudioRenderer based on current state,
     * signalling the buffer event when all processed.
     */
    void SendCommandToDsp();

    /**
     * Generate a list of commands for the AudioRenderer based on current state.
     *
     * @param command_buffer      - Buffer for commands to be written to.
     * @param command_buffer_size - Size of the command_buffer.
     *
     * @return Number of bytes written.
     */
    u64 GenerateCommand(std::span<u8> command_buffer, u64 command_buffer_size);

    /**
     * Try to drop some voices if the AudioRenderer fell behind.
     *
     * @param command_buffer         - Command buffer to drop voices from.
     * @param estimated_process_time - Current estimated processing time of all commands.
     * @param time_limit             - Time limit for rendering, voices are dropped if estimated
     *                                 exceeds this.
     *
     * @return Number of voices dropped.
     */
    u32 DropVoices(CommandBuffer& command_buffer, u32 estimated_process_time, u32 time_limit);

private:
    /// Core system
    Core::System& core;
    /// Reference to the ADSP for communication
    ADSP::ADSP& adsp;
    /// Is this system initialized?
    bool initialized{};
    /// Is this system currently active?
    std::atomic<bool> active{};
    /// State of the system
    State state{State::Stopped};
    /// Sample rate for the system
    u32 sample_rate{};
    /// Sample count of the system
    u32 sample_count{};
    /// Number of mix buffers in use by the system
    s16 mix_buffer_count{};
    /// Workbuffer for mix buffers, used by the AudioRenderer
    std::span<s32> samples_workbuffer{};
    /// Depop samples for depopping commands
    std::span<s32> depop_buffer{};
    /// Number of memory pools in the buffer
    u32 memory_pool_count{};
    /// Workbuffer for memory pools
    std::span<MemoryPoolInfo> memory_pool_workbuffer{};
    /// System memory pool info
    MemoryPoolInfo memory_pool_info{};
    /// Workbuffer that commands will be generated into
    std::span<u8> command_workbuffer{};
    /// Size of command workbuffer
    u64 command_workbuffer_size{};
    /// Numebr of commands in the workbuffer
    u64 command_buffer_size{};
    /// Manager for upsamplers
    UpsamplerManager* upsampler_manager{};
    /// Upsampler workbuffer
    std::span<UpsamplerInfo> upsampler_infos{};
    /// Number of upsamplers in the workbuffer
    u32 upsampler_count{};
    /// Holds and controls all voices
    VoiceContext voice_context{};
    /// Holds and controls all mixes
    MixContext mix_context{};
    /// Holds and controls all effects
    EffectContext effect_context{};
    /// Holds and controls all sinks
    SinkContext sink_context{};
    /// Holds and controls all splitters
    SplitterContext splitter_context{};
    /// Estimates the time taken for each command
    std::unique_ptr<ICommandProcessingTimeEstimator> command_processing_time_estimator{};
    /// Session id of this system
    s32 session_id{};
    /// Number of channels in use by voices
    s32 voice_channels{};
    /// Event to be called when the AudioRenderer processes a command list
    Kernel::KEvent* adsp_rendered_event{};
    /// Event signalled on system terminate
    Common::Event terminate_event{};
    /// Does what locks do
    std::mutex lock{};
    /// Handle for the process for this system, unused
    u32 process_handle{};
    /// Applet resource id for this system, unused
    u64 applet_resource_user_id{};
    /// Controls performance input and output
    PerformanceManager performance_manager{};
    /// Workbuffer for performance metrics
    std::span<u8> performance_workbuffer{};
    /// Main workbuffer, from which all other workbuffers here allocate into
    std::unique_ptr<u8[]> workbuffer{};
    /// Size of the main workbuffer
    u64 workbuffer_size{};
    /// Unknown buffer/marker
    std::span<u8> unk_2A8{};
    /// Size of the above unknown buffer/marker
    u64 unk_2B0{};
    /// Rendering time limit (percent)
    u32 render_time_limit_percent{};
    /// Should any voices be dropped?
    bool drop_voice{};
    /// Should the backend stream have its buffers flushed?
    bool reset_command_buffers{};
    /// Execution mode of this system, only Auto is supported
    ExecutionMode execution_mode{ExecutionMode::Auto};
    /// Render device, unused
    u32 render_device{};
    /// Behaviour to check which features are supported by the user revision
    BehaviorInfo behavior{};
    /// Total ticks the audio system has been running
    u64 total_ticks_elapsed{};
    /// Ticks the system has spent in updates
    u64 ticks_spent_updating{};
    /// Number of times a command list was generated
    u64 num_command_lists_generated{};
    /// Number of times the system has updated
    u64 num_times_updated{};
    /// Number of frames generated, written back to the game
    std::atomic<u64> frames_elapsed{};
    /// Is the AudioRenderer running too slow?
    bool adsp_behind{};
    /// Number of voices dropped
    u32 num_voices_dropped{};
    /// Tick that rendering started
    u64 render_start_tick{};
};

} // namespace AudioRenderer
} // namespace AudioCore

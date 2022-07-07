// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "audio_core/renderer/command/commands.h"
#include "audio_core/renderer/performance/performance_manager.h"
#include "common/common_types.h"

namespace AudioCore {
struct AudioRendererSystemContext;

namespace AudioRenderer {
class CommandBuffer;
struct CommandListHeader;
class VoiceContext;
class MixContext;
class EffectContext;
class SplitterContext;
class SinkContext;
class BehaviorInfo;
class VoiceInfo;
struct VoiceState;
class MixInfo;
class SinkInfoBase;

/**
 * Generates all commands to build up a command list, which are sent to the AudioRender for
 * processing.
 */
class CommandGenerator {
public:
    explicit CommandGenerator(CommandBuffer& command_buffer,
                              const CommandListHeader& command_list_header,
                              const AudioRendererSystemContext& render_context,
                              VoiceContext& voice_context, MixContext& mix_context,
                              EffectContext& effect_context, SinkContext& sink_context,
                              SplitterContext& splitter_context,
                              PerformanceManager* performance_manager);

    /**
     * Calculate the buffer size needed for commands.
     *
     * @param behavior - Used to check what features are enabled.
     * @param params    - Input rendering parameters for numbers of voices/mixes/sinks etc.
     */
    static u64 CalculateCommandBufferSize(const BehaviorInfo& behavior,
                                          const AudioRendererParameterInternal& params) {
        u64 size{0};

        // Effects
        size += params.effects * sizeof(EffectInfoBase);

        // Voices
        u64 voice_size{0};
        if (behavior.IsWaveBufferVer2Supported()) {
            voice_size = std::max(std::max(sizeof(AdpcmDataSourceVersion2Command),
                                           sizeof(PcmInt16DataSourceVersion2Command)),
                                  sizeof(PcmFloatDataSourceVersion2Command));
        } else {
            voice_size = std::max(std::max(sizeof(AdpcmDataSourceVersion1Command),
                                           sizeof(PcmInt16DataSourceVersion1Command)),
                                  sizeof(PcmFloatDataSourceVersion1Command));
        }
        voice_size += sizeof(BiquadFilterCommand) * MaxBiquadFilters;
        voice_size += sizeof(VolumeRampCommand);
        voice_size += sizeof(MixRampGroupedCommand);

        size += params.voices * (params.splitter_infos * sizeof(DepopPrepareCommand) + voice_size);

        // Sub mixes
        size += sizeof(DepopForMixBuffersCommand) +
                (sizeof(MixCommand) * MaxMixBuffers) * MaxMixBuffers;

        // Final mix
        size += sizeof(DepopForMixBuffersCommand) + sizeof(VolumeCommand) * MaxMixBuffers;

        // Splitters
        size += params.splitter_destinations * sizeof(MixRampCommand) * MaxMixBuffers;

        // Sinks
        size +=
            params.sinks * std::max(sizeof(DeviceSinkCommand), sizeof(CircularBufferSinkCommand));

        // Performance
        size += (params.effects + params.voices + params.sinks + params.sub_mixes + 1 +
                 PerformanceManager::MaxDetailEntries) *
                sizeof(PerformanceCommand);
        return size;
    }

    /**
     * Get the current command buffer used to generate commands.
     *
     * @return The command buffer.
     */
    CommandBuffer& GetCommandBuffer() {
        return command_buffer;
    }

    /**
     * Get the current performance manager,
     *
     * @return The performance manager. May be nullptr.
     */
    PerformanceManager* GetPerformanceManager() {
        return performance_manager;
    }

    /**
     * Generate a data source command.
     * These are the basis for all audio output.
     *
     * @param voice_info  - Generate the command from this voice.
     * @param voice_state - State used by the AudioRenderer across calls.
     * @param channel     - Channel index to generate the command into.
     */
    void GenerateDataSourceCommand(VoiceInfo& voice_info, const VoiceState& voice_state,
                                   s8 channel);

    /**
     * Generate voice mixing commands.
     * These are used to mix buffers together, to mix one input to many outputs,
     * and also used as copy commands to move data around and prevent it being accidentally
     * overwritten, e.g by another data source command into the same channel.
     *
     * @param mix_volumes      - Current volumes of the mix.
     * @param prev_mix_volumes - Previous volumes of the mix.
     * @param voice_state      - State used by the AudioRenderer across calls.
     * @param output_index     - Output mix buffer index.
     * @param buffer_count     - Number of active mix buffers.
     * @param input_index      - Input mix buffer index.
     * @param node_id          - Node id of the voice this command is generated for.
     */
    void GenerateVoiceMixCommand(std::span<const f32> mix_volumes,
                                 std::span<const f32> prev_mix_volumes,
                                 const VoiceState& voice_state, s16 output_index, s16 buffer_count,
                                 s16 input_index, s32 node_id);

    /**
     * Generate a biquad filter command for a voice.
     *
     * @param voice_info   - Voice info this command is generated from.
     * @param voice_state  - State used by the AudioRenderer across calls.
     * @param buffer_count - Number of active mix buffers.
     * @param channel      - Channel index of this command.
     * @param node_id      - Node id of the voice this command is generated for.
     */
    void GenerateBiquadFilterCommandForVoice(VoiceInfo& voice_info, const VoiceState& voice_state,
                                             s16 buffer_count, s8 channel, s32 node_id);

    /**
     * Generate commands for a voice.
     * Includes a data source, biquad filter, volume and mixing.
     *
     * @param voice_info - Voice info these commands are generated from.
     */
    void GenerateVoiceCommand(VoiceInfo& voice_info);

    /**
     * Generate commands for all voices.
     */
    void GenerateVoiceCommands();

    /**
     * Generate a mixing command.
     *
     * @param buffer_offset    - Base mix buffer offset to use.
     * @param effect_info_base - BufferMixer effect info.
     * @param node_id          - Node id of the mix this command is generated for.
     */
    void GenerateBufferMixerCommand(s16 buffer_offset, EffectInfoBase& effect_info_base,
                                    s32 node_id);

    /**
     * Generate a delay effect command.
     *
     * @param buffer_offset    - Base mix buffer offset to use.
     * @param effect_info_base - Delay effect info.
     * @param node_id          - Node id of the mix this command is generated for.
     */
    void GenerateDelayCommand(s16 buffer_offset, EffectInfoBase& effect_info_base, s32 node_id);

    /**
     * Generate a reverb effect command.
     *
     * @param buffer_offset                 - Base mix buffer offset to use.
     * @param effect_info_base              - Reverb effect info.
     * @param node_id                       - Node id of the mix this command is generated for.
     * @param long_size_pre_delay_supported - Use a longer pre-delay time before reverb starts.
     */
    void GenerateReverbCommand(s16 buffer_offset, EffectInfoBase& effect_info_base, s32 node_id,
                               bool long_size_pre_delay_supported);

    /**
     * Generate an I3DL2 reverb effect command.
     *
     * @param buffer_offset    - Base mix buffer offset to use.
     * @param effect_info_base - I3DL2Reverb effect info.
     * @param node_id          - Node id of the mix this command is generated for.
     */
    void GenerateI3dl2ReverbEffectCommand(s16 buffer_offset, EffectInfoBase& effect_info,
                                          s32 node_id);

    /**
     * Generate an aux effect command.
     *
     * @param buffer_offset    - Base mix buffer offset to use.
     * @param effect_info_base - Aux effect info.
     * @param node_id          - Node id of the mix this command is generated for.
     */
    void GenerateAuxCommand(s16 buffer_offset, EffectInfoBase& effect_info, s32 node_id);

    /**
     * Generate a biquad filter effect command.
     *
     * @param buffer_offset    - Base mix buffer offset to use.
     * @param effect_info_base - Aux effect info.
     * @param node_id          - Node id of the mix this command is generated for.
     */
    void GenerateBiquadFilterEffectCommand(s16 buffer_offset, EffectInfoBase& effect_info,
                                           s32 node_id);

    /**
     * Generate a light limiter effect command.
     *
     * @param buffer_offset    - Base mix buffer offset to use.
     * @param effect_info_base - Limiter effect info.
     * @param node_id          - Node id of the mix this command is generated for.
     * @param effect_index     - Index for the statistics state.
     */
    void GenerateLightLimiterEffectCommand(s16 buffer_offset, EffectInfoBase& effect_info,
                                           s32 node_id, u32 effect_index);

    /**
     * Generate a capture effect command.
     * Writes a mix buffer back to game memory.
     *
     * @param buffer_offset    - Base mix buffer offset to use.
     * @param effect_info_base - Capture effect info.
     * @param node_id          - Node id of the mix this command is generated for.
     */
    void GenerateCaptureCommand(s16 buffer_offset, EffectInfoBase& effect_info, s32 node_id);

    /**
     * Generate all effect commands for a mix.
     *
     * @param mix_info - Mix to generate effects from.
     */
    void GenerateEffectCommand(MixInfo& mix_info);

    /**
     * Generate all mix commands.
     *
     * @param mix_info - Mix to generate effects from.
     */
    void GenerateMixCommands(MixInfo& mix_info);

    /**
     * Generate a submix command.
     * Generates all effects and all mixing commands.
     *
     * @param mix_info - Mix to generate effects from.
     */
    void GenerateSubMixCommand(MixInfo& mix_info);

    /**
     * Generate all submix command.
     */
    void GenerateSubMixCommands();

    /**
     * Generate the final mix.
     */
    void GenerateFinalMixCommand();

    /**
     * Generate the final mix commands.
     */
    void GenerateFinalMixCommands();

    /**
     * Generate all sink commands.
     */
    void GenerateSinkCommands();

    /**
     * Generate a sink command.
     * Sends samples out to the backend, or a game-supplied circular buffer.
     *
     * @param buffer_offset - Base mix buffer offset to use.
     * @param sink_info     - Sink info to generate the commands from.
     */
    void GenerateSinkCommand(s16 buffer_offset, SinkInfoBase& sink_info);

    /**
     * Generate a device sink command.
     * Sends samples out to the backend.
     *
     * @param buffer_offset - Base mix buffer offset to use.
     * @param sink_info     - Sink info to generate the commands from.
     */
    void GenerateDeviceSinkCommand(s16 buffer_offset, SinkInfoBase& sink_info);

    /**
     * Generate a performance command.
     * Used to report performance metrics of the AudioRenderer back to the game.
     *
     * @param buffer_offset - Base mix buffer offset to use.
     * @param sink_info     - Sink info to generate the commands from.
     */
    void GeneratePerformanceCommand(s32 node_id, PerformanceState state,
                                    const PerformanceEntryAddresses& entry_addresses);

private:
    /// Commands will be written by this buffer
    CommandBuffer& command_buffer;
    /// Header information for the commands generated
    const CommandListHeader& command_header;
    /// Various things to control generation
    const AudioRendererSystemContext& render_context;
    /// Used for generating voices
    VoiceContext& voice_context;
    /// Used for generating mixes
    MixContext& mix_context;
    /// Used for generating effects
    EffectContext& effect_context;
    /// Used for generating sinks
    SinkContext& sink_context;
    /// Used for generating submixes
    SplitterContext& splitter_context;
    /// Used for generating performance
    PerformanceManager* performance_manager;
};

} // namespace AudioRenderer
} // namespace AudioCore

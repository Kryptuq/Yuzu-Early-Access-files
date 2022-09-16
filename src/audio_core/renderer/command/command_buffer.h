// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "audio_core/renderer/command/commands.h"
#include "audio_core/renderer/effect/light_limiter.h"
#include "audio_core/renderer/performance/performance_manager.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
struct UpsamplerInfo;
struct VoiceState;
class EffectInfoBase;
class ICommandProcessingTimeEstimator;
class MixInfo;
class MemoryPoolInfo;
class SinkInfoBase;
class VoiceInfo;

/**
 * Utility functions to generate and add commands into the current command list.
 */
class CommandBuffer {
public:
    /**
     * Generate a PCM s16 version 1 command, adding it to the command list.
     *
     * @param node_id      - Node id of the voice this command is generated for.
     * @param memory_pool  - Memory pool for translating buffer addresses to the DSP.
     * @param voice_info   - The voice info this command is generated from.
     * @param voice_state  - The voice state the DSP will use for this command.
     * @param buffer_count - Number of mix buffers in use,
     *                       data will be read into this index + channel.
     * @param channel      - Channel index for this command.
     */
    void GeneratePcmInt16Version1Command(s32 node_id, const MemoryPoolInfo& memory_pool,
                                         VoiceInfo& voice_info, const VoiceState& voice_state,
                                         s16 buffer_count, s8 channel);

    /**
     * Generate a PCM s16 version 2 command, adding it to the command list.
     *
     * @param node_id      - Node id of the voice this command is generated for.
     * @param voice_info   - The voice info this command is generated from.
     * @param voice_state  - The voice state the DSP will use for this command.
     * @param buffer_count - Number of mix buffers in use,
     *                       data will be read into this index + channel.
     * @param channel      - Channel index for this command.
     */
    void GeneratePcmInt16Version2Command(s32 node_id, VoiceInfo& voice_info,
                                         const VoiceState& voice_state, s16 buffer_count,
                                         s8 channel);

    /**
     * Generate a PCM f32 version 1 command, adding it to the command list.
     *
     * @param node_id      - Node id of the voice this command is generated for.
     * @param memory_pool  - Memory pool for translating buffer addresses to the DSP.
     * @param voice_info   - The voice info this command is generated from.
     * @param voice_state  - The voice state the DSP will use for this command.
     * @param buffer_count - Number of mix buffers in use,
     *                       data will be read into this index + channel.
     * @param channel      - Channel index for this command.
     */
    void GeneratePcmFloatVersion1Command(s32 node_id, const MemoryPoolInfo& memory_pool,
                                         VoiceInfo& voice_info, const VoiceState& voice_state,
                                         s16 buffer_count, s8 channel);

    /**
     * Generate a PCM f32 version 2 command, adding it to the command list.
     *
     * @param node_id      - Node id of the voice this command is generated for.
     * @param voice_info   - The voice info this command is generated from.
     * @param voice_state  - The voice state the DSP will use for this command.
     * @param buffer_count - Number of mix buffers in use,
     *                       data will be read into this index + channel.
     * @param channel      - Channel index for this command.
     */
    void GeneratePcmFloatVersion2Command(s32 node_id, VoiceInfo& voice_info,
                                         const VoiceState& voice_state, s16 buffer_count,
                                         s8 channel);

    /**
     * Generate an ADPCM version 1 command, adding it to the command list.
     *
     * @param node_id      - Node id of the voice this command is generated for.
     * @param memory_pool  - Memory pool for translating buffer addresses to the DSP.
     * @param voice_info   - The voice info this command is generated from.
     * @param voice_state  - The voice state the DSP will use for this command.
     * @param buffer_count - Number of mix buffers in use,
     *                       data will be read into this index + channel.
     * @param channel      - Channel index for this command.
     */
    void GenerateAdpcmVersion1Command(s32 node_id, const MemoryPoolInfo& memory_pool,
                                      VoiceInfo& voice_info, const VoiceState& voice_state,
                                      s16 buffer_count, s8 channel);

    /**
     * Generate an ADPCM version 2 command, adding it to the command list.
     *
     * @param node_id      - Node id of the voice this command is generated for.
     * @param voice_info   - The voice info this command is generated from.
     * @param voice_state  - The voice state the DSP will use for this command.
     * @param buffer_count - Number of mix buffers in use,
     *                       data will be read into this index + channel.
     * @param channel      - Channel index for this command.
     */
    void GenerateAdpcmVersion2Command(s32 node_id, VoiceInfo& voice_info,
                                      const VoiceState& voice_state, s16 buffer_count, s8 channel);

    /**
     * Generate a volume command, adding it to the command list.
     *
     * @param node_id       - Node id of the voice this command is generated for.
     * @param buffer_offset - Base mix buffer index to generate this command at.
     * @param input_index   - Channel index and mix buffer offset for this command.
     * @param volume        - Mix volume added to the input samples.
     * @param precision     - Number of decimal bits for fixed point operations.
     */
    void GenerateVolumeCommand(s32 node_id, s16 buffer_offset, s16 input_index, f32 volume,
                               u8 precision);

    /**
     * Generate a volume ramp command, adding it to the command list.
     *
     * @param node_id      - Node id of the voice this command is generated for.
     * @param voice_info   - The voice info this command takes its volumes from.
     * @param buffer_count - Number of active mix buffers, command will generate at this index.
     * @param precision    - Number of decimal bits for fixed point operations.
     */
    void GenerateVolumeRampCommand(s32 node_id, VoiceInfo& voice_info, s16 buffer_count,
                                   u8 precision);

    /**
     * Generate a biquad filter command from a voice, adding it to the command list.
     *
     * @param node_id              - Node id of the voice this command is generated for.
     * @param voice_info           - The voice info this command takes biquad parameters from.
     * @param voice_state          - Used by the AudioRenderer to track previous samples.
     * @param buffer_count         - Number of active mix buffers,
     *                               command will generate at this index + channel.
     * @param channel              - Channel index for this filter to work on.
     * @param biquad_index         - Which biquad filter to use for this command (0-1).
     * @param use_float_processing - Should int or float processing be used?
     */
    void GenerateBiquadFilterCommand(s32 node_id, VoiceInfo& voice_info,
                                     const VoiceState& voice_state, s16 buffer_count, s8 channel,
                                     u32 biquad_index, bool use_float_processing);

    /**
     * Generate a biquad filter effect command, adding it to the command list.
     *
     * @param node_id              - Node id of the voice this command is generated for.
     * @param effect_info          - The effect info this command takes biquad parameters from.
     * @param buffer_offset        - Mix buffer offset this command will use,
     *                               command will generate at this index + channel.
     * @param channel              - Channel index for this filter to work on.
     * @param needs_init           - True if the biquad state needs initialisation.
     * @param use_float_processing - Should int or float processing be used?
     */
    void GenerateBiquadFilterCommand(s32 node_id, EffectInfoBase& effect_info, s16 buffer_offset,
                                     s8 channel, bool needs_init, bool use_float_processing);

    /**
     * Generate a mix command, adding it to the command list.
     *
     * @param node_id       - Node id of the voice this command is generated for.
     * @param input_index   - Input mix buffer index for this command.
     *                        Added to the buffer offset.
     * @param output_index  - Output mix buffer index for this command.
     *                        Added to the buffer offset.
     * @param buffer_offset - Mix buffer offset this command will use.
     * @param volume        - Volume to be applied to the input.
     * @param precision     - Number of decimal bits for fixed point operations.
     */
    void GenerateMixCommand(s32 node_id, s16 input_index, s16 output_index, s16 buffer_offset,
                            f32 volume, u8 precision);

    /**
     * Generate a mix ramp command, adding it to the command list.
     *
     * @param node_id      - Node id of the voice this command is generated for.
     * @param buffer_count - Number of active mix buffers.
     * @param input_index  - Input mix buffer index for this command.
     *                       Added to buffer_count.
     * @param output_index - Output mix buffer index for this command.
     *                       Added to buffer_count.
     * @param volume       - Current mix volume used for calculating the ramp.
     * @param prev_volume  - Previous mix volume, used for calculating the ramp,
     *                       also applied to the input.
     * @param prev_samples - Previous sample buffer. Used for depopping.
     * @param precision    - Number of decimal bits for fixed point operations.
     */
    void GenerateMixRampCommand(s32 node_id, s16 buffer_count, s16 input_index, s16 output_index,
                                f32 volume, f32 prev_volume, CpuAddr prev_samples, u8 precision);

    /**
     * Generate a mix ramp grouped command, adding it to the command list.
     *
     * @param node_id      - Node id of the voice this command is generated for.
     * @param buffer_count - Number of active mix buffers.
     * @param input_index  - Input mix buffer index for this command.
     *                       Added to buffer_count.
     * @param output_index - Output mix buffer index for this command.
     *                       Added to buffer_count.
     * @param volumes      - Current mix volumes used for calculating the ramp.
     * @param prev_volumes - Previous mix volumes, used for calculating the ramp,
     *                       also applied to the input.
     * @param prev_samples - Previous sample buffer. Used for depopping.
     * @param precision    - Number of decimal bits for fixed point operations.
     */
    void GenerateMixRampGroupedCommand(s32 node_id, s16 buffer_count, s16 input_index,
                                       s16 output_index, std::span<const f32> volumes,
                                       std::span<const f32> prev_volumes, CpuAddr prev_samples,
                                       u8 precision);

    /**
     * Generate a depop prepare command, adding it to the command list.
     *
     * @param node_id       - Node id of the voice this command is generated for.
     * @param voice_state   - State to track the previous depop samples for each mix buffer.
     * @param buffer        - State to track the current depop samples for each mix buffer.
     * @param buffer_count  - Number of active mix buffers.
     * @param buffer_offset - Base mix buffer index to generate the channel depops at.
     * @param was_playing   - Command only needs to work if the voice was previously playing.
     */
    void GenerateDepopPrepareCommand(s32 node_id, const VoiceState& voice_state,
                                     std::span<const s32> buffer, s16 buffer_count,
                                     s16 buffer_offset, bool was_playing);

    /**
     * Generate a depop command, adding it to the command list.
     *
     * @param node_id      - Node id of the voice this command is generated for.
     * @param mix_info     - Mix info to get the buffer count and base offsets from.
     * @param depop_buffer - Buffer of current depop sample values to be added to the input
     *                       channels.
     */
    void GenerateDepopForMixBuffersCommand(s32 node_id, const MixInfo& mix_info,
                                           std::span<const s32> depop_buffer);

    /**
     * Generate a delay command, adding it to the command list.
     *
     * @param node_id       - Node id of the voice this command is generated for.
     * @param effect_info   - Delay effect info to generate this command from.
     * @param buffer_offset - Base mix buffer offset to apply the apply the delay.
     */
    void GenerateDelayCommand(s32 node_id, EffectInfoBase& effect_info, s16 buffer_offset);

    /**
     * Generate an upsample command, adding it to the command list.
     *
     * @param node_id        - Node id of the voice this command is generated for.
     * @param buffer_offset  - Base mix buffer offset to upsample.
     * @param upsampler_info - Upsampler info to control the upsampling.
     * @param input_count    - Number of input channels to upsample.
     * @param inputs         - Input mix buffer indexes.
     * @param buffer_count   - Number of active mix buffers.
     * @param sample_count   - Source sample count of the input.
     * @param sample_rate    - Source sample rate of the input.
     */
    void GenerateUpsampleCommand(s32 node_id, s16 buffer_offset, UpsamplerInfo& upsampler_info,
                                 u32 input_count, std::span<const s8> inputs, s16 buffer_count,
                                 u32 sample_count, u32 sample_rate);

    /**
     * Generate a downmix 6 -> 2 command, adding it to the command list.
     *
     * @param node_id       - Node id of the voice this command is generated for.
     * @param inputs        - Input mix buffer indexes.
     * @param buffer_offset - Base mix buffer offset of the channels to downmix.
     * @param downmix_coeff - Downmixing coefficients.
     */
    void GenerateDownMix6chTo2chCommand(s32 node_id, std::span<const s8> inputs, s16 buffer_offset,
                                        std::span<const f32> downmix_coeff);

    /**
     * Generate an aux buffer command, adding it to the command list.
     *
     * @param node_id       - Node id of the voice this command is generated for.
     * @param effect_info   - Aux effect info to generate this command from.
     * @param input_index   - Input mix buffer index for this command.
     *                        Added to buffer_offset.
     * @param output_index  - Output mix buffer index for this command.
     *                        Added to buffer_offset.
     * @param buffer_offset - Base mix buffer offset to use.
     * @param update_count  - Number of samples to write back to the game as updated, can be 0.
     * @param count_max     - Maximum number of samples to read or write.
     * @param write_offset  - Current read or write offset within the buffer.
     */
    void GenerateAuxCommand(s32 node_id, EffectInfoBase& effect_info, s16 input_index,
                            s16 output_index, s16 buffer_offset, u32 update_count, u32 count_max,
                            u32 write_offset);

    /**
     * Generate a device sink command, adding it to the command list.
     *
     * @param node_id        - Node id of the voice this command is generated for.
     * @param buffer_offset  - Base mix buffer offset to use.
     * @param sink_info      - The sink_info to generate this command from.
     * @param session_id     - System session id this command is generated from.
     * @param samples_buffer - The buffer to be sent to the sink if upsampling is not used.
     */
    void GenerateDeviceSinkCommand(s32 node_id, s16 buffer_offset, SinkInfoBase& sink_info,
                                   u32 session_id, std::span<s32> samples_buffer);

    /**
     * Generate a circular buffer sink command, adding it to the command list.
     *
     * @param node_id       - Node id of the voice this command is generated for.
     * @param sink_info     - The sink_info to generate this command from.
     * @param buffer_offset - Base mix buffer offset to use.
     */
    void GenerateCircularBufferSinkCommand(s32 node_id, SinkInfoBase& sink_info, s16 buffer_offset);

    /**
     * Generate a reverb command, adding it to the command list.
     *
     * @param node_id                       - Node id of the voice this command is generated for.
     * @param effect_info                   - Reverb effect info to generate this command from.
     * @param buffer_offset                 - Base mix buffer offset to use.
     * @param long_size_pre_delay_supported - Should a longer pre-delay time be used before reverb
     *                                        begins?
     */
    void GenerateReverbCommand(s32 node_id, EffectInfoBase& effect_info, s16 buffer_offset,
                               bool long_size_pre_delay_supported);

    /**
     * Generate an I3DL2 reverb command, adding it to the command list.
     *
     * @param node_id                       - Node id of the voice this command is generated for.
     * @param effect_info                   - I3DL2Reverb effect info to generate this command from.
     * @param buffer_offset                 - Base mix buffer offset to use.
     */
    void GenerateI3dl2ReverbCommand(s32 node_id, EffectInfoBase& effect_info, s16 buffer_offset);

    /**
     * Generate a performance command, adding it to the command list.
     *
     * @param node_id         - Node id of the voice this command is generated for.
     * @param state           - State of the performance.
     * @param entry_addresses - The addresses to be filled in by the AudioRenderer.
     */
    void GeneratePerformanceCommand(s32 node_id, PerformanceState state,
                                    const PerformanceEntryAddresses& entry_addresses);

    /**
     * Generate a clear mix command, adding it to the command list.
     *
     * @param node_id         - Node id of the voice this command is generated for.
     */
    void GenerateClearMixCommand(s32 node_id);

    /**
     * Generate a copy mix command, adding it to the command list.
     *
     * @param node_id       - Node id of the voice this command is generated for.
     * @param effect_info   - BiquadFilter effect info to generate this command from.
     * @param buffer_offset - Base mix buffer offset to use.
     * @param channel       - Index to the effect's parameters input indexes for this command.
     */
    void GenerateCopyMixBufferCommand(s32 node_id, EffectInfoBase& effect_info, s16 buffer_offset,
                                      s8 channel);

    /**
     * Generate a light limiter version 1 command, adding it to the command list.
     *
     * @param node_id       - Node id of the voice this command is generated for.
     * @param buffer_offset - Base mix buffer offset to use.
     * @param parameter     - Effect parameter to generate from.
     * @param state         - State used by the AudioRenderer between commands.
     * @param enabled       - Is this command enabled?
     * @param workbuffer    - Game-supplied memory for the state.
     */
    void GenerateLightLimiterCommand(s32 node_id, s16 buffer_offset,
                                     const LightLimiterInfo::ParameterVersion1& parameter,
                                     const LightLimiterInfo::State& state, bool enabled,
                                     CpuAddr workbuffer);

    /**
     * Generate a light limiter version 2 command, adding it to the command list.
     *
     * @param node_id       - Node id of the voice this command is generated for.
     * @param buffer_offset - Base mix buffer offset to use.
     * @param parameter     - Effect parameter to generate from.
     * @param statistics    - Statistics reported by the AudioRenderer on the limiter's state.
     * @param state         - State used by the AudioRenderer between commands.
     * @param enabled       - Is this command enabled?
     * @param workbuffer    - Game-supplied memory for the state.
     */
    void GenerateLightLimiterCommand(s32 node_id, s16 buffer_offset,
                                     const LightLimiterInfo::ParameterVersion2& parameter,
                                     const LightLimiterInfo::StatisticsInternal& statistics,
                                     const LightLimiterInfo::State& state, bool enabled,
                                     CpuAddr workbuffer);

    /**
     * Generate a multitap biquad filter command, adding it to the command list.
     *
     * @param node_id      - Node id of the voice this command is generated for.
     * @param voice_info   - The voice info this command takes biquad parameters from.
     * @param voice_state  - Used by the AudioRenderer to track previous samples.
     * @param buffer_count - Number of active mix buffers,
     *                       command will generate at this index + channel.
     * @param channel      - Channel index for this filter to work on.
     */
    void GenerateMultitapBiquadFilterCommand(s32 node_id, VoiceInfo& voice_info,
                                             const VoiceState& voice_state, s16 buffer_count,
                                             s8 channel);

    /**
     * Generate a capture command, adding it to the command list.
     *
     * @param node_id       - Node id of the voice this command is generated for.
     * @param effect_info   - Capture effect info to generate this command from.
     * @param input_index   - Input mix buffer index for this command.
     *                        Added to buffer_offset.
     * @param output_index  - Output mix buffer index for this command (unused).
     *                        Added to buffer_offset.
     * @param buffer_offset - Base mix buffer offset to use.
     * @param update_count  - Number of samples to write back to the game as updated, can be 0.
     * @param count_max     - Maximum number of samples to read or write.
     * @param write_offset  - Current read or write offset within the buffer.
     */
    void GenerateCaptureCommand(s32 node_id, EffectInfoBase& effect_info, s16 input_index,
                                s16 output_index, s16 buffer_offset, u32 update_count,
                                u32 count_max, u32 write_offset);

    /**
     * Generate a compressor command, adding it to the command list.
     *
     * @param buffer_offset - Base mix buffer offset to use.
     * @param effect_info   - Capture effect info to generate this command from.
     * @param node_id       - Node id of the voice this command is generated for.
     */
    void GenerateCompressorCommand(s16 buffer_offset, EffectInfoBase& effect_info, s32 node_id);

    /// Command list buffer generated commands will be added to
    std::span<u8> command_list{};
    /// Input sample count, unused
    u32 sample_count{};
    /// Input sample rate, unused
    u32 sample_rate{};
    /// Current size of the command buffer
    u64 size{};
    /// Current number of commands added
    u32 count{};
    /// Current estimated processing time for all commands
    u32 estimated_process_time{};
    /// Used for mapping buffers for the AudioRenderer
    MemoryPoolInfo* memory_pool{};
    /// Used for estimating command process times
    ICommandProcessingTimeEstimator* time_estimator{};
    /// Used to check which rendering features are currently enabled
    BehaviorInfo* behavior{};

private:
    template <typename T, CommandId Id>
    T& GenerateStart(const s32 node_id);
    template <typename T>
    void GenerateEnd(T& cmd);
};

} // namespace AudioCore::AudioRenderer

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <bitset>

#include "audio_core/common/common.h"
#include "audio_core/common/wave_buffer.h"
#include "audio_core/renderer/behavior/behavior_info.h"
#include "audio_core/renderer/memory/address_info.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
class PoolMapper;
class VoiceContext;
struct VoiceState;

/**
 * Represents one voice. Voices are essentially noises, and they can be further mixed and have
 * effects applied to them, but voices are the basis of all sounds.
 */
class VoiceInfo {
public:
    enum class ServerPlayState {
        Started,
        Stopped,
        RequestStop,
        Paused,
    };

    struct Flags {
        u8 IsVoicePlayedSampleCountResetAtLoopPointSupported : 1;
        u8 IsVoicePitchAndSrcSkippedSupported : 1;
    };

    /**
     * A wavebuffer contains information on the data source buffers.
     */
    struct WaveBuffer {
        void Copy(WaveBufferVersion1& other) {
            other.buffer = buffer_address.GetReference(true);
            other.buffer_size = buffer_address.GetSize();
            other.start_offset = start_offset;
            other.end_offset = end_offset;
            other.loop = loop;
            other.stream_ended = stream_ended;

            if (context_address.GetCpuAddr()) {
                other.context = context_address.GetReference(true);
                other.context_size = context_address.GetSize();
            } else {
                other.context = CpuAddr(0);
                other.context_size = 0;
            }
        }

        void Copy(WaveBufferVersion2& other) {
            other.buffer = buffer_address.GetReference(true);
            other.buffer_size = buffer_address.GetSize();
            other.start_offset = start_offset;
            other.end_offset = end_offset;
            other.loop_start_offset = loop_start_offset;
            other.loop_end_offset = loop_end_offset;
            other.loop = loop;
            other.loop_count = loop_count;
            other.stream_ended = stream_ended;

            if (context_address.GetCpuAddr()) {
                other.context = context_address.GetReference(true);
                other.context_size = context_address.GetSize();
            } else {
                other.context = CpuAddr(0);
                other.context_size = 0;
            }
        }

        void Initialize() {
            buffer_address.Setup(0, 0);
            context_address.Setup(0, 0);
            start_offset = 0;
            end_offset = 0;
            loop = false;
            stream_ended = false;
            sent_to_DSP = true;
            loop_start_offset = 0;
            loop_end_offset = 0;
            loop_count = 0;
        }
        /// Game memory address of the wavebuffer data
        AddressInfo buffer_address{0, 0};
        /// Context for decoding, used for ADPCM
        AddressInfo context_address{0, 0};
        /// Starting offset for the wavebuffer
        u32 start_offset{};
        /// Ending offset the wavebuffer
        u32 end_offset{};
        /// Should this wavebuffer loop?
        bool loop{};
        /// Has this wavebuffer ended?
        bool stream_ended{};
        /// Has this wavebuffer been sent to the AudioRenderer?
        bool sent_to_DSP{true};
        /// Starting offset when looping, can differ from start_offset
        u32 loop_start_offset{};
        /// Ending offset when looping, can differ from end_offset
        u32 loop_end_offset{};
        /// Number of times to loop this wavebuffer
        s32 loop_count{};
    };

    struct WaveBufferInternal {
        /* 0x00 */ CpuAddr address;
        /* 0x08 */ u64 size;
        /* 0x10 */ s32 start_offset;
        /* 0x14 */ s32 end_offset;
        /* 0x18 */ bool loop;
        /* 0x19 */ bool stream_ended;
        /* 0x1A */ bool sent_to_DSP;
        /* 0x1C */ s32 loop_count;
        /* 0x20 */ CpuAddr context_address;
        /* 0x28 */ u64 context_size;
        /* 0x30 */ u32 loop_start;
        /* 0x34 */ u32 loop_end;
    };
    static_assert(sizeof(WaveBufferInternal) == 0x38,
                  "VoiceInfo::WaveBufferInternal has the wrong size!");

    struct BiquadFilterParameter {
        /* 0x00 */ bool enabled;
        /* 0x02 */ std::array<s16, 3> b;
        /* 0x08 */ std::array<s16, 2> a;
    };
    static_assert(sizeof(BiquadFilterParameter) == 0xC,
                  "VoiceInfo::BiquadFilterParameter has the wrong size!");

    struct InParameter {
        /* 0x000 */ u32 id;
        /* 0x004 */ u32 node_id;
        /* 0x008 */ bool is_new;
        /* 0x009 */ bool in_use;
        /* 0x00A */ PlayState play_state;
        /* 0x00B */ SampleFormat sample_format;
        /* 0x00C */ u32 sample_rate;
        /* 0x010 */ s32 priority;
        /* 0x014 */ s32 sort_order;
        /* 0x018 */ u32 channel_count;
        /* 0x01C */ f32 pitch;
        /* 0x020 */ f32 volume;
        /* 0x024 */ std::array<BiquadFilterParameter, MaxBiquadFilters> biquads;
        /* 0x03C */ u32 wave_buffer_count;
        /* 0x040 */ u16 wave_buffer_index;
        /* 0x042 */ char unk042[0x6];
        /* 0x048 */ CpuAddr src_data_address;
        /* 0x050 */ u64 src_data_size;
        /* 0x058 */ u32 mix_id;
        /* 0x05C */ u32 splitter_id;
        /* 0x060 */ std::array<WaveBufferInternal, MaxWaveBuffers> wave_buffer_internal;
        /* 0x140 */ std::array<u32, MaxChannels> channel_resource_ids;
        /* 0x158 */ bool clear_voice_drop;
        /* 0x159 */ u8 flush_buffer_count;
        /* 0x15A */ char unk15A[0x2];
        /* 0x15C */ Flags flags;
        /* 0x15D */ char unk15D[0x1];
        /* 0x15E */ SrcQuality src_quality;
        /* 0x15F */ char unk15F[0x11];
    };
    static_assert(sizeof(InParameter) == 0x170, "VoiceInfo::InParameter has the wrong size!");

    struct OutStatus {
        /* 0x00 */ u64 played_sample_count;
        /* 0x08 */ u32 wave_buffers_consumed;
        /* 0x0C */ bool voice_dropped;
    };
    static_assert(sizeof(OutStatus) == 0x10, "OutStatus::InParameter has the wrong size!");

    VoiceInfo();

    /**
     * Initialize this voice.
     */
    void Initialize();

    /**
     * Does this voice need an update?
     *
     * @param params - Input parameters to check matching.
     *
     * @return True if this voice needs an update, otherwise false.
     */
    bool ShouldUpdateParameters(const InParameter& params) const;

    /**
     * Update the parameters of this voice.
     *
     * @param error_info  - Output error code.
     * @param params      - Input parameters to update from.
     * @param pool_mapper - Used to map buffers.
     * @param behavior    - behavior to check supported features.
     */
    void UpdateParameters(BehaviorInfo::ErrorInfo& error_info, const InParameter& params,
                          const PoolMapper& pool_mapper, const BehaviorInfo& behavior);

    /**
     * Update the current play state.
     *
     * @param state - New play state for this voice.
     */
    void UpdatePlayState(PlayState state);

    /**
     * Update the current sample rate conversion quality.
     *
     * @param quality - New quality.
     */
    void UpdateSrcQuality(SrcQuality quality);

    /**
     * Update all wavebuffers.
     *
     * @param error_infos  - Output 2D array of errors, 2 per wavebuffer.
     * @param error_count  - Number of errors provided. Unused.
     * @param params       - Input parameters to be used for the update.
     * @param voice_states - The voice states for each channel in this voice to be updated.
     * @param pool_mapper  - Used to map the wavebuffers.
     * @param behavior     - Used to check for supported features.
     */
    void UpdateWaveBuffers(std::span<std::array<BehaviorInfo::ErrorInfo, 2>> error_infos,
                           u32 error_count, const InParameter& params,
                           std::span<VoiceState*> voice_states, const PoolMapper& pool_mapper,
                           const BehaviorInfo& behavior);

    /**
     * Update a wavebuffer.
     *
     * @param error_info           - Output array of errors.
     * @param wave_buffer          - The wavebuffer to be updated.
     * @param wave_buffer_internal - Input parameters to be used for the update.
     * @param sample_format        - Sample format of the wavebuffer.
     * @param valid                - Is this wavebuffer valid?
     * @param pool_mapper          - Used to map the wavebuffers.
     * @param behavior             - Used to check for supported features.
     */
    void UpdateWaveBuffer(std::span<BehaviorInfo::ErrorInfo> error_info, WaveBuffer& wave_buffer,
                          const WaveBufferInternal& wave_buffer_internal,
                          SampleFormat sample_format, bool valid, const PoolMapper& pool_mapper,
                          const BehaviorInfo& behavior);

    /**
     * Check if the input wavebuffer needs an update.
     *
     * @param wave_buffer_internal - Input wavebuffer parameters to check.
     * @return True if the given wavebuffer needs an update, otherwise false.
     */
    bool ShouldUpdateWaveBuffer(const WaveBufferInternal& wave_buffer_internal) const;

    /**
     * Write the number of played samples, number of consumed wavebuffers and if this voice was
     * dropped, to the given out_status.
     *
     * @param out_status   - Output status to be written to.
     * @param in_params    - Input parameters to check if the wavebuffer is new.
     * @param voice_states - Current host voice states for this voice, source of the output.
     */
    void WriteOutStatus(OutStatus& out_status, const InParameter& in_params,
                        std::span<VoiceState*> voice_states);

    /**
     * Check if this voice should be skipped for command generation.
     * Checks various things such as usage state, whether data is mapped etc.
     *
     * @return True if this voice should not be generated, otherwise false.
     */
    bool ShouldSkip() const;

    /**
     * Check if this voice has any mixing connections.
     *
     * @return True if this voice participates in mixing, otherwise false.
     */
    bool HasAnyConnection() const;

    /**
     * Flush flush_count wavebuffers, marking them as consumed.
     *
     * @param flush_count   - Number of wavebuffers to flush.
     * @param voice_states  - Voice states for these wavebuffers.
     * @param channel_count - Number of active channels.
     */
    void FlushWaveBuffers(u32 flush_count, std::span<VoiceState*> voice_states, s8 channel_count);

    /**
     * Update this voice's parameters on command generation,
     * updating voice states and flushing if needed.
     *
     * @param voice_states  - Voice states for these wavebuffers.
     * @return True if this voice should be generated, otherwise false.
     */
    bool UpdateParametersForCommandGeneration(std::span<VoiceState*> voice_states);

    /**
     * Update this voice on command generation.
     *
     * @param voice_context - Voice context for these wavebuffers.
     *
     * @return True if this voice should be generated, otherwise false.
     */
    bool UpdateForCommandGeneration(VoiceContext& voice_context);

    /**
     * Reset the AudioRenderer-side voice states, and the channel resources for this voice.
     *
     * @param voice_context - Context from which to get the resources.
     */
    void ResetResources(VoiceContext& voice_context) const;

    /// Is this voice in use?
    bool in_use{};
    /// Is this voice new?
    bool is_new{};
    /// Was this voice last playing? Used for depopping
    bool was_playing{};
    /// Sample format of the wavebuffers in this voice
    SampleFormat sample_format{};
    /// Sample rate of the wavebuffers in this voice
    u32 sample_rate{};
    /// Number of channels in this voice
    s8 channel_count{};
    /// Id of this voice
    u32 id{};
    /// Node id of this voice
    u32 node_id{};
    /// Mix id this voice is mixed to
    u32 mix_id{};
    /// Play state of this voice
    ServerPlayState current_play_state{ServerPlayState::Stopped};
    /// Last play state of this voice
    ServerPlayState last_play_state{ServerPlayState::Started};
    /// Priority of this voice, lower is higher
    s32 priority{};
    /// Sort order of this voice, used when same priority
    s32 sort_order{};
    /// Pitch of this voice (for sample rate conversion)
    f32 pitch{};
    /// Current volume of this voice
    f32 volume{};
    /// Previous volume of this voice
    f32 prev_volume{};
    /// Biquad filters for generating filter commands on this voice
    std::array<BiquadFilterParameter, MaxBiquadFilters> biquads{};
    /// Number of active wavebuffers
    u32 wave_buffer_count{};
    /// Current playing wavebuffer index
    u16 wave_buffer_index{};
    /// Flags controlling decode behavior
    u16 flags{};
    /// Game memory for ADPCM coefficients
    AddressInfo data_address{0, 0};
    /// Wavebuffers
    std::array<WaveBuffer, MaxWaveBuffers> wavebuffers{};
    /// Channel resources for this voice
    std::array<u32, MaxChannels> channel_resource_ids{};
    /// Splitter id this voice is connected with
    s32 splitter_id{UnusedSplitterId};
    /// Sample rate conversion quality
    SrcQuality src_quality{SrcQuality::Medium};
    /// Was this voice dropped due to limited time?
    bool voice_dropped{};
    /// Is this voice's coefficient (data_address) unmapped?
    bool data_unmapped{};
    /// Is this voice's buffers (wavebuffer data and ADPCM context) unmapped?
    bool buffer_unmapped{};
    /// Initialisation state of the biquads
    std::array<bool, MaxBiquadFilters> biquad_initialized{};
    /// Number of wavebuffers to flush
    u8 flush_buffer_count{};
};

} // namespace AudioCore::AudioRenderer

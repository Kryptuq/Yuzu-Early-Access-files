// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "audio_core/algorithm/interpolate.h"
#include "audio_core/codec.h"
#include "audio_core/common.h"
#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"

namespace Core::Memory {
class Memory;
}

namespace AudioCore {

class BehaviorInfo;
class VoiceContext;

enum class SampleFormat : u8 {
    Invalid = 0,
    Pcm8 = 1,
    Pcm16 = 2,
    Pcm24 = 3,
    Pcm32 = 4,
    PcmFloat = 5,
    Adpcm = 6,
};

enum class PlayState : u8 {
    Started = 0,
    Stopped = 1,
    Paused = 2,
};

enum class ServerPlayState {
    Play = 0,
    Stop = 1,
    RequestStop = 2,
    Paused = 3,
};

struct BiquadFilterParameter {
    bool enabled{};
    INSERT_PADDING_BYTES(1);
    std::array<s16, 3> numerator{};
    std::array<s16, 2> denominator{};
};
static_assert(sizeof(BiquadFilterParameter) == 0xc, "BiquadFilterParameter is an invalid size");

struct WaveBuffer {
    u64_le buffer_address{};
    u64_le buffer_size{};
    s32_le start_sample_offset{};
    s32_le end_sample_offset{};
    u8 is_looping{};
    u8 end_of_stream{};
    u8 sent_to_server{};
    INSERT_PADDING_BYTES(5);
    u64 context_address{};
    u64 context_size{};
    INSERT_PADDING_BYTES(8);
};
static_assert(sizeof(WaveBuffer) == 0x38, "WaveBuffer is an invalid size");

struct ServerWaveBuffer {
    VAddr buffer_address{};
    std::size_t buffer_size{};
    s32 start_sample_offset{};
    s32 end_sample_offset{};
    bool is_looping{};
    bool end_of_stream{};
    VAddr context_address{};
    std::size_t context_size{};
    bool sent_to_dsp{true};
};

struct BehaviorFlags {
    BitField<0, 1, u16> is_played_samples_reset_at_loop_point;
    BitField<1, 1, u16> is_pitch_and_src_skipped;
};
static_assert(sizeof(BehaviorFlags) == 0x4, "BehaviorFlags is an invalid size");

struct ADPCMContext {
    u16 header;
    s16 yn1;
    s16 yn2;
};
static_assert(sizeof(ADPCMContext) == 0x6, "ADPCMContext is an invalid size");

struct VoiceState {
    s64 played_sample_count;
    s32 offset;
    s32 wave_buffer_index;
    std::array<bool, AudioCommon::MAX_WAVE_BUFFERS> is_wave_buffer_valid;
    s32 wave_buffer_consumed;
    std::array<s32, AudioCommon::MAX_SAMPLE_HISTORY> sample_history;
    s32 fraction;
    VAddr context_address;
    Codec::ADPCM_Coeff coeff;
    ADPCMContext context;
    std::array<s64, 2> biquad_filter_state;
    std::array<s32, AudioCommon::MAX_MIX_BUFFERS> previous_samples;
    u32 external_context_size;
    bool is_external_context_used;
    bool voice_dropped;
};

class VoiceChannelResource {
public:
    struct InParams {
        s32_le id{};
        std::array<float_le, AudioCommon::MAX_MIX_BUFFERS> mix_volume{};
        bool in_use{};
        INSERT_PADDING_BYTES(11);
    };
    static_assert(sizeof(InParams) == 0x70, "InParams is an invalid size");
};

class ServerVoiceChannelResource {
public:
    explicit ServerVoiceChannelResource(s32 id_);
    ~ServerVoiceChannelResource();

    bool InUse() const;
    float GetCurrentMixVolumeAt(std::size_t i) const;
    float GetLastMixVolumeAt(std::size_t i) const;
    void Update(VoiceChannelResource::InParams& in_params);
    void UpdateLastMixVolumes();

    const std::array<float, AudioCommon::MAX_MIX_BUFFERS>& GetCurrentMixVolume() const;
    const std::array<float, AudioCommon::MAX_MIX_BUFFERS>& GetLastMixVolume() const;

private:
    s32 id{};
    std::array<float, AudioCommon::MAX_MIX_BUFFERS> mix_volume{};
    std::array<float, AudioCommon::MAX_MIX_BUFFERS> last_mix_volume{};
    bool in_use{};
};

class VoiceInfo {
public:
    struct InParams {
        s32_le id{};
        u32_le node_id{};
        u8 is_new{};
        u8 is_in_use{};
        PlayState play_state{};
        SampleFormat sample_format{};
        s32_le sample_rate{};
        s32_le priority{};
        s32_le sorting_order{};
        s32_le channel_count{};
        float_le pitch{};
        float_le volume{};
        std::array<BiquadFilterParameter, 2> biquad_filter{};
        s32_le wave_buffer_count{};
        s16_le wave_buffer_head{};
        INSERT_PADDING_BYTES(6);
        u64_le additional_params_address{};
        u64_le additional_params_size{};
        s32_le mix_id{};
        s32_le splitter_info_id{};
        std::array<WaveBuffer, 4> wave_buffer{};
        std::array<u32_le, 6> voice_channel_resource_ids{};
        // TODO(ogniK): Remaining flags
        u8 is_voice_drop_flag_clear_requested{};
        u8 wave_buffer_flush_request_count{};
        INSERT_PADDING_BYTES(2);
        BehaviorFlags behavior_flags{};
        INSERT_PADDING_BYTES(16);
    };
    static_assert(sizeof(InParams) == 0x170, "InParams is an invalid size");

    struct OutParams {
        u64_le played_sample_count{};
        u32_le wave_buffer_consumed{};
        u8 voice_dropped{};
        INSERT_PADDING_BYTES(3);
    };
    static_assert(sizeof(OutParams) == 0x10, "OutParams is an invalid size");
};

class ServerVoiceInfo {
public:
    struct InParams {
        bool in_use{};
        bool is_new{};
        bool should_depop{};
        SampleFormat sample_format{};
        s32 sample_rate{};
        s32 channel_count{};
        s32 id{};
        s32 node_id{};
        s32 mix_id{};
        ServerPlayState current_playstate{};
        ServerPlayState last_playstate{};
        s32 priority{};
        s32 sorting_order{};
        float pitch{};
        float volume{};
        float last_volume{};
        std::array<BiquadFilterParameter, AudioCommon::MAX_BIQUAD_FILTERS> biquad_filter{};
        s32 wave_buffer_count{};
        s16 wave_bufffer_head{};
        INSERT_PADDING_BYTES(2);
        BehaviorFlags behavior_flags{};
        VAddr additional_params_address{};
        std::size_t additional_params_size{};
        std::array<ServerWaveBuffer, AudioCommon::MAX_WAVE_BUFFERS> wave_buffer{};
        std::array<s32, AudioCommon::MAX_CHANNEL_COUNT> voice_channel_resource_id{};
        s32 splitter_info_id{};
        u8 wave_buffer_flush_request_count{};
        bool voice_drop_flag{};
        bool buffer_mapped{};
        std::array<bool, AudioCommon::MAX_BIQUAD_FILTERS> was_biquad_filter_enabled{};
    };

    struct OutParams {
        s64 played_sample_count{};
        s32 wave_buffer_consumed{};
    };

    ServerVoiceInfo();
    ~ServerVoiceInfo();
    void Initialize();
    void UpdateParameters(const VoiceInfo::InParams& voice_in, BehaviorInfo& behavior_info);
    void UpdateWaveBuffers(const VoiceInfo::InParams& voice_in,
                           std::array<VoiceState*, AudioCommon::MAX_CHANNEL_COUNT>& voice_states,
                           BehaviorInfo& behavior_info);
    void UpdateWaveBuffer(ServerWaveBuffer& out_wavebuffer, const WaveBuffer& in_wave_buffer,
                          SampleFormat sample_format, bool is_buffer_valid,
                          BehaviorInfo& behavior_info);
    void WriteOutStatus(VoiceInfo::OutParams& voice_out, VoiceInfo::InParams& voice_in,
                        std::array<VoiceState*, AudioCommon::MAX_CHANNEL_COUNT>& voice_states);

    const InParams& GetInParams() const;
    InParams& GetInParams();

    const OutParams& GetOutParams() const;
    OutParams& GetOutParams();

    bool ShouldSkip() const;
    bool UpdateForCommandGeneration(VoiceContext& voice_context);
    void ResetResources(VoiceContext& voice_context);
    bool UpdateParametersForCommandGeneration(
        std::array<VoiceState*, AudioCommon::MAX_CHANNEL_COUNT>& dsp_voice_states);
    void FlushWaveBuffers(u8 flush_count,
                          std::array<VoiceState*, AudioCommon::MAX_CHANNEL_COUNT>& dsp_voice_states,
                          s32 channel_count);

private:
    std::vector<s16> stored_samples;
    InParams in_params{};
    OutParams out_params{};

    bool HasValidWaveBuffer(const VoiceState* state) const;
};

class VoiceContext {
public:
    explicit VoiceContext(std::size_t voice_count_);
    ~VoiceContext();

    std::size_t GetVoiceCount() const;
    ServerVoiceChannelResource& GetChannelResource(std::size_t i);
    const ServerVoiceChannelResource& GetChannelResource(std::size_t i) const;
    VoiceState& GetState(std::size_t i);
    const VoiceState& GetState(std::size_t i) const;
    VoiceState& GetDspSharedState(std::size_t i);
    const VoiceState& GetDspSharedState(std::size_t i) const;
    ServerVoiceInfo& GetInfo(std::size_t i);
    const ServerVoiceInfo& GetInfo(std::size_t i) const;
    ServerVoiceInfo& GetSortedInfo(std::size_t i);
    const ServerVoiceInfo& GetSortedInfo(std::size_t i) const;

    s32 DecodePcm16(s32* output_buffer, ServerWaveBuffer* wave_buffer, s32 channel,
                    s32 channel_count, s32 buffer_offset, s32 sample_count,
                    Core::Memory::Memory& memory);
    void SortInfo();
    void UpdateStateByDspShared();

private:
    std::size_t voice_count{};
    std::vector<ServerVoiceChannelResource> voice_channel_resources{};
    std::vector<VoiceState> voice_states{};
    std::vector<VoiceState> dsp_voice_states{};
    std::vector<ServerVoiceInfo> voice_info{};
    std::vector<ServerVoiceInfo*> sorted_voice_info{};
};

} // namespace AudioCore

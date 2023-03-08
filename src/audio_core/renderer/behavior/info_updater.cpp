// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/common/feature_support.h"
#include "audio_core/renderer/behavior/behavior_info.h"
#include "audio_core/renderer/behavior/info_updater.h"
#include "audio_core/renderer/effect/effect_context.h"
#include "audio_core/renderer/effect/effect_reset.h"
#include "audio_core/renderer/memory/memory_pool_info.h"
#include "audio_core/renderer/mix/mix_context.h"
#include "audio_core/renderer/performance/performance_manager.h"
#include "audio_core/renderer/sink/circular_buffer_sink_info.h"
#include "audio_core/renderer/sink/device_sink_info.h"
#include "audio_core/renderer/sink/sink_context.h"
#include "audio_core/renderer/splitter/splitter_context.h"
#include "audio_core/renderer/voice/voice_context.h"

namespace AudioCore::AudioRenderer {

InfoUpdater::InfoUpdater(std::span<const u8> input_, std::span<u8> output_,
                         const u32 process_handle_, BehaviorInfo& behaviour_)
    : input{input_.data() + sizeof(UpdateDataHeader)},
      input_origin{input_}, output{output_.data() + sizeof(UpdateDataHeader)},
      output_origin{output_}, in_header{reinterpret_cast<const UpdateDataHeader*>(
                                  input_origin.data())},
      out_header{reinterpret_cast<UpdateDataHeader*>(output_origin.data())},
      expected_input_size{input_.size()}, expected_output_size{output_.size()},
      process_handle{process_handle_}, behaviour{behaviour_} {
    std::construct_at<UpdateDataHeader>(out_header, behaviour.GetProcessRevision());
}

Result InfoUpdater::UpdateVoiceChannelResources(VoiceContext& voice_context) {
    const auto voice_count{voice_context.GetCount()};
    std::span<const VoiceChannelResource::InParameter> in_params{
        reinterpret_cast<const VoiceChannelResource::InParameter*>(input), voice_count};

    for (u32 i = 0; i < voice_count; i++) {
        auto& resource{voice_context.GetChannelResource(i)};
        resource.in_use = in_params[i].in_use;
        if (in_params[i].in_use) {
            resource.mix_volumes = in_params[i].mix_volumes;
        }
    }

    const auto consumed_input_size{voice_count *
                                   static_cast<u32>(sizeof(VoiceChannelResource::InParameter))};
    if (consumed_input_size != in_header->voice_resources_size) {
        LOG_ERROR(Service_Audio,
                  "Consumed an incorrect voice resource size, header size={}, consumed={}",
                  in_header->voice_resources_size, consumed_input_size);
        return Service::Audio::ResultInvalidUpdateInfo;
    }

    input += consumed_input_size;
    return ResultSuccess;
}

Result InfoUpdater::UpdateVoices(VoiceContext& voice_context,
                                 std::span<MemoryPoolInfo> memory_pools,
                                 const u32 memory_pool_count) {
    const PoolMapper pool_mapper(process_handle, memory_pools, memory_pool_count,
                                 behaviour.IsMemoryForceMappingEnabled());
    const auto voice_count{voice_context.GetCount()};
    std::span<const VoiceInfo::InParameter> in_params{
        reinterpret_cast<const VoiceInfo::InParameter*>(input), voice_count};
    std::span<VoiceInfo::OutStatus> out_params{reinterpret_cast<VoiceInfo::OutStatus*>(output),
                                               voice_count};

    for (u32 i = 0; i < voice_count; i++) {
        auto& voice_info{voice_context.GetInfo(i)};
        voice_info.in_use = false;
    }

    u32 new_voice_count{0};

    for (u32 i = 0; i < voice_count; i++) {
        const auto& in_param{in_params[i]};
        std::array<VoiceState*, MaxChannels> voice_states{};

        if (!in_param.in_use) {
            continue;
        }

        auto& voice_info{voice_context.GetInfo(in_param.id)};

        for (u32 channel = 0; channel < in_param.channel_count; channel++) {
            voice_states[channel] = &voice_context.GetState(in_param.channel_resource_ids[channel]);
        }

        if (in_param.is_new) {
            voice_info.Initialize();

            for (u32 channel = 0; channel < in_param.channel_count; channel++) {
                *voice_states[channel] = {};
            }
        }

        BehaviorInfo::ErrorInfo update_error{};
        voice_info.UpdateParameters(update_error, in_param, pool_mapper, behaviour);

        if (!update_error.error_code.IsSuccess()) {
            behaviour.AppendError(update_error);
        }

        std::array<std::array<BehaviorInfo::ErrorInfo, 2>, MaxWaveBuffers> wavebuffer_errors{};
        voice_info.UpdateWaveBuffers(wavebuffer_errors, MaxWaveBuffers * 2, in_param, voice_states,
                                     pool_mapper, behaviour);

        for (auto& wavebuffer_error : wavebuffer_errors) {
            for (auto& error : wavebuffer_error) {
                if (error.error_code.IsError()) {
                    behaviour.AppendError(error);
                }
            }
        }

        voice_info.WriteOutStatus(out_params[i], in_param, voice_states);
        new_voice_count += in_param.channel_count;
    }

    auto consumed_input_size{voice_count * static_cast<u32>(sizeof(VoiceInfo::InParameter))};
    auto consumed_output_size{voice_count * static_cast<u32>(sizeof(VoiceInfo::OutStatus))};
    if (consumed_input_size != in_header->voices_size) {
        LOG_ERROR(Service_Audio, "Consumed an incorrect voices size, header size={}, consumed={}",
                  in_header->voices_size, consumed_input_size);
        return Service::Audio::ResultInvalidUpdateInfo;
    }

    out_header->voices_size = consumed_output_size;
    out_header->size += consumed_output_size;
    input += consumed_input_size;
    output += consumed_output_size;

    voice_context.SetActiveCount(new_voice_count);

    return ResultSuccess;
}

Result InfoUpdater::UpdateEffects(EffectContext& effect_context, const bool renderer_active,
                                  std::span<MemoryPoolInfo> memory_pools,
                                  const u32 memory_pool_count) {
    if (behaviour.IsEffectInfoVersion2Supported()) {
        return UpdateEffectsVersion2(effect_context, renderer_active, memory_pools,
                                     memory_pool_count);
    } else {
        return UpdateEffectsVersion1(effect_context, renderer_active, memory_pools,
                                     memory_pool_count);
    }
}

Result InfoUpdater::UpdateEffectsVersion1(EffectContext& effect_context, const bool renderer_active,
                                          std::span<MemoryPoolInfo> memory_pools,
                                          const u32 memory_pool_count) {
    PoolMapper pool_mapper(process_handle, memory_pools, memory_pool_count,
                           behaviour.IsMemoryForceMappingEnabled());

    const auto effect_count{effect_context.GetCount()};

    std::span<const EffectInfoBase::InParameterVersion1> in_params{
        reinterpret_cast<const EffectInfoBase::InParameterVersion1*>(input), effect_count};
    std::span<EffectInfoBase::OutStatusVersion1> out_params{
        reinterpret_cast<EffectInfoBase::OutStatusVersion1*>(output), effect_count};

    for (u32 i = 0; i < effect_count; i++) {
        auto effect_info{&effect_context.GetInfo(i)};
        if (effect_info->GetType() != in_params[i].type) {
            effect_info->ForceUnmapBuffers(pool_mapper);
            ResetEffect(effect_info, in_params[i].type);
        }

        BehaviorInfo::ErrorInfo error_info{};
        effect_info->Update(error_info, in_params[i], pool_mapper);
        if (error_info.error_code.IsError()) {
            behaviour.AppendError(error_info);
        }

        effect_info->StoreStatus(out_params[i], renderer_active);
    }

    auto consumed_input_size{effect_count *
                             static_cast<u32>(sizeof(EffectInfoBase::InParameterVersion1))};
    auto consumed_output_size{effect_count *
                              static_cast<u32>(sizeof(EffectInfoBase::OutStatusVersion1))};
    if (consumed_input_size != in_header->effects_size) {
        LOG_ERROR(Service_Audio, "Consumed an incorrect effects size, header size={}, consumed={}",
                  in_header->effects_size, consumed_input_size);
        return Service::Audio::ResultInvalidUpdateInfo;
    }

    out_header->effects_size = consumed_output_size;
    out_header->size += consumed_output_size;
    input += consumed_input_size;
    output += consumed_output_size;

    return ResultSuccess;
}

Result InfoUpdater::UpdateEffectsVersion2(EffectContext& effect_context, const bool renderer_active,
                                          std::span<MemoryPoolInfo> memory_pools,
                                          const u32 memory_pool_count) {
    PoolMapper pool_mapper(process_handle, memory_pools, memory_pool_count,
                           behaviour.IsMemoryForceMappingEnabled());

    const auto effect_count{effect_context.GetCount()};

    std::span<const EffectInfoBase::InParameterVersion2> in_params{
        reinterpret_cast<const EffectInfoBase::InParameterVersion2*>(input), effect_count};
    std::span<EffectInfoBase::OutStatusVersion2> out_params{
        reinterpret_cast<EffectInfoBase::OutStatusVersion2*>(output), effect_count};

    for (u32 i = 0; i < effect_count; i++) {
        auto effect_info{&effect_context.GetInfo(i)};
        if (effect_info->GetType() != in_params[i].type) {
            effect_info->ForceUnmapBuffers(pool_mapper);
            ResetEffect(effect_info, in_params[i].type);
        }

        BehaviorInfo::ErrorInfo error_info{};
        effect_info->Update(error_info, in_params[i], pool_mapper);

        if (error_info.error_code.IsError()) {
            behaviour.AppendError(error_info);
        }

        effect_info->StoreStatus(out_params[i], renderer_active);

        if (in_params[i].is_new) {
            effect_info->InitializeResultState(effect_context.GetDspSharedResultState(i));
            effect_info->InitializeResultState(effect_context.GetResultState(i));
        }
        effect_info->UpdateResultState(out_params[i].result_state,
                                       effect_context.GetResultState(i));
    }

    auto consumed_input_size{effect_count *
                             static_cast<u32>(sizeof(EffectInfoBase::InParameterVersion2))};
    auto consumed_output_size{effect_count *
                              static_cast<u32>(sizeof(EffectInfoBase::OutStatusVersion2))};
    if (consumed_input_size != in_header->effects_size) {
        LOG_ERROR(Service_Audio, "Consumed an incorrect effects size, header size={}, consumed={}",
                  in_header->effects_size, consumed_input_size);
        return Service::Audio::ResultInvalidUpdateInfo;
    }

    out_header->effects_size = consumed_output_size;
    out_header->size += consumed_output_size;
    input += consumed_input_size;
    output += consumed_output_size;

    return ResultSuccess;
}

Result InfoUpdater::UpdateMixes(MixContext& mix_context, const u32 mix_buffer_count,
                                EffectContext& effect_context, SplitterContext& splitter_context) {
    s32 mix_count{0};
    u32 consumed_input_size{0};

    if (behaviour.IsMixInParameterDirtyOnlyUpdateSupported()) {
        auto in_dirty_params{reinterpret_cast<const MixInfo::InDirtyParameter*>(input)};
        mix_count = in_dirty_params->count;
        input += sizeof(MixInfo::InDirtyParameter);
        consumed_input_size = static_cast<u32>(sizeof(MixInfo::InDirtyParameter) +
                                               mix_count * sizeof(MixInfo::InParameter));
    } else {
        mix_count = mix_context.GetCount();
        consumed_input_size = static_cast<u32>(mix_count * sizeof(MixInfo::InParameter));
    }

    if (mix_buffer_count == 0) {
        return Service::Audio::ResultInvalidUpdateInfo;
    }

    std::span<const MixInfo::InParameter> in_params{
        reinterpret_cast<const MixInfo::InParameter*>(input), static_cast<size_t>(mix_count)};

    u32 total_buffer_count{0};
    for (s32 i = 0; i < mix_count; i++) {
        const auto& params{in_params[i]};

        if (params.in_use) {
            total_buffer_count += params.buffer_count;
            if (params.dest_mix_id > static_cast<s32>(mix_context.GetCount()) &&
                params.dest_mix_id != UnusedMixId && params.mix_id != FinalMixId) {
                return Service::Audio::ResultInvalidUpdateInfo;
            }
        }
    }

    if (total_buffer_count > mix_buffer_count) {
        return Service::Audio::ResultInvalidUpdateInfo;
    }

    bool mix_dirty{false};
    for (s32 i = 0; i < mix_count; i++) {
        const auto& params{in_params[i]};

        s32 mix_id{i};
        if (behaviour.IsMixInParameterDirtyOnlyUpdateSupported()) {
            mix_id = params.mix_id;
        }

        auto mix_info{mix_context.GetInfo(mix_id)};
        if (mix_info->in_use != params.in_use) {
            mix_info->in_use = params.in_use;
            if (!params.in_use) {
                mix_info->ClearEffectProcessingOrder();
            }
            mix_dirty = true;
        }

        if (params.in_use) {
            mix_dirty |= mix_info->Update(mix_context.GetEdgeMatrix(), params, effect_context,
                                          splitter_context, behaviour);
        }
    }

    if (mix_dirty) {
        if (behaviour.IsSplitterSupported() && splitter_context.UsingSplitter()) {
            if (!mix_context.TSortInfo(splitter_context)) {
                return Service::Audio::ResultInvalidUpdateInfo;
            }
        } else {
            mix_context.SortInfo();
        }
    }

    if (consumed_input_size != in_header->mix_size) {
        LOG_ERROR(Service_Audio, "Consumed an incorrect mixes size, header size={}, consumed={}",
                  in_header->mix_size, consumed_input_size);
        return Service::Audio::ResultInvalidUpdateInfo;
    }

    input += mix_count * sizeof(MixInfo::InParameter);

    return ResultSuccess;
}

Result InfoUpdater::UpdateSinks(SinkContext& sink_context, std::span<MemoryPoolInfo> memory_pools,
                                const u32 memory_pool_count) {
    PoolMapper pool_mapper(process_handle, memory_pools, memory_pool_count,
                           behaviour.IsMemoryForceMappingEnabled());

    std::span<const SinkInfoBase::InParameter> in_params{
        reinterpret_cast<const SinkInfoBase::InParameter*>(input), memory_pool_count};
    std::span<SinkInfoBase::OutStatus> out_params{
        reinterpret_cast<SinkInfoBase::OutStatus*>(output), memory_pool_count};

    const auto sink_count{sink_context.GetCount()};

    for (u32 i = 0; i < sink_count; i++) {
        const auto& params{in_params[i]};
        auto sink_info{sink_context.GetInfo(i)};

        if (sink_info->GetType() != params.type) {
            sink_info->CleanUp();
            switch (params.type) {
            case SinkInfoBase::Type::Invalid:
                std::construct_at<SinkInfoBase>(reinterpret_cast<SinkInfoBase*>(sink_info));
                break;
            case SinkInfoBase::Type::DeviceSink:
                std::construct_at<DeviceSinkInfo>(reinterpret_cast<DeviceSinkInfo*>(sink_info));
                break;
            case SinkInfoBase::Type::CircularBufferSink:
                std::construct_at<CircularBufferSinkInfo>(
                    reinterpret_cast<CircularBufferSinkInfo*>(sink_info));
                break;
            default:
                LOG_ERROR(Service_Audio, "Invalid sink type {}", static_cast<u32>(params.type));
                break;
            }
        }

        BehaviorInfo::ErrorInfo error_info{};
        sink_info->Update(error_info, out_params[i], params, pool_mapper);

        if (error_info.error_code.IsError()) {
            behaviour.AppendError(error_info);
        }
    }

    const auto consumed_input_size{sink_count *
                                   static_cast<u32>(sizeof(SinkInfoBase::InParameter))};
    const auto consumed_output_size{sink_count * static_cast<u32>(sizeof(SinkInfoBase::OutStatus))};
    if (consumed_input_size != in_header->sinks_size) {
        LOG_ERROR(Service_Audio, "Consumed an incorrect sinks size, header size={}, consumed={}",
                  in_header->sinks_size, consumed_input_size);
        return Service::Audio::ResultInvalidUpdateInfo;
    }

    input += consumed_input_size;
    output += consumed_output_size;
    out_header->sinks_size = consumed_output_size;
    out_header->size += consumed_output_size;

    return ResultSuccess;
}

Result InfoUpdater::UpdateMemoryPools(std::span<MemoryPoolInfo> memory_pools,
                                      const u32 memory_pool_count) {
    PoolMapper pool_mapper(process_handle, memory_pools, memory_pool_count,
                           behaviour.IsMemoryForceMappingEnabled());
    std::span<const MemoryPoolInfo::InParameter> in_params{
        reinterpret_cast<const MemoryPoolInfo::InParameter*>(input), memory_pool_count};
    std::span<MemoryPoolInfo::OutStatus> out_params{
        reinterpret_cast<MemoryPoolInfo::OutStatus*>(output), memory_pool_count};

    for (size_t i = 0; i < memory_pool_count; i++) {
        auto state{pool_mapper.Update(memory_pools[i], in_params[i], out_params[i])};
        if (state != MemoryPoolInfo::ResultState::Success &&
            state != MemoryPoolInfo::ResultState::BadParam &&
            state != MemoryPoolInfo::ResultState::MapFailed &&
            state != MemoryPoolInfo::ResultState::InUse) {
            LOG_WARNING(Service_Audio, "Invalid ResultState from updating memory pools");
            return Service::Audio::ResultInvalidUpdateInfo;
        }
    }

    const auto consumed_input_size{memory_pool_count *
                                   static_cast<u32>(sizeof(MemoryPoolInfo::InParameter))};
    const auto consumed_output_size{memory_pool_count *
                                    static_cast<u32>(sizeof(MemoryPoolInfo::OutStatus))};
    if (consumed_input_size != in_header->memory_pool_size) {
        LOG_ERROR(Service_Audio,
                  "Consumed an incorrect memory pool size, header size={}, consumed={}",
                  in_header->memory_pool_size, consumed_input_size);
        return Service::Audio::ResultInvalidUpdateInfo;
    }

    input += consumed_input_size;
    output += consumed_output_size;
    out_header->memory_pool_size = consumed_output_size;
    out_header->size += consumed_output_size;
    return ResultSuccess;
}

Result InfoUpdater::UpdatePerformanceBuffer(std::span<u8> performance_output,
                                            const u64 performance_output_size,
                                            PerformanceManager* performance_manager) {
    auto in_params{reinterpret_cast<const PerformanceManager::InParameter*>(input)};
    auto out_params{reinterpret_cast<PerformanceManager::OutStatus*>(output)};

    if (performance_manager != nullptr) {
        out_params->history_size =
            performance_manager->CopyHistories(performance_output.data(), performance_output_size);
        performance_manager->SetDetailTarget(in_params->target_node_id);
    } else {
        out_params->history_size = 0;
    }

    const auto consumed_input_size{static_cast<u32>(sizeof(PerformanceManager::InParameter))};
    const auto consumed_output_size{static_cast<u32>(sizeof(PerformanceManager::OutStatus))};
    if (consumed_input_size != in_header->performance_buffer_size) {
        LOG_ERROR(Service_Audio,
                  "Consumed an incorrect performance size, header size={}, consumed={}",
                  in_header->performance_buffer_size, consumed_input_size);
        return Service::Audio::ResultInvalidUpdateInfo;
    }

    input += consumed_input_size;
    output += consumed_output_size;
    out_header->performance_buffer_size = consumed_output_size;
    out_header->size += consumed_output_size;
    return ResultSuccess;
}

Result InfoUpdater::UpdateBehaviorInfo(BehaviorInfo& behaviour_) {
    const auto in_params{reinterpret_cast<const BehaviorInfo::InParameter*>(input)};

    if (!CheckValidRevision(in_params->revision)) {
        return Service::Audio::ResultInvalidUpdateInfo;
    }

    if (in_params->revision != behaviour_.GetUserRevision()) {
        return Service::Audio::ResultInvalidUpdateInfo;
    }

    behaviour_.ClearError();
    behaviour_.UpdateFlags(in_params->flags);

    if (in_header->behaviour_size != sizeof(BehaviorInfo::InParameter)) {
        return Service::Audio::ResultInvalidUpdateInfo;
    }

    input += sizeof(BehaviorInfo::InParameter);
    return ResultSuccess;
}

Result InfoUpdater::UpdateErrorInfo(const BehaviorInfo& behaviour_) {
    auto out_params{reinterpret_cast<BehaviorInfo::OutStatus*>(output)};
    behaviour_.CopyErrorInfo(out_params->errors, out_params->error_count);

    const auto consumed_output_size{static_cast<u32>(sizeof(BehaviorInfo::OutStatus))};

    output += consumed_output_size;
    out_header->behaviour_size = consumed_output_size;
    out_header->size += consumed_output_size;
    return ResultSuccess;
}

Result InfoUpdater::UpdateSplitterInfo(SplitterContext& splitter_context) {
    u32 consumed_size{0};
    if (!splitter_context.Update(input, consumed_size)) {
        return Service::Audio::ResultInvalidUpdateInfo;
    }

    input += consumed_size;

    return ResultSuccess;
}

Result InfoUpdater::UpdateRendererInfo(const u64 elapsed_frames) {
    struct RenderInfo {
        /* 0x00 */ u64 frames_elapsed;
        /* 0x08 */ char unk08[0x8];
    };
    static_assert(sizeof(RenderInfo) == 0x10, "RenderInfo has the wrong size!");

    auto out_params{reinterpret_cast<RenderInfo*>(output)};
    out_params->frames_elapsed = elapsed_frames;

    const auto consumed_output_size{static_cast<u32>(sizeof(RenderInfo))};

    output += consumed_output_size;
    out_header->render_info_size = consumed_output_size;
    out_header->size += consumed_output_size;

    return ResultSuccess;
}

Result InfoUpdater::CheckConsumedSize() {
    if (CpuAddr(input) - CpuAddr(input_origin.data()) != expected_input_size) {
        return Service::Audio::ResultInvalidUpdateInfo;
    } else if (CpuAddr(output) - CpuAddr(output_origin.data()) != expected_output_size) {
        return Service::Audio::ResultInvalidUpdateInfo;
    }
    return ResultSuccess;
}

} // namespace AudioCore::AudioRenderer

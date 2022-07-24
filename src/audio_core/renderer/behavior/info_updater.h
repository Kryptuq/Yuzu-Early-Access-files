// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "common/common_types.h"
#include "core/hle/service/audio/errors.h"

namespace AudioCore::AudioRenderer {
class BehaviorInfo;
class VoiceContext;
class MixContext;
class SinkContext;
class SplitterContext;
class EffectContext;
class MemoryPoolInfo;
class PerformanceManager;

class InfoUpdater {
    struct UpdateDataHeader {
        explicit UpdateDataHeader(u32 revision_) : revision{revision_} {}

        /* 0x00 */ u32 revision;
        /* 0x04 */ u32 behaviour_size{};
        /* 0x08 */ u32 memory_pool_size{};
        /* 0x0C */ u32 voices_size{};
        /* 0x10 */ u32 voice_resources_size{};
        /* 0x14 */ u32 effects_size{};
        /* 0x18 */ u32 mix_size{};
        /* 0x1C */ u32 sinks_size{};
        /* 0x20 */ u32 performance_buffer_size{};
        /* 0x24 */ char unk24[4];
        /* 0x28 */ u32 render_info_size{};
        /* 0x2C */ char unk2C[0x10];
        /* 0x3C */ u32 size{sizeof(UpdateDataHeader)};
    };
    static_assert(sizeof(UpdateDataHeader) == 0x40, "UpdateDataHeader has the wrong size!");

public:
    explicit InfoUpdater(std::span<const u8> input, std::span<u8> output, u32 process_handle,
                         BehaviorInfo& behaviour);

    /**
     * Update the voice channel resources.
     *
     * @param voice_context - Voice context to update.
     * @return Result code.
     */
    Result UpdateVoiceChannelResources(VoiceContext& voice_context);

    /**
     * Update voices.
     *
     * @param voice_context     - Voice context to update.
     * @param memory_pools      - Memory pools to use for these voices.
     * @param memory_pool_count - Number of memory pools.
     * @return Result code.
     */
    Result UpdateVoices(VoiceContext& voice_context, std::span<MemoryPoolInfo> memory_pools,
                        u32 memory_pool_count);

    /**
     * Update effects.
     *
     * @param effect_context    - Effect context to update.
     * @param renderer_active   - Whether the AudioRenderer is active.
     * @param memory_pools      - Memory pools to use for these voices.
     * @param memory_pool_count - Number of memory pools.
     * @return Result code.
     */
    Result UpdateEffects(EffectContext& effect_context, bool renderer_active,
                         std::span<MemoryPoolInfo> memory_pools, u32 memory_pool_count);

    /**
     * Update mixes.
     *
     * @param mix_context       - Mix context to update.
     * @param mix_buffer_count  - Number of mix buffers.
     * @param effect_context    - Effect context to update effort order.
     * @param splitter_context  - Splitter context for the mixes.
     * @return Result code.
     */
    Result UpdateMixes(MixContext& mix_context, u32 mix_buffer_count, EffectContext& effect_context,
                       SplitterContext& splitter_context);

    /**
     * Update sinks.
     *
     * @param sink_context      - Sink context to update.
     * @param memory_pools      - Memory pools to use for these voices.
     * @param memory_pool_count - Number of memory pools.
     * @return Result code.
     */
    Result UpdateSinks(SinkContext& sink_context, std::span<MemoryPoolInfo> memory_pools,
                       u32 memory_pool_count);

    /**
     * Update memory pools.
     *
     * @param memory_pools      - Memory pools to use for these voices.
     * @param memory_pool_count - Number of memory pools.
     * @return Result code.
     */
    Result UpdateMemoryPools(std::span<MemoryPoolInfo> memory_pools, u32 memory_pool_count);

    /**
     * Update the performance buffer.
     *
     * @param output              - Output buffer for performance metrics.
     * @param output_size         - Output buffer size.
     * @param performance_manager - Performance manager..
     * @return Result code.
     */
    Result UpdatePerformanceBuffer(std::span<u8> output, u64 output_size,
                                   PerformanceManager* performance_manager);

    /**
     * Update behaviour.
     *
     * @param behaviour - Behaviour to update.
     * @return Result code.
     */
    Result UpdateBehaviorInfo(BehaviorInfo& behaviour);

    /**
     * Update errors.
     *
     * @param behaviour - Behaviour to update.
     * @return Result code.
     */
    Result UpdateErrorInfo(BehaviorInfo& behaviour);

    /**
     * Update splitter.
     *
     * @param splitter_context - Splitter context to update.
     * @return Result code.
     */
    Result UpdateSplitterInfo(SplitterContext& splitter_context);

    /**
     * Update renderer info.
     *
     * @param elapsed_frames - Number of elapsed frames.
     * @return Result code.
     */
    Result UpdateRendererInfo(u64 elapsed_frames);

    /**
     * Check that the input.output sizes match their expected values.
     *
     * @return Result code.
     */
    Result CheckConsumedSize();

private:
    /**
     * Update effects version 1.
     *
     * @param effect_context    - Effect context to update.
     * @param renderer_active   - Is the AudioRenderer active?
     * @param memory_pools      - Memory pools to use for these voices.
     * @param memory_pool_count - Number of memory pools.
     * @return Result code.
     */
    Result UpdateEffectsVersion1(EffectContext& effect_context, bool renderer_active,
                                 std::span<MemoryPoolInfo> memory_pools, u32 memory_pool_count);

    /**
     * Update effects version 2.
     *
     * @param effect_context    - Effect context to update.
     * @param renderer_active   - Is the AudioRenderer active?
     * @param memory_pools      - Memory pools to use for these voices.
     * @param memory_pool_count - Number of memory pools.
     * @return Result code.
     */
    Result UpdateEffectsVersion2(EffectContext& effect_context, bool renderer_active,
                                 std::span<MemoryPoolInfo> memory_pools, u32 memory_pool_count);

    /// Input buffer
    u8 const* input;
    /// Input buffer start
    std::span<const u8> input_origin;
    /// Output buffer start
    u8* output;
    /// Output buffer start
    std::span<u8> output_origin;
    /// Input header
    const UpdateDataHeader* in_header;
    /// Output header
    UpdateDataHeader* out_header;
    /// Expected input size, see CheckConsumedSize
    u64 expected_input_size;
    /// Expected output size, see CheckConsumedSize
    u64 expected_output_size;
    /// Unused
    u32 process_handle;
    /// Behaviour
    BehaviorInfo& behaviour;
};

} // namespace AudioCore::AudioRenderer

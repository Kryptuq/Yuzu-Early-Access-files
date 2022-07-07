// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "audio_core/renderer/behavior/behavior_info.h"
#include "audio_core/renderer/memory/memory_pool_info.h"
#include "audio_core/renderer/upsampler/upsampler_manager.h"
#include "common/common_types.h"

namespace AudioCore {
/**
 * Execution mode of the audio renderer.
 * Only Auto is currently supported.
 */
enum class ExecutionMode : u8 {
    Auto,
    Manual,
};

/**
 * Parameters from the game, passed to the audio renderer for initialisation.
 */
struct AudioRendererParameterInternal {
    /* 0x00 */ u32 sample_rate;
    /* 0x04 */ u32 sample_count;
    /* 0x08 */ u32 mixes;
    /* 0x0C */ u32 sub_mixes;
    /* 0x10 */ u32 voices;
    /* 0x14 */ u32 sinks;
    /* 0x18 */ u32 effects;
    /* 0x1C */ u32 perf_frames;
    /* 0x20 */ u16 voice_drop_enabled;
    /* 0x22 */ u8 rendering_device;
    /* 0x23 */ ExecutionMode execution_mode;
    /* 0x24 */ u32 splitter_infos;
    /* 0x28 */ s32 splitter_destinations;
    /* 0x2C */ u32 external_context_size;
    /* 0x30 */ u32 revision;
    /* 0x34 */ char unk34[0x4];
};
static_assert(sizeof(AudioRendererParameterInternal) == 0x38,
              "AudioRendererParameterInternal has the wrong size!");

/**
 * Context for rendering, contains a bunch of useful fields for the command generator.
 */
struct AudioRendererSystemContext {
    s32 session_id;
    s8 channels;
    s16 mix_buffer_count;
    AudioRenderer::BehaviorInfo* behavior;
    std::span<s32> depop_buffer;
    AudioRenderer::UpsamplerManager* upsampler_manager;
    AudioRenderer::MemoryPoolInfo* memory_pool_info;
};

} // namespace AudioCore

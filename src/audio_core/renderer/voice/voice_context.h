// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "audio_core/renderer/voice/voice_channel_resource.h"
#include "audio_core/renderer/voice/voice_info.h"
#include "audio_core/renderer/voice/voice_state.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
/**
 * Contains all voices, with utility functions for managing them.
 */
class VoiceContext {
public:
    /**
     * Get the AudioRenderer state for a given index
     *
     * @param index - State index to get.
     * @return The requested voice state.
     */
    VoiceState& GetDspSharedState(u32 index);

    /**
     * Get the channel resource for a given index
     *
     * @param index - Resource index to get.
     * @return The requested voice resource.
     */
    VoiceChannelResource& GetChannelResource(u32 index);

    /**
     * Initialize the voice context.
     *
     * @param sorted_voice_infos      - Workbuffer for the sorted voices.
     * @param voice_infos             - Workbuffer for the voices.
     * @param voice_channel_resources - Workbuffer for the voice channel resources.
     * @param cpu_states              - Workbuffer for the host-side voice states.
     * @param dsp_states              - Workbuffer for the AudioRenderer-side voice states.
     * @param voice_count             - The number of voices in each workbuffer.
     */
    void Initialize(std::span<VoiceInfo*> sorted_voice_infos, std::span<VoiceInfo> voice_infos,
                    std::span<VoiceChannelResource> voice_channel_resources,
                    std::span<VoiceState> cpu_states, std::span<VoiceState> dsp_states,
                    u32 voice_count);

    /**
     * Get a sorted voice with the given index.
     *
     * @param index - The sorted voice index to get.
     * @return The sorted voice.
     */
    VoiceInfo* GetSortedInfo(u32 index);

    /**
     * Get a voice with the given index.
     *
     * @param index - The voice index to get.
     * @return The voice.
     */
    VoiceInfo& GetInfo(u32 index);

    /**
     * Get a host voice state with the given index.
     *
     * @param index - The host voice state index to get.
     * @return The voice state.
     */
    VoiceState& GetState(u32 index);

    /**
     * Get the maximum number of voices.
     * Not all voices in the buffers may be in use, see GetActiveCount.
     *
     * @return The maximum number of voices.
     */
    u32 GetCount() const;

    /**
     * Get the number of active voices.
     * Can be less than or equal to the maximum number of voices.
     *
     * @return The number of active voices.
     */
    u32 GetActiveCount() const;

    /**
     * Set the number of active voices.
     * Can be less than or equal to the maximum number of voices.
     *
     * @param active_count - The new number of active voices.
     */
    void SetActiveCount(u32 active_count);

    /**
     * Sort all voices. Results are available via GetSortedInfo.
     * Voices are sorted descendingly, according to priority, and then sort order.
     */
    void SortInfo();

    /**
     * Update all voice states, copying AudioRenderer-side states to host-side states.
     */
    void UpdateStateByDspShared();

private:
    /// Sorted voices
    std::span<VoiceInfo*> sorted_voice_info{};
    /// Voices
    std::span<VoiceInfo> voices{};
    /// Channel resources
    std::span<VoiceChannelResource> channel_resources{};
    /// Host-side voice states
    std::span<VoiceState> cpu_states{};
    /// AudioRenderer-side voice states
    std::span<VoiceState> dsp_states{};
    /// Maximum number of voices
    u32 voice_count{};
    /// Number of active voices
    u32 active_count{};
};

} // namespace AudioCore::AudioRenderer

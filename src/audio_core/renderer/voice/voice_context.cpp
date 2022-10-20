// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <ranges>

#include "audio_core/renderer/voice/voice_context.h"

namespace AudioCore::AudioRenderer {

VoiceState& VoiceContext::GetDspSharedState(const u32 index) {
    if (index >= dsp_states.size()) {
        LOG_ERROR(Service_Audio, "Invalid voice dsp state index {:04X}", index);
    }
    return dsp_states[index];
}

VoiceChannelResource& VoiceContext::GetChannelResource(const u32 index) {
    if (index >= channel_resources.size()) {
        LOG_ERROR(Service_Audio, "Invalid voice channel resource index {:04X}", index);
    }
    return channel_resources[index];
}

void VoiceContext::Initialize(std::span<VoiceInfo*> sorted_voice_infos_,
                              std::span<VoiceInfo> voice_infos_,
                              std::span<VoiceChannelResource> voice_channel_resources_,
                              std::span<VoiceState> cpu_states_, std::span<VoiceState> dsp_states_,
                              const u32 voice_count_) {
    sorted_voice_info = sorted_voice_infos_;
    voices = voice_infos_;
    channel_resources = voice_channel_resources_;
    cpu_states = cpu_states_;
    dsp_states = dsp_states_;
    voice_count = voice_count_;
    active_count = 0;
}

VoiceInfo* VoiceContext::GetSortedInfo(const u32 index) {
    if (index >= sorted_voice_info.size()) {
        LOG_ERROR(Service_Audio, "Invalid voice sorted info index {:04X}", index);
    }
    return sorted_voice_info[index];
}

VoiceInfo& VoiceContext::GetInfo(const u32 index) {
    if (index >= voices.size()) {
        LOG_ERROR(Service_Audio, "Invalid voice info index {:04X}", index);
    }
    return voices[index];
}

VoiceState& VoiceContext::GetState(const u32 index) {
    if (index >= cpu_states.size()) {
        LOG_ERROR(Service_Audio, "Invalid voice cpu state index {:04X}", index);
    }
    return cpu_states[index];
}

u32 VoiceContext::GetCount() const {
    return voice_count;
}

u32 VoiceContext::GetActiveCount() const {
    return active_count;
}

void VoiceContext::SetActiveCount(const u32 active_count_) {
    active_count = active_count_;
}

void VoiceContext::SortInfo() {
    for (u32 i = 0; i < voice_count; i++) {
        sorted_voice_info[i] = &voices[i];
    }

    std::ranges::sort(sorted_voice_info, [](const VoiceInfo* a, const VoiceInfo* b) {
        return a->priority != b->priority ? a->priority > b->priority
                                          : a->sort_order > b->sort_order;
    });
}

void VoiceContext::UpdateStateByDspShared() {
    std::memcpy(cpu_states.data(), dsp_states.data(), voice_count * sizeof(VoiceState));
}

} // namespace AudioCore::AudioRenderer

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/splitter/splitter_destinations_data.h"

namespace AudioCore::AudioRenderer {

SplitterDestinationData::SplitterDestinationData(const s32 id_) : id{id_} {}

void SplitterDestinationData::ClearMixVolume() {
    mix_volumes.fill(0.0f);
    prev_mix_volumes.fill(0.0f);
}

s32 SplitterDestinationData::GetId() const {
    return id;
}

bool SplitterDestinationData::IsConfigured() const {
    return in_use && destination_id != UnusedMixId;
}

s32 SplitterDestinationData::GetMixId() const {
    return destination_id;
}

f32 SplitterDestinationData::GetMixVolume(const u32 index) const {
    if (index >= mix_volumes.size()) {
        LOG_ERROR(Service_Audio, "SplitterDestinationData::GetMixVolume Invalid index {}", index);
        return 0.0f;
    }
    return mix_volumes[index];
}

std::span<f32> SplitterDestinationData::GetMixVolume() {
    return mix_volumes;
}

f32 SplitterDestinationData::GetMixVolumePrev(const u32 index) const {
    if (index >= prev_mix_volumes.size()) {
        LOG_ERROR(Service_Audio, "SplitterDestinationData::GetMixVolumePrev Invalid index {}",
                  index);
        return 0.0f;
    }
    return prev_mix_volumes[index];
}

std::span<f32> SplitterDestinationData::GetMixVolumePrev() {
    return prev_mix_volumes;
}

void SplitterDestinationData::Update(const InParameter& params) {
    if (params.id != id || params.magic != GetSplitterSendDataMagic()) {
        return;
    }

    destination_id = params.mix_id;
    mix_volumes = params.mix_volumes;

    if (!in_use && params.in_use) {
        prev_mix_volumes = mix_volumes;
        need_update = false;
    }

    in_use = params.in_use;
}

void SplitterDestinationData::MarkAsNeedToUpdateInternalState() {
    need_update = true;
}

void SplitterDestinationData::UpdateInternalState() {
    if (in_use && need_update) {
        prev_mix_volumes = mix_volumes;
    }
    need_update = false;
}

SplitterDestinationData* SplitterDestinationData::GetNext() const {
    return next;
}

void SplitterDestinationData::SetNext(SplitterDestinationData* next_) {
    next = next_;
}

} // namespace AudioCore::AudioRenderer

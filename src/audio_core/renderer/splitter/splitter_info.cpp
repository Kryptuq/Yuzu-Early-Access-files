// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/splitter/splitter_info.h"

namespace AudioCore::AudioRenderer {

SplitterInfo::SplitterInfo(const s32 id_) : id{id_} {}

void SplitterInfo::InitializeInfos(SplitterInfo* splitters, const u32 count) {
    if (splitters == nullptr) {
        return;
    }

    for (u32 i = 0; i < count; i++) {
        auto& splitter{splitters[i]};
        splitter.destinations = nullptr;
        splitter.destination_count = 0;
        splitter.has_new_connection = true;
    }
}

u32 SplitterInfo::Update(const InParameter* params) {
    if (params->id != id) {
        return 0;
    }
    sample_rate = params->sample_rate;
    has_new_connection = true;
    return static_cast<u32>((sizeof(InParameter) + 3 * sizeof(s32)) +
                            params->destination_count * sizeof(s32));
}

SplitterDestinationData* SplitterInfo::GetData(const u32 destination_id) {
    auto out_destination{destinations};
    u32 i{0};
    while (i < destination_id) {
        if (out_destination == nullptr) {
            break;
        }
        out_destination = out_destination->GetNext();
        i++;
    }

    return out_destination;
}

u32 SplitterInfo::GetDestinationCount() const {
    return destination_count;
}

void SplitterInfo::SetDestinationCount(const u32 count) {
    destination_count = count;
}

bool SplitterInfo::HasNewConnection() const {
    return has_new_connection;
}

void SplitterInfo::ClearNewConnectionFlag() {
    has_new_connection = false;
}

void SplitterInfo::SetNewConnectionFlag() {
    has_new_connection = true;
}

void SplitterInfo::UpdateInternalState() {
    auto destination{destinations};
    while (destination != nullptr) {
        destination->UpdateInternalState();
        destination = destination->GetNext();
    }
}

void SplitterInfo::SetDestinations(SplitterDestinationData* destinations_) {
    destinations = destinations_;
}

} // namespace AudioCore::AudioRenderer

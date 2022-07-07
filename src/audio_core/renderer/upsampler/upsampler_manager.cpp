// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/upsampler/upsampler_manager.h"

namespace AudioCore::AudioRenderer {

UpsamplerManager::UpsamplerManager(const u32 count_, std::span<UpsamplerInfo> infos_,
                                   std::span<s32> workbuffer_)
    : count{count_}, upsampler_infos{infos_}, workbuffer{workbuffer_} {}

UpsamplerInfo* UpsamplerManager::Allocate() {
    std::scoped_lock l{lock};

    if (count == 0) {
        return nullptr;
    }

    u32 free_index{0};
    for (auto& upsampler : upsampler_infos) {
        if (!upsampler.enabled) {
            break;
        }
        free_index++;
    }

    if (free_index >= count) {
        return nullptr;
    }

    auto& upsampler{upsampler_infos[free_index]};
    upsampler.manager = this;
    upsampler.sample_count = TargetSampleCount;
    upsampler.samples_pos = CpuAddr(&workbuffer[upsampler.sample_count * MaxChannels]);
    upsampler.enabled = true;
    return &upsampler;
}

void UpsamplerManager::Free(UpsamplerInfo* info) {
    std::scoped_lock l{lock};
    info->enabled = false;
}

} // namespace AudioCore::AudioRenderer

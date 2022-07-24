// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "audio_core/common/common.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
/**
 * Represents one channel for mixing a voice.
 */
class VoiceChannelResource {
public:
    struct InParameter {
        /* 0x00 */ u32 id;
        /* 0x04 */ std::array<f32, MaxMixBuffers> mix_volumes;
        /* 0x64 */ bool in_use;
        /* 0x65 */ char unk65[0xB];
    };
    static_assert(sizeof(InParameter) == 0x70,
                  "VoiceChannelResource::InParameter has the wrong size!");

    explicit VoiceChannelResource(u32 id_) : id{id_} {}

    /// Current volume for each mix buffer
    std::array<f32, MaxMixBuffers> mix_volumes{};
    /// Previous volume for each mix buffer
    std::array<f32, MaxMixBuffers> prev_mix_volumes{};
    /// Id of this resource
    const u32 id;
    /// Is this resource in use?
    bool in_use{};
};

} // namespace AudioCore::AudioRenderer

// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <span>

#include "audio_core/renderer/upsampler/upsampler_info.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
/**
 * Manages and has utility functions for upsampler infos.
 */
class UpsamplerManager {
public:
    UpsamplerManager(u32 count, std::span<UpsamplerInfo> infos, std::span<s32> workbuffer);

    /**
     * Allocate a new UpsamplerInfo.
     *
     * @return The allocated upsampler, may be nullptr if alloc failed.
     */
    UpsamplerInfo* Allocate();

    /**
     * Free the given upsampler.
     *
     * @param The upsampler to be freed.
     */
    void Free(UpsamplerInfo* info);

private:
    /// Maximum number of upsamplers in the buffer
    const u32 count;
    /// Upsamplers buffer
    std::span<UpsamplerInfo> upsampler_infos;
    /// Workbuffer for upsampling samples
    std::span<s32> workbuffer;
    /// Lock for allocate/free
    std::mutex lock{};
};

} // namespace AudioCore::AudioRenderer

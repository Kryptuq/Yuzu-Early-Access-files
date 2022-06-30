// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "video_core/control/channel_state_cache.inc"
#include "video_core/texture_cache/texture_cache_base.h"

namespace VideoCommon {

TextureCacheChannelInfo::TextureCacheChannelInfo(Tegra::Control::ChannelState& state) noexcept
    : ChannelInfo(state), graphics_image_table{gpu_memory}, graphics_sampler_table{gpu_memory},
      compute_image_table{gpu_memory}, compute_sampler_table{gpu_memory} {}

template class VideoCommon::ChannelSetupCaches<VideoCommon::TextureCacheChannelInfo>;

} // namespace VideoCommon

// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv3 or any later version
// Refer to the license.txt file included.

#include "video_core/control/channel_state_cache.inc"
#include "video_core/texture_cache/texture_cache_base.h"

namespace VideoCommon {

TextureCacheChannelInfo::TextureCacheChannelInfo(Tegra::Control::ChannelState& state) noexcept
    : ChannelInfo(state), graphics_image_table{gpu_memory}, graphics_sampler_table{gpu_memory},
      compute_image_table{gpu_memory}, compute_sampler_table{gpu_memory} {}

template class VideoCommon::ChannelSetupCaches<VideoCommon::TextureCacheChannelInfo>;

} // namespace VideoCommon

#include "video_core/control/channel_state_cache.inc"

namespace VideoCommon {

ChannelInfo::ChannelInfo(Tegra::Control::ChannelState& channel_state)
    : maxwell3d{*channel_state.maxwell_3d}, kepler_compute{*channel_state.kepler_compute},
      gpu_memory{*channel_state.memory_manager} {}

template class VideoCommon::ChannelSetupCaches<VideoCommon::ChannelInfo>;

} // namespace VideoCommon

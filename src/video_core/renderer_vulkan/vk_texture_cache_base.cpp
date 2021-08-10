// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "video_core/renderer_vulkan/vk_texture_cache.h"
#include "video_core/texture_cache/texture_cache.h"

namespace VideoCommon {
template class VideoCommon::TextureCache<Vulkan::TextureCacheParams>;
}

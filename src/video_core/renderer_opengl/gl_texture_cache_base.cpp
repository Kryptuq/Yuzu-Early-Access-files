// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/renderer_opengl/gl_texture_cache.h"
#include "video_core/texture_cache/texture_cache.h"

namespace VideoCommon {
template class VideoCommon::TextureCache<OpenGL::TextureCacheParams>;
}

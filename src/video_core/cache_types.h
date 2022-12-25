// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"

namespace VideoCommon {

enum class CacheType : u32 {
    None = 0,
    TextureCache = 1 << 0,
    QueryCache = 1 << 1,
    BufferCache = 1 << 2,
    ShaderCache = 1 << 3,
    NoTextureCache = QueryCache | BufferCache | ShaderCache,
    NoBufferCache = TextureCache | QueryCache | ShaderCache,
    NoQueryCache = TextureCache | BufferCache | ShaderCache,
    All = TextureCache | QueryCache | BufferCache | ShaderCache,
};
DECLARE_ENUM_FLAG_OPERATORS(CacheType)

} // namespace VideoCommon

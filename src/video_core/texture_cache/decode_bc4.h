// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <span>

#include "common/common_types.h"
#include "video_core/texture_cache/types.h"

namespace VideoCommon {

void DecompressBC4(std::span<const u8> data, Extent3D extent, std::span<u8> output);

} // namespace VideoCommon

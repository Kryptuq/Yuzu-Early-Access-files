// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <unordered_map>

#include "common/common_types.h"
#include "video_core/shader/registry.h"

namespace VideoCommon::Shader {

struct VaryingTFB {
    std::size_t buffer;
    std::size_t stride;
    std::size_t offset;
    std::size_t components;
};

std::unordered_map<u8, VaryingTFB> BuildTransformFeedback(const GraphicsInfo& info);

} // namespace VideoCommon::Shader

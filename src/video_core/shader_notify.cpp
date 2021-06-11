// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <mutex>
#include "video_core/shader_notify.h"

using namespace std::chrono_literals;

namespace VideoCore {
namespace {
constexpr auto UPDATE_TICK = 32ms;
}

ShaderNotify::ShaderNotify() = default;
ShaderNotify::~ShaderNotify() = default;

std::size_t ShaderNotify::GetShadersBuilding() {
    const auto now = std::chrono::high_resolution_clock::now();
    const auto diff = now - last_update;
    if (diff > UPDATE_TICK) {
        std::shared_lock lock(mutex);
        last_updated_count = accurate_count;
    }
    return last_updated_count;
}

std::size_t ShaderNotify::GetShadersBuildingAccurate() {
    std::shared_lock lock{mutex};
    return accurate_count;
}

void ShaderNotify::MarkShaderComplete() {
    std::unique_lock lock{mutex};
    accurate_count--;
}

void ShaderNotify::MarkSharderBuilding() {
    std::unique_lock lock{mutex};
    accurate_count++;
}

} // namespace VideoCore

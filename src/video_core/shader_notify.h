// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <chrono>
#include <shared_mutex>
#include "common/common_types.h"

namespace VideoCore {
class ShaderNotify {
public:
    ShaderNotify();
    ~ShaderNotify();

    std::size_t GetShadersBuilding();
    std::size_t GetShadersBuildingAccurate();

    void MarkShaderComplete();
    void MarkSharderBuilding();

private:
    std::size_t last_updated_count{};
    std::size_t accurate_count{};
    std::shared_mutex mutex;
    std::chrono::high_resolution_clock::time_point last_update{};
};
} // namespace VideoCore

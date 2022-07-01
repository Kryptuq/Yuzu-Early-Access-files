// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv3 or any later version
// Refer to the license.txt file included.

#include "core/core.h"
#include "video_core/host1x/host1x.h"

namespace Tegra {

namespace Host1x {

Host1x::Host1x(Core::System& system_)
    : system{system_}, syncpoint_manager{}, memory_manager{system, 32, 12},
      allocator{std::make_unique<Common::FlatAllocator<u32, 0, 32>>(1 << 12)} {}

} // namespace Host1x

} // namespace Tegra

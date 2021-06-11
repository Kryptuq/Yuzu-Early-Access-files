// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Core::Frontend {
class EmuWindow;
}

namespace Vulkan {

[[nodiscard]] vk::SurfaceKHR CreateSurface(const vk::Instance& instance,
                                           const Core::Frontend::EmuWindow& emu_window);

} // namespace Vulkan

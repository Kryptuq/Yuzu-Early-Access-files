// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Core::Frontend {
class EmuWindow;
}

namespace Vulkan {

[[nodiscard]] vk::SurfaceKHR CreateSurface(const vk::Instance& instance,
                                           const Core::Frontend::EmuWindow& emu_window);

} // namespace Vulkan

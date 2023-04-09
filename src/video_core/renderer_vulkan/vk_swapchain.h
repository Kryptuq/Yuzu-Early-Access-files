// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>

#include "common/common_types.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Layout {
struct FramebufferLayout;
}

namespace Vulkan {

class Device;
class Scheduler;

class Swapchain {
public:
    explicit Swapchain(VkSurfaceKHR surface, const Device& device, Scheduler& scheduler, u32 width,
                       u32 height, bool srgb);
    ~Swapchain();

    /// Creates (or recreates) the swapchain with a given size.
    void Create(u32 width, u32 height, bool srgb);

    /// Acquires the next image in the swapchain, waits as needed.
    bool AcquireNextImage();

    /// Presents the rendered image to the swapchain.
    void Present(VkSemaphore render_semaphore);

    /// Returns true when the swapchain needs to be recreated.
    bool NeedsRecreation(bool is_srgb) const {
        return HasColorSpaceChanged(is_srgb) || IsSubOptimal() || NeedsPresentModeUpdate();
    }

    /// Returns true when the color space has changed.
    bool HasColorSpaceChanged(bool is_srgb) const {
        return current_srgb != is_srgb;
    }

    /// Returns true when the swapchain is outdated.
    bool IsOutDated() const {
        return is_outdated;
    }

    /// Returns true when the swapchain is suboptimal.
    bool IsSubOptimal() const {
        return is_suboptimal;
    }

    /// Returns true when the swapchain format is in the srgb color space
    bool IsSrgb() const {
        return current_srgb;
    }

    VkExtent2D GetSize() const {
        return extent;
    }

    std::size_t GetImageCount() const {
        return image_count;
    }

    std::size_t GetImageIndex() const {
        return image_index;
    }

    std::size_t GetFrameIndex() const {
        return frame_index;
    }

    VkImage GetImageIndex(std::size_t index) const {
        return images[index];
    }

    VkImage CurrentImage() const {
        return images[image_index];
    }

    VkFormat GetImageViewFormat() const {
        return image_view_format;
    }

    VkFormat GetImageFormat() const {
        return surface_format.format;
    }

    VkSemaphore CurrentPresentSemaphore() const {
        return *present_semaphores[frame_index];
    }

    VkSemaphore CurrentRenderSemaphore() const {
        return *render_semaphores[frame_index];
    }

    u32 GetWidth() const {
        return width;
    }

    u32 GetHeight() const {
        return height;
    }

    VkExtent2D GetExtent() const {
        return extent;
    }

private:
    void CreateSwapchain(const VkSurfaceCapabilitiesKHR& capabilities, bool srgb);
    void CreateSemaphores();
    void CreateImageViews();

    void Destroy();

    bool HasFpsUnlockChanged() const;

    bool NeedsPresentModeUpdate() const;

    const VkSurfaceKHR surface;
    const Device& device;
    Scheduler& scheduler;

    vk::SwapchainKHR swapchain;

    std::size_t image_count{};
    std::vector<VkImage> images;
    std::vector<u64> resource_ticks;
    std::vector<vk::Semaphore> present_semaphores;
    std::vector<vk::Semaphore> render_semaphores;

    u32 width;
    u32 height;

    u32 image_index{};
    u32 frame_index{};

    VkFormat image_view_format{};
    VkExtent2D extent{};
    VkPresentModeKHR present_mode{};
    VkSurfaceFormatKHR surface_format{};

    bool current_srgb{};
    bool current_fps_unlocked{};
    bool is_outdated{};
    bool is_suboptimal{};
};

} // namespace Vulkan

// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

#include "common/logging/log.h"
#include "common/settings.h"
#include "core/core.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_swapchain.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

namespace {

VkSurfaceFormatKHR ChooseSwapSurfaceFormat(vk::Span<VkSurfaceFormatKHR> formats) {
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        VkSurfaceFormatKHR format;
        format.format = VK_FORMAT_B8G8R8A8_UNORM;
        format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        return format;
    }
    const auto& found = std::find_if(formats.begin(), formats.end(), [](const auto& format) {
        return format.format == VK_FORMAT_B8G8R8A8_UNORM &&
               format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });
    return found != formats.end() ? *found : formats[0];
}

VkPresentModeKHR ChooseSwapPresentMode(vk::Span<VkPresentModeKHR> modes) {
    // Mailbox doesn't lock the application like fifo (vsync), prefer it
    const auto found_mailbox = std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_MAILBOX_KHR);
    if (Settings::values.fullscreen_mode.GetValue() == Settings::FullscreenMode::Borderless &&
        found_mailbox != modes.end()) {
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }
    if (!Settings::values.use_speed_limit.GetValue()) {
        // FIFO present mode locks the framerate to the monitor's refresh rate,
        // Find an alternative to surpass this limitation if FPS is unlocked.
        const auto found_imm = std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR);
        if (found_imm != modes.end()) {
            return VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, u32 width, u32 height) {
    constexpr auto undefined_size{std::numeric_limits<u32>::max()};
    if (capabilities.currentExtent.width != undefined_size) {
        return capabilities.currentExtent;
    }
    VkExtent2D extent;
    extent.width = std::max(capabilities.minImageExtent.width,
                            std::min(capabilities.maxImageExtent.width, width));
    extent.height = std::max(capabilities.minImageExtent.height,
                             std::min(capabilities.maxImageExtent.height, height));
    return extent;
}

} // Anonymous namespace

Swapchain::Swapchain(VkSurfaceKHR surface_, const Device& device_, Scheduler& scheduler_, u32 width,
                     u32 height, bool srgb)
    : surface{surface_}, device{device_}, scheduler{scheduler_} {
    Create(width, height, srgb);
}

Swapchain::~Swapchain() = default;

void Swapchain::Create(u32 width, u32 height, bool srgb) {
    is_outdated = false;
    is_suboptimal = false;

    const auto physical_device = device.GetPhysical();
    const auto capabilities{physical_device.GetSurfaceCapabilitiesKHR(surface)};
    if (capabilities.maxImageExtent.width == 0 || capabilities.maxImageExtent.height == 0) {
        return;
    }

    device.GetLogical().WaitIdle();
    Destroy();

    CreateSwapchain(capabilities, width, height, srgb);
    CreateSemaphores();
    CreateImageViews();

    resource_ticks.clear();
    resource_ticks.resize(image_count);
}

void Swapchain::AcquireNextImage() {
    const VkResult result = device.GetLogical().AcquireNextImageKHR(
        *swapchain, std::numeric_limits<u64>::max(), *present_semaphores[frame_index],
        VK_NULL_HANDLE, &image_index);
    switch (result) {
    case VK_SUCCESS:
        break;
    case VK_SUBOPTIMAL_KHR:
        is_suboptimal = true;
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        is_outdated = true;
        break;
    default:
        LOG_ERROR(Render_Vulkan, "vkAcquireNextImageKHR returned {}", vk::ToString(result));
        break;
    }
    scheduler.Wait(resource_ticks[image_index]);
    resource_ticks[image_index] = scheduler.CurrentTick();
}

void Swapchain::Present(VkSemaphore render_semaphore) {
    const auto present_queue{device.GetPresentQueue()};
    const VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = render_semaphore ? 1U : 0U,
        .pWaitSemaphores = &render_semaphore,
        .swapchainCount = 1,
        .pSwapchains = swapchain.address(),
        .pImageIndices = &image_index,
        .pResults = nullptr,
    };
    switch (const VkResult result = present_queue.Present(present_info)) {
    case VK_SUCCESS:
        break;
    case VK_SUBOPTIMAL_KHR:
        LOG_DEBUG(Render_Vulkan, "Suboptimal swapchain");
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        is_outdated = true;
        break;
    default:
        LOG_CRITICAL(Render_Vulkan, "Failed to present with error {}", vk::ToString(result));
        break;
    }
    ++frame_index;
    if (frame_index >= image_count) {
        frame_index = 0;
    }
}

void Swapchain::CreateSwapchain(const VkSurfaceCapabilitiesKHR& capabilities, u32 width, u32 height,
                                bool srgb) {
    const auto physical_device{device.GetPhysical()};
    const auto formats{physical_device.GetSurfaceFormatsKHR(surface)};
    const auto present_modes{physical_device.GetSurfacePresentModesKHR(surface)};

    const VkSurfaceFormatKHR surface_format{ChooseSwapSurfaceFormat(formats)};
    present_mode = ChooseSwapPresentMode(present_modes);

    u32 requested_image_count{capabilities.minImageCount + 1};
    // Ensure Tripple buffering if possible.
    if (capabilities.maxImageCount > 0) {
        if (requested_image_count > capabilities.maxImageCount) {
            requested_image_count = capabilities.maxImageCount;
        } else {
            requested_image_count =
                std::max(requested_image_count, std::min(3U, capabilities.maxImageCount));
        }
    } else {
        requested_image_count = std::max(requested_image_count, 3U);
    }
    VkSwapchainCreateInfoKHR swapchain_ci{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = surface,
        .minImageCount = requested_image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = {},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_FALSE,
        .oldSwapchain = nullptr,
    };
    const u32 graphics_family{device.GetGraphicsFamily()};
    const u32 present_family{device.GetPresentFamily()};
    const std::array<u32, 2> queue_indices{graphics_family, present_family};
    if (graphics_family != present_family) {
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_ci.queueFamilyIndexCount = static_cast<u32>(queue_indices.size());
        swapchain_ci.pQueueFamilyIndices = queue_indices.data();
    }
    static constexpr std::array view_formats{VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SRGB};
    VkImageFormatListCreateInfo format_list{
        .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR,
        .pNext = nullptr,
        .viewFormatCount = static_cast<u32>(view_formats.size()),
        .pViewFormats = view_formats.data(),
    };
    if (device.IsKhrSwapchainMutableFormatEnabled()) {
        format_list.pNext = std::exchange(swapchain_ci.pNext, &format_list);
        swapchain_ci.flags |= VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;
    }
    // Request the size again to reduce the possibility of a TOCTOU race condition.
    const auto updated_capabilities = physical_device.GetSurfaceCapabilitiesKHR(surface);
    swapchain_ci.imageExtent = ChooseSwapExtent(updated_capabilities, width, height);
    // Don't add code within this and the swapchain creation.
    swapchain = device.GetLogical().CreateSwapchainKHR(swapchain_ci);

    extent = swapchain_ci.imageExtent;
    current_srgb = srgb;
    current_fps_unlocked = !Settings::values.use_speed_limit.GetValue();

    images = swapchain.GetImages();
    image_count = static_cast<u32>(images.size());
    image_view_format = srgb ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM;
}

void Swapchain::CreateSemaphores() {
    present_semaphores.resize(image_count);
    std::ranges::generate(present_semaphores,
                          [this] { return device.GetLogical().CreateSemaphore(); });
}

void Swapchain::CreateImageViews() {
    VkImageViewCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = {},
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = image_view_format,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    image_views.resize(image_count);
    for (std::size_t i = 0; i < image_count; i++) {
        ci.image = images[i];
        image_views[i] = device.GetLogical().CreateImageView(ci);
    }
}

void Swapchain::Destroy() {
    frame_index = 0;
    present_semaphores.clear();
    framebuffers.clear();
    image_views.clear();
    swapchain.reset();
}

bool Swapchain::HasFpsUnlockChanged() const {
    return current_fps_unlocked != !Settings::values.use_speed_limit.GetValue();
}

bool Swapchain::NeedsPresentModeUpdate() const {
    // Mailbox present mode is the ideal for all scenarios. If it is not available,
    // A different present mode is needed to support unlocked FPS above the monitor's refresh rate.
    return present_mode != VK_PRESENT_MODE_MAILBOX_KHR && HasFpsUnlockChanged();
}

} // namespace Vulkan

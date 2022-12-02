// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/frontend/emu_window.h"
#include "video_core/vulkan_common/vulkan_surface.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

// Include these late to avoid polluting previous headers
#ifdef _WIN32
#include <windows.h>
// ensure include order
#include <vulkan/vulkan_win32.h>
#elif defined(__APPLE__)
#include <vulkan/vulkan_macos.h>
#elif defined(__ANDROID__)
#include <vulkan/vulkan_android.h>
#else
#include <X11/Xlib.h>
#include <vulkan/vulkan_wayland.h>
#include <vulkan/vulkan_xlib.h>
#endif

namespace Vulkan {

vk::SurfaceKHR CreateSurface(const vk::Instance& instance,
                             const Core::Frontend::EmuWindow& emu_window) {
    [[maybe_unused]] const vk::InstanceDispatch& dld = instance.Dispatch();
    [[maybe_unused]] const auto& window_info = emu_window.GetWindowInfo();
    VkSurfaceKHR unsafe_surface = nullptr;

#ifdef _WIN32
    if (window_info.type == Core::Frontend::WindowSystemType::Windows) {
        const HWND hWnd = static_cast<HWND>(window_info.render_surface);
        const VkWin32SurfaceCreateInfoKHR win32_ci{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                                                   nullptr, 0, nullptr, hWnd};
        const auto vkCreateWin32SurfaceKHR = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
            dld.vkGetInstanceProcAddr(*instance, "vkCreateWin32SurfaceKHR"));
        if (!vkCreateWin32SurfaceKHR ||
            vkCreateWin32SurfaceKHR(*instance, &win32_ci, nullptr, &unsafe_surface) != VK_SUCCESS) {
            LOG_ERROR(Render_Vulkan, "Failed to initialize Win32 surface");
            throw vk::Exception(VK_ERROR_INITIALIZATION_FAILED);
        }
    }
#elif defined(__APPLE__)
    if (window_info.type == Core::Frontend::WindowSystemType::Cocoa) {
        const VkMacOSSurfaceCreateInfoMVK mvk_ci{VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
                                                 nullptr, 0, window_info.render_surface};
        const auto vkCreateMacOSSurfaceMVK = reinterpret_cast<PFN_vkCreateMacOSSurfaceMVK>(
            dld.vkGetInstanceProcAddr(*instance, "vkCreateMacOSSurfaceMVK"));
        if (!vkCreateMacOSSurfaceMVK ||
            vkCreateMacOSSurfaceMVK(*instance, &mvk_ci, nullptr, &unsafe_surface) != VK_SUCCESS) {
            LOG_ERROR(Render_Vulkan, "Failed to initialize Metal surface");
            throw vk::Exception(VK_ERROR_INITIALIZATION_FAILED);
        }
    }
#elif defined(__ANDROID__)
    if (window_info.type == Core::Frontend::WindowSystemType::Android) {
        const VkAndroidSurfaceCreateInfoKHR android_ci{
            VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, nullptr, 0,
            reinterpret_cast<ANativeWindow*>(window_info.render_surface)};
        const auto vkCreateAndroidSurfaceKHR = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(
            dld.vkGetInstanceProcAddr(*instance, "vkCreateAndroidSurfaceKHR"));
        if (!vkCreateAndroidSurfaceKHR ||
            vkCreateAndroidSurfaceKHR(*instance, &android_ci, nullptr, &unsafe_surface) !=
                VK_SUCCESS) {
            LOG_ERROR(Render_Vulkan, "Failed to initialize Android surface");
            throw vk::Exception(VK_ERROR_INITIALIZATION_FAILED);
        }
    }
#else
    if (window_info.type == Core::Frontend::WindowSystemType::X11) {
        const VkXlibSurfaceCreateInfoKHR xlib_ci{
            VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR, nullptr, 0,
            static_cast<Display*>(window_info.display_connection),
            reinterpret_cast<Window>(window_info.render_surface)};
        const auto vkCreateXlibSurfaceKHR = reinterpret_cast<PFN_vkCreateXlibSurfaceKHR>(
            dld.vkGetInstanceProcAddr(*instance, "vkCreateXlibSurfaceKHR"));
        if (!vkCreateXlibSurfaceKHR ||
            vkCreateXlibSurfaceKHR(*instance, &xlib_ci, nullptr, &unsafe_surface) != VK_SUCCESS) {
            LOG_ERROR(Render_Vulkan, "Failed to initialize Xlib surface");
            throw vk::Exception(VK_ERROR_INITIALIZATION_FAILED);
        }
    }
    if (window_info.type == Core::Frontend::WindowSystemType::Wayland) {
        const VkWaylandSurfaceCreateInfoKHR wayland_ci{
            VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR, nullptr, 0,
            static_cast<wl_display*>(window_info.display_connection),
            static_cast<wl_surface*>(window_info.render_surface)};
        const auto vkCreateWaylandSurfaceKHR = reinterpret_cast<PFN_vkCreateWaylandSurfaceKHR>(
            dld.vkGetInstanceProcAddr(*instance, "vkCreateWaylandSurfaceKHR"));
        if (!vkCreateWaylandSurfaceKHR ||
            vkCreateWaylandSurfaceKHR(*instance, &wayland_ci, nullptr, &unsafe_surface) !=
                VK_SUCCESS) {
            LOG_ERROR(Render_Vulkan, "Failed to initialize Wayland surface");
            throw vk::Exception(VK_ERROR_INITIALIZATION_FAILED);
        }
    }
#endif

    if (!unsafe_surface) {
        LOG_ERROR(Render_Vulkan, "Presentation not supported on this platform");
        throw vk::Exception(VK_ERROR_INITIALIZATION_FAILED);
    }
    return vk::SurfaceKHR(unsafe_surface, *instance, dld);
}

} // namespace Vulkan

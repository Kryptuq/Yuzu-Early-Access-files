// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <future>
#include <optional>
#include <span>
#include <vector>

#include "common/common_types.h"
#include "common/dynamic_library.h"
#include "common/logging/log.h"
#include "core/frontend/emu_window.h"
#include "video_core/vulkan_common/vulkan_instance.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

// Include these late to avoid polluting previous headers
#ifdef _WIN32
#include <windows.h>
// ensure include order
#include <vulkan/vulkan_win32.h>
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
#include <X11/Xlib.h>
#include <vulkan/vulkan_wayland.h>
#include <vulkan/vulkan_xlib.h>
#endif

namespace Vulkan {
namespace {
[[nodiscard]] std::vector<const char*> RequiredExtensions(
    Core::Frontend::WindowSystemType window_type, bool enable_debug_utils) {
    std::vector<const char*> extensions;
    extensions.reserve(6);
    switch (window_type) {
    case Core::Frontend::WindowSystemType::Headless:
        break;
#ifdef _WIN32
    case Core::Frontend::WindowSystemType::Windows:
        extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
        break;
#endif
#if !defined(_WIN32) && !defined(__APPLE__)
    case Core::Frontend::WindowSystemType::X11:
        extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
        break;
    case Core::Frontend::WindowSystemType::Wayland:
        extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
        break;
#endif
    default:
        LOG_ERROR(Render_Vulkan, "Presentation not supported on this platform");
        break;
    }
    if (window_type != Core::Frontend::WindowSystemType::Headless) {
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    }
    if (enable_debug_utils) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    return extensions;
}

[[nodiscard]] bool AreExtensionsSupported(const vk::InstanceDispatch& dld,
                                          std::span<const char* const> extensions) {
    const std::optional properties = vk::EnumerateInstanceExtensionProperties(dld);
    if (!properties) {
        LOG_ERROR(Render_Vulkan, "Failed to query extension properties");
        return false;
    }
    for (const char* extension : extensions) {
        const auto it = std::ranges::find_if(*properties, [extension](const auto& prop) {
            return std::strcmp(extension, prop.extensionName) == 0;
        });
        if (it == properties->end()) {
            LOG_ERROR(Render_Vulkan, "Required instance extension {} is not available", extension);
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::vector<const char*> Layers(bool enable_layers) {
    std::vector<const char*> layers;
    if (enable_layers) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }
    return layers;
}

void RemoveUnavailableLayers(const vk::InstanceDispatch& dld, std::vector<const char*>& layers) {
    const std::optional layer_properties = vk::EnumerateInstanceLayerProperties(dld);
    if (!layer_properties) {
        LOG_ERROR(Render_Vulkan, "Failed to query layer properties, disabling layers");
        layers.clear();
    }
    std::erase_if(layers, [&layer_properties](const char* layer) {
        const auto comp = [layer](const VkLayerProperties& layer_property) {
            return std::strcmp(layer, layer_property.layerName) == 0;
        };
        const auto it = std::ranges::find_if(*layer_properties, comp);
        if (it == layer_properties->end()) {
            LOG_ERROR(Render_Vulkan, "Layer {} not available, removing it", layer);
            return true;
        }
        return false;
    });
}
} // Anonymous namespace

vk::Instance CreateInstance(const Common::DynamicLibrary& library, vk::InstanceDispatch& dld,
                            u32 required_version, Core::Frontend::WindowSystemType window_type,
                            bool enable_debug_utils, bool enable_layers) {
    if (!library.IsOpen()) {
        LOG_ERROR(Render_Vulkan, "Vulkan library not available");
        throw vk::Exception(VK_ERROR_INITIALIZATION_FAILED);
    }
    if (!library.GetSymbol("vkGetInstanceProcAddr", &dld.vkGetInstanceProcAddr)) {
        LOG_ERROR(Render_Vulkan, "vkGetInstanceProcAddr not present in Vulkan");
        throw vk::Exception(VK_ERROR_INITIALIZATION_FAILED);
    }
    if (!vk::Load(dld)) {
        LOG_ERROR(Render_Vulkan, "Failed to load Vulkan function pointers");
        throw vk::Exception(VK_ERROR_INITIALIZATION_FAILED);
    }
    const std::vector<const char*> extensions = RequiredExtensions(window_type, enable_debug_utils);
    if (!AreExtensionsSupported(dld, extensions)) {
        throw vk::Exception(VK_ERROR_EXTENSION_NOT_PRESENT);
    }
    std::vector<const char*> layers = Layers(enable_layers);
    RemoveUnavailableLayers(dld, layers);

    const u32 available_version = vk::AvailableVersion(dld);
    if (available_version < required_version) {
        LOG_ERROR(Render_Vulkan, "Vulkan {}.{} is not supported, {}.{} is required",
                  VK_VERSION_MAJOR(available_version), VK_VERSION_MINOR(available_version),
                  VK_VERSION_MAJOR(required_version), VK_VERSION_MINOR(required_version));
        throw vk::Exception(VK_ERROR_INCOMPATIBLE_DRIVER);
    }
    vk::Instance instance =
        std::async([&] {
            return vk::Instance::Create(required_version, layers, extensions, dld);
        }).get();
    if (!vk::Load(*instance, dld)) {
        LOG_ERROR(Render_Vulkan, "Failed to load Vulkan instance function pointers");
        throw vk::Exception(VK_ERROR_INITIALIZATION_FAILED);
    }
    return instance;
}

} // namespace Vulkan

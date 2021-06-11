/*
 * Copyright (c) 2015-2020 The Khronos Group Inc.
 * Copyright (c) 2015-2020 Valve Corporation
 * Copyright (c) 2015-2020 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: David Pinedo <david@lunarg.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Rene Lindsay <rene@lunarg.com>
 * Author: Jeremy Kniager <jeremyk@lunarg.com>
 * Author: Shannon McPherson <shannon@lunarg.com>
 * Author: Bob Ellison <bob@lunarg.com>
 * Author: Richard Wright <richard@lunarg.com>
 * Author: Charles Giessen <charles@lunarg.com>
 *
 */

#include "vulkaninfo.hpp"

#ifdef _WIN32
// Initialize User32 pointers
PFN_AdjustWindowRect User32Handles::pfnAdjustWindowRect = nullptr;
PFN_CreateWindowExA User32Handles::pfnCreateWindowExA = nullptr;
PFN_DefWindowProcA User32Handles::pfnDefWindowProcA = nullptr;
PFN_DestroyWindow User32Handles::pfnDestroyWindow = nullptr;
PFN_LoadIconA User32Handles::pfnLoadIconA = nullptr;
PFN_RegisterClassExA User32Handles::pfnRegisterClassExA = nullptr;

HMODULE User32Handles::user32DllHandle = nullptr;

#endif

// =========== Dump Functions ========= //

void DumpExtensions(Printer &p, std::string layer_name, std::vector<VkExtensionProperties> extensions, bool do_indent) {
    std::sort(extensions.begin(), extensions.end(), [](VkExtensionProperties &a, VkExtensionProperties &b) -> int {
        return std::string(a.extensionName) < std::string(b.extensionName);
    });

    size_t max_length = 0;
    for (const auto &ext : extensions) {
        max_length = std::max(max_length, std::strlen(ext.extensionName));
    }

    ObjectWrapper obj(p, layer_name + " Extensions", extensions.size());
    if (do_indent) p.IndentDecrease();
    for (auto &ext : extensions) {
        p.PrintExtension(ext.extensionName, ext.specVersion, max_length);
    }
    if (do_indent) p.IndentIncrease();
}
void DumpExtensions(Printer &p, std::string layer_name, std::vector<VkExtensionProperties> extensions) {
    DumpExtensions(p, layer_name, extensions, false);
}
void DumpLayers(Printer &p, std::vector<LayerExtensionList> layers, const std::vector<std::unique_ptr<AppGpu>> &gpus) {
    std::sort(layers.begin(), layers.end(), [](LayerExtensionList &left, LayerExtensionList &right) -> int {
        return std::strncmp(left.layer_properties.layerName, right.layer_properties.layerName, VK_MAX_DESCRIPTION_SIZE) < 0;
    });
    switch (p.Type()) {
        case OutputType::text:
        case OutputType::html: {
            p.SetHeader();
            ArrayWrapper arr(p, "Layers", layers.size());
            IndentWrapper indent(p);

            for (auto &layer : layers) {
                auto v_str = VkVersionString(layer.layer_properties.specVersion);
                auto props = layer.layer_properties;

                std::string header = p.DecorateAsType(props.layerName) + " (" + props.description + ") Vulkan version " +
                                     p.DecorateAsValue(v_str) + ", layer version " +
                                     p.DecorateAsValue(std::to_string(props.implementationVersion));
                ObjectWrapper obj(p, header);
                DumpExtensions(p, "Layer", layer.extension_properties);

                ArrayWrapper arr(p, "Devices", gpus.size());
                for (auto &gpu : gpus) {
                    p.PrintKeyValue("GPU id", gpu->id, 0, gpu->props.deviceName);
                    auto exts = gpu->AppGetPhysicalDeviceLayerExtensions(props.layerName);
                    DumpExtensions(p, "Layer-Device", exts);
                    p.AddNewline();
                }
            }
            break;
        }

        case OutputType::json: {
            ArrayWrapper arr(p, "ArrayOfVkLayerProperties", layers.size());
            int i = 0;
            for (auto &layer : layers) {
                p.SetElementIndex(i++);
                DumpVkLayerProperties(p, "layerProperty", layer.layer_properties);
            }
            break;
        }
        case OutputType::vkconfig_output: {
            ObjectWrapper obj(p, "Layer Properties");
            for (auto &layer : layers) {
                ObjectWrapper obj_name(p, layer.layer_properties.layerName);
                p.PrintKeyString("layerName", layer.layer_properties.layerName, 21);
                p.PrintKeyString("version", VkVersionString(layer.layer_properties.specVersion), 21);
                p.PrintKeyValue("implementation version", layer.layer_properties.implementationVersion, 21);
                p.PrintKeyString("description", layer.layer_properties.description, 21);
                DumpExtensions(p, "Layer", layer.extension_properties);
                ObjectWrapper obj_devices(p, "Devices");
                for (auto &gpu : gpus) {
                    ObjectWrapper obj(p, gpu->props.deviceName);
                    p.PrintKeyValue("GPU id", gpu->id, 0, gpu->props.deviceName);
                    auto exts = gpu->AppGetPhysicalDeviceLayerExtensions(layer.layer_properties.layerName);
                    DumpExtensions(p, "Layer-Device", exts);
                }
            }
            break;
        }
    }
}

void DumpSurfaceFormats(Printer &p, AppInstance &inst, AppSurface &surface) {
    std::vector<VkSurfaceFormatKHR> formats;
    if (inst.CheckExtensionEnabled(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME)) {
        for (auto &format : surface.surf_formats2) {
            formats.push_back(format.surfaceFormat);
        }
    } else {
        for (auto &format : surface.surf_formats) {
            formats.push_back(format);
        }
    }
    ObjectWrapper obj(p, "Formats", formats.size());
    int i = 0;
    for (auto &format : formats) {
        p.SetElementIndex(i++);
        DumpVkSurfaceFormatKHR(p, "SurfaceFormat", format);
    }
}

void DumpPresentModes(Printer &p, AppSurface &surface) {
    ArrayWrapper arr(p, "Present Modes", surface.surf_present_modes.size());
    for (auto &mode : surface.surf_present_modes) {
        p.SetAsType().PrintString(VkPresentModeKHRString(mode));
    }
}

void DumpSurfaceCapabilities(Printer &p, AppInstance &inst, AppGpu &gpu, AppSurface &surface) {
    auto &surf_cap = surface.surface_capabilities;
    p.SetSubHeader();
    DumpVkSurfaceCapabilitiesKHR(p, "VkSurfaceCapabilitiesKHR", surf_cap);

    if (inst.CheckExtensionEnabled(VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME)) {
        p.SetSubHeader();
        ObjectWrapper obj(p, "VkSurfaceCapabilities2EXT");
        {
            ArrayWrapper arr(p, "supportedSurfaceCounters");
            if (surface.surface_capabilities2_ext.supportedSurfaceCounters == 0) p.PrintString("None");
            if (surface.surface_capabilities2_ext.supportedSurfaceCounters & VK_SURFACE_COUNTER_VBLANK_EXT) {
                p.SetAsType().PrintString("VK_SURFACE_COUNTER_VBLANK_EXT");
            }
        }
    }
    if (inst.CheckExtensionEnabled(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME)) {
        chain_iterator_surface_capabilities2(p, inst, gpu, surface.surface_capabilities2_khr.pNext, inst.vk_version);
    }
}

void DumpSurface(Printer &p, AppInstance &inst, AppGpu &gpu, AppSurface &surface, std::set<std::string> surface_types) {
    ObjectWrapper obj(p, std::string("GPU id : ") + p.DecorateAsValue(std::to_string(gpu.id)) + " (" + gpu.props.deviceName + ")");

    if (surface_types.size() == 0) {
        p.SetAsType().PrintKeyString("Surface type", "No type found");
    } else if (surface_types.size() == 1) {
        p.SetAsType().PrintKeyString("Surface type", surface.surface_extension.name);
    } else {
        ArrayWrapper arr(p, "Surface types", surface_types.size());
        for (auto &name : surface_types) {
            p.PrintString(name);
        }
    }

    DumpSurfaceFormats(p, inst, surface);
    DumpPresentModes(p, surface);
    DumpSurfaceCapabilities(p, inst, gpu, surface);

    p.AddNewline();
}

struct SurfaceTypeGroup {
    AppSurface *surface;
    AppGpu *gpu;
    std::set<std::string> surface_types;
};

bool operator==(AppSurface const &a, AppSurface const &b) {
    return a.phys_device == b.phys_device && a.surf_present_modes == b.surf_present_modes && a.surf_formats == b.surf_formats &&
           a.surf_formats2 == b.surf_formats2 && a.surface_capabilities == b.surface_capabilities &&
           a.surface_capabilities2_khr == b.surface_capabilities2_khr && a.surface_capabilities2_ext == b.surface_capabilities2_ext;
}

void DumpPresentableSurfaces(Printer &p, AppInstance &inst, const std::vector<std::unique_ptr<AppGpu>> &gpus,
                             const std::vector<std::unique_ptr<AppSurface>> &surfaces) {
    p.SetHeader();
    ObjectWrapper obj(p, "Presentable Surfaces");
    IndentWrapper indent(p);

    std::vector<SurfaceTypeGroup> surface_list;

    for (auto &surface : surfaces) {
        auto exists = surface_list.end();
        for (auto it = surface_list.begin(); it != surface_list.end(); it++) {
            // check for duplicat surfaces that differ only by the surface extension
            if (*(it->surface) == *(surface.get())) {
                exists = it;
                break;
            }
        }
        if (exists != surface_list.end()) {
            exists->surface_types.insert(surface.get()->surface_extension.name);
        } else {
            // find surface.phys_device's corresponding AppGpu
            AppGpu *corresponding_gpu = nullptr;
            for (auto &gpu : gpus) {
                if (gpu->phys_device == surface->phys_device) corresponding_gpu = gpu.get();
            }
            if (corresponding_gpu != nullptr)
                surface_list.push_back({surface.get(), corresponding_gpu, {surface.get()->surface_extension.name}});
        }
    }
    for (auto &group : surface_list) {
        DumpSurface(p, inst, *group.gpu, *group.surface, group.surface_types);
    }
    p.AddNewline();
}

void DumpGroups(Printer &p, AppInstance &inst) {
    if (inst.CheckExtensionEnabled(VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME)) {
        auto groups = GetGroups(inst);
        if (groups.size() == 0) {
            p.SetHeader();
            ObjectWrapper obj(p, "Groups");
            p.PrintString("No Device Groups Found");
            p.AddNewline();
            return;
        }

        p.SetHeader();
        ObjectWrapper obj(p, "Device Groups");
        IndentWrapper indent(p);

        int group_id = 0;
        for (auto &group : groups) {
            ObjectWrapper obj(p, "Group " + std::to_string(group_id));
            auto group_props = GetGroupProps(inst, group);
            {
                ObjectWrapper obj(p, "Properties");
                {
                    ArrayWrapper arr(p, "physicalDevices", group.physicalDeviceCount);
                    int id = 0;
                    for (auto &prop : group_props) {
                        p.PrintString(std::string(prop.deviceName) + " (ID: " + p.DecorateAsValue(std::to_string(id++)) + ")");
                    }
                }
                p.PrintKeyValue("subsetAllocation", group.subsetAllocation);
            }
            p.AddNewline();

            auto group_capabilities = GetGroupCapabilities(inst, group);
            if (group_capabilities.first == false) {
                p.PrintKeyString("Present Capabilities",
                                 "Group does not support VK_KHR_device_group, skipping printing present capabilities");
            } else {
                ObjectWrapper obj(p, "Present Capabilities");
                for (uint32_t i = 0; i < group.physicalDeviceCount; i++) {
                    ObjectWrapper obj(
                        p, std::string(group_props[i].deviceName) + " (ID: " + p.DecorateAsValue(std::to_string(i)) + ")");
                    ArrayWrapper arr(p, "Can present images from the following devices", group.physicalDeviceCount);

                    for (uint32_t j = 0; j < group.physicalDeviceCount; j++) {
                        uint32_t mask = 1 << j;
                        if (group_capabilities.second.presentMask[i] & mask) {
                            p.PrintString(std::string(group_props[j].deviceName) + " (ID: " + p.DecorateAsValue(std::to_string(j)) +
                                          ")");
                        }
                    }
                }
                DumpVkDeviceGroupPresentModeFlagsKHR(p, "Present modes", group_capabilities.second.modes);
            }
            p.AddNewline();
            group_id++;
        }
        p.AddNewline();
    }
}

void GpuDumpProps(Printer &p, AppGpu &gpu) {
    auto props = gpu.GetDeviceProperties();
    p.SetSubHeader();
    {
        ObjectWrapper obj(p, "VkPhysicalDeviceProperties");
        p.PrintKeyValue("apiVersion", props.apiVersion, 14, VkVersionString(props.apiVersion));
        p.PrintKeyValue("driverVersion", props.driverVersion, 14, to_hex_str(props.driverVersion));
        p.PrintKeyString("vendorID", to_hex_str(props.vendorID), 14);
        p.PrintKeyString("deviceID", to_hex_str(props.deviceID), 14);
        p.PrintKeyString("deviceType", VkPhysicalDeviceTypeString(props.deviceType), 14);
        p.PrintKeyString("deviceName", props.deviceName, 14);
        if (p.Type() == OutputType::vkconfig_output) {
            ArrayWrapper arr(p, "pipelineCacheUUID", VK_UUID_SIZE);
            for (uint32_t i = 0; i < VK_UUID_SIZE; ++i) {
                p.PrintElement(static_cast<uint32_t>(props.pipelineCacheUUID[i]));
            }
        }
    }
    p.AddNewline();
    DumpVkPhysicalDeviceLimits(p, "VkPhysicalDeviceLimits",
                               gpu.inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
                                   ? gpu.props2.properties.limits
                                   : gpu.props.limits);
    p.AddNewline();
    DumpVkPhysicalDeviceSparseProperties(p, "VkPhysicalDeviceSparseProperties",
                                         gpu.inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
                                             ? gpu.props2.properties.sparseProperties
                                             : gpu.props.sparseProperties);
    p.AddNewline();
    if (gpu.inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
        void *place = gpu.props2.pNext;
        chain_iterator_phys_device_props2(p, gpu.inst, gpu, place, gpu.api_version);
        p.AddNewline();
    }
}
void GpuDumpPropsJson(Printer &p, AppGpu &gpu) {
    auto props = gpu.GetDeviceProperties();
    ObjectWrapper obj(p, "VkPhysicalDeviceProperties");
    p.PrintKeyValue("apiVersion", props.apiVersion, 14, VkVersionString(props.apiVersion));
    p.PrintKeyValue("driverVersion", props.driverVersion, 14, to_hex_str(props.driverVersion));
    p.PrintKeyValue("vendorID", props.vendorID, 14);
    p.PrintKeyValue("deviceID", props.deviceID, 14);
    p.PrintKeyValue("deviceType", props.deviceType, 14);
    p.PrintKeyString("deviceName", props.deviceName, 14);
    {
        ArrayWrapper arr(p, "pipelineCacheUUID", VK_UUID_SIZE);
        for (uint32_t i = 0; i < VK_UUID_SIZE; ++i) {
            p.PrintElement(static_cast<uint32_t>(props.pipelineCacheUUID[i]));
        }
    }

    DumpVkPhysicalDeviceLimits(p, "VkPhysicalDeviceLimits",
                               gpu.inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
                                   ? gpu.props2.properties.limits
                                   : gpu.props.limits);
    DumpVkPhysicalDeviceSparseProperties(p, "VkPhysicalDeviceSparseProperties",
                                         gpu.inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
                                             ? gpu.props2.properties.sparseProperties
                                             : gpu.props.sparseProperties);
}

void GpuDumpQueueProps(Printer &p, std::vector<SurfaceExtension> &surfaces, AppQueueFamilyProperties &queue) {
    p.SetSubHeader().SetElementIndex(static_cast<int>(queue.queue_index));
    ObjectWrapper obj(p, "queueProperties");
    if (p.Type() == OutputType::vkconfig_output) {
        DumpVkExtent3D(p, "minImageTransferGranularity", queue.props.minImageTransferGranularity);
    } else {
        p.PrintKeyValue("minImageTransferGranularity", queue.props.minImageTransferGranularity, 27);
    }
    p.PrintKeyValue("queueCount", queue.props.queueCount, 27);
    p.PrintKeyString("queueFlags", VkQueueFlagsString(queue.props.queueFlags), 27);
    p.PrintKeyValue("timestampValidBits", queue.props.timestampValidBits, 27);

    if (queue.is_present_platform_agnostic) {
        p.PrintKeyString("present support", queue.platforms_support_present ? "true" : "false", 27);
    } else {
        size_t width = 0;
        for (auto &surface : surfaces) {
            if (surface.name.size() > width) width = surface.name.size();
        }
        ObjectWrapper obj(p, "present support");
        for (auto &surface : surfaces) {
            p.PrintKeyString(surface.name, surface.supports_present ? "true" : "false", width);
        }
    }

    p.AddNewline();
}

void GpuDumpQueuePropsJson(Printer &p, std::vector<SurfaceExtension> &surfaces, AppQueueFamilyProperties &queue) {
    ObjectWrapper obj(p, "");
    DumpVkExtent3D(p, "minImageTransferGranularity", queue.props.minImageTransferGranularity);
    p.PrintKeyValue("queueCount", queue.props.queueCount, 27);
    p.PrintKeyValue("queueFlags", queue.props.queueFlags, 27);
    p.PrintKeyValue("timestampValidBits", queue.props.timestampValidBits, 27);
}

// This prints a number of bytes in a human-readable format according to prefixes of the International System of Quantities (ISQ),
// defined in ISO/IEC 80000. The prefixes used here are not SI prefixes, but rather the binary prefixes based on powers of 1024
// (kibi-, mebi-, gibi- etc.).
#define kBufferSize 32

std::string NumToNiceStr(const size_t sz) {
    const char prefixes[] = "KMGTPEZY";
    char buf[kBufferSize];
    int which = -1;
    double result = (double)sz;
    while (result > 1024 && which < 7) {
        result /= 1024;
        ++which;
    }

    char unit[] = "\0i";
    if (which >= 0) {
        unit[0] = prefixes[which];
    }
#ifdef _WIN32
    _snprintf_s(buf, kBufferSize * sizeof(char), kBufferSize, "%.2f %sB", result, unit);
#else
    snprintf(buf, kBufferSize, "%.2f %sB", result, unit);
#endif
    return std::string(buf);
}

std::string append_human_readible(VkDeviceSize memory) {
    return std::to_string(memory) + " (" + to_hex_str(memory) + ") (" + NumToNiceStr(static_cast<size_t>(memory)) + ")";
}

void GpuDumpMemoryProps(Printer &p, AppGpu &gpu) {
    p.SetHeader();
    ObjectWrapper obj(p, "VkPhysicalDeviceMemoryProperties");
    IndentWrapper indent(p);
    {
        ObjectWrapper obj(p, "memoryHeaps", gpu.memory_props.memoryHeapCount);

        for (uint32_t i = 0; i < gpu.memory_props.memoryHeapCount; ++i) {
            p.SetElementIndex(static_cast<int>(i));
            ObjectWrapper obj(p, "memoryHeaps");

            p.PrintKeyString("size", append_human_readible(gpu.memory_props.memoryHeaps[i].size), 6);
            p.PrintKeyString("budget", append_human_readible(gpu.heapBudget[i]), 6);
            p.PrintKeyString("usage", append_human_readible(gpu.heapUsage[i]), 6);
            DumpVkMemoryHeapFlags(p, "flags", gpu.memory_props.memoryHeaps[i].flags, 6);
        }
    }
    {
        ObjectWrapper obj(p, "memoryTypes", gpu.memory_props.memoryTypeCount);

        for (uint32_t i = 0; i < gpu.memory_props.memoryTypeCount; ++i) {
            p.SetElementIndex(static_cast<int>(i));
            ObjectWrapper obj(p, "memoryTypes");
            p.PrintKeyValue("heapIndex", gpu.memory_props.memoryTypes[i].heapIndex, 13);

            auto flags = gpu.memory_props.memoryTypes[i].propertyFlags;
            DumpVkMemoryPropertyFlags(p, "propertyFlags = " + to_hex_str(flags), flags);

            ObjectWrapper usable_for(p, "usable for");
            const uint32_t memtype_bit = 1U << i;

            // only linear and optimal tiling considered
            std::vector<VkFormat> tiling_optimal_formats;
            std::vector<VkFormat> tiling_linear_formats;
            for (auto &image_tiling : gpu.memory_image_support_types) {
                p.SetOpenDetails();
                ArrayWrapper arr(p, VkImageTilingString(VkImageTiling(image_tiling.tiling)), -1);
                bool has_any_support_types = false;
                bool regular = false;
                bool transient = false;
                bool sparse = false;
                for (auto &image_format : image_tiling.formats) {
                    if (image_format.type_support.size() > 0) {
                        bool has_a_support_type = false;
                        for (auto &img_type : image_format.type_support) {
                            if (img_type.Compatible(memtype_bit)) {
                                has_a_support_type = true;
                                has_any_support_types = true;
                                if (img_type.type == ImageTypeSupport::Type::regular) regular = true;
                                if (img_type.type == ImageTypeSupport::Type::transient) transient = true;
                                if (img_type.type == ImageTypeSupport::Type::sparse) sparse = true;
                            }
                        }
                        if (has_a_support_type) {
                            if (image_format.format == color_format) {
                                p.PrintString("color images");
                            } else {
                                p.PrintString(VkFormatString(image_format.format));
                            }
                        }
                    }
                }
                if (!has_any_support_types) {
                    p.PrintString("None");
                } else {
                    if (regular && !transient && sparse) p.PrintString("(non-transient)");
                    if (regular && transient && !sparse) p.PrintString("(non-sparse)");
                    if (regular && !transient && !sparse) p.PrintString("(non-sparse, non-transient)");
                    if (!regular && transient && sparse) p.PrintString("(sparse and transient only)");
                    if (!regular && !transient && sparse) p.PrintString("(sparse only)");
                    if (!regular && transient && !sparse) p.PrintString("(transient only)");
                }
            }
        }
    }
    p.AddNewline();
}

void GpuDumpMemoryPropsJson(Printer &p, AppGpu &gpu) {
    ObjectWrapper obj(p, "VkPhysicalDeviceMemoryProperties");
    {
        ArrayWrapper arr(p, "memoryHeaps", gpu.memory_props.memoryHeapCount);
        for (uint32_t i = 0; i < gpu.memory_props.memoryHeapCount; ++i) {
            ObjectWrapper obj(p, "");
            p.PrintKeyValue("flags", gpu.memory_props.memoryHeaps[i].flags);
            p.PrintKeyValue("size", gpu.memory_props.memoryHeaps[i].size);
        }
    }
    {
        ArrayWrapper arr(p, "memoryTypes", gpu.memory_props.memoryTypeCount);
        for (uint32_t i = 0; i < gpu.memory_props.memoryTypeCount; ++i) {
            ObjectWrapper obj(p, "");
            p.PrintKeyValue("heapIndex", gpu.memory_props.memoryTypes[i].heapIndex, 13);
            p.PrintKeyValue("propertyFlags", gpu.memory_props.memoryTypes[i].propertyFlags, 13);
        }
    }
}

void GpuDumpFeatures(Printer &p, AppGpu &gpu) {
    p.SetHeader();
    DumpVkPhysicalDeviceFeatures(p, "VkPhysicalDeviceFeatures", gpu.features);
    p.AddNewline();
    if (gpu.inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
        void *place = gpu.features2.pNext;
        chain_iterator_phys_device_features2(p, gpu, place, gpu.api_version);
    }
}

void GpuDumpFormatProperty(Printer &p, VkFormat fmt, VkFormatProperties prop) {
    switch (p.Type()) {
        case OutputType::text: {
            ObjectWrapper obj(p, "Properties");
            DumpVkFormatFeatureFlags(p, "linearTiling", prop.linearTilingFeatures);
            DumpVkFormatFeatureFlags(p, "optimalTiling", prop.optimalTilingFeatures);
            DumpVkFormatFeatureFlags(p, "bufferFeatures", prop.bufferFeatures);
            break;
        }
        case OutputType::html: {
            p.SetTitleAsType();
            ObjectWrapper obj(p, VkFormatString(fmt));
            p.SetOpenDetails();
            DumpVkFormatFeatureFlags(p, "linearTiling", prop.linearTilingFeatures);
            p.SetOpenDetails();
            DumpVkFormatFeatureFlags(p, "optimalTiling", prop.optimalTilingFeatures);
            p.SetOpenDetails();
            DumpVkFormatFeatureFlags(p, "bufferFeatures", prop.bufferFeatures);
            break;
        }
        case OutputType::json: {
            ObjectWrapper obj(p, "");
            p.PrintKeyValue("formatID", fmt);
            p.PrintKeyValue("linearTilingFeatures", prop.linearTilingFeatures);
            p.PrintKeyValue("optimalTilingFeatures", prop.optimalTilingFeatures);
            p.PrintKeyValue("bufferFeatures", prop.bufferFeatures);
            break;
        }
        case OutputType::vkconfig_output: {
            ObjectWrapper obj(p, VkFormatString(fmt));
            DumpVkFormatFeatureFlags(p, "linearTiling", prop.linearTilingFeatures);
            DumpVkFormatFeatureFlags(p, "optimalTiling", prop.optimalTilingFeatures);
            DumpVkFormatFeatureFlags(p, "bufferFeatures", prop.bufferFeatures);
            break;
        }
    }
}

void GpuDumpToolingInfo(Printer &p, AppGpu &gpu) {
    auto tools = GetToolingInfo(gpu);
    if (tools.size() > 0) {
        p.SetSubHeader();
        ObjectWrapper obj(p, "Tooling Info");
        for (auto tool : tools) {
            DumpVkPhysicalDeviceToolPropertiesEXT(p, tool.name, tool);
            p.AddNewline();
        }
    }
}

void GpuDevDump(Printer &p, AppGpu &gpu) {
    p.SetHeader();
    ObjectWrapper obj(p, "Format Properties");
    IndentWrapper indent(p);

    if (p.Type() == OutputType::text) {
        auto fmtPropMap = FormatPropMap(gpu);

        int counter = 0;
        std::vector<VkFormat> unsupported_formats;
        for (auto &prop : fmtPropMap) {
            VkFormatProperties props;
            props.linearTilingFeatures = prop.first.linear;
            props.optimalTilingFeatures = prop.first.optimal;
            props.bufferFeatures = prop.first.buffer;
            if (props.linearTilingFeatures == 0 && props.optimalTilingFeatures == 0 && props.bufferFeatures == 0) {
                unsupported_formats = prop.second;
                continue;
            }

            p.SetElementIndex(counter++);
            ObjectWrapper obj(p, "Common Format Group");
            IndentWrapper indent(p);
            {
                ArrayWrapper arr(p, "Formats", prop.second.size());
                for (auto &fmt : prop.second) {
                    p.SetAsType().PrintString(VkFormatString(fmt));
                }
            }
            GpuDumpFormatProperty(p, VK_FORMAT_UNDEFINED, props);
            p.AddNewline();
        }

        ArrayWrapper arr(p, "Unsupported Formats", unsupported_formats.size());
        for (auto &fmt : unsupported_formats) {
            p.SetAsType().PrintString(VkFormatString(fmt));
        }
    } else {
        for (auto &format : gpu.supported_format_ranges) {
            if (gpu.FormatRangeSupported(format)) {
                for (int32_t fmt_counter = format.first_format; fmt_counter <= format.last_format; ++fmt_counter) {
                    VkFormat fmt = static_cast<VkFormat>(fmt_counter);

                    VkFormatProperties props;
                    gpu.inst.dll.fp_vkGetPhysicalDeviceFormatProperties(gpu.phys_device, fmt, &props);

                    GpuDumpFormatProperty(p, fmt, props);
                }
            }
        }
    }

    p.AddNewline();
}

void GpuDevDumpJson(Printer &p, AppGpu &gpu) {
    ArrayWrapper arr(p, "ArrayOfVkFormatProperties");
    for (auto &format : gpu.supported_format_ranges) {
        if (gpu.FormatRangeSupported(format)) {
            for (int32_t fmt_counter = format.first_format; fmt_counter <= format.last_format; ++fmt_counter) {
                VkFormat fmt = static_cast<VkFormat>(fmt_counter);

                VkFormatProperties props;
                gpu.inst.dll.fp_vkGetPhysicalDeviceFormatProperties(gpu.phys_device, fmt, &props);

                // don't print format properties that are unsupported
                if ((props.linearTilingFeatures || props.optimalTilingFeatures || props.bufferFeatures) == 0) continue;

                GpuDumpFormatProperty(p, fmt, props);
            }
        }
    }
}
// Print gpu info for text, html, & vkconfig_output
// Uses a seperate function than schema-json for clarity
void DumpGpu(Printer &p, AppGpu &gpu, bool show_formats) {
    ObjectWrapper obj(p, "GPU" + std::to_string(gpu.id));
    IndentWrapper indent(p);

    GpuDumpProps(p, gpu);
    DumpExtensions(p, "Device", gpu.device_extensions);
    p.AddNewline();
    {
        p.SetHeader();
        ObjectWrapper obj(p, "VkQueueFamilyProperties");
        for (uint32_t i = 0; i < gpu.queue_count; i++) {
            AppQueueFamilyProperties queue_props = AppQueueFamilyProperties(gpu, i);
            GpuDumpQueueProps(p, gpu.inst.surface_extensions, queue_props);
        }
    }
    GpuDumpMemoryProps(p, gpu);
    GpuDumpFeatures(p, gpu);
    GpuDumpToolingInfo(p, gpu);

    if (p.Type() != OutputType::text || show_formats) {
        GpuDevDump(p, gpu);
    }

    p.AddNewline();
}

// Print gpu info for json
void DumpGpuJson(Printer &p, AppGpu &gpu) {
    GpuDumpPropsJson(p, gpu);
    {
        ArrayWrapper arr(p, "ArrayOfVkQueueFamilyProperties");
        for (uint32_t i = 0; i < gpu.queue_count; i++) {
            AppQueueFamilyProperties queue_props = AppQueueFamilyProperties(gpu, i);
            GpuDumpQueuePropsJson(p, gpu.inst.surface_extensions, queue_props);
        }
    }
    {
        ArrayWrapper arr(p, "ArrayOfVkExtensionProperties");
        for (auto &ext : gpu.device_extensions) {
            p.PrintExtension(ext.extensionName, ext.specVersion);
        }
    }

    GpuDumpMemoryPropsJson(p, gpu);
    DumpVkPhysicalDeviceFeatures(p, "VkPhysicalDeviceFeatures", gpu.features);
    GpuDevDumpJson(p, gpu);
}

// Print summary of system
void DumpSummaryInstance(Printer &p, AppInstance &inst) {
    p.SetSubHeader();
    DumpExtensions(p, "Instance", inst.global_extensions, true);
    p.AddNewline();

    p.SetSubHeader();
    ArrayWrapper arr(p, "Instance Layers", inst.global_layers.size());
    IndentWrapper indent(p);
    std::sort(inst.global_layers.begin(), inst.global_layers.end(), [](LayerExtensionList &left, LayerExtensionList &right) -> int {
        return std::strncmp(left.layer_properties.layerName, right.layer_properties.layerName, VK_MAX_DESCRIPTION_SIZE) < 0;
    });
    size_t layer_name_max = 0;
    size_t layer_desc_max = 0;
    size_t layer_version_max = 0;

    // find max of each type to align everything in columns
    for (auto &layer : inst.global_layers) {
        auto props = layer.layer_properties;
        layer_name_max = std::max(layer_name_max, strlen(props.layerName));
        layer_desc_max = std::max(layer_desc_max, strlen(props.description));
        layer_version_max = std::max(layer_version_max, VkVersionString(layer.layer_properties.specVersion).size());
    }
    for (auto &layer : inst.global_layers) {
        auto v_str = VkVersionString(layer.layer_properties.specVersion);
        auto props = layer.layer_properties;

        auto name_padding = std::string(layer_name_max - strlen(props.layerName), ' ');
        auto desc_padding = std::string(layer_desc_max - strlen(props.description), ' ');
        auto version_padding = std::string(layer_version_max - v_str.size(), ' ');
        p.PrintString(std::string(props.layerName) + name_padding + " " + props.description + desc_padding + " " + v_str + " " +
                      version_padding + " version " + std::to_string(props.implementationVersion));
    }
    p.AddNewline();
}

void DumpSummaryGPU(Printer &p, AppGpu &gpu) {
    ObjectWrapper obj(p, "GPU" + std::to_string(gpu.id));
    auto props = gpu.GetDeviceProperties();
    p.PrintKeyValue("apiVersion", props.apiVersion, 18, VkVersionString(props.apiVersion));
    p.PrintKeyValue("driverVersion", props.driverVersion, 18, to_hex_str(props.driverVersion));
    p.PrintKeyString("vendorID", to_hex_str(props.vendorID), 18);
    p.PrintKeyString("deviceID", to_hex_str(props.deviceID), 18);
    p.PrintKeyString("deviceType", VkPhysicalDeviceTypeString(props.deviceType), 18);
    p.PrintKeyString("deviceName", props.deviceName, 18);

    if (gpu.inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) &&
        (gpu.CheckPhysicalDeviceExtensionIncluded(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME) || gpu.api_version.minor >= 2)) {
        void *place = gpu.props2.pNext;
        while (place) {
            struct VkStructureHeader *structure = (struct VkStructureHeader *)place;
            if (structure->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES) {
                VkPhysicalDeviceDriverProperties *props = (VkPhysicalDeviceDriverProperties *)structure;
                DumpVkDriverId(p, "driverID", props->driverID, 18);
                p.PrintKeyString("driverName", props->driverName, 18);
                p.PrintKeyString("driverInfo", props->driverInfo, 18);
                DumpVkConformanceVersion(p, "conformanceVersion", props->conformanceVersion, 18);
            }
            place = structure->pNext;
        }
    }
}

#if defined(VK_ENABLE_BETA_EXTENSIONS)
void DumpPortability(Printer &p, AppGpu &gpu) {
    if (gpu.CheckPhysicalDeviceExtensionIncluded(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) {
        if (gpu.inst.CheckExtensionEnabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
            void *props_place = gpu.props2.pNext;
            while (props_place) {
                struct VkStructureHeader *structure = (struct VkStructureHeader *)props_place;
                if (structure->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_PROPERTIES_KHR) {
                    VkPhysicalDevicePortabilitySubsetPropertiesKHR *props =
                        (VkPhysicalDevicePortabilitySubsetPropertiesKHR *)structure;
                    DumpVkPhysicalDevicePortabilitySubsetPropertiesKHR(p, "VkPhysicalDevicePortabilitySubsetPropertiesKHR", *props);
                    break;
                }
                props_place = structure->pNext;
            }

            void *feats_place = gpu.features2.pNext;
            while (feats_place) {
                struct VkStructureHeader *structure = (struct VkStructureHeader *)feats_place;
                if (structure->sType == VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR) {
                    VkPhysicalDevicePortabilitySubsetFeaturesKHR *features =
                        (VkPhysicalDevicePortabilitySubsetFeaturesKHR *)structure;
                    DumpVkPhysicalDevicePortabilitySubsetFeaturesKHR(p, "VkPhysicalDevicePortabilitySubsetFeaturesKHR", *features);
                    break;
                }
                feats_place = structure->pNext;
            }
        }
    }
}
#endif  // defined(VK_ENABLE_BETA_EXTENSIONS)

// ============ Printing Logic ============= //

#ifdef _WIN32
// Enlarges the console window to have a large scrollback size.
static void ConsoleEnlarge() {
    const HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    // make the console window bigger
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD buffer_size;
    if (GetConsoleScreenBufferInfo(console_handle, &csbi)) {
        buffer_size.X = csbi.dwSize.X + 30;
        buffer_size.Y = 20000;
        SetConsoleScreenBufferSize(console_handle, buffer_size);
    }

    SMALL_RECT r;
    r.Left = r.Top = 0;
    r.Right = csbi.dwSize.X - 1 + 30;
    r.Bottom = 50;
    SetConsoleWindowInfo(console_handle, true, &r);

    // change the console window title
    SetConsoleTitle(TEXT(app_short_name));
}
#endif

void print_usage(const char *argv0) {
    std::cout << "\nvulkaninfo - Summarize Vulkan information in relation to the current environment.\n\n";
    std::cout << "USAGE: " << argv0 << " [options]\n\n";
    std::cout << "OPTIONS:\n";
    std::cout << "-h, --help          Print this help.\n";
    std::cout << "--html              Produce an html version of vulkaninfo output, saved as\n";
    std::cout << "                    \"vulkaninfo.html\" in the directory in which the command\n";
    std::cout << "                    is run.\n";
    std::cout << "-j, --json          Produce a json version of vulkaninfo to standard output of the\n";
    std::cout << "                    first gpu in the system conforming to the DevSim schema.\n";
    std::cout << "--json=<gpu-number> For a multi-gpu system, a single gpu can be targetted by\n";
    std::cout << "                    specifying the gpu-number associated with the gpu of \n";
    std::cout << "                    interest. This number can be determined by running\n";
    std::cout << "                    vulkaninfo without any options specified.\n";
#if defined(VK_ENABLE_BETA_EXTENSIONS)
    std::cout << "--portability       Produce a json version of vulkaninfo to standard output of the first\n";
    std::cout << "                    gpu in the system conforming to the DevSim Portability Subset schema.\n";
    std::cout << "--portability=<N>   Produce the json output conforming to the DevSim Portability\n";
    std::cout << "                    Subset Schema for the GPU specified to standard output,\n";
    std::cout << "                    where N is the GPU desired.\n";
#endif  // defined(VK_ENABLE_BETA_EXTENSIONS)
    std::cout << "--show-formats      Display the format properties of each physical device.\n";
    std::cout << "                    Note: This option does not affect html or json output;\n";
    std::cout << "                    they will always print format properties.\n\n";
    std::cout << "--summary           Show a summary of the instance and GPU's on a system.\n\n";
}

#ifdef VK_USE_PLATFORM_IOS_MVK
// On iOS, we'll call this ourselves from a parent routine in the GUI
int vulkanInfoMain(int argc, char **argv) {
#else
int main(int argc, char **argv) {
#endif

#ifdef _WIN32
    if (ConsoleIsExclusive()) ConsoleEnlarge();
    if (!LoadUser32Dll()) {
        fprintf(stderr, "Failed to load user32.dll library!\n");
        WAIT_FOR_CONSOLE_DESTROY;
        exit(1);
    }
#endif

    uint32_t selected_gpu = 0;
    bool show_formats = false;
    char *output_path = nullptr;

    // Combinations of output: html only, html AND json, json only, human readable only
    for (int i = 1; i < argc; ++i) {
        // A internal-use-only format for communication with the Vulkan Configurator tool
        // Usage "--vkconfig_output <path>"
        if (0 == strcmp("--vkconfig_output", argv[i]) && argc > (i + 1)) {
            human_readable_output = false;
            vkconfig_output = true;
            output_path = argv[i + 1];
            ++i;
        } else if (strncmp("--json", argv[i], 6) == 0 || strcmp(argv[i], "-j") == 0) {
            if (strlen(argv[i]) > 7 && strncmp("--json=", argv[i], 7) == 0) {
                selected_gpu = static_cast<uint32_t>(strtol(argv[i] + 7, nullptr, 10));
            }
            human_readable_output = false;
            json_output = true;
            portability_json = false;
#if defined(VK_ENABLE_BETA_EXTENSIONS)
        } else if (strncmp("--portability", argv[i], 13) == 0) {
            if (strlen(argv[i]) > 14 && strncmp("--portability=", argv[i], 14) == 0) {
                selected_gpu = static_cast<uint32_t>(strtol(argv[i] + 14, nullptr, 10));
            }
            human_readable_output = false;
            portability_json = true;
            json_output = false;
#endif  // defined(VK_ENABLE_BETA_EXTENSIONS)
        } else if (strcmp(argv[i], "--summary") == 0) {
            summary = true;
        } else if (strcmp(argv[i], "--html") == 0) {
            human_readable_output = false;
            html_output = true;
        } else if (strcmp(argv[i], "--show-formats") == 0) {
            show_formats = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 1;
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }
    std::vector<std::unique_ptr<Printer>> printers;
    std::ostream out(std::cout.rdbuf());
    std::ofstream json_out;
    std::ofstream portability_out;
    std::ofstream html_out;
    std::ofstream vkconfig_out;

    // if any essential vulkan call fails, it throws an exception
    try {
        AppInstance instance = {};
        SetupWindowExtensions(instance);

        auto pNext_chains = get_chain_infos();

        auto phys_devices = instance.FindPhysicalDevices();

        std::vector<std::unique_ptr<AppSurface>> surfaces;
#if defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_WIN32_KHR) ||      \
    defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT) || defined(VK_USE_PLATFORM_WAYLAND_KHR) || \
    defined(VK_USE_PLATFORM_DIRECTFB_EXT)
        for (auto &surface_extension : instance.surface_extensions) {
            surface_extension.create_window(instance);
            surface_extension.surface = surface_extension.create_surface(instance);
            for (auto &phys_device : phys_devices) {
                surfaces.push_back(std::unique_ptr<AppSurface>(
                    new AppSurface(instance, phys_device, surface_extension, pNext_chains.surface_capabilities2)));
            }
        }
#endif

        std::vector<std::unique_ptr<AppGpu>> gpus;

        uint32_t gpu_counter = 0;
        for (auto &phys_device : phys_devices) {
            gpus.push_back(std::unique_ptr<AppGpu>(new AppGpu(instance, gpu_counter++, phys_device, pNext_chains)));
        }

        if (selected_gpu >= gpus.size()) {
            std::cout << "The selected gpu (" << selected_gpu << ") is not a valid GPU index. ";
            if (gpus.size() == 1)
                std::cout << "The only available GPU selection is 0.\n";
            else
                std::cout << "The available GPUs are in the range of 0 to " << gpus.size() - 1 << ".\n";
            return 0;
        }

        if (human_readable_output) {
            printers.push_back(std::unique_ptr<Printer>(new Printer(OutputType::text, out, selected_gpu, instance.vk_version)));
        }
        if (html_output) {
            html_out = std::ofstream("vulkaninfo.html");
            printers.push_back(
                std::unique_ptr<Printer>(new Printer(OutputType::html, html_out, selected_gpu, instance.vk_version)));
        }
        if (json_output) {
            std::string start_string =
                std::string("{\n\t\"$schema\": \"https://schema.khronos.org/vulkan/devsim_1_0_0.json#\",\n") +
                "\t\"comments\": {\n\t\t\"desc\": \"JSON configuration file describing GPU " + std::to_string(selected_gpu) + " (" +
                gpus.at(selected_gpu)->props.deviceName +
                "). Generated using the vulkaninfo program.\",\n\t\t\"vulkanApiVersion\": \"" +
                VkVersionString(instance.vk_version) + "\"\n" + "\t}";
#ifdef VK_USE_PLATFORM_IOS_MVK
            json_out = std::ofstream("vulkaninfo.json");
            printers.push_back(
                std::unique_ptr<Printer>(new Printer(OutputType::json, json_out, selected_gpu, instance.vk_version, start_string)));
#else
            printers.push_back(
                std::unique_ptr<Printer>(new Printer(OutputType::json, out, selected_gpu, instance.vk_version, start_string)));
#endif
        }
#if defined(VK_ENABLE_BETA_EXTENSIONS)
        if (portability_json) {
            if (!gpus.at(selected_gpu)->CheckPhysicalDeviceExtensionIncluded(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) {
                std::cerr << "Cannot create a json because the current selected GPU (" << selected_gpu
                          << ") does not support the VK_KHR_portability_subset extension.\n";
            } else {
                std::string start_string =
                    std::string(
                        "{\n\t\"$schema\": "
                        "\"https://schema.khronos.org/vulkan/devsim_VK_KHR_portability_subset-provisional-1.json#\",\n") +
                    "\t\"comments\": {\n\t\t\"desc\": \"JSON configuration file describing GPU " + std::to_string(selected_gpu) +
                    "'s (" + gpus.at(selected_gpu)->props.deviceName +
                    "( portability features and properties. Generated using the vulkaninfo program.\",\n\t\t\"vulkanApiVersion\": "
                    "\"" +
                    VkVersionString(instance.vk_version) + "\"\n" + "\t}";
#ifdef VK_USE_PLATFORM_IOS_MVK
                portability_out = std::ofstream("portability.json");
                printers.push_back(std::unique_ptr<Printer>(
                    new Printer(OutputType::json, portability_out, selected_gpu, instance.vk_version, start_string)));
#else
                printers.push_back(
                    std::unique_ptr<Printer>(new Printer(OutputType::json, out, selected_gpu, instance.vk_version, start_string)));
#endif
            }
        }
#endif  // defined(VK_ENABLE_BETA_EXTENSIONS)
        if (vkconfig_output) {
#ifdef WIN32
            vkconfig_out = std::ofstream(std::string(output_path) + "\\vulkaninfo.json");
#else
            vkconfig_out = std::ofstream(std::string(output_path) + "/vulkaninfo.json");
#endif
            std::string start_string = "{\n\t\"Vulkan Instance Version\": \"" + VkVersionString(instance.vk_version) + "\"";
            printers.push_back(std::unique_ptr<Printer>(
                new Printer(OutputType::vkconfig_output, vkconfig_out, selected_gpu, instance.vk_version, start_string)));
        }

        for (auto &p : printers) {
#ifdef VK_USE_PLATFORM_IOS_MVK
            p->SetAlwaysOpenDetails(true);
#endif
            if (summary) {
                DumpSummaryInstance(*p.get(), instance);
                p->SetHeader();
                ObjectWrapper obj(*p, "Devices");
                IndentWrapper indent(*p);
                for (auto &gpu : gpus) {
                    DumpSummaryGPU(*p.get(), *gpu.get());
                }
            } else if (p->Type() == OutputType::json) {
                if (portability_json) {
#if defined(VK_ENABLE_BETA_EXTENSIONS)
                    DumpPortability(*p.get(), *gpus.at(selected_gpu).get());
#endif  // defined(VK_ENABLE_BETA_EXTENSIONS)
                } else if (json_output) {
                    DumpLayers(*p.get(), instance.global_layers, gpus);
                    DumpGpuJson(*p.get(), *gpus.at(selected_gpu).get());
                }
            } else {
                // text, html, vkconfig_output
                p->SetHeader();
                DumpExtensions(*p.get(), "Instance", instance.global_extensions);
                p->AddNewline();

                DumpLayers(*p.get(), instance.global_layers, gpus);

#if defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_WIN32_KHR) ||      \
    defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT) || defined(VK_USE_PLATFORM_WAYLAND_KHR) || \
    defined(VK_USE_PLATFORM_DIRECTFB_EXT)
                DumpPresentableSurfaces(*p.get(), instance, gpus, surfaces);
#endif
                DumpGroups(*p.get(), instance);

                p->SetHeader();
                ObjectWrapper obj(*p, "Device Properties and Extensions");
                IndentWrapper indent(*p);

                for (auto &gpu : gpus) {
                    DumpGpu(*p.get(), *gpu.get(), show_formats);
                }
            }
        }

#if defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_WIN32_KHR) ||      \
    defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT) || defined(VK_USE_PLATFORM_WAYLAND_KHR) || \
    defined(VK_USE_PLATFORM_DIRECTFB_EXT)

        for (auto &surface_extension : instance.surface_extensions) {
            AppDestroySurface(instance, surface_extension.surface);
            surface_extension.destroy_window(instance);
        }
#endif
    } catch (std::exception &e) {
        // Print the error to stderr and leave all outputs in a valid state (mainly for json)
        std::cerr << "ERROR at " << e.what() << "\n";
        for (auto &p : printers) {
            if (p) {
                p->FinishOutput();
            }
        }
    }
    // Call the printer's descrtuctor before the file handle gets closed
    for (auto &p : printers) {
        p.reset(nullptr);
    }

    WAIT_FOR_CONSOLE_DESTROY;
#ifdef _WIN32
    FreeUser32Dll();
#endif

    return 0;
}

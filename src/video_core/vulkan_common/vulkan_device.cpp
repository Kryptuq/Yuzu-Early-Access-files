// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <bitset>
#include <chrono>
#include <optional>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common/assert.h"
#include "common/literals.h"
#include "common/polyfill_ranges.h"
#include "common/settings.h"
#include "video_core/vulkan_common/nsight_aftermath_tracker.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {
using namespace Common::Literals;
namespace {
namespace Alternatives {
constexpr std::array STENCIL8_UINT{
    VK_FORMAT_D16_UNORM_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_UNDEFINED,
};

constexpr std::array DEPTH24_UNORM_STENCIL8_UINT{
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D16_UNORM_S8_UINT,
    VK_FORMAT_UNDEFINED,
};

constexpr std::array DEPTH16_UNORM_STENCIL8_UINT{
    VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_UNDEFINED,
};

constexpr std::array B5G6R5_UNORM_PACK16{
    VK_FORMAT_R5G6B5_UNORM_PACK16,
    VK_FORMAT_UNDEFINED,
};

constexpr std::array R4G4_UNORM_PACK8{
    VK_FORMAT_R8_UNORM,
    VK_FORMAT_UNDEFINED,
};

constexpr std::array R16G16B16_SFLOAT{
    VK_FORMAT_R16G16B16A16_SFLOAT,
    VK_FORMAT_UNDEFINED,
};

constexpr std::array R16G16B16_SSCALED{
    VK_FORMAT_R16G16B16A16_SSCALED,
    VK_FORMAT_UNDEFINED,
};

constexpr std::array R8G8B8_SSCALED{
    VK_FORMAT_R8G8B8A8_SSCALED,
    VK_FORMAT_UNDEFINED,
};

} // namespace Alternatives

enum class NvidiaArchitecture {
    AmpereOrNewer,
    Turing,
    VoltaOrOlder,
};

template <typename T>
void SetNext(void**& next, T& data) {
    *next = &data;
    next = &data.pNext;
}

constexpr const VkFormat* GetFormatAlternatives(VkFormat format) {
    switch (format) {
    case VK_FORMAT_S8_UINT:
        return Alternatives::STENCIL8_UINT.data();
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return Alternatives::DEPTH24_UNORM_STENCIL8_UINT.data();
    case VK_FORMAT_D16_UNORM_S8_UINT:
        return Alternatives::DEPTH16_UNORM_STENCIL8_UINT.data();
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
        return Alternatives::B5G6R5_UNORM_PACK16.data();
    case VK_FORMAT_R4G4_UNORM_PACK8:
        return Alternatives::R4G4_UNORM_PACK8.data();
    case VK_FORMAT_R16G16B16_SFLOAT:
        return Alternatives::R16G16B16_SFLOAT.data();
    case VK_FORMAT_R16G16B16_SSCALED:
        return Alternatives::R16G16B16_SSCALED.data();
    case VK_FORMAT_R8G8B8_SSCALED:
        return Alternatives::R8G8B8_SSCALED.data();
    default:
        return nullptr;
    }
}

VkFormatFeatureFlags GetFormatFeatures(VkFormatProperties properties, FormatType format_type) {
    switch (format_type) {
    case FormatType::Linear:
        return properties.linearTilingFeatures;
    case FormatType::Optimal:
        return properties.optimalTilingFeatures;
    case FormatType::Buffer:
        return properties.bufferFeatures;
    default:
        return {};
    }
}

std::unordered_map<VkFormat, VkFormatProperties> GetFormatProperties(vk::PhysicalDevice physical) {
    static constexpr std::array formats{
        VK_FORMAT_A1R5G5B5_UNORM_PACK16,
        VK_FORMAT_A2B10G10R10_SINT_PACK32,
        VK_FORMAT_A2B10G10R10_SNORM_PACK32,
        VK_FORMAT_A2B10G10R10_SSCALED_PACK32,
        VK_FORMAT_A2B10G10R10_UINT_PACK32,
        VK_FORMAT_A2B10G10R10_UNORM_PACK32,
        VK_FORMAT_A2B10G10R10_USCALED_PACK32,
        VK_FORMAT_A8B8G8R8_SINT_PACK32,
        VK_FORMAT_A8B8G8R8_SNORM_PACK32,
        VK_FORMAT_A8B8G8R8_SRGB_PACK32,
        VK_FORMAT_A8B8G8R8_UINT_PACK32,
        VK_FORMAT_A8B8G8R8_UNORM_PACK32,
        VK_FORMAT_ASTC_10x10_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
        VK_FORMAT_ASTC_10x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
        VK_FORMAT_ASTC_10x6_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
        VK_FORMAT_ASTC_10x8_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
        VK_FORMAT_ASTC_12x10_SRGB_BLOCK,
        VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
        VK_FORMAT_ASTC_12x12_SRGB_BLOCK,
        VK_FORMAT_ASTC_12x12_UNORM_BLOCK,
        VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
        VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
        VK_FORMAT_ASTC_5x4_SRGB_BLOCK,
        VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
        VK_FORMAT_ASTC_5x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
        VK_FORMAT_ASTC_6x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
        VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
        VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
        VK_FORMAT_ASTC_8x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
        VK_FORMAT_ASTC_8x6_SRGB_BLOCK,
        VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
        VK_FORMAT_ASTC_8x8_SRGB_BLOCK,
        VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
        VK_FORMAT_B10G11R11_UFLOAT_PACK32,
        VK_FORMAT_B4G4R4A4_UNORM_PACK16,
        VK_FORMAT_B5G5R5A1_UNORM_PACK16,
        VK_FORMAT_B5G6R5_UNORM_PACK16,
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
        VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
        VK_FORMAT_BC2_SRGB_BLOCK,
        VK_FORMAT_BC2_UNORM_BLOCK,
        VK_FORMAT_BC3_SRGB_BLOCK,
        VK_FORMAT_BC3_UNORM_BLOCK,
        VK_FORMAT_BC4_SNORM_BLOCK,
        VK_FORMAT_BC4_UNORM_BLOCK,
        VK_FORMAT_BC5_SNORM_BLOCK,
        VK_FORMAT_BC5_UNORM_BLOCK,
        VK_FORMAT_BC6H_SFLOAT_BLOCK,
        VK_FORMAT_BC6H_UFLOAT_BLOCK,
        VK_FORMAT_BC7_SRGB_BLOCK,
        VK_FORMAT_BC7_UNORM_BLOCK,
        VK_FORMAT_D16_UNORM,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R16G16B16A16_SINT,
        VK_FORMAT_R16G16B16A16_SNORM,
        VK_FORMAT_R16G16B16A16_SSCALED,
        VK_FORMAT_R16G16B16A16_UINT,
        VK_FORMAT_R16G16B16A16_UNORM,
        VK_FORMAT_R16G16B16A16_USCALED,
        VK_FORMAT_R16G16B16_SFLOAT,
        VK_FORMAT_R16G16B16_SINT,
        VK_FORMAT_R16G16B16_SNORM,
        VK_FORMAT_R16G16B16_SSCALED,
        VK_FORMAT_R16G16B16_UINT,
        VK_FORMAT_R16G16B16_UNORM,
        VK_FORMAT_R16G16B16_USCALED,
        VK_FORMAT_R16G16_SFLOAT,
        VK_FORMAT_R16G16_SINT,
        VK_FORMAT_R16G16_SNORM,
        VK_FORMAT_R16G16_SSCALED,
        VK_FORMAT_R16G16_UINT,
        VK_FORMAT_R16G16_UNORM,
        VK_FORMAT_R16G16_USCALED,
        VK_FORMAT_R16_SFLOAT,
        VK_FORMAT_R16_SINT,
        VK_FORMAT_R16_SNORM,
        VK_FORMAT_R16_SSCALED,
        VK_FORMAT_R16_UINT,
        VK_FORMAT_R16_UNORM,
        VK_FORMAT_R16_USCALED,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_FORMAT_R32G32B32A32_SINT,
        VK_FORMAT_R32G32B32A32_UINT,
        VK_FORMAT_R32G32B32_SFLOAT,
        VK_FORMAT_R32G32B32_SINT,
        VK_FORMAT_R32G32B32_UINT,
        VK_FORMAT_R32G32_SFLOAT,
        VK_FORMAT_R32G32_SINT,
        VK_FORMAT_R32G32_UINT,
        VK_FORMAT_R32_SFLOAT,
        VK_FORMAT_R32_SINT,
        VK_FORMAT_R32_UINT,
        VK_FORMAT_R4G4B4A4_UNORM_PACK16,
        VK_FORMAT_R4G4_UNORM_PACK8,
        VK_FORMAT_R5G5B5A1_UNORM_PACK16,
        VK_FORMAT_R5G6B5_UNORM_PACK16,
        VK_FORMAT_R8G8B8A8_SINT,
        VK_FORMAT_R8G8B8A8_SNORM,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R8G8B8A8_SSCALED,
        VK_FORMAT_R8G8B8A8_UINT,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_USCALED,
        VK_FORMAT_R8G8B8_SINT,
        VK_FORMAT_R8G8B8_SNORM,
        VK_FORMAT_R8G8B8_SSCALED,
        VK_FORMAT_R8G8B8_UINT,
        VK_FORMAT_R8G8B8_UNORM,
        VK_FORMAT_R8G8B8_USCALED,
        VK_FORMAT_R8G8_SINT,
        VK_FORMAT_R8G8_SNORM,
        VK_FORMAT_R8G8_SSCALED,
        VK_FORMAT_R8G8_UINT,
        VK_FORMAT_R8G8_UNORM,
        VK_FORMAT_R8G8_USCALED,
        VK_FORMAT_R8_SINT,
        VK_FORMAT_R8_SNORM,
        VK_FORMAT_R8_SSCALED,
        VK_FORMAT_R8_UINT,
        VK_FORMAT_R8_UNORM,
        VK_FORMAT_R8_USCALED,
        VK_FORMAT_S8_UINT,
    };
    std::unordered_map<VkFormat, VkFormatProperties> format_properties;
    for (const auto format : formats) {
        format_properties.emplace(format, physical.GetFormatProperties(format));
    }
    return format_properties;
}

NvidiaArchitecture GetNvidiaArchitecture(vk::PhysicalDevice physical,
                                         const std::set<std::string, std::less<>>& exts) {
    if (exts.contains(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME)) {
        VkPhysicalDeviceFragmentShadingRatePropertiesKHR shading_rate_props{};
        shading_rate_props.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
        VkPhysicalDeviceProperties2 physical_properties{};
        physical_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        physical_properties.pNext = &shading_rate_props;
        physical.GetProperties2(physical_properties);
        if (shading_rate_props.primitiveFragmentShadingRateWithMultipleViewports) {
            // Only Ampere and newer support this feature
            return NvidiaArchitecture::AmpereOrNewer;
        }
    }
    if (exts.contains(VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME)) {
        return NvidiaArchitecture::Turing;
    }
    return NvidiaArchitecture::VoltaOrOlder;
}

std::vector<const char*> ExtensionListForVulkan(
    const std::set<std::string, std::less<>>& extensions) {
    std::vector<const char*> output;
    for (const auto& extension : extensions) {
        output.push_back(extension.c_str());
    }
    return output;
}

} // Anonymous namespace

Device::Device(VkInstance instance_, vk::PhysicalDevice physical_, VkSurfaceKHR surface,
               const vk::InstanceDispatch& dld_)
    : instance{instance_}, dld{dld_}, physical{physical_},
      format_properties(GetFormatProperties(physical)) {
    // Get suitability and device properties.
    const bool is_suitable = GetSuitability(surface != nullptr);

    const VkDriverId driver_id = properties.driver.driverID;
    const bool is_radv = driver_id == VK_DRIVER_ID_MESA_RADV;
    const bool is_amd_driver =
        driver_id == VK_DRIVER_ID_AMD_PROPRIETARY || driver_id == VK_DRIVER_ID_AMD_OPEN_SOURCE;
    const bool is_amd = is_amd_driver || is_radv;
    const bool is_intel_windows = driver_id == VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS;
    const bool is_intel_anv = driver_id == VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA;
    const bool is_nvidia = driver_id == VK_DRIVER_ID_NVIDIA_PROPRIETARY;
    const bool is_mvk = driver_id == VK_DRIVER_ID_MOLTENVK;

    if (is_mvk && !is_suitable) {
        LOG_WARNING(Render_Vulkan, "Unsuitable driver is MoltenVK, continuing anyway");
    } else if (!is_suitable) {
        throw vk::Exception(VK_ERROR_INCOMPATIBLE_DRIVER);
    }

    SetupFamilies(surface);
    const auto queue_cis = GetDeviceQueueCreateInfos();

    // GetSuitability has already configured the linked list of features for us.
    // Reuse it here.
    const void* first_next = &features2;

    VkDeviceDiagnosticsConfigCreateInfoNV diagnostics_nv{};
    if (Settings::values.enable_nsight_aftermath && extensions.device_diagnostics_config) {
        nsight_aftermath_tracker = std::make_unique<NsightAftermathTracker>();

        diagnostics_nv = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV,
            .pNext = &features2,
            .flags = VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV |
                     VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |
                     VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV,
        };
        first_next = &diagnostics_nv;
    }

    is_blit_depth_stencil_supported = TestDepthStencilBlits();
    is_optimal_astc_supported = ComputeIsOptimalAstcSupported();
    is_warp_potentially_bigger = !extensions.subgroup_size_control ||
                                 properties.subgroup_size_control.maxSubgroupSize > GuestWarpSize;

    is_integrated = properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    is_virtual = properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;
    is_non_gpu = properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_OTHER ||
                 properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;

    supports_d24_depth =
        IsFormatSupported(VK_FORMAT_D24_UNORM_S8_UINT,
                          VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, FormatType::Optimal);

    CollectPhysicalMemoryInfo();
    CollectToolingInfo();

    if (is_nvidia) {
        const u32 nv_major_version = (properties.properties.driverVersion >> 22) & 0x3ff;
        const auto arch = GetNvidiaArchitecture(physical, supported_extensions);
        switch (arch) {
        case NvidiaArchitecture::AmpereOrNewer:
            LOG_WARNING(Render_Vulkan, "Ampere and newer have broken float16 math");
            features.shader_float16_int8.shaderFloat16 = false;
            break;
        case NvidiaArchitecture::Turing:
            break;
        case NvidiaArchitecture::VoltaOrOlder:
            if (nv_major_version < 527) {
                LOG_WARNING(Render_Vulkan, "Volta and older have broken VK_KHR_push_descriptor");
                extensions.push_descriptor = false;
                loaded_extensions.erase(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
            }
            break;
        }
        if (nv_major_version >= 510) {
            LOG_WARNING(Render_Vulkan, "NVIDIA Drivers >= 510 do not support MSAA image blits");
            cant_blit_msaa = true;
        }
    }
    if (extensions.extended_dynamic_state && is_radv) {
        // Mask driver version variant
        const u32 version = (properties.properties.driverVersion << 3) >> 3;
        if (version < VK_MAKE_API_VERSION(0, 21, 2, 0)) {
            LOG_WARNING(Render_Vulkan,
                        "RADV versions older than 21.2 have broken VK_EXT_extended_dynamic_state");
            extensions.extended_dynamic_state = false;
            loaded_extensions.erase(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
        }
    }
    if (extensions.extended_dynamic_state2 && is_radv) {
        const u32 version = (properties.properties.driverVersion << 3) >> 3;
        if (version < VK_MAKE_API_VERSION(0, 22, 3, 1)) {
            LOG_WARNING(
                Render_Vulkan,
                "RADV versions older than 22.3.1 have broken VK_EXT_extended_dynamic_state2");
            features.extended_dynamic_state2.extendedDynamicState2 = false;
            features.extended_dynamic_state2.extendedDynamicState2LogicOp = false;
            features.extended_dynamic_state2.extendedDynamicState2PatchControlPoints = false;
            extensions.extended_dynamic_state2 = false;
            loaded_extensions.erase(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
        }
    }
    if (extensions.vertex_input_dynamic_state && is_radv) {
        // TODO(ameerj): Blacklist only offending driver versions
        // TODO(ameerj): Confirm if RDNA1 is affected
        const bool is_rdna2 =
            supported_extensions.contains(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
        if (is_rdna2) {
            LOG_WARNING(Render_Vulkan,
                        "RADV has broken VK_EXT_vertex_input_dynamic_state on RDNA2 hardware");
            features.vertex_input_dynamic_state.vertexInputDynamicState = false;
            extensions.vertex_input_dynamic_state = false;
            loaded_extensions.erase(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);
        }
    }

    sets_per_pool = 64;
    if (is_amd_driver) {
        // AMD drivers need a higher amount of Sets per Pool in certain circumstances like in XC2.
        sets_per_pool = 96;
        // Disable VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT on AMD GCN4 and lower as it is broken.
        if (!features.shader_float16_int8.shaderFloat16) {
            LOG_WARNING(Render_Vulkan,
                        "AMD GCN4 and earlier have broken VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT");
            has_broken_cube_compatibility = true;
        }
    }
    if (extensions.sampler_filter_minmax && is_amd) {
        // Disable ext_sampler_filter_minmax on AMD GCN4 and lower as it is broken.
        if (!features.shader_float16_int8.shaderFloat16) {
            LOG_WARNING(Render_Vulkan,
                        "AMD GCN4 and earlier have broken VK_EXT_sampler_filter_minmax");
            extensions.sampler_filter_minmax = false;
            loaded_extensions.erase(VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME);
        }
    }

    if (extensions.vertex_input_dynamic_state && is_intel_windows) {
        const u32 version = (properties.properties.driverVersion << 3) >> 3;
        if (version < VK_MAKE_API_VERSION(27, 20, 100, 0)) {
            LOG_WARNING(Render_Vulkan, "Intel has broken VK_EXT_vertex_input_dynamic_state");
            extensions.vertex_input_dynamic_state = false;
            loaded_extensions.erase(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);
        }
    }
    if (features.shader_float16_int8.shaderFloat16 && is_intel_windows) {
        // Intel's compiler crashes when using fp16 on Astral Chain, disable it for the time being.
        LOG_WARNING(Render_Vulkan, "Intel has broken float16 math");
        features.shader_float16_int8.shaderFloat16 = false;
    }
    if (is_intel_windows) {
        LOG_WARNING(Render_Vulkan, "Intel proprietary drivers do not support MSAA image blits");
        cant_blit_msaa = true;
    }
    if (is_intel_anv) {
        LOG_WARNING(Render_Vulkan, "ANV driver does not support native BGR format");
        must_emulate_bgr565 = true;
    }
    if (is_mvk) {
        LOG_WARNING(Render_Vulkan,
                    "MVK driver breaks when using more than 16 vertex attributes/bindings");
        properties.properties.limits.maxVertexInputAttributes =
            std::min(properties.properties.limits.maxVertexInputAttributes, 16U);
        properties.properties.limits.maxVertexInputBindings =
            std::min(properties.properties.limits.maxVertexInputBindings, 16U);
    }

    logical = vk::Device::Create(physical, queue_cis, ExtensionListForVulkan(loaded_extensions),
                                 first_next, dld);

    graphics_queue = logical.GetQueue(graphics_family);
    present_queue = logical.GetQueue(present_family);
}

Device::~Device() = default;

VkFormat Device::GetSupportedFormat(VkFormat wanted_format, VkFormatFeatureFlags wanted_usage,
                                    FormatType format_type) const {
    if (IsFormatSupported(wanted_format, wanted_usage, format_type)) {
        return wanted_format;
    }
    // The wanted format is not supported by hardware, search for alternatives
    const VkFormat* alternatives = GetFormatAlternatives(wanted_format);
    if (alternatives == nullptr) {
        ASSERT_MSG(false,
                   "Format={} with usage={} and type={} has no defined alternatives and host "
                   "hardware does not support it",
                   wanted_format, wanted_usage, format_type);
        return wanted_format;
    }

    std::size_t i = 0;
    for (VkFormat alternative = *alternatives; alternative; alternative = alternatives[++i]) {
        if (!IsFormatSupported(alternative, wanted_usage, format_type)) {
            continue;
        }
        LOG_DEBUG(Render_Vulkan,
                  "Emulating format={} with alternative format={} with usage={} and type={}",
                  wanted_format, alternative, wanted_usage, format_type);
        return alternative;
    }

    // No alternatives found, panic
    ASSERT_MSG(false,
               "Format={} with usage={} and type={} is not supported by the host hardware and "
               "doesn't support any of the alternatives",
               wanted_format, wanted_usage, format_type);
    return wanted_format;
}

void Device::ReportLoss() const {
    LOG_CRITICAL(Render_Vulkan, "Device loss occurred!");

    // Wait for the log to flush and for Nsight Aftermath to dump the results
    std::this_thread::sleep_for(std::chrono::seconds{15});
}

void Device::SaveShader(std::span<const u32> spirv) const {
    if (nsight_aftermath_tracker) {
        nsight_aftermath_tracker->SaveShader(spirv);
    }
}

bool Device::ComputeIsOptimalAstcSupported() const {
    // Disable for now to avoid converting ASTC twice.
    static constexpr std::array astc_formats = {
        VK_FORMAT_ASTC_4x4_UNORM_BLOCK,   VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
        VK_FORMAT_ASTC_5x4_UNORM_BLOCK,   VK_FORMAT_ASTC_5x4_SRGB_BLOCK,
        VK_FORMAT_ASTC_5x5_UNORM_BLOCK,   VK_FORMAT_ASTC_5x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_6x5_UNORM_BLOCK,   VK_FORMAT_ASTC_6x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_6x6_UNORM_BLOCK,   VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
        VK_FORMAT_ASTC_8x5_UNORM_BLOCK,   VK_FORMAT_ASTC_8x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_8x6_UNORM_BLOCK,   VK_FORMAT_ASTC_8x6_SRGB_BLOCK,
        VK_FORMAT_ASTC_8x8_UNORM_BLOCK,   VK_FORMAT_ASTC_8x8_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x5_UNORM_BLOCK,  VK_FORMAT_ASTC_10x5_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x6_UNORM_BLOCK,  VK_FORMAT_ASTC_10x6_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x8_UNORM_BLOCK,  VK_FORMAT_ASTC_10x8_SRGB_BLOCK,
        VK_FORMAT_ASTC_10x10_UNORM_BLOCK, VK_FORMAT_ASTC_10x10_SRGB_BLOCK,
        VK_FORMAT_ASTC_12x10_UNORM_BLOCK, VK_FORMAT_ASTC_12x10_SRGB_BLOCK,
        VK_FORMAT_ASTC_12x12_UNORM_BLOCK, VK_FORMAT_ASTC_12x12_SRGB_BLOCK,
    };
    if (!features.features.textureCompressionASTC_LDR) {
        return false;
    }
    const auto format_feature_usage{
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT |
        VK_FORMAT_FEATURE_BLIT_DST_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
        VK_FORMAT_FEATURE_TRANSFER_DST_BIT};
    for (const auto format : astc_formats) {
        const auto physical_format_properties{physical.GetFormatProperties(format)};
        if ((physical_format_properties.optimalTilingFeatures & format_feature_usage) == 0) {
            return false;
        }
    }
    return true;
}

bool Device::TestDepthStencilBlits() const {
    static constexpr VkFormatFeatureFlags required_features =
        VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT;
    const auto test_features = [](VkFormatProperties props) {
        return (props.optimalTilingFeatures & required_features) == required_features;
    };
    return test_features(format_properties.at(VK_FORMAT_D32_SFLOAT_S8_UINT)) &&
           test_features(format_properties.at(VK_FORMAT_D24_UNORM_S8_UINT));
}

bool Device::IsFormatSupported(VkFormat wanted_format, VkFormatFeatureFlags wanted_usage,
                               FormatType format_type) const {
    const auto it = format_properties.find(wanted_format);
    if (it == format_properties.end()) {
        UNIMPLEMENTED_MSG("Unimplemented format query={}", wanted_format);
        return true;
    }
    const auto supported_usage = GetFormatFeatures(it->second, format_type);
    return (supported_usage & wanted_usage) == wanted_usage;
}

std::string Device::GetDriverName() const {
    switch (properties.driver.driverID) {
    case VK_DRIVER_ID_AMD_PROPRIETARY:
        return "AMD";
    case VK_DRIVER_ID_AMD_OPEN_SOURCE:
        return "AMDVLK";
    case VK_DRIVER_ID_MESA_RADV:
        return "RADV";
    case VK_DRIVER_ID_NVIDIA_PROPRIETARY:
        return "NVIDIA";
    case VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS:
        return "INTEL";
    case VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA:
        return "ANV";
    case VK_DRIVER_ID_MESA_LLVMPIPE:
        return "LAVAPIPE";
    default:
        return properties.driver.driverName;
    }
}

bool Device::ShouldBoostClocks() const {
    const auto driver_id = properties.driver.driverID;
    const auto vendor_id = properties.properties.vendorID;
    const auto device_id = properties.properties.deviceID;

    const bool validated_driver =
        driver_id == VK_DRIVER_ID_AMD_PROPRIETARY || driver_id == VK_DRIVER_ID_AMD_OPEN_SOURCE ||
        driver_id == VK_DRIVER_ID_MESA_RADV || driver_id == VK_DRIVER_ID_NVIDIA_PROPRIETARY ||
        driver_id == VK_DRIVER_ID_INTEL_PROPRIETARY_WINDOWS ||
        driver_id == VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA;

    const bool is_steam_deck = vendor_id == 0x1002 && device_id == 0x163F;

    return validated_driver && !is_steam_deck;
}

bool Device::GetSuitability(bool requires_swapchain) {
    // Assume we will be suitable.
    bool suitable = true;

    // Configure properties.
    properties.properties = physical.GetProperties();

    // Set instance version.
    instance_version = properties.properties.apiVersion;

    // Minimum of API version 1.1 is required. (This is well-supported.)
    ASSERT(instance_version >= VK_API_VERSION_1_1);

    // Get available extensions.
    auto extension_properties = physical.EnumerateDeviceExtensionProperties();

    // Get the set of supported extensions.
    supported_extensions.clear();
    for (const VkExtensionProperties& property : extension_properties) {
        supported_extensions.insert(property.extensionName);
    }

    // Generate list of extensions to load.
    loaded_extensions.clear();

#define EXTENSION(prefix, macro_name, var_name)                                                    \
    if (supported_extensions.contains(VK_##prefix##_##macro_name##_EXTENSION_NAME)) {              \
        loaded_extensions.insert(VK_##prefix##_##macro_name##_EXTENSION_NAME);                     \
        extensions.var_name = true;                                                                \
    }
#define FEATURE_EXTENSION(prefix, struct_name, macro_name, var_name)                               \
    if (supported_extensions.contains(VK_##prefix##_##macro_name##_EXTENSION_NAME)) {              \
        loaded_extensions.insert(VK_##prefix##_##macro_name##_EXTENSION_NAME);                     \
        extensions.var_name = true;                                                                \
    }

    if (instance_version < VK_API_VERSION_1_2) {
        FOR_EACH_VK_FEATURE_1_2(FEATURE_EXTENSION);
    }
    if (instance_version < VK_API_VERSION_1_3) {
        FOR_EACH_VK_FEATURE_1_3(FEATURE_EXTENSION);
    }

    FOR_EACH_VK_FEATURE_EXT(FEATURE_EXTENSION);
    FOR_EACH_VK_EXTENSION(EXTENSION);
#ifdef _WIN32
    FOR_EACH_VK_EXTENSION_WIN32(EXTENSION);
#endif

#undef FEATURE_EXTENSION
#undef EXTENSION

    // Some extensions are mandatory. Check those.
#define CHECK_EXTENSION(extension_name)                                                            \
    if (!loaded_extensions.contains(extension_name)) {                                             \
        LOG_ERROR(Render_Vulkan, "Missing required extension {}", extension_name);                 \
        suitable = false;                                                                          \
    }

#define LOG_EXTENSION(extension_name)                                                              \
    if (!loaded_extensions.contains(extension_name)) {                                             \
        LOG_INFO(Render_Vulkan, "Device doesn't support extension {}", extension_name);            \
    }

    FOR_EACH_VK_RECOMMENDED_EXTENSION(LOG_EXTENSION);
    FOR_EACH_VK_MANDATORY_EXTENSION(CHECK_EXTENSION);
#ifdef _WIN32
    FOR_EACH_VK_MANDATORY_EXTENSION_WIN32(CHECK_EXTENSION);
#else
    FOR_EACH_VK_MANDATORY_EXTENSION_GENERIC(CHECK_EXTENSION);
#endif

    if (requires_swapchain) {
        CHECK_EXTENSION(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

#undef LOG_EXTENSION
#undef CHECK_EXTENSION

    // Generate the linked list of features to test.
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    // Set next pointer.
    void** next = &features2.pNext;

    // Test all features we know about. If the feature is not available in core at our
    // current API version, and was not enabled by an extension, skip testing the feature.
    // We set the structure sType explicitly here as it is zeroed by the constructor.
#define FEATURE(prefix, struct_name, macro_name, var_name)                                         \
    features.var_name.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_##macro_name##_FEATURES;           \
    SetNext(next, features.var_name);

#define EXT_FEATURE(prefix, struct_name, macro_name, var_name)                                     \
    if (extensions.var_name) {                                                                     \
        features.var_name.sType =                                                                  \
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_##macro_name##_FEATURES_##prefix;                    \
        SetNext(next, features.var_name);                                                          \
    }

    FOR_EACH_VK_FEATURE_1_1(FEATURE);
    FOR_EACH_VK_FEATURE_EXT(EXT_FEATURE);
    if (instance_version >= VK_API_VERSION_1_2) {
        FOR_EACH_VK_FEATURE_1_2(FEATURE);
    } else {
        FOR_EACH_VK_FEATURE_1_2(EXT_FEATURE);
    }
    if (instance_version >= VK_API_VERSION_1_3) {
        FOR_EACH_VK_FEATURE_1_3(FEATURE);
    } else {
        FOR_EACH_VK_FEATURE_1_3(EXT_FEATURE);
    }

#undef EXT_FEATURE
#undef FEATURE

    // Perform the feature test.
    physical.GetFeatures2(features2);
    features.features = features2.features;

    // Some features are mandatory. Check those.
#define CHECK_FEATURE(feature, name)                                                               \
    if (!features.feature.name) {                                                                  \
        LOG_ERROR(Render_Vulkan, "Missing required feature {}", #name);                            \
        suitable = false;                                                                          \
    }

#define LOG_FEATURE(feature, name)                                                                 \
    if (!features.feature.name) {                                                                  \
        LOG_INFO(Render_Vulkan, "Device doesn't support feature {}", #name);                       \
    }

    FOR_EACH_VK_RECOMMENDED_FEATURE(LOG_FEATURE);
    FOR_EACH_VK_MANDATORY_FEATURE(CHECK_FEATURE);

#undef LOG_FEATURE
#undef CHECK_FEATURE

    // Generate linked list of properties.
    properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

    // Set next pointer.
    next = &properties2.pNext;

    // Get driver info.
    properties.driver.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
    SetNext(next, properties.driver);

    // Retrieve relevant extension properties.
    if (extensions.shader_float_controls) {
        properties.float_controls.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES;
        SetNext(next, properties.float_controls);
    }
    if (extensions.push_descriptor) {
        properties.push_descriptor.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR;
        SetNext(next, properties.push_descriptor);
    }
    if (extensions.subgroup_size_control) {
        properties.subgroup_size_control.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES;
        SetNext(next, properties.subgroup_size_control);
    }
    if (extensions.transform_feedback) {
        properties.transform_feedback.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT;
        SetNext(next, properties.transform_feedback);
    }

    // Perform the property fetch.
    physical.GetProperties2(properties2);
    properties.properties = properties2.properties;

    // Unload extensions if feature support is insufficient.
    RemoveUnsuitableExtensions();

    // Check limits.
    struct Limit {
        u32 minimum;
        u32 value;
        const char* name;
    };

    const VkPhysicalDeviceLimits& limits{properties.properties.limits};
    const std::array limits_report{
        Limit{65536, limits.maxUniformBufferRange, "maxUniformBufferRange"},
        Limit{16, limits.maxViewports, "maxViewports"},
        Limit{8, limits.maxColorAttachments, "maxColorAttachments"},
        Limit{8, limits.maxClipDistances, "maxClipDistances"},
    };

    for (const auto& [min, value, name] : limits_report) {
        if (value < min) {
            LOG_ERROR(Render_Vulkan, "{} has to be {} or greater but it is {}", name, min, value);
            suitable = false;
        }
    }

    // Return whether we were suitable.
    return suitable;
}

void Device::RemoveExtensionIfUnsuitable(bool is_suitable, const std::string& extension_name) {
    if (loaded_extensions.contains(extension_name) && !is_suitable) {
        LOG_WARNING(Render_Vulkan, "Removing unsuitable extension {}", extension_name);
        loaded_extensions.erase(extension_name);
    }
}

void Device::RemoveUnsuitableExtensions() {
    // VK_EXT_custom_border_color
    extensions.custom_border_color = features.custom_border_color.customBorderColors &&
                                     features.custom_border_color.customBorderColorWithoutFormat;
    RemoveExtensionIfUnsuitable(extensions.custom_border_color,
                                VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);

    // VK_EXT_depth_clip_control
    extensions.depth_clip_control = features.depth_clip_control.depthClipControl;
    RemoveExtensionIfUnsuitable(extensions.depth_clip_control,
                                VK_EXT_DEPTH_CLIP_CONTROL_EXTENSION_NAME);

    // VK_EXT_extended_dynamic_state
    extensions.extended_dynamic_state = features.extended_dynamic_state.extendedDynamicState;
    RemoveExtensionIfUnsuitable(extensions.extended_dynamic_state,
                                VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);

    // VK_EXT_extended_dynamic_state2
    extensions.extended_dynamic_state2 = features.extended_dynamic_state2.extendedDynamicState2;
    RemoveExtensionIfUnsuitable(extensions.extended_dynamic_state2,
                                VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);

    // VK_EXT_extended_dynamic_state3
    dynamic_state3_blending =
        features.extended_dynamic_state3.extendedDynamicState3ColorBlendEnable &&
        features.extended_dynamic_state3.extendedDynamicState3ColorBlendEquation &&
        features.extended_dynamic_state3.extendedDynamicState3ColorWriteMask;
    dynamic_state3_enables =
        features.extended_dynamic_state3.extendedDynamicState3DepthClampEnable &&
        features.extended_dynamic_state3.extendedDynamicState3LogicOpEnable;

    extensions.extended_dynamic_state3 = dynamic_state3_blending || dynamic_state3_enables;
    dynamic_state3_blending = dynamic_state3_blending && extensions.extended_dynamic_state3;
    dynamic_state3_enables = dynamic_state3_enables && extensions.extended_dynamic_state3;
    RemoveExtensionIfUnsuitable(extensions.extended_dynamic_state3,
                                VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);

    // VK_EXT_provoking_vertex
    extensions.provoking_vertex =
        features.provoking_vertex.provokingVertexLast &&
        features.provoking_vertex.transformFeedbackPreservesProvokingVertex;
    RemoveExtensionIfUnsuitable(extensions.provoking_vertex,
                                VK_EXT_PROVOKING_VERTEX_EXTENSION_NAME);

    // VK_KHR_shader_atomic_int64
    extensions.shader_atomic_int64 = features.shader_atomic_int64.shaderBufferInt64Atomics &&
                                     features.shader_atomic_int64.shaderSharedInt64Atomics;
    RemoveExtensionIfUnsuitable(extensions.shader_atomic_int64,
                                VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME);

    // VK_EXT_shader_demote_to_helper_invocation
    extensions.shader_demote_to_helper_invocation =
        features.shader_demote_to_helper_invocation.shaderDemoteToHelperInvocation;
    RemoveExtensionIfUnsuitable(extensions.shader_demote_to_helper_invocation,
                                VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME);

    // VK_EXT_subgroup_size_control
    extensions.subgroup_size_control =
        features.subgroup_size_control.subgroupSizeControl &&
        properties.subgroup_size_control.minSubgroupSize <= GuestWarpSize &&
        properties.subgroup_size_control.maxSubgroupSize >= GuestWarpSize;
    RemoveExtensionIfUnsuitable(extensions.subgroup_size_control,
                                VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME);

    // VK_EXT_transform_feedback
    extensions.transform_feedback =
        features.transform_feedback.transformFeedback &&
        features.transform_feedback.geometryStreams &&
        properties.transform_feedback.maxTransformFeedbackStreams >= 4 &&
        properties.transform_feedback.maxTransformFeedbackBuffers > 0 &&
        properties.transform_feedback.transformFeedbackQueries &&
        properties.transform_feedback.transformFeedbackDraw;
    RemoveExtensionIfUnsuitable(extensions.transform_feedback,
                                VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME);

    // VK_EXT_vertex_input_dynamic_state
    extensions.vertex_input_dynamic_state =
        features.vertex_input_dynamic_state.vertexInputDynamicState;
    RemoveExtensionIfUnsuitable(extensions.vertex_input_dynamic_state,
                                VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);

    // VK_KHR_pipeline_executable_properties
    if (Settings::values.renderer_shader_feedback.GetValue()) {
        extensions.pipeline_executable_properties =
            features.pipeline_executable_properties.pipelineExecutableInfo;
        RemoveExtensionIfUnsuitable(extensions.pipeline_executable_properties,
                                    VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME);
    } else {
        extensions.pipeline_executable_properties = false;
        loaded_extensions.erase(VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME);
    }

    // VK_KHR_workgroup_memory_explicit_layout
    extensions.workgroup_memory_explicit_layout =
        features.features.shaderInt16 &&
        features.workgroup_memory_explicit_layout.workgroupMemoryExplicitLayout &&
        features.workgroup_memory_explicit_layout.workgroupMemoryExplicitLayout8BitAccess &&
        features.workgroup_memory_explicit_layout.workgroupMemoryExplicitLayout16BitAccess &&
        features.workgroup_memory_explicit_layout.workgroupMemoryExplicitLayoutScalarBlockLayout;
    RemoveExtensionIfUnsuitable(extensions.workgroup_memory_explicit_layout,
                                VK_KHR_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_EXTENSION_NAME);
}

void Device::SetupFamilies(VkSurfaceKHR surface) {
    const std::vector queue_family_properties = physical.GetQueueFamilyProperties();
    std::optional<u32> graphics;
    std::optional<u32> present;
    for (u32 index = 0; index < static_cast<u32>(queue_family_properties.size()); ++index) {
        if (graphics && (present || !surface)) {
            break;
        }
        const VkQueueFamilyProperties& queue_family = queue_family_properties[index];
        if (queue_family.queueCount == 0) {
            continue;
        }
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics = index;
        }
        if (surface && physical.GetSurfaceSupportKHR(index, surface)) {
            present = index;
        }
    }
    if (!graphics) {
        LOG_ERROR(Render_Vulkan, "Device lacks a graphics queue");
        throw vk::Exception(VK_ERROR_FEATURE_NOT_PRESENT);
    }
    if (surface && !present) {
        LOG_ERROR(Render_Vulkan, "Device lacks a present queue");
        throw vk::Exception(VK_ERROR_FEATURE_NOT_PRESENT);
    }
    if (graphics) {
        graphics_family = *graphics;
    }
    if (present) {
        present_family = *present;
    }
}

u64 Device::GetDeviceMemoryUsage() const {
    VkPhysicalDeviceMemoryBudgetPropertiesEXT budget;
    budget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
    budget.pNext = nullptr;
    physical.GetMemoryProperties(&budget);
    u64 result{};
    for (const size_t heap : valid_heap_memory) {
        result += budget.heapUsage[heap];
    }
    return result;
}

void Device::CollectPhysicalMemoryInfo() {
    VkPhysicalDeviceMemoryBudgetPropertiesEXT budget{};
    budget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
    const auto mem_info =
        physical.GetMemoryProperties(extensions.memory_budget ? &budget : nullptr);
    const auto& mem_properties = mem_info.memoryProperties;
    const size_t num_properties = mem_properties.memoryHeapCount;
    device_access_memory = 0;
    u64 device_initial_usage = 0;
    u64 local_memory = 0;
    for (size_t element = 0; element < num_properties; ++element) {
        const bool is_heap_local =
            (mem_properties.memoryHeaps[element].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0;
        if (!is_integrated && !is_heap_local) {
            continue;
        }
        valid_heap_memory.push_back(element);
        if (is_heap_local) {
            local_memory += mem_properties.memoryHeaps[element].size;
        }
        if (extensions.memory_budget) {
            device_initial_usage += budget.heapUsage[element];
            device_access_memory += budget.heapBudget[element];
            continue;
        }
        device_access_memory += mem_properties.memoryHeaps[element].size;
    }
    if (!is_integrated) {
        return;
    }
    const s64 available_memory = static_cast<s64>(device_access_memory - device_initial_usage);
    device_access_memory = static_cast<u64>(std::max<s64>(
        std::min<s64>(available_memory - 8_GiB, 4_GiB), static_cast<s64>(local_memory)));
}

void Device::CollectToolingInfo() {
    if (!extensions.tooling_info) {
        return;
    }
    auto tools{physical.GetPhysicalDeviceToolProperties()};
    for (const VkPhysicalDeviceToolProperties& tool : tools) {
        const std::string_view name = tool.name;
        LOG_INFO(Render_Vulkan, "Attached debugging tool: {}", name);
        has_renderdoc = has_renderdoc || name == "RenderDoc";
        has_nsight_graphics = has_nsight_graphics || name == "NVIDIA Nsight Graphics";
    }
}

std::vector<VkDeviceQueueCreateInfo> Device::GetDeviceQueueCreateInfos() const {
    static constexpr float QUEUE_PRIORITY = 1.0f;

    std::unordered_set<u32> unique_queue_families{graphics_family, present_family};
    std::vector<VkDeviceQueueCreateInfo> queue_cis;
    queue_cis.reserve(unique_queue_families.size());

    for (const u32 queue_family : unique_queue_families) {
        auto& ci = queue_cis.emplace_back(VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queue_family,
            .queueCount = 1,
            .pQueuePriorities = nullptr,
        });
        ci.pQueuePriorities = &QUEUE_PRIORITY;
    }

    return queue_cis;
}

} // namespace Vulkan

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/common_types.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

// Define all features which may be used by the implementation here.
// Vulkan version in the macro describes the minimum version required for feature availability.
// If the Vulkan version is lower than the required version, the named extension is required.
#define FOR_EACH_VK_FEATURE_1_1(FEATURE)                                                           \
    FEATURE(EXT, SubgroupSizeControl, SUBGROUP_SIZE_CONTROL, subgroup_size_control)                \
    FEATURE(KHR, 16BitStorage, 16BIT_STORAGE, bit16_storage)                                       \
    FEATURE(KHR, ShaderAtomicInt64, SHADER_ATOMIC_INT64, shader_atomic_int64)                      \
    FEATURE(KHR, ShaderDrawParameters, SHADER_DRAW_PARAMETERS, shader_draw_parameters)             \
    FEATURE(KHR, ShaderFloat16Int8, SHADER_FLOAT16_INT8, shader_float16_int8)                      \
    FEATURE(KHR, UniformBufferStandardLayout, UNIFORM_BUFFER_STANDARD_LAYOUT,                      \
            uniform_buffer_standard_layout)                                                        \
    FEATURE(KHR, VariablePointer, VARIABLE_POINTERS, variable_pointer)

#define FOR_EACH_VK_FEATURE_1_2(FEATURE)                                                           \
    FEATURE(EXT, HostQueryReset, HOST_QUERY_RESET, host_query_reset)                               \
    FEATURE(KHR, 8BitStorage, 8BIT_STORAGE, bit8_storage)                                          \
    FEATURE(KHR, TimelineSemaphore, TIMELINE_SEMAPHORE, timeline_semaphore)

#define FOR_EACH_VK_FEATURE_1_3(FEATURE)                                                           \
    FEATURE(EXT, ShaderDemoteToHelperInvocation, SHADER_DEMOTE_TO_HELPER_INVOCATION,               \
            shader_demote_to_helper_invocation)

// Define all features which may be used by the implementation and require an extension here.
#define FOR_EACH_VK_FEATURE_EXT(FEATURE)                                                           \
    FEATURE(EXT, CustomBorderColor, CUSTOM_BORDER_COLOR, custom_border_color)                      \
    FEATURE(EXT, DepthClipControl, DEPTH_CLIP_CONTROL, depth_clip_control)                         \
    FEATURE(EXT, ExtendedDynamicState, EXTENDED_DYNAMIC_STATE, extended_dynamic_state)             \
    FEATURE(EXT, ExtendedDynamicState2, EXTENDED_DYNAMIC_STATE_2, extended_dynamic_state2)         \
    FEATURE(EXT, ExtendedDynamicState3, EXTENDED_DYNAMIC_STATE_3, extended_dynamic_state3)         \
    FEATURE(EXT, IndexTypeUint8, INDEX_TYPE_UINT8, index_type_uint8)                               \
    FEATURE(EXT, LineRasterization, LINE_RASTERIZATION, line_rasterization)                        \
    FEATURE(EXT, PrimitiveTopologyListRestart, PRIMITIVE_TOPOLOGY_LIST_RESTART,                    \
            primitive_topology_list_restart)                                                       \
    FEATURE(EXT, ProvokingVertex, PROVOKING_VERTEX, provoking_vertex)                              \
    FEATURE(EXT, Robustness2, ROBUSTNESS_2, robustness2)                                           \
    FEATURE(EXT, TransformFeedback, TRANSFORM_FEEDBACK, transform_feedback)                        \
    FEATURE(EXT, VertexInputDynamicState, VERTEX_INPUT_DYNAMIC_STATE, vertex_input_dynamic_state)  \
    FEATURE(KHR, PipelineExecutableProperties, PIPELINE_EXECUTABLE_PROPERTIES,                     \
            pipeline_executable_properties)                                                        \
    FEATURE(KHR, WorkgroupMemoryExplicitLayout, WORKGROUP_MEMORY_EXPLICIT_LAYOUT,                  \
            workgroup_memory_explicit_layout)

// Define miscellaneous extensions which may be used by the implementation here.
#define FOR_EACH_VK_EXTENSION(EXTENSION)                                                           \
    EXTENSION(EXT, CONSERVATIVE_RASTERIZATION, conservative_rasterization)                         \
    EXTENSION(EXT, DEPTH_RANGE_UNRESTRICTED, depth_range_unrestricted)                             \
    EXTENSION(EXT, MEMORY_BUDGET, memory_budget)                                                   \
    EXTENSION(EXT, ROBUSTNESS_2, robustness_2)                                                     \
    EXTENSION(EXT, SAMPLER_FILTER_MINMAX, sampler_filter_minmax)                                   \
    EXTENSION(EXT, SHADER_STENCIL_EXPORT, shader_stencil_export)                                   \
    EXTENSION(EXT, SHADER_VIEWPORT_INDEX_LAYER, shader_viewport_index_layer)                       \
    EXTENSION(EXT, TOOLING_INFO, tooling_info)                                                     \
    EXTENSION(EXT, VERTEX_ATTRIBUTE_DIVISOR, vertex_attribute_divisor)                             \
    EXTENSION(KHR, DRAW_INDIRECT_COUNT, draw_indirect_count)                                       \
    EXTENSION(KHR, DRIVER_PROPERTIES, driver_properties)                                           \
    EXTENSION(KHR, EXTERNAL_MEMORY_FD, external_memory_fd)                                         \
    EXTENSION(KHR, PUSH_DESCRIPTOR, push_descriptor)                                               \
    EXTENSION(KHR, SAMPLER_MIRROR_CLAMP_TO_EDGE, sampler_mirror_clamp_to_edge)                     \
    EXTENSION(KHR, SHADER_FLOAT_CONTROLS, shader_float_controls)                                   \
    EXTENSION(KHR, SPIRV_1_4, spirv_1_4)                                                           \
    EXTENSION(KHR, SWAPCHAIN, swapchain)                                                           \
    EXTENSION(KHR, SWAPCHAIN_MUTABLE_FORMAT, swapchain_mutable_format)                             \
    EXTENSION(NV, DEVICE_DIAGNOSTICS_CONFIG, device_diagnostics_config)                            \
    EXTENSION(NV, GEOMETRY_SHADER_PASSTHROUGH, geometry_shader_passthrough)                        \
    EXTENSION(NV, VIEWPORT_ARRAY2, viewport_array2)                                                \
    EXTENSION(NV, VIEWPORT_SWIZZLE, viewport_swizzle)

#define FOR_EACH_VK_EXTENSION_WIN32(EXTENSION)                                                     \
    EXTENSION(KHR, EXTERNAL_MEMORY_WIN32, external_memory_win32)

// Define extensions which must be supported.
#define FOR_EACH_VK_MANDATORY_EXTENSION(EXTENSION_NAME)                                            \
    EXTENSION_NAME(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME)                                             \
    EXTENSION_NAME(VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME)                                 \
    EXTENSION_NAME(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME)                                        \
    EXTENSION_NAME(VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME)                             \
    EXTENSION_NAME(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME)

#define FOR_EACH_VK_MANDATORY_EXTENSION_GENERIC(EXTENSION_NAME)                                    \
    EXTENSION_NAME(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME)

#define FOR_EACH_VK_MANDATORY_EXTENSION_WIN32(EXTENSION_NAME)                                      \
    EXTENSION_NAME(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME)

// Define extensions where the absence of the extension may result in a degraded experience.
#define FOR_EACH_VK_RECOMMENDED_EXTENSION(EXTENSION_NAME)                                          \
    EXTENSION_NAME(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME)                               \
    EXTENSION_NAME(VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME)                                 \
    EXTENSION_NAME(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME)                                   \
    EXTENSION_NAME(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME)                                 \
    EXTENSION_NAME(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME)                                 \
    EXTENSION_NAME(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME)                                       \
    EXTENSION_NAME(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME)                               \
    EXTENSION_NAME(VK_NV_GEOMETRY_SHADER_PASSTHROUGH_EXTENSION_NAME)                               \
    EXTENSION_NAME(VK_NV_VIEWPORT_ARRAY2_EXTENSION_NAME)                                           \
    EXTENSION_NAME(VK_NV_VIEWPORT_SWIZZLE_EXTENSION_NAME)

// Define features which must be supported.
#define FOR_EACH_VK_MANDATORY_FEATURE(FEATURE_NAME)                                                \
    FEATURE_NAME(bit16_storage, storageBuffer16BitAccess)                                          \
    FEATURE_NAME(bit16_storage, uniformAndStorageBuffer16BitAccess)                                \
    FEATURE_NAME(bit8_storage, storageBuffer8BitAccess)                                            \
    FEATURE_NAME(bit8_storage, uniformAndStorageBuffer8BitAccess)                                  \
    FEATURE_NAME(features, depthBiasClamp)                                                         \
    FEATURE_NAME(features, depthClamp)                                                             \
    FEATURE_NAME(features, drawIndirectFirstInstance)                                              \
    FEATURE_NAME(features, dualSrcBlend)                                                           \
    FEATURE_NAME(features, fillModeNonSolid)                                                       \
    FEATURE_NAME(features, fragmentStoresAndAtomics)                                               \
    FEATURE_NAME(features, geometryShader)                                                         \
    FEATURE_NAME(features, imageCubeArray)                                                         \
    FEATURE_NAME(features, independentBlend)                                                       \
    FEATURE_NAME(features, largePoints)                                                            \
    FEATURE_NAME(features, logicOp)                                                                \
    FEATURE_NAME(features, multiDrawIndirect)                                                      \
    FEATURE_NAME(features, multiViewport)                                                          \
    FEATURE_NAME(features, occlusionQueryPrecise)                                                  \
    FEATURE_NAME(features, robustBufferAccess)                                                     \
    FEATURE_NAME(features, samplerAnisotropy)                                                      \
    FEATURE_NAME(features, sampleRateShading)                                                      \
    FEATURE_NAME(features, shaderClipDistance)                                                     \
    FEATURE_NAME(features, shaderCullDistance)                                                     \
    FEATURE_NAME(features, shaderImageGatherExtended)                                              \
    FEATURE_NAME(features, shaderStorageImageWriteWithoutFormat)                                   \
    FEATURE_NAME(features, tessellationShader)                                                     \
    FEATURE_NAME(features, vertexPipelineStoresAndAtomics)                                         \
    FEATURE_NAME(features, wideLines)                                                              \
    FEATURE_NAME(host_query_reset, hostQueryReset)                                                 \
    FEATURE_NAME(robustness2, nullDescriptor)                                                      \
    FEATURE_NAME(robustness2, robustBufferAccess2)                                                 \
    FEATURE_NAME(robustness2, robustImageAccess2)                                                  \
    FEATURE_NAME(shader_demote_to_helper_invocation, shaderDemoteToHelperInvocation)               \
    FEATURE_NAME(shader_draw_parameters, shaderDrawParameters)                                     \
    FEATURE_NAME(timeline_semaphore, timelineSemaphore)                                            \
    FEATURE_NAME(variable_pointer, variablePointers)                                               \
    FEATURE_NAME(variable_pointer, variablePointersStorageBuffer)

// Define features where the absence of the feature may result in a degraded experience.
#define FOR_EACH_VK_RECOMMENDED_FEATURE(FEATURE_NAME)                                              \
    FEATURE_NAME(custom_border_color, customBorderColors)                                          \
    FEATURE_NAME(extended_dynamic_state, extendedDynamicState)                                     \
    FEATURE_NAME(index_type_uint8, indexTypeUint8)                                                 \
    FEATURE_NAME(primitive_topology_list_restart, primitiveTopologyListRestart)                    \
    FEATURE_NAME(provoking_vertex, provokingVertexLast)                                            \
    FEATURE_NAME(shader_float16_int8, shaderFloat16)                                               \
    FEATURE_NAME(shader_float16_int8, shaderInt8)                                                  \
    FEATURE_NAME(transform_feedback, transformFeedback)                                            \
    FEATURE_NAME(uniform_buffer_standard_layout, uniformBufferStandardLayout)                      \
    FEATURE_NAME(vertex_input_dynamic_state, vertexInputDynamicState)

namespace Vulkan {

class NsightAftermathTracker;

/// Format usage descriptor.
enum class FormatType { Linear, Optimal, Buffer };

/// Subgroup size of the guest emulated hardware (Nvidia has 32 threads per subgroup).
const u32 GuestWarpSize = 32;

/// Handles data specific to a physical device.
class Device {
public:
    explicit Device(VkInstance instance, vk::PhysicalDevice physical, VkSurfaceKHR surface,
                    const vk::InstanceDispatch& dld);
    ~Device();

    /**
     * Returns a format supported by the device for the passed requirements.
     * @param wanted_format The ideal format to be returned. It may not be the returned format.
     * @param wanted_usage The usage that must be fulfilled even if the format is not supported.
     * @param format_type Format type usage.
     * @returns A format supported by the device.
     */
    VkFormat GetSupportedFormat(VkFormat wanted_format, VkFormatFeatureFlags wanted_usage,
                                FormatType format_type) const;

    /// Returns true if a format is supported.
    bool IsFormatSupported(VkFormat wanted_format, VkFormatFeatureFlags wanted_usage,
                           FormatType format_type) const;

    /// Reports a device loss.
    void ReportLoss() const;

    /// Reports a shader to Nsight Aftermath.
    void SaveShader(std::span<const u32> spirv) const;

    /// Returns the name of the VkDriverId reported from Vulkan.
    std::string GetDriverName() const;

    /// Returns the dispatch loader with direct function pointers of the device.
    const vk::DeviceDispatch& GetDispatchLoader() const {
        return dld;
    }

    /// Returns the logical device.
    const vk::Device& GetLogical() const {
        return logical;
    }

    /// Returns the physical device.
    vk::PhysicalDevice GetPhysical() const {
        return physical;
    }

    /// Returns the main graphics queue.
    vk::Queue GetGraphicsQueue() const {
        return graphics_queue;
    }

    /// Returns the main present queue.
    vk::Queue GetPresentQueue() const {
        return present_queue;
    }

    /// Returns main graphics queue family index.
    u32 GetGraphicsFamily() const {
        return graphics_family;
    }

    /// Returns main present queue family index.
    u32 GetPresentFamily() const {
        return present_family;
    }

    /// Returns the current Vulkan API version provided in Vulkan-formatted version numbers.
    u32 ApiVersion() const {
        return properties.properties.apiVersion;
    }

    /// Returns the current driver version provided in Vulkan-formatted version numbers.
    u32 GetDriverVersion() const {
        return properties.properties.driverVersion;
    }

    /// Returns the device name.
    std::string_view GetModelName() const {
        return properties.properties.deviceName;
    }

    /// Returns the driver ID.
    VkDriverIdKHR GetDriverID() const {
        return properties.driver.driverID;
    }

    bool ShouldBoostClocks() const;

    /// Returns uniform buffer alignment requirement.
    VkDeviceSize GetUniformBufferAlignment() const {
        return properties.properties.limits.minUniformBufferOffsetAlignment;
    }

    /// Returns storage alignment requirement.
    VkDeviceSize GetStorageBufferAlignment() const {
        return properties.properties.limits.minStorageBufferOffsetAlignment;
    }

    /// Returns the maximum range for storage buffers.
    VkDeviceSize GetMaxStorageBufferRange() const {
        return properties.properties.limits.maxStorageBufferRange;
    }

    /// Returns the maximum size for push constants.
    VkDeviceSize GetMaxPushConstantsSize() const {
        return properties.properties.limits.maxPushConstantsSize;
    }

    /// Returns the maximum size for shared memory.
    u32 GetMaxComputeSharedMemorySize() const {
        return properties.properties.limits.maxComputeSharedMemorySize;
    }

    /// Returns float control properties of the device.
    const VkPhysicalDeviceFloatControlsPropertiesKHR& FloatControlProperties() const {
        return properties.float_controls;
    }

    /// Returns true if ASTC is natively supported.
    bool IsOptimalAstcSupported() const {
        return features.features.textureCompressionASTC_LDR;
    }

    /// Returns true if the device supports float16 natively.
    bool IsFloat16Supported() const {
        return features.shader_float16_int8.shaderFloat16;
    }

    /// Returns true if the device supports int8 natively.
    bool IsInt8Supported() const {
        return features.shader_float16_int8.shaderInt8;
    }

    /// Returns true if the device warp size can potentially be bigger than guest's warp size.
    bool IsWarpSizePotentiallyBiggerThanGuest() const {
        return is_warp_potentially_bigger;
    }

    /// Returns true if the device can be forced to use the guest warp size.
    bool IsGuestWarpSizeSupported(VkShaderStageFlagBits stage) const {
        return properties.subgroup_size_control.requiredSubgroupSizeStages & stage;
    }

    /// Returns the maximum number of push descriptors.
    u32 MaxPushDescriptors() const {
        return properties.push_descriptor.maxPushDescriptors;
    }

    /// Returns true if formatless image load is supported.
    bool IsFormatlessImageLoadSupported() const {
        return features.features.shaderStorageImageReadWithoutFormat;
    }

    /// Returns true if shader int64 is supported.
    bool IsShaderInt64Supported() const {
        return features.features.shaderInt64;
    }

    /// Returns true if shader int16 is supported.
    bool IsShaderInt16Supported() const {
        return features.features.shaderInt16;
    }

    // Returns true if depth bounds is supported.
    bool IsDepthBoundsSupported() const {
        return features.features.depthBounds;
    }

    /// Returns true when blitting from and to depth stencil images is supported.
    bool IsBlitDepthStencilSupported() const {
        return is_blit_depth_stencil_supported;
    }

    /// Returns true if the device supports VK_NV_viewport_swizzle.
    bool IsNvViewportSwizzleSupported() const {
        return extensions.viewport_swizzle;
    }

    /// Returns true if the device supports VK_NV_viewport_array2.
    bool IsNvViewportArray2Supported() const {
        return extensions.viewport_array2;
    }

    /// Returns true if the device supports VK_NV_geometry_shader_passthrough.
    bool IsNvGeometryShaderPassthroughSupported() const {
        return extensions.geometry_shader_passthrough;
    }

    /// Returns true if the device supports VK_KHR_uniform_buffer_standard_layout.
    bool IsKhrUniformBufferStandardLayoutSupported() const {
        return extensions.uniform_buffer_standard_layout;
    }

    /// Returns true if the device supports VK_KHR_push_descriptor.
    bool IsKhrPushDescriptorSupported() const {
        return extensions.push_descriptor;
    }

    /// Returns true if VK_KHR_pipeline_executable_properties is enabled.
    bool IsKhrPipelineExecutablePropertiesEnabled() const {
        return extensions.pipeline_executable_properties;
    }

    /// Returns true if VK_KHR_swapchain_mutable_format is enabled.
    bool IsKhrSwapchainMutableFormatEnabled() const {
        return extensions.swapchain_mutable_format;
    }

    /// Returns true if the device supports VK_KHR_workgroup_memory_explicit_layout.
    bool IsKhrWorkgroupMemoryExplicitLayoutSupported() const {
        return extensions.workgroup_memory_explicit_layout;
    }

    /// Returns true if the device supports VK_EXT_primitive_topology_list_restart.
    bool IsTopologyListPrimitiveRestartSupported() const {
        return features.primitive_topology_list_restart.primitiveTopologyListRestart;
    }

    /// Returns true if the device supports VK_EXT_primitive_topology_list_restart.
    bool IsPatchListPrimitiveRestartSupported() const {
        return features.primitive_topology_list_restart.primitiveTopologyPatchListRestart;
    }

    /// Returns true if the device supports VK_EXT_index_type_uint8.
    bool IsExtIndexTypeUint8Supported() const {
        return extensions.index_type_uint8;
    }

    /// Returns true if the device supports VK_EXT_sampler_filter_minmax.
    bool IsExtSamplerFilterMinmaxSupported() const {
        return extensions.sampler_filter_minmax;
    }

    /// Returns true if the device supports VK_EXT_depth_range_unrestricted.
    bool IsExtDepthRangeUnrestrictedSupported() const {
        return extensions.depth_range_unrestricted;
    }

    /// Returns true if the device supports VK_EXT_depth_clip_control.
    bool IsExtDepthClipControlSupported() const {
        return extensions.depth_clip_control;
    }

    /// Returns true if the device supports VK_EXT_shader_viewport_index_layer.
    bool IsExtShaderViewportIndexLayerSupported() const {
        return extensions.shader_viewport_index_layer;
    }

    /// Returns true if the device supports VK_EXT_subgroup_size_control.
    bool IsExtSubgroupSizeControlSupported() const {
        return extensions.subgroup_size_control;
    }

    /// Returns true if the device supports VK_EXT_transform_feedback.
    bool IsExtTransformFeedbackSupported() const {
        return extensions.transform_feedback;
    }

    /// Returns true if the device supports VK_EXT_custom_border_color.
    bool IsExtCustomBorderColorSupported() const {
        return extensions.custom_border_color;
    }

    /// Returns true if the device supports VK_EXT_extended_dynamic_state.
    bool IsExtExtendedDynamicStateSupported() const {
        return extensions.extended_dynamic_state;
    }

    /// Returns true if the device supports VK_EXT_extended_dynamic_state2.
    bool IsExtExtendedDynamicState2Supported() const {
        return extensions.extended_dynamic_state2;
    }

    bool IsExtExtendedDynamicState2ExtrasSupported() const {
        return features.extended_dynamic_state2.extendedDynamicState2LogicOp;
    }

    /// Returns true if the device supports VK_EXT_extended_dynamic_state3.
    bool IsExtExtendedDynamicState3Supported() const {
        return extensions.extended_dynamic_state3;
    }

    /// Returns true if the device supports VK_EXT_extended_dynamic_state3.
    bool IsExtExtendedDynamicState3BlendingSupported() const {
        return dynamic_state3_blending;
    }

    /// Returns true if the device supports VK_EXT_extended_dynamic_state3.
    bool IsExtExtendedDynamicState3EnablesSupported() const {
        return dynamic_state3_enables;
    }

    /// Returns true if the device supports VK_EXT_line_rasterization.
    bool IsExtLineRasterizationSupported() const {
        return extensions.line_rasterization;
    }

    /// Returns true if the device supports VK_EXT_vertex_input_dynamic_state.
    bool IsExtVertexInputDynamicStateSupported() const {
        return extensions.vertex_input_dynamic_state;
    }

    /// Returns true if the device supports VK_EXT_shader_stencil_export.
    bool IsExtShaderStencilExportSupported() const {
        return extensions.shader_stencil_export;
    }

    /// Returns true if the device supports VK_EXT_conservative_rasterization.
    bool IsExtConservativeRasterizationSupported() const {
        return extensions.conservative_rasterization;
    }

    /// Returns true if the device supports VK_EXT_provoking_vertex.
    bool IsExtProvokingVertexSupported() const {
        return extensions.provoking_vertex;
    }

    /// Returns true if the device supports VK_KHR_shader_atomic_int64.
    bool IsExtShaderAtomicInt64Supported() const {
        return extensions.shader_atomic_int64;
    }

    /// Returns the minimum supported version of SPIR-V.
    u32 SupportedSpirvVersion() const {
        if (instance_version >= VK_API_VERSION_1_3) {
            return 0x00010600U;
        }
        if (extensions.spirv_1_4) {
            return 0x00010400U;
        }
        return 0x00010000U;
    }

    /// Returns true when a known debugging tool is attached.
    bool HasDebuggingToolAttached() const {
        return has_renderdoc || has_nsight_graphics;
    }

    /// Returns true when the device does not properly support cube compatibility.
    bool HasBrokenCubeImageCompability() const {
        return has_broken_cube_compatibility;
    }

    /// Returns the vendor name reported from Vulkan.
    std::string_view GetVendorName() const {
        return properties.driver.driverName;
    }

    /// Returns the list of available extensions.
    const std::set<std::string, std::less<>>& GetAvailableExtensions() const {
        return supported_extensions;
    }

    u64 GetDeviceLocalMemory() const {
        return device_access_memory;
    }

    bool CanReportMemoryUsage() const {
        return extensions.memory_budget;
    }

    u64 GetDeviceMemoryUsage() const;

    u32 GetSetsPerPool() const {
        return sets_per_pool;
    }

    bool SupportsD24DepthBuffer() const {
        return supports_d24_depth;
    }

    bool CantBlitMSAA() const {
        return cant_blit_msaa;
    }

    bool MustEmulateBGR565() const {
        return must_emulate_bgr565;
    }

    bool HasNullDescriptor() const {
        return features.robustness2.nullDescriptor;
    }

    bool NeedsGatherSubpixelOffset() const {
        return need_gather_subpixel_offset;
    }

    u32 GetMaxVertexInputAttributes() const {
        return properties.properties.limits.maxVertexInputAttributes;
    }

    u32 GetMaxVertexInputBindings() const {
        return properties.properties.limits.maxVertexInputBindings;
    }

private:
    /// Checks if the physical device is suitable and configures the object state
    /// with all necessary info about its properties.
    bool GetSuitability(bool requires_swapchain);

    // Remove extensions which have incomplete feature support.
    void RemoveUnsuitableExtensions();
    void RemoveExtensionIfUnsuitable(bool is_suitable, const std::string& extension_name);

    /// Sets up queue families.
    void SetupFamilies(VkSurfaceKHR surface);

    /// Collects information about attached tools.
    void CollectToolingInfo();

    /// Collects information about the device's local memory.
    void CollectPhysicalMemoryInfo();

    /// Returns a list of queue initialization descriptors.
    std::vector<VkDeviceQueueCreateInfo> GetDeviceQueueCreateInfos() const;

    /// Returns true if ASTC textures are natively supported.
    bool ComputeIsOptimalAstcSupported() const;

    /// Returns true if the device natively supports blitting depth stencil images.
    bool TestDepthStencilBlits() const;

private:
    VkInstance instance;         ///< Vulkan instance.
    vk::DeviceDispatch dld;      ///< Device function pointers.
    vk::PhysicalDevice physical; ///< Physical device.
    vk::Device logical;          ///< Logical device.
    vk::Queue graphics_queue;    ///< Main graphics queue.
    vk::Queue present_queue;     ///< Main present queue.
    u32 instance_version{};      ///< Vulkan instance version.
    u32 graphics_family{};       ///< Main graphics queue family index.
    u32 present_family{};        ///< Main present queue family index.

    struct Extensions {
#define EXTENSION(prefix, macro_name, var_name) bool var_name{};
#define FEATURE(prefix, struct_name, macro_name, var_name) bool var_name{};

        FOR_EACH_VK_FEATURE_1_1(FEATURE);
        FOR_EACH_VK_FEATURE_1_2(FEATURE);
        FOR_EACH_VK_FEATURE_1_3(FEATURE);
        FOR_EACH_VK_FEATURE_EXT(FEATURE);
        FOR_EACH_VK_EXTENSION(EXTENSION);
        FOR_EACH_VK_EXTENSION_WIN32(EXTENSION);

#undef EXTENSION
#undef FEATURE
    };

    struct Features {
#define FEATURE_CORE(prefix, struct_name, macro_name, var_name)                                    \
    VkPhysicalDevice##struct_name##Features var_name{};
#define FEATURE_EXT(prefix, struct_name, macro_name, var_name)                                     \
    VkPhysicalDevice##struct_name##Features##prefix var_name{};

        FOR_EACH_VK_FEATURE_1_1(FEATURE_CORE);
        FOR_EACH_VK_FEATURE_1_2(FEATURE_CORE);
        FOR_EACH_VK_FEATURE_1_3(FEATURE_CORE);
        FOR_EACH_VK_FEATURE_EXT(FEATURE_EXT);

#undef FEATURE_CORE
#undef FEATURE_EXT

        VkPhysicalDeviceFeatures features{};
    };

    struct Properties {
        VkPhysicalDeviceDriverProperties driver{};
        VkPhysicalDeviceFloatControlsProperties float_controls{};
        VkPhysicalDevicePushDescriptorPropertiesKHR push_descriptor{};
        VkPhysicalDeviceSubgroupSizeControlProperties subgroup_size_control{};
        VkPhysicalDeviceTransformFeedbackPropertiesEXT transform_feedback{};

        VkPhysicalDeviceProperties properties{};
    };

    Extensions extensions{};
    Features features{};
    Properties properties{};

    VkPhysicalDeviceFeatures2 features2{};
    VkPhysicalDeviceProperties2 properties2{};

    // Misc features
    bool is_optimal_astc_supported{};       ///< Support for all guest ASTC formats.
    bool is_blit_depth_stencil_supported{}; ///< Support for blitting from and to depth stencil.
    bool is_warp_potentially_bigger{};      ///< Host warp size can be bigger than guest.
    bool is_integrated{};                   ///< Is GPU an iGPU.
    bool is_virtual{};                      ///< Is GPU a virtual GPU.
    bool is_non_gpu{};                      ///< Is SoftwareRasterizer, FPGA, non-GPU device.
    bool has_broken_cube_compatibility{};   ///< Has broken cube compatibility bit
    bool has_renderdoc{};                   ///< Has RenderDoc attached
    bool has_nsight_graphics{};             ///< Has Nsight Graphics attached
    bool supports_d24_depth{};              ///< Supports D24 depth buffers.
    bool cant_blit_msaa{};                  ///< Does not support MSAA<->MSAA blitting.
    bool must_emulate_bgr565{};             ///< Emulates BGR565 by swizzling RGB565 format.
    bool dynamic_state3_blending{};         ///< Has all blending features of dynamic_state3.
    bool dynamic_state3_enables{};          ///< Has all enables features of dynamic_state3.
    bool need_gather_subpixel_offset{};     ///< Needs offset at ImageGather for correct rounding.
    u64 device_access_memory{};             ///< Total size of device local memory in bytes.
    u32 sets_per_pool{};                    ///< Sets per Description Pool

    // Telemetry parameters
    std::set<std::string, std::less<>> supported_extensions; ///< Reported Vulkan extensions.
    std::set<std::string, std::less<>> loaded_extensions;    ///< Loaded Vulkan extensions.
    std::vector<size_t> valid_heap_memory;                   ///< Heaps used.

    /// Format properties dictionary.
    std::unordered_map<VkFormat, VkFormatProperties> format_properties;

    /// Nsight Aftermath GPU crash tracker
    std::unique_ptr<NsightAftermathTracker> nsight_aftermath_tracker;
};

} // namespace Vulkan

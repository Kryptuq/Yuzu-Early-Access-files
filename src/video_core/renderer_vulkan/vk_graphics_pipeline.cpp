// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

#include "common/common_types.h"
#include "common/microprofile.h"
#include "video_core/renderer_vulkan/fixed_pipeline_state.h"
#include "video_core/renderer_vulkan/maxwell_to_vk.h"
#include "video_core/renderer_vulkan/vk_descriptor_pool.h"
#include "video_core/renderer_vulkan/vk_graphics_pipeline.h"
#include "video_core/renderer_vulkan/vk_pipeline_cache.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_update_descriptor.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

MICROPROFILE_DECLARE(Vulkan_PipelineCache);

namespace {

template <class StencilFace>
VkStencilOpState GetStencilFaceState(const StencilFace& face) {
    return {
        .failOp = MaxwellToVK::StencilOp(face.ActionStencilFail()),
        .passOp = MaxwellToVK::StencilOp(face.ActionDepthPass()),
        .depthFailOp = MaxwellToVK::StencilOp(face.ActionDepthFail()),
        .compareOp = MaxwellToVK::ComparisonOp(face.TestFunc()),
        .compareMask = 0,
        .writeMask = 0,
        .reference = 0,
    };
}

bool SupportsPrimitiveRestart(VkPrimitiveTopology topology) {
    static constexpr std::array unsupported_topologies = {
        VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
        VK_PRIMITIVE_TOPOLOGY_PATCH_LIST};
    return std::find(std::begin(unsupported_topologies), std::end(unsupported_topologies),
                     topology) == std::end(unsupported_topologies);
}

VkViewportSwizzleNV UnpackViewportSwizzle(u16 swizzle) {
    union Swizzle {
        u32 raw;
        BitField<0, 3, Maxwell::ViewportSwizzle> x;
        BitField<4, 3, Maxwell::ViewportSwizzle> y;
        BitField<8, 3, Maxwell::ViewportSwizzle> z;
        BitField<12, 3, Maxwell::ViewportSwizzle> w;
    };
    const Swizzle unpacked{swizzle};

    return {
        .x = MaxwellToVK::ViewportSwizzle(unpacked.x),
        .y = MaxwellToVK::ViewportSwizzle(unpacked.y),
        .z = MaxwellToVK::ViewportSwizzle(unpacked.z),
        .w = MaxwellToVK::ViewportSwizzle(unpacked.w),
    };
}

VkSampleCountFlagBits ConvertMsaaMode(Tegra::Texture::MsaaMode msaa_mode) {
    switch (msaa_mode) {
    case Tegra::Texture::MsaaMode::Msaa1x1:
        return VK_SAMPLE_COUNT_1_BIT;
    case Tegra::Texture::MsaaMode::Msaa2x1:
    case Tegra::Texture::MsaaMode::Msaa2x1_D3D:
        return VK_SAMPLE_COUNT_2_BIT;
    case Tegra::Texture::MsaaMode::Msaa2x2:
    case Tegra::Texture::MsaaMode::Msaa2x2_VC4:
    case Tegra::Texture::MsaaMode::Msaa2x2_VC12:
        return VK_SAMPLE_COUNT_4_BIT;
    case Tegra::Texture::MsaaMode::Msaa4x2:
    case Tegra::Texture::MsaaMode::Msaa4x2_D3D:
    case Tegra::Texture::MsaaMode::Msaa4x2_VC8:
    case Tegra::Texture::MsaaMode::Msaa4x2_VC24:
        return VK_SAMPLE_COUNT_8_BIT;
    case Tegra::Texture::MsaaMode::Msaa4x4:
        return VK_SAMPLE_COUNT_16_BIT;
    default:
        UNREACHABLE_MSG("Invalid msaa_mode={}", static_cast<int>(msaa_mode));
        return VK_SAMPLE_COUNT_1_BIT;
    }
}

} // Anonymous namespace

VKGraphicsPipeline::VKGraphicsPipeline(const Device& device_, VKScheduler& scheduler_,
                                       VKDescriptorPool& descriptor_pool_,
                                       VKUpdateDescriptorQueue& update_descriptor_queue_,
                                       const GraphicsPipelineCacheKey& key,
                                       vk::Span<VkDescriptorSetLayoutBinding> bindings,
                                       const SPIRVProgram& program, u32 num_color_buffers)
    : device{device_}, scheduler{scheduler_}, cache_key{key}, hash{cache_key.Hash()},
      descriptor_set_layout{CreateDescriptorSetLayout(bindings)},
      descriptor_allocator{descriptor_pool_, *descriptor_set_layout},
      update_descriptor_queue{update_descriptor_queue_}, layout{CreatePipelineLayout()},
      descriptor_template{CreateDescriptorUpdateTemplate(program)},
      modules(CreateShaderModules(program)),
      pipeline(CreatePipeline(program, cache_key.renderpass, num_color_buffers)) {}

VKGraphicsPipeline::~VKGraphicsPipeline() = default;

VkDescriptorSet VKGraphicsPipeline::CommitDescriptorSet() {
    if (!descriptor_template) {
        return {};
    }
    const VkDescriptorSet set = descriptor_allocator.Commit();
    update_descriptor_queue.Send(*descriptor_template, set);
    return set;
}

vk::DescriptorSetLayout VKGraphicsPipeline::CreateDescriptorSetLayout(
    vk::Span<VkDescriptorSetLayoutBinding> bindings) const {
    const VkDescriptorSetLayoutCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = bindings.size(),
        .pBindings = bindings.data(),
    };
    return device.GetLogical().CreateDescriptorSetLayout(ci);
}

vk::PipelineLayout VKGraphicsPipeline::CreatePipelineLayout() const {
    const VkPipelineLayoutCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = descriptor_set_layout.address(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };
    return device.GetLogical().CreatePipelineLayout(ci);
}

vk::DescriptorUpdateTemplateKHR VKGraphicsPipeline::CreateDescriptorUpdateTemplate(
    const SPIRVProgram& program) const {
    std::vector<VkDescriptorUpdateTemplateEntry> template_entries;
    u32 binding = 0;
    u32 offset = 0;
    for (const auto& stage : program) {
        if (stage) {
            FillDescriptorUpdateTemplateEntries(stage->entries, binding, offset, template_entries);
        }
    }
    if (template_entries.empty()) {
        // If the shader doesn't use descriptor sets, skip template creation.
        return {};
    }

    const VkDescriptorUpdateTemplateCreateInfoKHR ci{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .descriptorUpdateEntryCount = static_cast<u32>(template_entries.size()),
        .pDescriptorUpdateEntries = template_entries.data(),
        .templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET_KHR,
        .descriptorSetLayout = *descriptor_set_layout,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .pipelineLayout = *layout,
        .set = DESCRIPTOR_SET,
    };
    return device.GetLogical().CreateDescriptorUpdateTemplateKHR(ci);
}

std::vector<vk::ShaderModule> VKGraphicsPipeline::CreateShaderModules(
    const SPIRVProgram& program) const {
    VkShaderModuleCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = 0,
        .pCode = nullptr,
    };

    std::vector<vk::ShaderModule> shader_modules;
    shader_modules.reserve(Maxwell::MaxShaderStage);
    for (std::size_t i = 0; i < Maxwell::MaxShaderStage; ++i) {
        const auto& stage = program[i];
        if (!stage) {
            continue;
        }

        device.SaveShader(stage->code);

        ci.codeSize = stage->code.size() * sizeof(u32);
        ci.pCode = stage->code.data();
        shader_modules.push_back(device.GetLogical().CreateShaderModule(ci));
    }
    return shader_modules;
}

vk::Pipeline VKGraphicsPipeline::CreatePipeline(const SPIRVProgram& program,
                                                VkRenderPass renderpass,
                                                u32 num_color_buffers) const {
    const auto& state = cache_key.fixed_state;
    const auto& viewport_swizzles = state.viewport_swizzles;

    FixedPipelineState::DynamicState dynamic;
    if (device.IsExtExtendedDynamicStateSupported()) {
        // Insert dummy values, as long as they are valid they don't matter as extended dynamic
        // state is ignored
        dynamic.raw1 = 0;
        dynamic.raw2 = 0;
        dynamic.vertex_strides.fill(0);
    } else {
        dynamic = state.dynamic_state;
    }

    std::vector<VkVertexInputBindingDescription> vertex_bindings;
    std::vector<VkVertexInputBindingDivisorDescriptionEXT> vertex_binding_divisors;
    for (std::size_t index = 0; index < Maxwell::NumVertexArrays; ++index) {
        const bool instanced = state.binding_divisors[index] != 0;
        const auto rate = instanced ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
        vertex_bindings.push_back({
            .binding = static_cast<u32>(index),
            .stride = dynamic.vertex_strides[index],
            .inputRate = rate,
        });
        if (instanced) {
            vertex_binding_divisors.push_back({
                .binding = static_cast<u32>(index),
                .divisor = state.binding_divisors[index],
            });
        }
    }

    std::vector<VkVertexInputAttributeDescription> vertex_attributes;
    const auto& input_attributes = program[0]->entries.attributes;
    for (std::size_t index = 0; index < state.attributes.size(); ++index) {
        const auto& attribute = state.attributes[index];
        if (!attribute.enabled) {
            continue;
        }
        if (!input_attributes.contains(static_cast<u32>(index))) {
            // Skip attributes not used by the vertex shaders.
            continue;
        }
        vertex_attributes.push_back({
            .location = static_cast<u32>(index),
            .binding = attribute.buffer,
            .format = MaxwellToVK::VertexFormat(attribute.Type(), attribute.Size()),
            .offset = attribute.offset,
        });
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = static_cast<u32>(vertex_bindings.size()),
        .pVertexBindingDescriptions = vertex_bindings.data(),
        .vertexAttributeDescriptionCount = static_cast<u32>(vertex_attributes.size()),
        .pVertexAttributeDescriptions = vertex_attributes.data(),
    };

    const VkPipelineVertexInputDivisorStateCreateInfoEXT input_divisor_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT,
        .pNext = nullptr,
        .vertexBindingDivisorCount = static_cast<u32>(vertex_binding_divisors.size()),
        .pVertexBindingDivisors = vertex_binding_divisors.data(),
    };
    if (!vertex_binding_divisors.empty()) {
        vertex_input_ci.pNext = &input_divisor_ci;
    }

    const auto input_assembly_topology = MaxwellToVK::PrimitiveTopology(device, state.topology);
    const VkPipelineInputAssemblyStateCreateInfo input_assembly_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = MaxwellToVK::PrimitiveTopology(device, state.topology),
        .primitiveRestartEnable = state.primitive_restart_enable != 0 &&
                                  SupportsPrimitiveRestart(input_assembly_topology),
    };

    const VkPipelineTessellationStateCreateInfo tessellation_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .patchControlPoints = state.patch_control_points_minus_one.Value() + 1,
    };

    VkPipelineViewportStateCreateInfo viewport_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = Maxwell::NumViewports,
        .pViewports = nullptr,
        .scissorCount = Maxwell::NumViewports,
        .pScissors = nullptr,
    };

    std::array<VkViewportSwizzleNV, Maxwell::NumViewports> swizzles;
    std::ranges::transform(viewport_swizzles, swizzles.begin(), UnpackViewportSwizzle);
    VkPipelineViewportSwizzleStateCreateInfoNV swizzle_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SWIZZLE_STATE_CREATE_INFO_NV,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = Maxwell::NumViewports,
        .pViewportSwizzles = swizzles.data(),
    };
    if (device.IsNvViewportSwizzleSupported()) {
        viewport_ci.pNext = &swizzle_ci;
    }

    const VkPipelineRasterizationStateCreateInfo rasterization_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable =
            static_cast<VkBool32>(state.depth_clamp_disabled == 0 ? VK_TRUE : VK_FALSE),
        .rasterizerDiscardEnable =
            static_cast<VkBool32>(state.rasterize_enable == 0 ? VK_TRUE : VK_FALSE),
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = static_cast<VkCullModeFlags>(
            dynamic.cull_enable ? MaxwellToVK::CullFace(dynamic.CullFace()) : VK_CULL_MODE_NONE),
        .frontFace = MaxwellToVK::FrontFace(dynamic.FrontFace()),
        .depthBiasEnable = state.depth_bias_enable,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    const VkPipelineMultisampleStateCreateInfo multisample_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = ConvertMsaaMode(state.msaa_mode),
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    const VkPipelineDepthStencilStateCreateInfo depth_stencil_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = dynamic.depth_test_enable,
        .depthWriteEnable = dynamic.depth_write_enable,
        .depthCompareOp = dynamic.depth_test_enable
                              ? MaxwellToVK::ComparisonOp(dynamic.DepthTestFunc())
                              : VK_COMPARE_OP_ALWAYS,
        .depthBoundsTestEnable = dynamic.depth_bounds_enable,
        .stencilTestEnable = dynamic.stencil_enable,
        .front = GetStencilFaceState(dynamic.front),
        .back = GetStencilFaceState(dynamic.back),
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 0.0f,
    };

    std::array<VkPipelineColorBlendAttachmentState, Maxwell::NumRenderTargets> cb_attachments;
    for (std::size_t index = 0; index < num_color_buffers; ++index) {
        static constexpr std::array COMPONENT_TABLE{
            VK_COLOR_COMPONENT_R_BIT,
            VK_COLOR_COMPONENT_G_BIT,
            VK_COLOR_COMPONENT_B_BIT,
            VK_COLOR_COMPONENT_A_BIT,
        };
        const auto& blend = state.attachments[index];

        VkColorComponentFlags color_components = 0;
        for (std::size_t i = 0; i < COMPONENT_TABLE.size(); ++i) {
            if (blend.Mask()[i]) {
                color_components |= COMPONENT_TABLE[i];
            }
        }

        cb_attachments[index] = {
            .blendEnable = blend.enable != 0,
            .srcColorBlendFactor = MaxwellToVK::BlendFactor(blend.SourceRGBFactor()),
            .dstColorBlendFactor = MaxwellToVK::BlendFactor(blend.DestRGBFactor()),
            .colorBlendOp = MaxwellToVK::BlendEquation(blend.EquationRGB()),
            .srcAlphaBlendFactor = MaxwellToVK::BlendFactor(blend.SourceAlphaFactor()),
            .dstAlphaBlendFactor = MaxwellToVK::BlendFactor(blend.DestAlphaFactor()),
            .alphaBlendOp = MaxwellToVK::BlendEquation(blend.EquationAlpha()),
            .colorWriteMask = color_components,
        };
    }

    const VkPipelineColorBlendStateCreateInfo color_blend_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = num_color_buffers,
        .pAttachments = cb_attachments.data(),
        .blendConstants = {},
    };

    std::vector dynamic_states{
        VK_DYNAMIC_STATE_VIEWPORT,           VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_BIAS,         VK_DYNAMIC_STATE_BLEND_CONSTANTS,
        VK_DYNAMIC_STATE_DEPTH_BOUNDS,       VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
        VK_DYNAMIC_STATE_STENCIL_WRITE_MASK, VK_DYNAMIC_STATE_STENCIL_REFERENCE,
    };
    if (device.IsExtExtendedDynamicStateSupported()) {
        static constexpr std::array extended{
            VK_DYNAMIC_STATE_CULL_MODE_EXT,
            VK_DYNAMIC_STATE_FRONT_FACE_EXT,
            VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT,
            VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT,
            VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT,
            VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT,
            VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE_EXT,
            VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE_EXT,
            VK_DYNAMIC_STATE_STENCIL_OP_EXT,
        };
        dynamic_states.insert(dynamic_states.end(), extended.begin(), extended.end());
    }

    const VkPipelineDynamicStateCreateInfo dynamic_state_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = static_cast<u32>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };

    const VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT subgroup_size_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT,
        .pNext = nullptr,
        .requiredSubgroupSize = GuestWarpSize,
    };

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    std::size_t module_index = 0;
    for (std::size_t stage = 0; stage < Maxwell::MaxShaderStage; ++stage) {
        if (!program[stage]) {
            continue;
        }

        VkPipelineShaderStageCreateInfo& stage_ci = shader_stages.emplace_back();
        stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage_ci.pNext = nullptr;
        stage_ci.flags = 0;
        stage_ci.stage = MaxwellToVK::ShaderStage(static_cast<Tegra::Engines::ShaderType>(stage));
        stage_ci.module = *modules[module_index++];
        stage_ci.pName = "main";
        stage_ci.pSpecializationInfo = nullptr;

        if (program[stage]->entries.uses_warps && device.IsGuestWarpSizeSupported(stage_ci.stage)) {
            stage_ci.pNext = &subgroup_size_ci;
        }
    }
    return device.GetLogical().CreateGraphicsPipeline(VkGraphicsPipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = static_cast<u32>(shader_stages.size()),
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_ci,
        .pInputAssemblyState = &input_assembly_ci,
        .pTessellationState = &tessellation_ci,
        .pViewportState = &viewport_ci,
        .pRasterizationState = &rasterization_ci,
        .pMultisampleState = &multisample_ci,
        .pDepthStencilState = &depth_stencil_ci,
        .pColorBlendState = &color_blend_ci,
        .pDynamicState = &dynamic_state_ci,
        .layout = *layout,
        .renderPass = renderpass,
        .subpass = 0,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = 0,
    });
}

} // namespace Vulkan

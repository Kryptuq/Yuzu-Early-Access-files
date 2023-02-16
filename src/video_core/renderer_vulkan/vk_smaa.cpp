// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <list>

#include "common/assert.h"
#include "common/polyfill_ranges.h"

#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"
#include "video_core/renderer_vulkan/vk_smaa.h"
#include "video_core/smaa_area_tex.h"
#include "video_core/smaa_search_tex.h"
#include "video_core/vulkan_common/vulkan_device.h"

#include "video_core/host_shaders/smaa_blending_weight_calculation_frag_spv.h"
#include "video_core/host_shaders/smaa_blending_weight_calculation_vert_spv.h"
#include "video_core/host_shaders/smaa_edge_detection_frag_spv.h"
#include "video_core/host_shaders/smaa_edge_detection_vert_spv.h"
#include "video_core/host_shaders/smaa_neighborhood_blending_frag_spv.h"
#include "video_core/host_shaders/smaa_neighborhood_blending_vert_spv.h"

namespace Vulkan {
namespace {

#define ARRAY_TO_SPAN(a) std::span(a, (sizeof(a) / sizeof(a[0])))

std::pair<vk::Image, MemoryCommit> CreateWrappedImage(const Device& device,
                                                      MemoryAllocator& allocator,
                                                      VkExtent2D dimensions, VkFormat format) {
    const VkImageCreateInfo image_ci{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {.width = dimensions.width, .height = dimensions.height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                 VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    auto image = device.GetLogical().CreateImage(image_ci);
    auto commit = allocator.Commit(image, Vulkan::MemoryUsage::DeviceLocal);

    return std::make_pair(std::move(image), std::move(commit));
}

void TransitionImageLayout(vk::CommandBuffer& cmdbuf, VkImage image, VkImageLayout target_layout,
                           VkImageLayout source_layout = VK_IMAGE_LAYOUT_GENERAL) {
    constexpr VkFlags flags{VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT};
    const VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = flags,
        .dstAccessMask = flags,
        .oldLayout = source_layout,
        .newLayout = target_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           0, barrier);
}

void UploadImage(const Device& device, MemoryAllocator& allocator, Scheduler& scheduler,
                 vk::Image& image, VkExtent2D dimensions, VkFormat format,
                 std::span<const u8> initial_contents = {}) {
    auto upload_buffer = device.GetLogical().CreateBuffer(VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = initial_contents.size_bytes(),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    });
    auto upload_commit = allocator.Commit(upload_buffer, MemoryUsage::Upload);
    std::ranges::copy(initial_contents, upload_commit.Map().begin());

    const std::array<VkBufferImageCopy, 1> regions{{{
        .bufferOffset = 0,
        .bufferRowLength = dimensions.width,
        .bufferImageHeight = dimensions.height,
        .imageSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                          .mipLevel = 0,
                          .baseArrayLayer = 0,
                          .layerCount = 1},
        .imageOffset{},
        .imageExtent{.width = dimensions.width, .height = dimensions.height, .depth = 1},
    }}};

    scheduler.RequestOutsideRenderPassOperationContext();
    scheduler.Record([&](vk::CommandBuffer cmdbuf) {
        TransitionImageLayout(cmdbuf, *image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_UNDEFINED);
        cmdbuf.CopyBufferToImage(*upload_buffer, *image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 regions);
        TransitionImageLayout(cmdbuf, *image, VK_IMAGE_LAYOUT_GENERAL,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    });
    scheduler.Finish();

    // This should go out of scope before the commit
    auto upload_buffer2 = std::move(upload_buffer);
}

vk::ImageView CreateWrappedImageView(const Device& device, vk::Image& image, VkFormat format) {
    return device.GetLogical().CreateImageView(VkImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = *image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components{},
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                          .baseMipLevel = 0,
                          .levelCount = 1,
                          .baseArrayLayer = 0,
                          .layerCount = 1},
    });
}

vk::RenderPass CreateWrappedRenderPass(const Device& device, VkFormat format) {
    const VkAttachmentDescription attachment{
        .flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT,
        .format = format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_GENERAL,
        .finalLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    constexpr VkAttachmentReference color_attachment_ref{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };

    const VkSubpassDescription subpass_description{
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = nullptr,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr,
    };

    constexpr VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0,
    };

    return device.GetLogical().CreateRenderPass(VkRenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass_description,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    });
}

vk::Framebuffer CreateWrappedFramebuffer(const Device& device, vk::RenderPass& render_pass,
                                         vk::ImageView& dest_image, VkExtent2D extent) {
    return device.GetLogical().CreateFramebuffer(VkFramebufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderPass = *render_pass,
        .attachmentCount = 1,
        .pAttachments = dest_image.address(),
        .width = extent.width,
        .height = extent.height,
        .layers = 1,
    });
}

vk::Sampler CreateWrappedSampler(const Device& device) {
    return device.GetLogical().CreateSampler(VkSamplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_NEVER,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    });
}

vk::ShaderModule CreateWrappedShaderModule(const Device& device, std::span<const u32> code) {
    return device.GetLogical().CreateShaderModule(VkShaderModuleCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = code.size_bytes(),
        .pCode = code.data(),
    });
}

vk::DescriptorPool CreateWrappedDescriptorPool(const Device& device, u32 max_descriptors,
                                               u32 max_sets) {
    const VkDescriptorPoolSize pool_size{
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<u32>(max_descriptors),
    };

    return device.GetLogical().CreateDescriptorPool(VkDescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = max_sets,
        .poolSizeCount = 1,
        .pPoolSizes = &pool_size,
    });
}

vk::DescriptorSetLayout CreateWrappedDescriptorSetLayout(const Device& device,
                                                         u32 max_sampler_bindings) {
    std::vector<VkDescriptorSetLayoutBinding> bindings(max_sampler_bindings);
    for (u32 i = 0; i < max_sampler_bindings; i++) {
        bindings[i] = {
            .binding = i,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };
    }

    return device.GetLogical().CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = static_cast<u32>(bindings.size()),
        .pBindings = bindings.data(),
    });
}

vk::DescriptorSets CreateWrappedDescriptorSets(vk::DescriptorPool& pool,
                                               vk::Span<VkDescriptorSetLayout> layouts) {
    return pool.Allocate(VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = *pool,
        .descriptorSetCount = layouts.size(),
        .pSetLayouts = layouts.data(),
    });
}

vk::PipelineLayout CreateWrappedPipelineLayout(const Device& device,
                                               vk::DescriptorSetLayout& layout) {
    return device.GetLogical().CreatePipelineLayout(VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = layout.address(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    });
}

vk::Pipeline CreateWrappedPipeline(const Device& device, vk::RenderPass& renderpass,
                                   vk::PipelineLayout& layout,
                                   std::tuple<vk::ShaderModule&, vk::ShaderModule&> shaders) {
    const std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{{
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = *std::get<0>(shaders),
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = *std::get<1>(shaders),
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
    }};

    constexpr VkPipelineVertexInputStateCreateInfo vertex_input_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr,
    };

    constexpr VkPipelineInputAssemblyStateCreateInfo input_assembly_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .primitiveRestartEnable = VK_FALSE,
    };

    constexpr VkPipelineViewportStateCreateInfo viewport_state_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };

    constexpr VkPipelineRasterizationStateCreateInfo rasterization_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    constexpr VkPipelineMultisampleStateCreateInfo multisampling_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    constexpr VkPipelineColorBlendAttachmentState color_blend_attachment{
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    const VkPipelineColorBlendStateCreateInfo color_blend_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
    };

    constexpr std::array dynamic_states{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    const VkPipelineDynamicStateCreateInfo dynamic_state_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = static_cast<u32>(dynamic_states.size()),
        .pDynamicStates = dynamic_states.data(),
    };

    return device.GetLogical().CreateGraphicsPipeline(VkGraphicsPipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = static_cast<u32>(shader_stages.size()),
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_ci,
        .pInputAssemblyState = &input_assembly_ci,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_state_ci,
        .pRasterizationState = &rasterization_ci,
        .pMultisampleState = &multisampling_ci,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &color_blend_ci,
        .pDynamicState = &dynamic_state_ci,
        .layout = *layout,
        .renderPass = *renderpass,
        .subpass = 0,
        .basePipelineHandle = 0,
        .basePipelineIndex = 0,
    });
}

VkWriteDescriptorSet CreateWriteDescriptorSet(std::vector<VkDescriptorImageInfo>& images,
                                              VkSampler sampler, VkImageView view,
                                              VkDescriptorSet set, u32 binding) {
    ASSERT(images.capacity() > images.size());
    auto& image_info = images.emplace_back(VkDescriptorImageInfo{
        .sampler = sampler,
        .imageView = view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    });

    return VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = set,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &image_info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    };
}

void ClearColorImage(vk::CommandBuffer& cmdbuf, VkImage image) {
    static constexpr std::array<VkImageSubresourceRange, 1> subresources{{{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    }}};
    TransitionImageLayout(cmdbuf, image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_UNDEFINED);
    cmdbuf.ClearColorImage(image, VK_IMAGE_LAYOUT_GENERAL, {}, subresources);
}

void BeginRenderPass(vk::CommandBuffer& cmdbuf, vk::RenderPass& render_pass,
                     VkFramebuffer framebuffer, VkExtent2D extent) {
    const VkRenderPassBeginInfo renderpass_bi{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = *render_pass,
        .framebuffer = framebuffer,
        .renderArea{
            .offset{},
            .extent = extent,
        },
        .clearValueCount = 0,
        .pClearValues = nullptr,
    };
    cmdbuf.BeginRenderPass(renderpass_bi, VK_SUBPASS_CONTENTS_INLINE);

    const VkViewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    const VkRect2D scissor{
        .offset = {0, 0},
        .extent = extent,
    };
    cmdbuf.SetViewport(0, viewport);
    cmdbuf.SetScissor(0, scissor);
}

} // Anonymous namespace

SMAA::SMAA(const Device& device, MemoryAllocator& allocator, size_t image_count, VkExtent2D extent)
    : m_device(device), m_allocator(allocator), m_extent(extent),
      m_image_count(static_cast<u32>(image_count)) {
    CreateImages();
    CreateRenderPasses();
    CreateSampler();
    CreateShaders();
    CreateDescriptorPool();
    CreateDescriptorSetLayouts();
    CreateDescriptorSets();
    CreatePipelineLayouts();
    CreatePipelines();
}

void SMAA::CreateImages() {
    static constexpr VkExtent2D area_extent{AREATEX_WIDTH, AREATEX_HEIGHT};
    static constexpr VkExtent2D search_extent{SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT};

    std::tie(m_static_images[Area], m_static_buffer_commits[Area]) =
        CreateWrappedImage(m_device, m_allocator, area_extent, VK_FORMAT_R8G8_UNORM);
    std::tie(m_static_images[Search], m_static_buffer_commits[Search]) =
        CreateWrappedImage(m_device, m_allocator, search_extent, VK_FORMAT_R8_UNORM);

    m_static_image_views[Area] =
        CreateWrappedImageView(m_device, m_static_images[Area], VK_FORMAT_R8G8_UNORM);
    m_static_image_views[Search] =
        CreateWrappedImageView(m_device, m_static_images[Search], VK_FORMAT_R8_UNORM);

    for (u32 i = 0; i < m_image_count; i++) {
        Images& images = m_dynamic_images.emplace_back();

        std::tie(images.images[Blend], images.buffer_commits[Blend]) =
            CreateWrappedImage(m_device, m_allocator, m_extent, VK_FORMAT_R16G16B16A16_SFLOAT);
        std::tie(images.images[Edges], images.buffer_commits[Edges]) =
            CreateWrappedImage(m_device, m_allocator, m_extent, VK_FORMAT_R16G16_SFLOAT);
        std::tie(images.images[Output], images.buffer_commits[Output]) =
            CreateWrappedImage(m_device, m_allocator, m_extent, VK_FORMAT_R16G16B16A16_SFLOAT);

        images.image_views[Blend] =
            CreateWrappedImageView(m_device, images.images[Blend], VK_FORMAT_R16G16B16A16_SFLOAT);
        images.image_views[Edges] =
            CreateWrappedImageView(m_device, images.images[Edges], VK_FORMAT_R16G16_SFLOAT);
        images.image_views[Output] =
            CreateWrappedImageView(m_device, images.images[Output], VK_FORMAT_R16G16B16A16_SFLOAT);
    }
}

void SMAA::CreateRenderPasses() {
    m_renderpasses[EdgeDetection] = CreateWrappedRenderPass(m_device, VK_FORMAT_R16G16_SFLOAT);
    m_renderpasses[BlendingWeightCalculation] =
        CreateWrappedRenderPass(m_device, VK_FORMAT_R16G16B16A16_SFLOAT);
    m_renderpasses[NeighborhoodBlending] =
        CreateWrappedRenderPass(m_device, VK_FORMAT_R16G16B16A16_SFLOAT);

    for (auto& images : m_dynamic_images) {
        images.framebuffers[EdgeDetection] = CreateWrappedFramebuffer(
            m_device, m_renderpasses[EdgeDetection], images.image_views[Edges], m_extent);

        images.framebuffers[BlendingWeightCalculation] =
            CreateWrappedFramebuffer(m_device, m_renderpasses[BlendingWeightCalculation],
                                     images.image_views[Blend], m_extent);

        images.framebuffers[NeighborhoodBlending] = CreateWrappedFramebuffer(
            m_device, m_renderpasses[NeighborhoodBlending], images.image_views[Output], m_extent);
    }
}

void SMAA::CreateSampler() {
    m_sampler = CreateWrappedSampler(m_device);
}

void SMAA::CreateShaders() {
    // These match the order of the SMAAStage enum
    static constexpr std::array vert_shader_sources{
        ARRAY_TO_SPAN(SMAA_EDGE_DETECTION_VERT_SPV),
        ARRAY_TO_SPAN(SMAA_BLENDING_WEIGHT_CALCULATION_VERT_SPV),
        ARRAY_TO_SPAN(SMAA_NEIGHBORHOOD_BLENDING_VERT_SPV),
    };
    static constexpr std::array frag_shader_sources{
        ARRAY_TO_SPAN(SMAA_EDGE_DETECTION_FRAG_SPV),
        ARRAY_TO_SPAN(SMAA_BLENDING_WEIGHT_CALCULATION_FRAG_SPV),
        ARRAY_TO_SPAN(SMAA_NEIGHBORHOOD_BLENDING_FRAG_SPV),
    };

    for (size_t i = 0; i < MaxSMAAStage; i++) {
        m_vertex_shaders[i] = CreateWrappedShaderModule(m_device, vert_shader_sources[i]);
        m_fragment_shaders[i] = CreateWrappedShaderModule(m_device, frag_shader_sources[i]);
    }
}

void SMAA::CreateDescriptorPool() {
    // Edge detection: 1 descriptor
    // Blending weight calculation: 3 descriptors
    // Neighborhood blending: 2 descriptors

    // 6 descriptors, 3 descriptor sets per image
    m_descriptor_pool = CreateWrappedDescriptorPool(m_device, 6 * m_image_count, 3 * m_image_count);
}

void SMAA::CreateDescriptorSetLayouts() {
    m_descriptor_set_layouts[EdgeDetection] = CreateWrappedDescriptorSetLayout(m_device, 1);
    m_descriptor_set_layouts[BlendingWeightCalculation] =
        CreateWrappedDescriptorSetLayout(m_device, 3);
    m_descriptor_set_layouts[NeighborhoodBlending] = CreateWrappedDescriptorSetLayout(m_device, 2);
}

void SMAA::CreateDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(m_descriptor_set_layouts.size());
    std::ranges::transform(m_descriptor_set_layouts, layouts.begin(),
                           [](auto& layout) { return *layout; });

    for (auto& images : m_dynamic_images) {
        images.descriptor_sets = CreateWrappedDescriptorSets(m_descriptor_pool, layouts);
    }
}

void SMAA::CreatePipelineLayouts() {
    for (size_t i = 0; i < MaxSMAAStage; i++) {
        m_pipeline_layouts[i] = CreateWrappedPipelineLayout(m_device, m_descriptor_set_layouts[i]);
    }
}

void SMAA::CreatePipelines() {
    for (size_t i = 0; i < MaxSMAAStage; i++) {
        m_pipelines[i] =
            CreateWrappedPipeline(m_device, m_renderpasses[i], m_pipeline_layouts[i],
                                  std::tie(m_vertex_shaders[i], m_fragment_shaders[i]));
    }
}

void SMAA::UpdateDescriptorSets(VkImageView image_view, size_t image_index) {
    Images& images = m_dynamic_images[image_index];
    std::vector<VkDescriptorImageInfo> image_infos;
    std::vector<VkWriteDescriptorSet> updates;
    image_infos.reserve(6);

    updates.push_back(CreateWriteDescriptorSet(image_infos, *m_sampler, image_view,
                                               images.descriptor_sets[EdgeDetection], 0));

    updates.push_back(CreateWriteDescriptorSet(image_infos, *m_sampler, *images.image_views[Edges],
                                               images.descriptor_sets[BlendingWeightCalculation],
                                               0));
    updates.push_back(CreateWriteDescriptorSet(image_infos, *m_sampler, *m_static_image_views[Area],
                                               images.descriptor_sets[BlendingWeightCalculation],
                                               1));
    updates.push_back(
        CreateWriteDescriptorSet(image_infos, *m_sampler, *m_static_image_views[Search],
                                 images.descriptor_sets[BlendingWeightCalculation], 2));

    updates.push_back(CreateWriteDescriptorSet(image_infos, *m_sampler, image_view,
                                               images.descriptor_sets[NeighborhoodBlending], 0));
    updates.push_back(CreateWriteDescriptorSet(image_infos, *m_sampler, *images.image_views[Blend],
                                               images.descriptor_sets[NeighborhoodBlending], 1));

    m_device.GetLogical().UpdateDescriptorSets(updates, {});
}

void SMAA::UploadImages(Scheduler& scheduler) {
    if (m_images_ready) {
        return;
    }

    static constexpr VkExtent2D area_extent{AREATEX_WIDTH, AREATEX_HEIGHT};
    static constexpr VkExtent2D search_extent{SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT};

    UploadImage(m_device, m_allocator, scheduler, m_static_images[Area], area_extent,
                VK_FORMAT_R8G8_UNORM, ARRAY_TO_SPAN(areaTexBytes));
    UploadImage(m_device, m_allocator, scheduler, m_static_images[Search], search_extent,
                VK_FORMAT_R8_UNORM, ARRAY_TO_SPAN(searchTexBytes));

    scheduler.Record([&](vk::CommandBuffer& cmdbuf) {
        for (auto& images : m_dynamic_images) {
            for (size_t i = 0; i < MaxDynamicImage; i++) {
                ClearColorImage(cmdbuf, *images.images[i]);
            }
        }
    });
    scheduler.Finish();

    m_images_ready = true;
}

VkImageView SMAA::Draw(Scheduler& scheduler, size_t image_index, VkImage source_image,
                       VkImageView source_image_view) {
    Images& images = m_dynamic_images[image_index];

    VkImage output_image = *images.images[Output];
    VkImage edges_image = *images.images[Edges];
    VkImage blend_image = *images.images[Blend];

    VkDescriptorSet edge_detection_descriptor_set = images.descriptor_sets[EdgeDetection];
    VkDescriptorSet blending_weight_calculation_descriptor_set =
        images.descriptor_sets[BlendingWeightCalculation];
    VkDescriptorSet neighborhood_blending_descriptor_set =
        images.descriptor_sets[NeighborhoodBlending];

    VkFramebuffer edge_detection_framebuffer = *images.framebuffers[EdgeDetection];
    VkFramebuffer blending_weight_calculation_framebuffer =
        *images.framebuffers[BlendingWeightCalculation];
    VkFramebuffer neighborhood_blending_framebuffer = *images.framebuffers[NeighborhoodBlending];

    UploadImages(scheduler);
    UpdateDescriptorSets(source_image_view, image_index);

    scheduler.RequestOutsideRenderPassOperationContext();
    scheduler.Record([=, this](vk::CommandBuffer& cmdbuf) {
        TransitionImageLayout(cmdbuf, source_image, VK_IMAGE_LAYOUT_GENERAL);
        TransitionImageLayout(cmdbuf, edges_image, VK_IMAGE_LAYOUT_GENERAL);
        BeginRenderPass(cmdbuf, m_renderpasses[EdgeDetection], edge_detection_framebuffer,
                        m_extent);
        cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipelines[EdgeDetection]);
        cmdbuf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  *m_pipeline_layouts[EdgeDetection], 0,
                                  edge_detection_descriptor_set, {});
        cmdbuf.Draw(3, 1, 0, 0);
        cmdbuf.EndRenderPass();

        TransitionImageLayout(cmdbuf, edges_image, VK_IMAGE_LAYOUT_GENERAL);
        TransitionImageLayout(cmdbuf, blend_image, VK_IMAGE_LAYOUT_GENERAL);
        BeginRenderPass(cmdbuf, m_renderpasses[BlendingWeightCalculation],
                        blending_weight_calculation_framebuffer, m_extent);
        cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS,
                            *m_pipelines[BlendingWeightCalculation]);
        cmdbuf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  *m_pipeline_layouts[BlendingWeightCalculation], 0,
                                  blending_weight_calculation_descriptor_set, {});
        cmdbuf.Draw(3, 1, 0, 0);
        cmdbuf.EndRenderPass();

        TransitionImageLayout(cmdbuf, blend_image, VK_IMAGE_LAYOUT_GENERAL);
        TransitionImageLayout(cmdbuf, output_image, VK_IMAGE_LAYOUT_GENERAL);
        BeginRenderPass(cmdbuf, m_renderpasses[NeighborhoodBlending],
                        neighborhood_blending_framebuffer, m_extent);
        cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipelines[NeighborhoodBlending]);
        cmdbuf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  *m_pipeline_layouts[NeighborhoodBlending], 0,
                                  neighborhood_blending_descriptor_set, {});
        cmdbuf.Draw(3, 1, 0, 0);
        cmdbuf.EndRenderPass();
        TransitionImageLayout(cmdbuf, output_image, VK_IMAGE_LAYOUT_GENERAL);
    });

    return *images.image_views[Output];
}

} // namespace Vulkan

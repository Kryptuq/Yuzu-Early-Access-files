// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/literals.h"
#include "video_core/host_shaders/vulkan_turbo_mode_comp_spv.h"
#include "video_core/renderer_vulkan/renderer_vulkan.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"
#include "video_core/renderer_vulkan/vk_turbo_mode.h"
#include "video_core/vulkan_common/vulkan_device.h"

namespace Vulkan {

using namespace Common::Literals;

TurboMode::TurboMode(const vk::Instance& instance, const vk::InstanceDispatch& dld)
    : m_device{CreateDevice(instance, dld, VK_NULL_HANDLE)}, m_allocator{m_device, false} {
    m_thread = std::jthread([&](auto stop_token) { Run(stop_token); });
}

TurboMode::~TurboMode() = default;

void TurboMode::Run(std::stop_token stop_token) {
    auto& dld = m_device.GetLogical();

    // Allocate buffer. 2MiB should be sufficient.
    auto buffer = dld.CreateBuffer(VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = 2_MiB,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    });

    // Commit some device local memory for the buffer.
    auto commit = m_allocator.Commit(buffer, MemoryUsage::DeviceLocal);

    // Create the descriptor pool to contain our descriptor.
    constexpr VkDescriptorPoolSize pool_size{
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
    };

    auto descriptor_pool = dld.CreateDescriptorPool(VkDescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &pool_size,
    });

    // Create the descriptor set layout from the pool.
    constexpr VkDescriptorSetLayoutBinding layout_binding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = nullptr,
    };

    auto descriptor_set_layout = dld.CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &layout_binding,
    });

    // Actually create the descriptor set.
    auto descriptor_set = descriptor_pool.Allocate(VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = *descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = descriptor_set_layout.address(),
    });

    // Create the shader.
    auto shader = BuildShader(m_device, VULKAN_TURBO_MODE_COMP_SPV);

    // Create the pipeline layout.
    auto pipeline_layout = dld.CreatePipelineLayout(VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = descriptor_set_layout.address(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    });

    // Actually create the pipeline.
    const VkPipelineShaderStageCreateInfo shader_stage{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = *shader,
        .pName = "main",
        .pSpecializationInfo = nullptr,
    };

    auto pipeline = dld.CreateComputePipeline(VkComputePipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = shader_stage,
        .layout = *pipeline_layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
    });

    // Create a fence to wait on.
    auto fence = dld.CreateFence(VkFenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    });

    // Create a command pool to allocate a command buffer from.
    auto command_pool = dld.CreateCommandPool(VkCommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags =
            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_device.GetGraphicsFamily(),
    });

    // Create a single command buffer.
    auto cmdbufs = command_pool.Allocate(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    auto cmdbuf = vk::CommandBuffer{cmdbufs[0], m_device.GetDispatchLoader()};

    while (!stop_token.stop_requested()) {
        // Reset the fence.
        fence.Reset();

        // Update descriptor set.
        const VkDescriptorBufferInfo buffer_info{
            .buffer = *buffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE,
        };

        const VkWriteDescriptorSet buffer_write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptor_set[0],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &buffer_info,
            .pTexelBufferView = nullptr,
        };

        dld.UpdateDescriptorSets(std::array{buffer_write}, {});

        // Set up the command buffer.
        cmdbuf.Begin(VkCommandBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        });

        // Clear the buffer.
        cmdbuf.FillBuffer(*buffer, 0, VK_WHOLE_SIZE, 0);

        // Bind descriptor set.
        cmdbuf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline_layout, 0,
                                  descriptor_set, {});

        // Bind the pipeline.
        cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);

        // Dispatch.
        cmdbuf.Dispatch(64, 64, 1);

        // Finish.
        cmdbuf.End();

        const VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = cmdbuf.address(),
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
        };

        m_device.GetGraphicsQueue().Submit(std::array{submit_info}, *fence);

        // Wait for completion.
        fence.Wait();
    }
}

} // namespace Vulkan

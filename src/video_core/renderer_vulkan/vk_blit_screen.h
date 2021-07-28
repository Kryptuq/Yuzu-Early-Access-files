// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>

#include "video_core/vulkan_common/vulkan_memory_allocator.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Core {
class System;
}

namespace Core::Memory {
class Memory;
}

namespace Core::Frontend {
class EmuWindow;
}

namespace Tegra {
struct FramebufferConfig;
}

namespace VideoCore {
class RasterizerInterface;
}

namespace Vulkan {

struct ScreenInfo;

class Device;
class RasterizerVulkan;
class VKScheduler;
class VKSwapchain;

struct VKScreenInfo {
    VkImageView image_view{};
    u32 width{};
    u32 height{};
    bool is_srgb{};
};

class VKBlitScreen {
public:
    explicit VKBlitScreen(Core::Memory::Memory& cpu_memory,
                          Core::Frontend::EmuWindow& render_window, const Device& device,
                          MemoryAllocator& memory_manager, VKSwapchain& swapchain,
                          VKScheduler& scheduler, const VKScreenInfo& screen_info);
    ~VKBlitScreen();

    void Recreate();

    [[nodiscard]] VkSemaphore Draw(const Tegra::FramebufferConfig& framebuffer,
                                   const VkFramebuffer& host_framebuffer,
                                   const Layout::FramebufferLayout layout, VkExtent2D render_area,
                                   bool use_accelerated);

    [[nodiscard]] VkSemaphore DrawToSwapchain(const Tegra::FramebufferConfig& framebuffer,
                                              bool use_accelerated);

    [[nodiscard]] vk::Framebuffer CreateFramebuffer(const VkImageView& image_view,
                                                    VkExtent2D extent);

private:
    struct BufferData;

    void CreateStaticResources();
    void CreateShaders();
    void CreateSemaphores();
    void CreateDescriptorPool();
    void CreateRenderPass();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();
    void CreatePipelineLayout();
    void CreateGraphicsPipeline();
    void CreateSampler();

    void CreateDynamicResources();
    void CreateFramebuffers();

    void RefreshResources(const Tegra::FramebufferConfig& framebuffer);
    void ReleaseRawImages();
    void CreateStagingBuffer(const Tegra::FramebufferConfig& framebuffer);
    void CreateRawImages(const Tegra::FramebufferConfig& framebuffer);

    void UpdateDescriptorSet(std::size_t image_index, VkImageView image_view) const;
    void SetUniformData(BufferData& data, const Layout::FramebufferLayout layout) const;
    void SetVertexData(BufferData& data, const Tegra::FramebufferConfig& framebuffer,
                       const Layout::FramebufferLayout layout) const;

    u64 CalculateBufferSize(const Tegra::FramebufferConfig& framebuffer) const;
    u64 GetRawImageOffset(const Tegra::FramebufferConfig& framebuffer,
                          std::size_t image_index) const;

    Core::Memory::Memory& cpu_memory;
    Core::Frontend::EmuWindow& render_window;
    const Device& device;
    MemoryAllocator& memory_allocator;
    VKSwapchain& swapchain;
    VKScheduler& scheduler;
    const std::size_t image_count;
    const VKScreenInfo& screen_info;

    vk::ShaderModule vertex_shader;
    vk::ShaderModule fragment_shader;
    vk::DescriptorPool descriptor_pool;
    vk::DescriptorSetLayout descriptor_set_layout;
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;
    vk::RenderPass renderpass;
    std::vector<vk::Framebuffer> framebuffers;
    vk::DescriptorSets descriptor_sets;
    vk::Sampler sampler;

    vk::Buffer buffer;
    MemoryCommit buffer_commit;

    std::vector<u64> resource_ticks;

    std::vector<vk::Semaphore> semaphores;
    std::vector<vk::Image> raw_images;
    std::vector<vk::ImageView> raw_image_views;
    std::vector<MemoryCommit> raw_buffer_commits;
    u32 raw_width = 0;
    u32 raw_height = 0;
};

} // namespace Vulkan

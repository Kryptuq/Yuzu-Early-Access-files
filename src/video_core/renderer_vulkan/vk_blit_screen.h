// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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

namespace Service::android {
enum class PixelFormat : u32;
}

namespace Vulkan {

struct ScreenInfo;

class Device;
class FSR;
class RasterizerVulkan;
class Scheduler;
class Swapchain;

struct ScreenInfo {
    VkImageView image_view{};
    u32 width{};
    u32 height{};
    bool is_srgb{};
};

class BlitScreen {
public:
    explicit BlitScreen(Core::Memory::Memory& cpu_memory, Core::Frontend::EmuWindow& render_window,
                        const Device& device, MemoryAllocator& memory_manager, Swapchain& swapchain,
                        Scheduler& scheduler, const ScreenInfo& screen_info);
    ~BlitScreen();

    void Recreate();

    [[nodiscard]] VkSemaphore Draw(const Tegra::FramebufferConfig& framebuffer,
                                   const VkFramebuffer& host_framebuffer,
                                   const Layout::FramebufferLayout layout, VkExtent2D render_area,
                                   bool use_accelerated);

    [[nodiscard]] VkSemaphore DrawToSwapchain(const Tegra::FramebufferConfig& framebuffer,
                                              bool use_accelerated);

    [[nodiscard]] vk::Framebuffer CreateFramebuffer(const VkImageView& image_view,
                                                    VkExtent2D extent);

    [[nodiscard]] vk::Framebuffer CreateFramebuffer(const VkImageView& image_view,
                                                    VkExtent2D extent, vk::RenderPass& rd);

private:
    struct BufferData;

    void CreateStaticResources();
    void CreateShaders();
    void CreateSemaphores();
    void CreateDescriptorPool();
    void CreateRenderPass();
    vk::RenderPass CreateRenderPassImpl(VkFormat, bool is_present = true);
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

    void UpdateDescriptorSet(std::size_t image_index, VkImageView image_view, bool nn) const;
    void UpdateAADescriptorSet(std::size_t image_index, VkImageView image_view, bool nn) const;
    void SetUniformData(BufferData& data, const Layout::FramebufferLayout layout) const;
    void SetVertexData(BufferData& data, const Tegra::FramebufferConfig& framebuffer,
                       const Layout::FramebufferLayout layout) const;

    void CreateFSR();

    u64 CalculateBufferSize(const Tegra::FramebufferConfig& framebuffer) const;
    u64 GetRawImageOffset(const Tegra::FramebufferConfig& framebuffer,
                          std::size_t image_index) const;

    Core::Memory::Memory& cpu_memory;
    Core::Frontend::EmuWindow& render_window;
    const Device& device;
    MemoryAllocator& memory_allocator;
    Swapchain& swapchain;
    Scheduler& scheduler;
    std::size_t image_count;
    const ScreenInfo& screen_info;

    vk::ShaderModule vertex_shader;
    vk::ShaderModule fxaa_vertex_shader;
    vk::ShaderModule fxaa_fragment_shader;
    vk::ShaderModule bilinear_fragment_shader;
    vk::ShaderModule bicubic_fragment_shader;
    vk::ShaderModule gaussian_fragment_shader;
    vk::ShaderModule scaleforce_fragment_shader;
    vk::DescriptorPool descriptor_pool;
    vk::DescriptorSetLayout descriptor_set_layout;
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline nearest_neightbor_pipeline;
    vk::Pipeline bilinear_pipeline;
    vk::Pipeline bicubic_pipeline;
    vk::Pipeline gaussian_pipeline;
    vk::Pipeline scaleforce_pipeline;
    vk::RenderPass renderpass;
    std::vector<vk::Framebuffer> framebuffers;
    vk::DescriptorSets descriptor_sets;
    vk::Sampler nn_sampler;
    vk::Sampler sampler;

    vk::Buffer buffer;
    MemoryCommit buffer_commit;

    std::vector<u64> resource_ticks;

    std::vector<vk::Semaphore> semaphores;
    std::vector<vk::Image> raw_images;
    std::vector<vk::ImageView> raw_image_views;
    std::vector<MemoryCommit> raw_buffer_commits;

    vk::DescriptorPool aa_descriptor_pool;
    vk::DescriptorSetLayout aa_descriptor_set_layout;
    vk::PipelineLayout aa_pipeline_layout;
    vk::Pipeline aa_pipeline;
    vk::RenderPass aa_renderpass;
    vk::Framebuffer aa_framebuffer;
    vk::DescriptorSets aa_descriptor_sets;
    vk::Image aa_image;
    vk::ImageView aa_image_view;
    MemoryCommit aa_commit;

    u32 raw_width = 0;
    u32 raw_height = 0;
    Service::android::PixelFormat pixel_format{};

    std::unique_ptr<FSR> fsr;
};

} // namespace Vulkan

// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <vector>

#include "common/assert.h"
#include "common/common_types.h"
#include "common/math_util.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/frontend/emu_window.h"
#include "core/memory.h"
#include "video_core/gpu.h"
#include "video_core/host_shaders/fxaa_frag_spv.h"
#include "video_core/host_shaders/fxaa_vert_spv.h"
#include "video_core/host_shaders/present_bicubic_frag_spv.h"
#include "video_core/host_shaders/present_gaussian_frag_spv.h"
#include "video_core/host_shaders/vulkan_present_frag_spv.h"
#include "video_core/host_shaders/vulkan_present_scaleforce_fp16_frag_spv.h"
#include "video_core/host_shaders/vulkan_present_scaleforce_fp32_frag_spv.h"
#include "video_core/host_shaders/vulkan_present_vert_spv.h"
#include "video_core/renderer_vulkan/renderer_vulkan.h"
#include "video_core/renderer_vulkan/vk_blit_screen.h"
#include "video_core/renderer_vulkan/vk_fsr.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"
#include "video_core/renderer_vulkan/vk_swapchain.h"
#include "video_core/surface.h"
#include "video_core/textures/decoders.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_memory_allocator.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

namespace {

struct ScreenRectVertex {
    ScreenRectVertex() = default;
    explicit ScreenRectVertex(f32 x, f32 y, f32 u, f32 v) : position{{x, y}}, tex_coord{{u, v}} {}

    std::array<f32, 2> position;
    std::array<f32, 2> tex_coord;

    static VkVertexInputBindingDescription GetDescription() {
        return {
            .binding = 0,
            .stride = sizeof(ScreenRectVertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
    }

    static std::array<VkVertexInputAttributeDescription, 2> GetAttributes() {
        return {{
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(ScreenRectVertex, position),
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(ScreenRectVertex, tex_coord),
            },
        }};
    }
};

constexpr std::array<f32, 4 * 4> MakeOrthographicMatrix(f32 width, f32 height) {
    // clang-format off
    return { 2.f / width, 0.f,          0.f, 0.f,
             0.f,         2.f / height, 0.f, 0.f,
             0.f,         0.f,          1.f, 0.f,
            -1.f,        -1.f,          0.f, 1.f};
    // clang-format on
}

u32 GetBytesPerPixel(const Tegra::FramebufferConfig& framebuffer) {
    using namespace VideoCore::Surface;
    return BytesPerBlock(PixelFormatFromGPUPixelFormat(framebuffer.pixel_format));
}

std::size_t GetSizeInBytes(const Tegra::FramebufferConfig& framebuffer) {
    return static_cast<std::size_t>(framebuffer.stride) *
           static_cast<std::size_t>(framebuffer.height) * GetBytesPerPixel(framebuffer);
}

VkFormat GetFormat(const Tegra::FramebufferConfig& framebuffer) {
    switch (framebuffer.pixel_format) {
    case Service::android::PixelFormat::Rgba8888:
        return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
    case Service::android::PixelFormat::Rgb565:
        return VK_FORMAT_R5G6B5_UNORM_PACK16;
    case Service::android::PixelFormat::Bgra8888:
        return VK_FORMAT_B8G8R8A8_UNORM;
    default:
        UNIMPLEMENTED_MSG("Unknown framebuffer pixel format: {}",
                          static_cast<u32>(framebuffer.pixel_format));
        return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
    }
}

} // Anonymous namespace

struct BlitScreen::BufferData {
    struct {
        std::array<f32, 4 * 4> modelview_matrix;
    } uniform;

    std::array<ScreenRectVertex, 4> vertices;

    // Unaligned image data goes here
};

BlitScreen::BlitScreen(Core::Memory::Memory& cpu_memory_, Core::Frontend::EmuWindow& render_window_,
                       const Device& device_, MemoryAllocator& memory_allocator_,
                       Swapchain& swapchain_, Scheduler& scheduler_, const ScreenInfo& screen_info_)
    : cpu_memory{cpu_memory_}, render_window{render_window_}, device{device_},
      memory_allocator{memory_allocator_}, swapchain{swapchain_}, scheduler{scheduler_},
      image_count{swapchain.GetImageCount()}, screen_info{screen_info_} {
    resource_ticks.resize(image_count);

    CreateStaticResources();
    CreateDynamicResources();
}

BlitScreen::~BlitScreen() = default;

void BlitScreen::Recreate() {
    CreateDynamicResources();
}

VkSemaphore BlitScreen::Draw(const Tegra::FramebufferConfig& framebuffer,
                             const VkFramebuffer& host_framebuffer,
                             const Layout::FramebufferLayout layout, VkExtent2D render_area,
                             bool use_accelerated) {
    RefreshResources(framebuffer);

    // Finish any pending renderpass
    scheduler.RequestOutsideRenderPassOperationContext();

    const std::size_t image_index = swapchain.GetImageIndex();

    scheduler.Wait(resource_ticks[image_index]);
    resource_ticks[image_index] = scheduler.CurrentTick();

    VkImageView source_image_view =
        use_accelerated ? screen_info.image_view : *raw_image_views[image_index];

    BufferData data;
    SetUniformData(data, layout);
    SetVertexData(data, framebuffer, layout);

    const std::span<u8> mapped_span = buffer_commit.Map();
    std::memcpy(mapped_span.data(), &data, sizeof(data));

    if (!use_accelerated) {
        const u64 image_offset = GetRawImageOffset(framebuffer, image_index);

        const VAddr framebuffer_addr = framebuffer.address + framebuffer.offset;
        const u8* const host_ptr = cpu_memory.GetPointer(framebuffer_addr);

        // TODO(Rodrigo): Read this from HLE
        constexpr u32 block_height_log2 = 4;
        const u32 bytes_per_pixel = GetBytesPerPixel(framebuffer);
        const u64 linear_size{GetSizeInBytes(framebuffer)};
        const u64 tiled_size{Tegra::Texture::CalculateSize(true, bytes_per_pixel,
                                                           framebuffer.stride, framebuffer.height,
                                                           1, block_height_log2, 0)};
        Tegra::Texture::UnswizzleTexture(
            mapped_span.subspan(image_offset, linear_size), std::span(host_ptr, tiled_size),
            bytes_per_pixel, framebuffer.width, framebuffer.height, 1, block_height_log2, 0);

        const VkBufferImageCopy copy{
            .bufferOffset = image_offset,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            .imageOffset = {.x = 0, .y = 0, .z = 0},
            .imageExtent =
                {
                    .width = framebuffer.width,
                    .height = framebuffer.height,
                    .depth = 1,
                },
        };
        scheduler.Record([this, copy, image_index](vk::CommandBuffer cmdbuf) {
            const VkImage image = *raw_images[image_index];
            const VkImageMemoryBarrier base_barrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = 0,
                .dstAccessMask = 0,
                .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
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
            VkImageMemoryBarrier read_barrier = base_barrier;
            read_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            read_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            read_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkImageMemoryBarrier write_barrier = base_barrier;
            write_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            write_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                   read_barrier);
            cmdbuf.CopyBufferToImage(*buffer, image, VK_IMAGE_LAYOUT_GENERAL, copy);
            cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                   0, write_barrier);
        });
    }

    const auto anti_alias_pass = Settings::values.anti_aliasing.GetValue();
    if (use_accelerated && anti_alias_pass != Settings::AntiAliasing::None) {
        UpdateAADescriptorSet(image_index, source_image_view, false);
        const u32 up_scale = Settings::values.resolution_info.up_scale;
        const u32 down_shift = Settings::values.resolution_info.down_shift;
        VkExtent2D size{
            .width = (up_scale * framebuffer.width) >> down_shift,
            .height = (up_scale * framebuffer.height) >> down_shift,
        };
        scheduler.Record([this, image_index, size, anti_alias_pass](vk::CommandBuffer cmdbuf) {
            const VkImageMemoryBarrier base_barrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = 0,
                .dstAccessMask = 0,
                .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = {},
                .subresourceRange =
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
            };

            {
                VkImageMemoryBarrier fsr_write_barrier = base_barrier;
                fsr_write_barrier.image = *aa_image;
                fsr_write_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, fsr_write_barrier);
            }

            const f32 bg_red = Settings::values.bg_red.GetValue() / 255.0f;
            const f32 bg_green = Settings::values.bg_green.GetValue() / 255.0f;
            const f32 bg_blue = Settings::values.bg_blue.GetValue() / 255.0f;
            const VkClearValue clear_color{
                .color = {.float32 = {bg_red, bg_green, bg_blue, 1.0f}},
            };
            const VkRenderPassBeginInfo renderpass_bi{
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext = nullptr,
                .renderPass = *aa_renderpass,
                .framebuffer = *aa_framebuffer,
                .renderArea =
                    {
                        .offset = {0, 0},
                        .extent = size,
                    },
                .clearValueCount = 1,
                .pClearValues = &clear_color,
            };
            const VkViewport viewport{
                .x = 0.0f,
                .y = 0.0f,
                .width = static_cast<float>(size.width),
                .height = static_cast<float>(size.height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };
            const VkRect2D scissor{
                .offset = {0, 0},
                .extent = size,
            };
            cmdbuf.BeginRenderPass(renderpass_bi, VK_SUBPASS_CONTENTS_INLINE);
            switch (anti_alias_pass) {
            case Settings::AntiAliasing::Fxaa:
                cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *aa_pipeline);
                break;
            default:
                cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *aa_pipeline);
                break;
            }
            cmdbuf.SetViewport(0, viewport);
            cmdbuf.SetScissor(0, scissor);

            cmdbuf.BindVertexBuffer(0, *buffer, offsetof(BufferData, vertices));
            cmdbuf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, *aa_pipeline_layout, 0,
                                      aa_descriptor_sets[image_index], {});
            cmdbuf.Draw(4, 1, 0, 0);
            cmdbuf.EndRenderPass();

            {
                VkImageMemoryBarrier blit_read_barrier = base_barrier;
                blit_read_barrier.image = *aa_image;
                blit_read_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                blit_read_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, blit_read_barrier);
            }
        });
        source_image_view = *aa_image_view;
    }

    if (fsr) {
        auto crop_rect = framebuffer.crop_rect;
        if (crop_rect.GetWidth() == 0) {
            crop_rect.right = framebuffer.width;
        }
        if (crop_rect.GetHeight() == 0) {
            crop_rect.bottom = framebuffer.height;
        }
        crop_rect = crop_rect.Scale(Settings::values.resolution_info.up_factor);
        VkExtent2D fsr_input_size{
            .width = Settings::values.resolution_info.ScaleUp(framebuffer.width),
            .height = Settings::values.resolution_info.ScaleUp(framebuffer.height),
        };
        VkImageView fsr_image_view =
            fsr->Draw(scheduler, image_index, source_image_view, fsr_input_size, crop_rect);
        UpdateDescriptorSet(image_index, fsr_image_view, true);
    } else {
        const bool is_nn =
            Settings::values.scaling_filter.GetValue() == Settings::ScalingFilter::NearestNeighbor;
        UpdateDescriptorSet(image_index, source_image_view, is_nn);
    }

    scheduler.Record(
        [this, host_framebuffer, image_index, size = render_area](vk::CommandBuffer cmdbuf) {
            const f32 bg_red = Settings::values.bg_red.GetValue() / 255.0f;
            const f32 bg_green = Settings::values.bg_green.GetValue() / 255.0f;
            const f32 bg_blue = Settings::values.bg_blue.GetValue() / 255.0f;
            const VkClearValue clear_color{
                .color = {.float32 = {bg_red, bg_green, bg_blue, 1.0f}},
            };
            const VkRenderPassBeginInfo renderpass_bi{
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext = nullptr,
                .renderPass = *renderpass,
                .framebuffer = host_framebuffer,
                .renderArea =
                    {
                        .offset = {0, 0},
                        .extent = size,
                    },
                .clearValueCount = 1,
                .pClearValues = &clear_color,
            };
            const VkViewport viewport{
                .x = 0.0f,
                .y = 0.0f,
                .width = static_cast<float>(size.width),
                .height = static_cast<float>(size.height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };
            const VkRect2D scissor{
                .offset = {0, 0},
                .extent = size,
            };
            cmdbuf.BeginRenderPass(renderpass_bi, VK_SUBPASS_CONTENTS_INLINE);
            auto graphics_pipeline = [this]() {
                switch (Settings::values.scaling_filter.GetValue()) {
                case Settings::ScalingFilter::NearestNeighbor:
                case Settings::ScalingFilter::Bilinear:
                    return *bilinear_pipeline;
                case Settings::ScalingFilter::Bicubic:
                    return *bicubic_pipeline;
                case Settings::ScalingFilter::Gaussian:
                    return *gaussian_pipeline;
                case Settings::ScalingFilter::ScaleForce:
                    return *scaleforce_pipeline;
                default:
                    return *bilinear_pipeline;
                }
            }();
            cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
            cmdbuf.SetViewport(0, viewport);
            cmdbuf.SetScissor(0, scissor);

            cmdbuf.BindVertexBuffer(0, *buffer, offsetof(BufferData, vertices));
            cmdbuf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline_layout, 0,
                                      descriptor_sets[image_index], {});
            cmdbuf.Draw(4, 1, 0, 0);
            cmdbuf.EndRenderPass();
        });
    return *semaphores[image_index];
}

VkSemaphore BlitScreen::DrawToSwapchain(const Tegra::FramebufferConfig& framebuffer,
                                        bool use_accelerated) {
    const std::size_t image_index = swapchain.GetImageIndex();
    const VkExtent2D render_area = swapchain.GetSize();
    const Layout::FramebufferLayout layout = render_window.GetFramebufferLayout();
    return Draw(framebuffer, *framebuffers[image_index], layout, render_area, use_accelerated);
}

vk::Framebuffer BlitScreen::CreateFramebuffer(const VkImageView& image_view, VkExtent2D extent) {
    return CreateFramebuffer(image_view, extent, renderpass);
}

vk::Framebuffer BlitScreen::CreateFramebuffer(const VkImageView& image_view, VkExtent2D extent,
                                              vk::RenderPass& rd) {
    return device.GetLogical().CreateFramebuffer(VkFramebufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderPass = *rd,
        .attachmentCount = 1,
        .pAttachments = &image_view,
        .width = extent.width,
        .height = extent.height,
        .layers = 1,
    });
}

void BlitScreen::CreateStaticResources() {
    CreateShaders();
    CreateSemaphores();
    CreateDescriptorPool();
    CreateDescriptorSetLayout();
    CreateDescriptorSets();
    CreatePipelineLayout();
    CreateSampler();
}

void BlitScreen::CreateDynamicResources() {
    CreateRenderPass();
    CreateFramebuffers();
    CreateGraphicsPipeline();
    fsr.reset();
    if (Settings::values.scaling_filter.GetValue() == Settings::ScalingFilter::Fsr) {
        CreateFSR();
    }
}

void BlitScreen::RefreshResources(const Tegra::FramebufferConfig& framebuffer) {
    if (Settings::values.scaling_filter.GetValue() == Settings::ScalingFilter::Fsr) {
        if (!fsr) {
            CreateFSR();
        }
    } else {
        fsr.reset();
    }

    if (framebuffer.width == raw_width && framebuffer.height == raw_height && !raw_images.empty()) {
        return;
    }
    raw_width = framebuffer.width;
    raw_height = framebuffer.height;
    ReleaseRawImages();

    CreateStagingBuffer(framebuffer);
    CreateRawImages(framebuffer);
}

void BlitScreen::CreateShaders() {
    vertex_shader = BuildShader(device, VULKAN_PRESENT_VERT_SPV);
    fxaa_vertex_shader = BuildShader(device, FXAA_VERT_SPV);
    fxaa_fragment_shader = BuildShader(device, FXAA_FRAG_SPV);
    bilinear_fragment_shader = BuildShader(device, VULKAN_PRESENT_FRAG_SPV);
    bicubic_fragment_shader = BuildShader(device, PRESENT_BICUBIC_FRAG_SPV);
    gaussian_fragment_shader = BuildShader(device, PRESENT_GAUSSIAN_FRAG_SPV);
    if (device.IsFloat16Supported()) {
        scaleforce_fragment_shader = BuildShader(device, VULKAN_PRESENT_SCALEFORCE_FP16_FRAG_SPV);
    } else {
        scaleforce_fragment_shader = BuildShader(device, VULKAN_PRESENT_SCALEFORCE_FP32_FRAG_SPV);
    }
}

void BlitScreen::CreateSemaphores() {
    semaphores.resize(image_count);
    std::ranges::generate(semaphores, [this] { return device.GetLogical().CreateSemaphore(); });
}

void BlitScreen::CreateDescriptorPool() {
    const std::array<VkDescriptorPoolSize, 2> pool_sizes{{
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = static_cast<u32>(image_count),
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = static_cast<u32>(image_count),
        },
    }};

    const std::array<VkDescriptorPoolSize, 1> pool_sizes_aa{{
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = static_cast<u32>(image_count * 2),
        },
    }};

    const VkDescriptorPoolCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = static_cast<u32>(image_count),
        .poolSizeCount = static_cast<u32>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };
    descriptor_pool = device.GetLogical().CreateDescriptorPool(ci);

    const VkDescriptorPoolCreateInfo ci_aa{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = static_cast<u32>(image_count),
        .poolSizeCount = static_cast<u32>(pool_sizes_aa.size()),
        .pPoolSizes = pool_sizes_aa.data(),
    };
    aa_descriptor_pool = device.GetLogical().CreateDescriptorPool(ci_aa);
}

void BlitScreen::CreateRenderPass() {
    renderpass = CreateRenderPassImpl(swapchain.GetImageViewFormat());
}

vk::RenderPass BlitScreen::CreateRenderPassImpl(VkFormat format, bool is_present) {
    const VkAttachmentDescription color_attachment{
        .flags = 0,
        .format = format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = is_present ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_GENERAL,
    };

    const VkAttachmentReference color_attachment_ref{
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

    const VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0,
    };

    const VkRenderPassCreateInfo renderpass_ci{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass_description,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    return device.GetLogical().CreateRenderPass(renderpass_ci);
}

void BlitScreen::CreateDescriptorSetLayout() {
    const std::array<VkDescriptorSetLayoutBinding, 2> layout_bindings{{
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        },
    }};

    const std::array<VkDescriptorSetLayoutBinding, 2> layout_bindings_aa{{
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        },
    }};

    const VkDescriptorSetLayoutCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = static_cast<u32>(layout_bindings.size()),
        .pBindings = layout_bindings.data(),
    };

    const VkDescriptorSetLayoutCreateInfo ci_aa{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = static_cast<u32>(layout_bindings_aa.size()),
        .pBindings = layout_bindings_aa.data(),
    };

    descriptor_set_layout = device.GetLogical().CreateDescriptorSetLayout(ci);
    aa_descriptor_set_layout = device.GetLogical().CreateDescriptorSetLayout(ci_aa);
}

void BlitScreen::CreateDescriptorSets() {
    const std::vector layouts(image_count, *descriptor_set_layout);
    const std::vector layouts_aa(image_count, *aa_descriptor_set_layout);

    const VkDescriptorSetAllocateInfo ai{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = *descriptor_pool,
        .descriptorSetCount = static_cast<u32>(image_count),
        .pSetLayouts = layouts.data(),
    };

    const VkDescriptorSetAllocateInfo ai_aa{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = *aa_descriptor_pool,
        .descriptorSetCount = static_cast<u32>(image_count),
        .pSetLayouts = layouts_aa.data(),
    };

    descriptor_sets = descriptor_pool.Allocate(ai);
    aa_descriptor_sets = aa_descriptor_pool.Allocate(ai_aa);
}

void BlitScreen::CreatePipelineLayout() {
    const VkPipelineLayoutCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = descriptor_set_layout.address(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };
    const VkPipelineLayoutCreateInfo ci_aa{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = aa_descriptor_set_layout.address(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };
    pipeline_layout = device.GetLogical().CreatePipelineLayout(ci);
    aa_pipeline_layout = device.GetLogical().CreatePipelineLayout(ci_aa);
}

void BlitScreen::CreateGraphicsPipeline() {
    const std::array<VkPipelineShaderStageCreateInfo, 2> bilinear_shader_stages{{
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = *vertex_shader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = *bilinear_fragment_shader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
    }};

    const std::array<VkPipelineShaderStageCreateInfo, 2> bicubic_shader_stages{{
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = *vertex_shader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = *bicubic_fragment_shader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
    }};

    const std::array<VkPipelineShaderStageCreateInfo, 2> gaussian_shader_stages{{
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = *vertex_shader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = *gaussian_fragment_shader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
    }};

    const std::array<VkPipelineShaderStageCreateInfo, 2> scaleforce_shader_stages{{
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = *vertex_shader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = *scaleforce_fragment_shader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
    }};

    const auto vertex_binding_description = ScreenRectVertex::GetDescription();
    const auto vertex_attrs_description = ScreenRectVertex::GetAttributes();

    const VkPipelineVertexInputStateCreateInfo vertex_input_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_binding_description,
        .vertexAttributeDescriptionCount = u32{vertex_attrs_description.size()},
        .pVertexAttributeDescriptions = vertex_attrs_description.data(),
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .primitiveRestartEnable = VK_FALSE,
    };

    const VkPipelineViewportStateCreateInfo viewport_state_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };

    const VkPipelineRasterizationStateCreateInfo rasterization_ci{
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

    const VkPipelineMultisampleStateCreateInfo multisampling_ci{
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

    const VkPipelineColorBlendAttachmentState color_blend_attachment{
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

    static constexpr std::array dynamic_states{
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

    const VkGraphicsPipelineCreateInfo bilinear_pipeline_ci{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = static_cast<u32>(bilinear_shader_stages.size()),
        .pStages = bilinear_shader_stages.data(),
        .pVertexInputState = &vertex_input_ci,
        .pInputAssemblyState = &input_assembly_ci,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_state_ci,
        .pRasterizationState = &rasterization_ci,
        .pMultisampleState = &multisampling_ci,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &color_blend_ci,
        .pDynamicState = &dynamic_state_ci,
        .layout = *pipeline_layout,
        .renderPass = *renderpass,
        .subpass = 0,
        .basePipelineHandle = 0,
        .basePipelineIndex = 0,
    };

    const VkGraphicsPipelineCreateInfo bicubic_pipeline_ci{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = static_cast<u32>(bicubic_shader_stages.size()),
        .pStages = bicubic_shader_stages.data(),
        .pVertexInputState = &vertex_input_ci,
        .pInputAssemblyState = &input_assembly_ci,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_state_ci,
        .pRasterizationState = &rasterization_ci,
        .pMultisampleState = &multisampling_ci,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &color_blend_ci,
        .pDynamicState = &dynamic_state_ci,
        .layout = *pipeline_layout,
        .renderPass = *renderpass,
        .subpass = 0,
        .basePipelineHandle = 0,
        .basePipelineIndex = 0,
    };

    const VkGraphicsPipelineCreateInfo gaussian_pipeline_ci{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = static_cast<u32>(gaussian_shader_stages.size()),
        .pStages = gaussian_shader_stages.data(),
        .pVertexInputState = &vertex_input_ci,
        .pInputAssemblyState = &input_assembly_ci,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_state_ci,
        .pRasterizationState = &rasterization_ci,
        .pMultisampleState = &multisampling_ci,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &color_blend_ci,
        .pDynamicState = &dynamic_state_ci,
        .layout = *pipeline_layout,
        .renderPass = *renderpass,
        .subpass = 0,
        .basePipelineHandle = 0,
        .basePipelineIndex = 0,
    };

    const VkGraphicsPipelineCreateInfo scaleforce_pipeline_ci{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = static_cast<u32>(scaleforce_shader_stages.size()),
        .pStages = scaleforce_shader_stages.data(),
        .pVertexInputState = &vertex_input_ci,
        .pInputAssemblyState = &input_assembly_ci,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_state_ci,
        .pRasterizationState = &rasterization_ci,
        .pMultisampleState = &multisampling_ci,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &color_blend_ci,
        .pDynamicState = &dynamic_state_ci,
        .layout = *pipeline_layout,
        .renderPass = *renderpass,
        .subpass = 0,
        .basePipelineHandle = 0,
        .basePipelineIndex = 0,
    };

    bilinear_pipeline = device.GetLogical().CreateGraphicsPipeline(bilinear_pipeline_ci);
    bicubic_pipeline = device.GetLogical().CreateGraphicsPipeline(bicubic_pipeline_ci);
    gaussian_pipeline = device.GetLogical().CreateGraphicsPipeline(gaussian_pipeline_ci);
    scaleforce_pipeline = device.GetLogical().CreateGraphicsPipeline(scaleforce_pipeline_ci);
}

void BlitScreen::CreateSampler() {
    const VkSamplerCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_NEVER,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    const VkSamplerCreateInfo ci_nn{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_NEVER,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    sampler = device.GetLogical().CreateSampler(ci);
    nn_sampler = device.GetLogical().CreateSampler(ci_nn);
}

void BlitScreen::CreateFramebuffers() {
    const VkExtent2D size{swapchain.GetSize()};
    framebuffers.resize(image_count);

    for (std::size_t i = 0; i < image_count; ++i) {
        const VkImageView image_view{swapchain.GetImageViewIndex(i)};
        framebuffers[i] = CreateFramebuffer(image_view, size, renderpass);
    }
}

void BlitScreen::ReleaseRawImages() {
    for (const u64 tick : resource_ticks) {
        scheduler.Wait(tick);
    }
    raw_images.clear();
    raw_buffer_commits.clear();

    aa_image_view.reset();
    aa_image.reset();
    aa_commit = MemoryCommit{};

    buffer.reset();
    buffer_commit = MemoryCommit{};
}

void BlitScreen::CreateStagingBuffer(const Tegra::FramebufferConfig& framebuffer) {
    const VkBufferCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = CalculateBufferSize(framebuffer),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    buffer = device.GetLogical().CreateBuffer(ci);
    buffer_commit = memory_allocator.Commit(buffer, MemoryUsage::Upload);
}

void BlitScreen::CreateRawImages(const Tegra::FramebufferConfig& framebuffer) {
    raw_images.resize(image_count);
    raw_image_views.resize(image_count);
    raw_buffer_commits.resize(image_count);

    const auto create_image = [&](bool used_on_framebuffer = false, u32 up_scale = 1,
                                  u32 down_shift = 0) {
        u32 extra_usages = used_on_framebuffer ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                               : VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        return device.GetLogical().CreateImage(VkImageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = GetFormat(framebuffer),
            .extent =
                {
                    .width = (up_scale * framebuffer.width) >> down_shift,
                    .height = (up_scale * framebuffer.height) >> down_shift,
                    .depth = 1,
                },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = used_on_framebuffer ? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | extra_usages,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        });
    };
    const auto create_commit = [&](vk::Image& image) {
        return memory_allocator.Commit(image, MemoryUsage::DeviceLocal);
    };
    const auto create_image_view = [&](vk::Image& image) {
        return device.GetLogical().CreateImageView(VkImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = *image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = GetFormat(framebuffer),
            .components =
                {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        });
    };

    for (size_t i = 0; i < image_count; ++i) {
        raw_images[i] = create_image();
        raw_buffer_commits[i] = create_commit(raw_images[i]);
        raw_image_views[i] = create_image_view(raw_images[i]);
    }

    // AA Resources
    const u32 up_scale = Settings::values.resolution_info.up_scale;
    const u32 down_shift = Settings::values.resolution_info.down_shift;
    aa_image = create_image(true, up_scale, down_shift);
    aa_commit = create_commit(aa_image);
    aa_image_view = create_image_view(aa_image);
    VkExtent2D size{
        .width = (up_scale * framebuffer.width) >> down_shift,
        .height = (up_scale * framebuffer.height) >> down_shift,
    };
    if (aa_renderpass) {
        aa_framebuffer = CreateFramebuffer(*aa_image_view, size, aa_renderpass);
        return;
    }
    aa_renderpass = CreateRenderPassImpl(GetFormat(framebuffer), false);
    aa_framebuffer = CreateFramebuffer(*aa_image_view, size, aa_renderpass);

    const std::array<VkPipelineShaderStageCreateInfo, 2> fxaa_shader_stages{{
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = *fxaa_vertex_shader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = *fxaa_fragment_shader,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
    }};

    const auto vertex_binding_description = ScreenRectVertex::GetDescription();
    const auto vertex_attrs_description = ScreenRectVertex::GetAttributes();

    const VkPipelineVertexInputStateCreateInfo vertex_input_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_binding_description,
        .vertexAttributeDescriptionCount = u32{vertex_attrs_description.size()},
        .pVertexAttributeDescriptions = vertex_attrs_description.data(),
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .primitiveRestartEnable = VK_FALSE,
    };

    const VkPipelineViewportStateCreateInfo viewport_state_ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };

    const VkPipelineRasterizationStateCreateInfo rasterization_ci{
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

    const VkPipelineMultisampleStateCreateInfo multisampling_ci{
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

    const VkPipelineColorBlendAttachmentState color_blend_attachment{
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

    static constexpr std::array dynamic_states{
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

    const VkGraphicsPipelineCreateInfo fxaa_pipeline_ci{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = static_cast<u32>(fxaa_shader_stages.size()),
        .pStages = fxaa_shader_stages.data(),
        .pVertexInputState = &vertex_input_ci,
        .pInputAssemblyState = &input_assembly_ci,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_state_ci,
        .pRasterizationState = &rasterization_ci,
        .pMultisampleState = &multisampling_ci,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &color_blend_ci,
        .pDynamicState = &dynamic_state_ci,
        .layout = *aa_pipeline_layout,
        .renderPass = *aa_renderpass,
        .subpass = 0,
        .basePipelineHandle = 0,
        .basePipelineIndex = 0,
    };

    // AA
    aa_pipeline = device.GetLogical().CreateGraphicsPipeline(fxaa_pipeline_ci);
}

void BlitScreen::UpdateAADescriptorSet(std::size_t image_index, VkImageView image_view,
                                       bool nn) const {
    const VkDescriptorImageInfo image_info{
        .sampler = nn ? *nn_sampler : *sampler,
        .imageView = image_view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    const VkWriteDescriptorSet sampler_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = aa_descriptor_sets[image_index],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &image_info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    };

    const VkWriteDescriptorSet sampler_write_2{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = aa_descriptor_sets[image_index],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &image_info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    };

    device.GetLogical().UpdateDescriptorSets(std::array{sampler_write, sampler_write_2}, {});
}

void BlitScreen::UpdateDescriptorSet(std::size_t image_index, VkImageView image_view,
                                     bool nn) const {
    const VkDescriptorBufferInfo buffer_info{
        .buffer = *buffer,
        .offset = offsetof(BufferData, uniform),
        .range = sizeof(BufferData::uniform),
    };

    const VkWriteDescriptorSet ubo_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = descriptor_sets[image_index],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &buffer_info,
        .pTexelBufferView = nullptr,
    };

    const VkDescriptorImageInfo image_info{
        .sampler = nn ? *nn_sampler : *sampler,
        .imageView = image_view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    const VkWriteDescriptorSet sampler_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = descriptor_sets[image_index],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &image_info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    };

    device.GetLogical().UpdateDescriptorSets(std::array{ubo_write, sampler_write}, {});
}

void BlitScreen::SetUniformData(BufferData& data, const Layout::FramebufferLayout layout) const {
    data.uniform.modelview_matrix =
        MakeOrthographicMatrix(static_cast<f32>(layout.width), static_cast<f32>(layout.height));
}

void BlitScreen::SetVertexData(BufferData& data, const Tegra::FramebufferConfig& framebuffer,
                               const Layout::FramebufferLayout layout) const {
    const auto& framebuffer_transform_flags = framebuffer.transform_flags;
    const auto& framebuffer_crop_rect = framebuffer.crop_rect;

    static constexpr Common::Rectangle<f32> texcoords{0.f, 0.f, 1.f, 1.f};
    auto left = texcoords.left;
    auto right = texcoords.right;

    switch (framebuffer_transform_flags) {
    case Service::android::BufferTransformFlags::Unset:
        break;
    case Service::android::BufferTransformFlags::FlipV:
        // Flip the framebuffer vertically
        left = texcoords.right;
        right = texcoords.left;
        break;
    default:
        UNIMPLEMENTED_MSG("Unsupported framebuffer_transform_flags={}",
                          static_cast<u32>(framebuffer_transform_flags));
        break;
    }

    UNIMPLEMENTED_IF(framebuffer_crop_rect.top != 0);
    UNIMPLEMENTED_IF(framebuffer_crop_rect.left != 0);

    f32 scale_u = static_cast<f32>(framebuffer.width) / static_cast<f32>(screen_info.width);
    f32 scale_v = static_cast<f32>(framebuffer.height) / static_cast<f32>(screen_info.height);

    // Scale the output by the crop width/height. This is commonly used with 1280x720 rendering
    // (e.g. handheld mode) on a 1920x1080 framebuffer.
    if (!fsr) {
        if (framebuffer_crop_rect.GetWidth() > 0) {
            scale_u = static_cast<f32>(framebuffer_crop_rect.GetWidth()) /
                      static_cast<f32>(screen_info.width);
        }
        if (framebuffer_crop_rect.GetHeight() > 0) {
            scale_v = static_cast<f32>(framebuffer_crop_rect.GetHeight()) /
                      static_cast<f32>(screen_info.height);
        }
    }

    const auto& screen = layout.screen;
    const auto x = static_cast<f32>(screen.left);
    const auto y = static_cast<f32>(screen.top);
    const auto w = static_cast<f32>(screen.GetWidth());
    const auto h = static_cast<f32>(screen.GetHeight());
    data.vertices[0] = ScreenRectVertex(x, y, texcoords.top * scale_u, left * scale_v);
    data.vertices[1] = ScreenRectVertex(x + w, y, texcoords.bottom * scale_u, left * scale_v);
    data.vertices[2] = ScreenRectVertex(x, y + h, texcoords.top * scale_u, right * scale_v);
    data.vertices[3] = ScreenRectVertex(x + w, y + h, texcoords.bottom * scale_u, right * scale_v);
}

void BlitScreen::CreateFSR() {
    const auto& layout = render_window.GetFramebufferLayout();
    const VkExtent2D fsr_size{
        .width = layout.screen.GetWidth(),
        .height = layout.screen.GetHeight(),
    };
    fsr = std::make_unique<FSR>(device, memory_allocator, image_count, fsr_size);
}

u64 BlitScreen::CalculateBufferSize(const Tegra::FramebufferConfig& framebuffer) const {
    return sizeof(BufferData) + GetSizeInBytes(framebuffer) * image_count;
}

u64 BlitScreen::GetRawImageOffset(const Tegra::FramebufferConfig& framebuffer,
                                  std::size_t image_index) const {
    constexpr auto first_image_offset = static_cast<u64>(sizeof(BufferData));
    return first_image_offset + GetSizeInBytes(framebuffer) * image_index;
}

} // namespace Vulkan

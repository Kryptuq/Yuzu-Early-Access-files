// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cmath>
#include "common/bit_cast.h"
#include "common/common_types.h"
#include "common/div_ceil.h"

#include "video_core/host_shaders/vulkan_fidelityfx_fsr_easu_fp16_comp_spv.h"
#include "video_core/host_shaders/vulkan_fidelityfx_fsr_easu_fp32_comp_spv.h"
#include "video_core/host_shaders/vulkan_fidelityfx_fsr_rcas_fp16_comp_spv.h"
#include "video_core/host_shaders/vulkan_fidelityfx_fsr_rcas_fp32_comp_spv.h"
#include "video_core/renderer_vulkan/vk_fsr.h"
#include "video_core/renderer_vulkan/vk_scheduler.h"
#include "video_core/renderer_vulkan/vk_shader_util.h"
#include "video_core/vulkan_common/vulkan_device.h"

namespace Vulkan {
namespace {
// Reimplementations of the constant generating functions in ffx_fsr1.h
// GCC generated a lot of warnings when using the official header.
u32 AU1_AH1_AF1(f32 f) {
    static constexpr u32 base[512]{
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040,
        0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x0c00, 0x1000, 0x1400, 0x1800, 0x1c00, 0x2000,
        0x2400, 0x2800, 0x2c00, 0x3000, 0x3400, 0x3800, 0x3c00, 0x4000, 0x4400, 0x4800, 0x4c00,
        0x5000, 0x5400, 0x5800, 0x5c00, 0x6000, 0x6400, 0x6800, 0x6c00, 0x7000, 0x7400, 0x7800,
        0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff,
        0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff,
        0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff,
        0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff,
        0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff,
        0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff,
        0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff,
        0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff,
        0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff,
        0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff, 0x7bff,
        0x7bff, 0x7bff, 0x7bff, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000,
        0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000,
        0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000,
        0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000,
        0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000,
        0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000,
        0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000,
        0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000,
        0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000,
        0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8001, 0x8002, 0x8004, 0x8008,
        0x8010, 0x8020, 0x8040, 0x8080, 0x8100, 0x8200, 0x8400, 0x8800, 0x8c00, 0x9000, 0x9400,
        0x9800, 0x9c00, 0xa000, 0xa400, 0xa800, 0xac00, 0xb000, 0xb400, 0xb800, 0xbc00, 0xc000,
        0xc400, 0xc800, 0xcc00, 0xd000, 0xd400, 0xd800, 0xdc00, 0xe000, 0xe400, 0xe800, 0xec00,
        0xf000, 0xf400, 0xf800, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff,
        0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff,
        0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff,
        0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff,
        0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff,
        0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff,
        0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff,
        0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff,
        0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff,
        0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff,
        0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff, 0xfbff,
    };
    static constexpr s8 shift[512]{
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x17, 0x16,
        0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d,
        0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d,
        0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x17,
        0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d,
        0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d,
        0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
        0x18, 0x18,
    };
    const u32 u = Common::BitCast<u32>(f);
    const u32 i = u >> 23;
    return base[i] + ((u & 0x7fffff) >> shift[i]);
}

u32 AU1_AH2_AF2(f32 a[2]) {
    return AU1_AH1_AF1(a[0]) + (AU1_AH1_AF1(a[1]) << 16);
}

void FsrEasuCon(u32 con0[4], u32 con1[4], u32 con2[4], u32 con3[4], f32 inputViewportInPixelsX,
                f32 inputViewportInPixelsY, f32 inputSizeInPixelsX, f32 inputSizeInPixelsY,
                f32 outputSizeInPixelsX, f32 outputSizeInPixelsY) {
    con0[0] = Common::BitCast<u32>(inputViewportInPixelsX / outputSizeInPixelsX);
    con0[1] = Common::BitCast<u32>(inputViewportInPixelsY / outputSizeInPixelsY);
    con0[2] = Common::BitCast<u32>(0.5f * inputViewportInPixelsX / outputSizeInPixelsX - 0.5f);
    con0[3] = Common::BitCast<u32>(0.5f * inputViewportInPixelsY / outputSizeInPixelsY - 0.5f);
    con1[0] = Common::BitCast<u32>(1.0f / inputSizeInPixelsX);
    con1[1] = Common::BitCast<u32>(1.0f / inputSizeInPixelsY);
    con1[2] = Common::BitCast<u32>(1.0f / inputSizeInPixelsX);
    con1[3] = Common::BitCast<u32>(-1.0f / inputSizeInPixelsY);
    con2[0] = Common::BitCast<u32>(-1.0f / inputSizeInPixelsX);
    con2[1] = Common::BitCast<u32>(2.0f / inputSizeInPixelsY);
    con2[2] = Common::BitCast<u32>(1.0f / inputSizeInPixelsX);
    con2[3] = Common::BitCast<u32>(2.0f / inputSizeInPixelsY);
    con3[0] = Common::BitCast<u32>(0.0f / inputSizeInPixelsX);
    con3[1] = Common::BitCast<u32>(4.0f / inputSizeInPixelsY);
    con3[2] = con3[3] = 0;
}

void FsrEasuConOffset(u32 con0[4], u32 con1[4], u32 con2[4], u32 con3[4],
                      f32 inputViewportInPixelsX, f32 inputViewportInPixelsY,
                      f32 inputSizeInPixelsX, f32 inputSizeInPixelsY, f32 outputSizeInPixelsX,
                      f32 outputSizeInPixelsY, f32 inputOffsetInPixelsX, f32 inputOffsetInPixelsY) {
    FsrEasuCon(con0, con1, con2, con3, inputViewportInPixelsX, inputViewportInPixelsY,
               inputSizeInPixelsX, inputSizeInPixelsY, outputSizeInPixelsX, outputSizeInPixelsY);
    con0[2] = Common::BitCast<u32>(0.5f * inputViewportInPixelsX / outputSizeInPixelsX - 0.5f +
                                   inputOffsetInPixelsX);
    con0[3] = Common::BitCast<u32>(0.5f * inputViewportInPixelsY / outputSizeInPixelsY - 0.5f +
                                   inputOffsetInPixelsY);
}

void FsrRcasCon(u32* con, f32 sharpness) {
    sharpness = std::exp2f(-sharpness);
    f32 hSharp[2]{sharpness, sharpness};
    con[0] = Common::BitCast<u32>(sharpness);
    con[1] = AU1_AH2_AF2(hSharp);
    con[2] = 0;
    con[3] = 0;
}
} // Anonymous namespace

FSR::FSR(const Device& device_, MemoryAllocator& memory_allocator_, size_t image_count_,
         VkExtent2D output_size_)
    : device{device_}, memory_allocator{memory_allocator_}, image_count{image_count_},
      output_size{output_size_} {

    CreateImages();
    CreateSampler();
    CreateShaders();
    CreateDescriptorPool();
    CreateDescriptorSetLayout();
    CreateDescriptorSets();
    CreatePipelineLayout();
    CreatePipeline();
}

VkImageView FSR::Draw(VKScheduler& scheduler, size_t image_index, VkImageView image_view,
                      VkExtent2D input_image_extent, const Common::Rectangle<int>& crop_rect) {

    UpdateDescriptorSet(image_index, image_view);

    scheduler.Record([this, image_index, input_image_extent, crop_rect](vk::CommandBuffer cmdbuf) {
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

        cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *easu_pipeline);

        std::array<u32, 4 * 4> push_constants;
        FsrEasuConOffset(
            push_constants.data() + 0, push_constants.data() + 4, push_constants.data() + 8,
            push_constants.data() + 12,

            static_cast<f32>(crop_rect.GetWidth()), static_cast<f32>(crop_rect.GetHeight()),
            static_cast<f32>(input_image_extent.width), static_cast<f32>(input_image_extent.height),
            static_cast<f32>(output_size.width), static_cast<f32>(output_size.height),
            static_cast<f32>(crop_rect.left), static_cast<f32>(crop_rect.top));
        cmdbuf.PushConstants(*pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, push_constants);

        {
            VkImageMemoryBarrier fsr_write_barrier = base_barrier;
            fsr_write_barrier.image = *images[image_index];
            fsr_write_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, fsr_write_barrier);
        }

        cmdbuf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline_layout, 0,
                                  descriptor_sets[image_index * 2], {});
        cmdbuf.Dispatch(Common::DivCeil(output_size.width, 16u),
                        Common::DivCeil(output_size.height, 16u), 1);

        cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *rcas_pipeline);

        FsrRcasCon(push_constants.data(), 0.25f);
        cmdbuf.PushConstants(*pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, push_constants);

        {
            std::array<VkImageMemoryBarrier, 2> barriers;
            auto& fsr_read_barrier = barriers[0];
            auto& blit_write_barrier = barriers[1];

            fsr_read_barrier = base_barrier;
            fsr_read_barrier.image = *images[image_index];
            fsr_read_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            fsr_read_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            blit_write_barrier = base_barrier;
            blit_write_barrier.image = *images[image_count + image_index];
            blit_write_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            blit_write_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

            cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, {}, {}, barriers);
        }

        cmdbuf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline_layout, 0,
                                  descriptor_sets[image_index * 2 + 1], {});
        cmdbuf.Dispatch(Common::DivCeil(output_size.width, 16u),
                        Common::DivCeil(output_size.height, 16u), 1);

        {
            std::array<VkImageMemoryBarrier, 1> barriers;
            auto& blit_read_barrier = barriers[0];

            blit_read_barrier = base_barrier;
            blit_read_barrier.image = *images[image_count + image_index];
            blit_read_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            blit_read_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, {}, {}, barriers);
        }
    });

    return *image_views[image_count + image_index];
}

void FSR::CreateDescriptorPool() {
    const std::array<VkDescriptorPoolSize, 2> pool_sizes{{
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = static_cast<u32>(image_count * 2),
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = static_cast<u32>(image_count * 2),
        },
    }};

    const VkDescriptorPoolCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = static_cast<u32>(image_count * 2),
        .poolSizeCount = static_cast<u32>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };
    descriptor_pool = device.GetLogical().CreateDescriptorPool(ci);
}

void FSR::CreateDescriptorSetLayout() {
    const std::array<VkDescriptorSetLayoutBinding, 2> layout_bindings{{
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = sampler.address(),
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = sampler.address(),
        },
    }};

    const VkDescriptorSetLayoutCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = static_cast<u32>(layout_bindings.size()),
        .pBindings = layout_bindings.data(),
    };

    descriptor_set_layout = device.GetLogical().CreateDescriptorSetLayout(ci);
}

void FSR::CreateDescriptorSets() {
    const u32 sets = static_cast<u32>(image_count * 2);
    const std::vector layouts(sets, *descriptor_set_layout);

    const VkDescriptorSetAllocateInfo ai{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = *descriptor_pool,
        .descriptorSetCount = sets,
        .pSetLayouts = layouts.data(),
    };

    descriptor_sets = descriptor_pool.Allocate(ai);
}

void FSR::CreateImages() {
    images.resize(image_count * 2);
    image_views.resize(image_count * 2);
    buffer_commits.resize(image_count * 2);

    for (size_t i = 0; i < image_count * 2; ++i) {
        images[i] = device.GetLogical().CreateImage(VkImageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
            .extent =
                {
                    .width = output_size.width,
                    .height = output_size.height,
                    .depth = 1,
                },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                     VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        });
        buffer_commits[i] = memory_allocator.Commit(images[i], MemoryUsage::DeviceLocal);
        image_views[i] = device.GetLogical().CreateImageView(VkImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = *images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_R16G16B16A16_SFLOAT,
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
    }
}

void FSR::CreatePipelineLayout() {
    VkPushConstantRange push_const{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(std::array<u32, 4 * 4>),
    };
    VkPipelineLayoutCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = descriptor_set_layout.address(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_const,
    };

    pipeline_layout = device.GetLogical().CreatePipelineLayout(ci);
}

void FSR::UpdateDescriptorSet(std::size_t image_index, VkImageView image_view) const {
    const auto fsr_image_view = *image_views[image_index];
    const auto blit_image_view = *image_views[image_count + image_index];

    const VkDescriptorImageInfo image_info{
        .sampler = VK_NULL_HANDLE,
        .imageView = image_view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    const VkDescriptorImageInfo fsr_image_info{
        .sampler = VK_NULL_HANDLE,
        .imageView = fsr_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    const VkDescriptorImageInfo blit_image_info{
        .sampler = VK_NULL_HANDLE,
        .imageView = blit_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    VkWriteDescriptorSet sampler_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = descriptor_sets[image_index * 2],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &image_info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    };

    VkWriteDescriptorSet output_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = descriptor_sets[image_index * 2],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &fsr_image_info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    };

    device.GetLogical().UpdateDescriptorSets(std::array{sampler_write, output_write}, {});

    sampler_write.dstSet = descriptor_sets[image_index * 2 + 1];
    sampler_write.pImageInfo = &fsr_image_info;
    output_write.dstSet = descriptor_sets[image_index * 2 + 1];
    output_write.pImageInfo = &blit_image_info;

    device.GetLogical().UpdateDescriptorSets(std::array{sampler_write, output_write}, {});
}

void FSR::CreateSampler() {
    const VkSamplerCreateInfo ci{
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
    };

    sampler = device.GetLogical().CreateSampler(ci);
}

void FSR::CreateShaders() {
    if (device.IsFloat16Supported()) {
        easu_shader = BuildShader(device, VULKAN_FIDELITYFX_FSR_EASU_FP16_COMP_SPV);
        rcas_shader = BuildShader(device, VULKAN_FIDELITYFX_FSR_RCAS_FP16_COMP_SPV);
    } else {
        easu_shader = BuildShader(device, VULKAN_FIDELITYFX_FSR_EASU_FP32_COMP_SPV);
        rcas_shader = BuildShader(device, VULKAN_FIDELITYFX_FSR_RCAS_FP32_COMP_SPV);
    }
}

void FSR::CreatePipeline() {
    VkPipelineShaderStageCreateInfo shader_stage_easu{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = *easu_shader,
        .pName = "main",
        .pSpecializationInfo = nullptr,
    };

    VkPipelineShaderStageCreateInfo shader_stage_rcas{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = *rcas_shader,
        .pName = "main",
        .pSpecializationInfo = nullptr,
    };

    VkComputePipelineCreateInfo pipeline_ci_easu{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = shader_stage_easu,
        .layout = *pipeline_layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
    };

    VkComputePipelineCreateInfo pipeline_ci_rcas{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = shader_stage_rcas,
        .layout = *pipeline_layout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
    };

    easu_pipeline = device.GetLogical().CreateComputePipeline(pipeline_ci_easu);
    rcas_pipeline = device.GetLogical().CreateComputePipeline(pipeline_ci_rcas);
}

} // namespace Vulkan

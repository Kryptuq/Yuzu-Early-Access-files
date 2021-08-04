// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>

extern "C" {
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
#include <libswscale/swscale.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
}

#include "common/assert.h"
#include "common/logging/log.h"

#include "video_core/command_classes/nvdec.h"
#include "video_core/command_classes/vic.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/gpu.h"
#include "video_core/memory_manager.h"
#include "video_core/textures/decoders.h"

namespace Tegra {

Vic::Vic(GPU& gpu_, std::shared_ptr<Nvdec> nvdec_processor_)
    : gpu(gpu_),
      nvdec_processor(std::move(nvdec_processor_)), converted_frame_buffer{nullptr, av_free} {}

Vic::~Vic() = default;

void Vic::ProcessMethod(Method method, u32 argument) {
    LOG_DEBUG(HW_GPU, "Vic method 0x{:X}", static_cast<u32>(method));
    const u64 arg = static_cast<u64>(argument) << 8;
    switch (method) {
    case Method::Execute:
        Execute();
        break;
    case Method::SetConfigStructOffset:
        config_struct_address = arg;
        break;
    case Method::SetOutputSurfaceLumaOffset:
        output_surface_luma_address = arg;
        break;
    case Method::SetOutputSurfaceChromaOffset:
        output_surface_chroma_address = arg;
        break;
    default:
        break;
    }
}

void Vic::Execute() {
    if (output_surface_luma_address == 0) {
        LOG_ERROR(Service_NVDRV, "VIC Luma address not set.");
        return;
    }
    const VicConfig config{gpu.MemoryManager().Read<u64>(config_struct_address + 0x20)};
    const AVFramePtr frame_ptr = nvdec_processor->GetFrame();
    const auto* frame = frame_ptr.get();
    if (!frame) {
        return;
    }
    const auto pixel_format = static_cast<VideoPixelFormat>(config.pixel_format.Value());
    switch (pixel_format) {
    case VideoPixelFormat::BGRA8:
    case VideoPixelFormat::RGBA8: {
        LOG_TRACE(Service_NVDRV, "Writing RGB Frame");

        if (scaler_ctx == nullptr || frame->width != scaler_width ||
            frame->height != scaler_height) {
            const AVPixelFormat target_format =
                (pixel_format == VideoPixelFormat::RGBA8) ? AV_PIX_FMT_RGBA : AV_PIX_FMT_BGRA;

            sws_freeContext(scaler_ctx);
            scaler_ctx = nullptr;

            // Frames are decoded into either YUV420 or NV12 formats. Convert to desired format
            scaler_ctx = sws_getContext(frame->width, frame->height,
                                        static_cast<AVPixelFormat>(frame->format), frame->width,
                                        frame->height, target_format, 0, nullptr, nullptr, nullptr);

            scaler_width = frame->width;
            scaler_height = frame->height;
        }
        // Get Converted frame
        const u32 width = static_cast<u32>(frame->width);
        const u32 height = static_cast<u32>(frame->height);
        const std::size_t linear_size = width * height * 4;

        // Only allocate frame_buffer once per stream, as the size is not expected to change
        if (!converted_frame_buffer) {
            converted_frame_buffer = AVMallocPtr{static_cast<u8*>(av_malloc(linear_size)), av_free};
        }

        const int converted_stride{frame->width * 4};
        u8* const converted_frame_buf_addr{converted_frame_buffer.get()};

        sws_scale(scaler_ctx, frame->data, frame->linesize, 0, frame->height,
                  &converted_frame_buf_addr, &converted_stride);

        const u32 blk_kind = static_cast<u32>(config.block_linear_kind);
        if (blk_kind != 0) {
            // swizzle pitch linear to block linear
            const u32 block_height = static_cast<u32>(config.block_linear_height_log2);
            const auto size =
                Tegra::Texture::CalculateSize(true, 4, width, height, 1, block_height, 0);
            luma_buffer.resize(size);
            Tegra::Texture::SwizzleSubrect(width, height, width * 4, width, 4, luma_buffer.data(),
                                           converted_frame_buffer.get(), block_height, 0, 0);

            gpu.MemoryManager().WriteBlock(output_surface_luma_address, luma_buffer.data(), size);
        } else {
            // send pitch linear frame
            gpu.MemoryManager().WriteBlock(output_surface_luma_address, converted_frame_buf_addr,
                                           linear_size);
        }
        break;
    }
    case VideoPixelFormat::Yuv420: {
        LOG_TRACE(Service_NVDRV, "Writing YUV420 Frame");

        const std::size_t surface_width = config.surface_width_minus1 + 1;
        const std::size_t surface_height = config.surface_height_minus1 + 1;
        const auto frame_width = std::min(surface_width, static_cast<size_t>(frame->width));
        const auto frame_height = std::min(surface_height, static_cast<size_t>(frame->height));
        const std::size_t aligned_width = (surface_width + 0xff) & ~0xffUL;

        const auto stride = static_cast<size_t>(frame->linesize[0]);

        luma_buffer.resize(aligned_width * surface_height);
        chroma_buffer.resize(aligned_width * surface_height / 2);

        // Populate luma buffer
        const u8* luma_src = frame->data[0];
        for (std::size_t y = 0; y < frame_height; ++y) {
            const std::size_t src = y * stride;
            const std::size_t dst = y * aligned_width;
            for (std::size_t x = 0; x < frame_width; ++x) {
                luma_buffer[dst + x] = luma_src[src + x];
            }
        }
        gpu.MemoryManager().WriteBlock(output_surface_luma_address, luma_buffer.data(),
                                       luma_buffer.size());

        // Chroma
        const std::size_t half_height = frame_height / 2;
        const auto half_stride = static_cast<size_t>(frame->linesize[1]);

        switch (frame->format) {
        case AV_PIX_FMT_YUV420P: {
            // Frame from FFmpeg software
            // Populate chroma buffer from both channels with interleaving.
            const std::size_t half_width = frame_width / 2;
            const u8* chroma_b_src = frame->data[1];
            const u8* chroma_r_src = frame->data[2];
            for (std::size_t y = 0; y < half_height; ++y) {
                const std::size_t src = y * half_stride;
                const std::size_t dst = y * aligned_width;

                for (std::size_t x = 0; x < half_width; ++x) {
                    chroma_buffer[dst + x * 2] = chroma_b_src[src + x];
                    chroma_buffer[dst + x * 2 + 1] = chroma_r_src[src + x];
                }
            }
            break;
        }
        case AV_PIX_FMT_NV12: {
            // Frame from VA-API hardware
            // This is already interleaved so just copy
            const u8* chroma_src = frame->data[1];
            for (std::size_t y = 0; y < half_height; ++y) {
                const std::size_t src = y * stride;
                const std::size_t dst = y * aligned_width;
                for (std::size_t x = 0; x < frame_width; ++x) {
                    chroma_buffer[dst + x] = chroma_src[src + x];
                }
            }
            break;
        }
        default:
            UNREACHABLE();
            break;
        }
        gpu.MemoryManager().WriteBlock(output_surface_chroma_address, chroma_buffer.data(),
                                       chroma_buffer.size());
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Unknown video pixel format {}", config.pixel_format.Value());
        break;
    }
}

} // namespace Tegra

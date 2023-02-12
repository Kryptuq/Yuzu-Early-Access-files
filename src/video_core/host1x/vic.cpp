// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
#include "common/bit_field.h"
#include "common/logging/log.h"

#include "video_core/engines/maxwell_3d.h"
#include "video_core/host1x/host1x.h"
#include "video_core/host1x/nvdec.h"
#include "video_core/host1x/vic.h"
#include "video_core/memory_manager.h"
#include "video_core/textures/decoders.h"

namespace Tegra {

namespace Host1x {

namespace {
enum class VideoPixelFormat : u64_le {
    RGBA8 = 0x1f,
    BGRA8 = 0x20,
    RGBX8 = 0x23,
    YUV420 = 0x44,
};
} // Anonymous namespace

union VicConfig {
    u64_le raw{};
    BitField<0, 7, VideoPixelFormat> pixel_format;
    BitField<7, 2, u64_le> chroma_loc_horiz;
    BitField<9, 2, u64_le> chroma_loc_vert;
    BitField<11, 4, u64_le> block_linear_kind;
    BitField<15, 4, u64_le> block_linear_height_log2;
    BitField<32, 14, u64_le> surface_width_minus1;
    BitField<46, 14, u64_le> surface_height_minus1;
};

Vic::Vic(Host1x& host1x_, std::shared_ptr<Nvdec> nvdec_processor_)
    : host1x(host1x_),
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
    const VicConfig config{host1x.MemoryManager().Read<u64>(config_struct_address + 0x20)};
    const AVFramePtr frame_ptr = nvdec_processor->GetFrame();
    const auto* frame = frame_ptr.get();
    if (!frame) {
        return;
    }
    const u64 surface_width = config.surface_width_minus1 + 1;
    const u64 surface_height = config.surface_height_minus1 + 1;
    if (static_cast<u64>(frame->width) != surface_width ||
        static_cast<u64>(frame->height) != surface_height) {
        // TODO: Properly support multiple video streams with differing frame dimensions
        LOG_WARNING(Service_NVDRV, "Frame dimensions {}x{} don't match surface dimensions {}x{}",
                    frame->width, frame->height, surface_width, surface_height);
    }
    switch (config.pixel_format) {
    case VideoPixelFormat::RGBA8:
    case VideoPixelFormat::BGRA8:
    case VideoPixelFormat::RGBX8:
        WriteRGBFrame(frame, config);
        break;
    case VideoPixelFormat::YUV420:
        WriteYUVFrame(frame, config);
        break;
    default:
        UNIMPLEMENTED_MSG("Unknown video pixel format {:X}", config.pixel_format.Value());
        break;
    }
}

void Vic::WriteRGBFrame(const AVFrame* frame, const VicConfig& config) {
    LOG_TRACE(Service_NVDRV, "Writing RGB Frame");

    if (!scaler_ctx || frame->width != scaler_width || frame->height != scaler_height) {
        const AVPixelFormat target_format = [pixel_format = config.pixel_format]() {
            switch (pixel_format) {
            case VideoPixelFormat::RGBA8:
                return AV_PIX_FMT_RGBA;
            case VideoPixelFormat::BGRA8:
                return AV_PIX_FMT_BGRA;
            case VideoPixelFormat::RGBX8:
                return AV_PIX_FMT_RGB0;
            default:
                return AV_PIX_FMT_RGBA;
            }
        }();

        sws_freeContext(scaler_ctx);
        // Frames are decoded into either YUV420 or NV12 formats. Convert to desired RGB format
        scaler_ctx = sws_getContext(frame->width, frame->height,
                                    static_cast<AVPixelFormat>(frame->format), frame->width,
                                    frame->height, target_format, 0, nullptr, nullptr, nullptr);
        scaler_width = frame->width;
        scaler_height = frame->height;
        converted_frame_buffer.reset();
    }
    if (!converted_frame_buffer) {
        const size_t frame_size = frame->width * frame->height * 4;
        converted_frame_buffer = AVMallocPtr{static_cast<u8*>(av_malloc(frame_size)), av_free};
    }
    const std::array<int, 4> converted_stride{frame->width * 4, frame->height * 4, 0, 0};
    u8* const converted_frame_buf_addr{converted_frame_buffer.get()};
    sws_scale(scaler_ctx, frame->data, frame->linesize, 0, frame->height, &converted_frame_buf_addr,
              converted_stride.data());

    // Use the minimum of surface/frame dimensions to avoid buffer overflow.
    const u32 surface_width = static_cast<u32>(config.surface_width_minus1) + 1;
    const u32 surface_height = static_cast<u32>(config.surface_height_minus1) + 1;
    const u32 width = std::min(surface_width, static_cast<u32>(frame->width));
    const u32 height = std::min(surface_height, static_cast<u32>(frame->height));
    const u32 blk_kind = static_cast<u32>(config.block_linear_kind);
    if (blk_kind != 0) {
        // swizzle pitch linear to block linear
        const u32 block_height = static_cast<u32>(config.block_linear_height_log2);
        const auto size = Texture::CalculateSize(true, 4, width, height, 1, block_height, 0);
        luma_buffer.resize_destructive(size);
        std::span<const u8> frame_buff(converted_frame_buf_addr, 4 * width * height);
        Texture::SwizzleSubrect(luma_buffer, frame_buff, 4, width, height, 1, 0, 0, width, height,
                                block_height, 0, width * 4);

        host1x.MemoryManager().WriteBlock(output_surface_luma_address, luma_buffer.data(), size);
    } else {
        // send pitch linear frame
        const size_t linear_size = width * height * 4;
        host1x.MemoryManager().WriteBlock(output_surface_luma_address, converted_frame_buf_addr,
                                          linear_size);
    }
}

void Vic::WriteYUVFrame(const AVFrame* frame, const VicConfig& config) {
    LOG_TRACE(Service_NVDRV, "Writing YUV420 Frame");

    const std::size_t surface_width = config.surface_width_minus1 + 1;
    const std::size_t surface_height = config.surface_height_minus1 + 1;
    const std::size_t aligned_width = (surface_width + 0xff) & ~0xffUL;
    // Use the minimum of surface/frame dimensions to avoid buffer overflow.
    const auto frame_width = std::min(surface_width, static_cast<size_t>(frame->width));
    const auto frame_height = std::min(surface_height, static_cast<size_t>(frame->height));

    const auto stride = static_cast<size_t>(frame->linesize[0]);

    luma_buffer.resize_destructive(aligned_width * surface_height);
    chroma_buffer.resize_destructive(aligned_width * surface_height / 2);

    // Populate luma buffer
    const u8* luma_src = frame->data[0];
    for (std::size_t y = 0; y < frame_height; ++y) {
        const std::size_t src = y * stride;
        const std::size_t dst = y * aligned_width;
        std::memcpy(luma_buffer.data() + dst, luma_src + src, frame_width);
    }
    host1x.MemoryManager().WriteBlock(output_surface_luma_address, luma_buffer.data(),
                                      luma_buffer.size());

    // Chroma
    const std::size_t half_height = frame_height / 2;
    const auto half_stride = static_cast<size_t>(frame->linesize[1]);

    switch (frame->format) {
    case AV_PIX_FMT_YUV420P: {
        // Frame from FFmpeg software
        // Populate chroma buffer from both channels with interleaving.
        const std::size_t half_width = frame_width / 2;
        u8* chroma_buffer_data = chroma_buffer.data();
        const u8* chroma_b_src = frame->data[1];
        const u8* chroma_r_src = frame->data[2];
        for (std::size_t y = 0; y < half_height; ++y) {
            const std::size_t src = y * half_stride;
            const std::size_t dst = y * aligned_width;
            for (std::size_t x = 0; x < half_width; ++x) {
                chroma_buffer_data[dst + x * 2] = chroma_b_src[src + x];
                chroma_buffer_data[dst + x * 2 + 1] = chroma_r_src[src + x];
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
            std::memcpy(chroma_buffer.data() + dst, chroma_src + src, frame_width);
        }
        break;
    }
    default:
        ASSERT(false);
        break;
    }
    host1x.MemoryManager().WriteBlock(output_surface_chroma_address, chroma_buffer.data(),
                                      chroma_buffer.size());
}

} // namespace Host1x

} // namespace Tegra

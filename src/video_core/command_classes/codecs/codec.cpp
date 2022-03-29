// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <fstream>
#include <vector>
#include "common/assert.h"
#include "common/settings.h"
#include "video_core/command_classes/codecs/codec.h"
#include "video_core/command_classes/codecs/h264.h"
#include "video_core/command_classes/codecs/vp8.h"
#include "video_core/command_classes/codecs/vp9.h"
#include "video_core/gpu.h"
#include "video_core/memory_manager.h"

extern "C" {
#include <libavutil/opt.h>
#ifdef LIBVA_FOUND
// for querying VAAPI driver information
#include <libavutil/hwcontext_vaapi.h>
#endif
}

namespace Tegra {
namespace {
constexpr AVPixelFormat PREFERRED_GPU_FMT = AV_PIX_FMT_NV12;
constexpr AVPixelFormat PREFERRED_CPU_FMT = AV_PIX_FMT_YUV420P;
constexpr std::array PREFERRED_GPU_DECODERS = {
    AV_HWDEVICE_TYPE_CUDA,
#ifdef _WIN32
    AV_HWDEVICE_TYPE_D3D11VA,
    AV_HWDEVICE_TYPE_DXVA2,
#elif defined(__unix__)
    AV_HWDEVICE_TYPE_VAAPI,
    AV_HWDEVICE_TYPE_VDPAU,
#endif
    // last resort for Linux Flatpak (w/ NVIDIA)
    AV_HWDEVICE_TYPE_VULKAN,
};

void AVPacketDeleter(AVPacket* ptr) {
    av_packet_free(&ptr);
}

using AVPacketPtr = std::unique_ptr<AVPacket, decltype(&AVPacketDeleter)>;

AVPixelFormat GetGpuFormat(AVCodecContext* av_codec_ctx, const AVPixelFormat* pix_fmts) {
    for (const AVPixelFormat* p = pix_fmts; *p != AV_PIX_FMT_NONE; ++p) {
        if (*p == av_codec_ctx->pix_fmt) {
            return av_codec_ctx->pix_fmt;
        }
    }
    LOG_INFO(Service_NVDRV, "Could not find compatible GPU AV format, falling back to CPU");
    av_buffer_unref(&av_codec_ctx->hw_device_ctx);
    av_codec_ctx->pix_fmt = PREFERRED_CPU_FMT;
    return PREFERRED_CPU_FMT;
}

// List all the currently available hwcontext in ffmpeg
std::vector<AVHWDeviceType> ListSupportedContexts() {
    std::vector<AVHWDeviceType> contexts{};
    AVHWDeviceType current_device_type = AV_HWDEVICE_TYPE_NONE;
    do {
        current_device_type = av_hwdevice_iterate_types(current_device_type);
        contexts.push_back(current_device_type);
    } while (current_device_type != AV_HWDEVICE_TYPE_NONE);
    return contexts;
}

} // namespace

void AVFrameDeleter(AVFrame* ptr) {
    av_frame_free(&ptr);
}

Codec::Codec(GPU& gpu_, const NvdecCommon::NvdecRegisters& regs)
    : gpu(gpu_), state{regs}, h264_decoder(std::make_unique<Decoder::H264>(gpu)),
      vp8_decoder(std::make_unique<Decoder::VP8>(gpu)),
      vp9_decoder(std::make_unique<Decoder::VP9>(gpu)) {}

Codec::~Codec() {
    if (!initialized) {
        return;
    }
    // Free libav memory
    avcodec_free_context(&av_codec_ctx);
    av_buffer_unref(&av_gpu_decoder);
}

bool Codec::CreateGpuAvDevice() {
    static constexpr auto HW_CONFIG_METHOD = AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX;
    static const auto supported_contexts = ListSupportedContexts();
    for (const auto& type : PREFERRED_GPU_DECODERS) {
        if (std::none_of(supported_contexts.begin(), supported_contexts.end(),
                         [&type](const auto& context) { return context == type; })) {
            LOG_DEBUG(Service_NVDRV, "{} explicitly unsupported", av_hwdevice_get_type_name(type));
            continue;
        }
        // Avoid memory leak from not cleaning up after av_hwdevice_ctx_create
        av_buffer_unref(&av_gpu_decoder);
        const int hwdevice_res = av_hwdevice_ctx_create(&av_gpu_decoder, type, nullptr, nullptr, 0);
        if (hwdevice_res < 0) {
            LOG_DEBUG(Service_NVDRV, "{} av_hwdevice_ctx_create failed {}",
                      av_hwdevice_get_type_name(type), hwdevice_res);
            continue;
        }
#ifdef LIBVA_FOUND
        if (type == AV_HWDEVICE_TYPE_VAAPI) {
            // we need to determine if this is an impersonated VAAPI driver
            AVHWDeviceContext* hwctx =
                static_cast<AVHWDeviceContext*>(static_cast<void*>(av_gpu_decoder->data));
            AVVAAPIDeviceContext* vactx = static_cast<AVVAAPIDeviceContext*>(hwctx->hwctx);
            const char* vendor_name = vaQueryVendorString(vactx->display);
            if (strstr(vendor_name, "VDPAU backend")) {
                // VDPAU impersonated VAAPI impl's are super buggy, we need to skip them
                LOG_DEBUG(Service_NVDRV, "Skipping vdapu impersonated VAAPI driver");
                continue;
            } else {
                // according to some user testing, certain vaapi driver (Intel?) could be buggy
                // so let's log the driver name which may help the developers/supporters
                LOG_DEBUG(Service_NVDRV, "Using VAAPI driver: {}", vendor_name);
            }
        }
#endif
        for (int i = 0;; i++) {
            const AVCodecHWConfig* config = avcodec_get_hw_config(av_codec, i);
            if (!config) {
                LOG_DEBUG(Service_NVDRV, "{} decoder does not support device type {}.",
                          av_codec->name, av_hwdevice_get_type_name(type));
                break;
            }
            if ((config->methods & HW_CONFIG_METHOD) != 0 && config->device_type == type) {
#if defined(__unix__)
                // Some linux decoding backends are reported to crash with this config method
                // TODO(ameerj): Properly support this method
                if ((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX) != 0) {
                    // skip zero-copy decoders, we don't currently support them
                    LOG_DEBUG(Service_NVDRV, "Skipping decoder {} with unsupported capability {}.",
                              av_hwdevice_get_type_name(type), config->methods);
                    continue;
                }
#endif
                LOG_INFO(Service_NVDRV, "Using {} GPU decoder", av_hwdevice_get_type_name(type));
                av_codec_ctx->pix_fmt = config->pix_fmt;
                return true;
            }
        }
    }
    return false;
}

void Codec::InitializeAvCodecContext() {
    av_codec_ctx = avcodec_alloc_context3(av_codec);
    av_opt_set(av_codec_ctx->priv_data, "tune", "zerolatency", 0);
}

void Codec::InitializeGpuDecoder() {
    if (!CreateGpuAvDevice()) {
        av_buffer_unref(&av_gpu_decoder);
        return;
    }
    auto* hw_device_ctx = av_buffer_ref(av_gpu_decoder);
    ASSERT_MSG(hw_device_ctx, "av_buffer_ref failed");
    av_codec_ctx->hw_device_ctx = hw_device_ctx;
    av_codec_ctx->get_format = GetGpuFormat;
}

void Codec::Initialize() {
    const AVCodecID codec = [&] {
        switch (current_codec) {
        case NvdecCommon::VideoCodec::H264:
            return AV_CODEC_ID_H264;
        case NvdecCommon::VideoCodec::VP8:
            return AV_CODEC_ID_VP8;
        case NvdecCommon::VideoCodec::VP9:
            return AV_CODEC_ID_VP9;
        default:
            UNIMPLEMENTED_MSG("Unknown codec {}", current_codec);
            return AV_CODEC_ID_NONE;
        }
    }();
    av_codec = avcodec_find_decoder(codec);

    InitializeAvCodecContext();
    if (Settings::values.nvdec_emulation.GetValue() == Settings::NvdecEmulation::GPU) {
        InitializeGpuDecoder();
    }
    if (const int res = avcodec_open2(av_codec_ctx, av_codec, nullptr); res < 0) {
        LOG_ERROR(Service_NVDRV, "avcodec_open2() Failed with result {}", res);
        avcodec_free_context(&av_codec_ctx);
        av_buffer_unref(&av_gpu_decoder);
        return;
    }
    if (!av_codec_ctx->hw_device_ctx) {
        LOG_INFO(Service_NVDRV, "Using FFmpeg software decoding");
    }
    initialized = true;
}

void Codec::SetTargetCodec(NvdecCommon::VideoCodec codec) {
    if (current_codec != codec) {
        current_codec = codec;
        LOG_INFO(Service_NVDRV, "NVDEC video codec initialized to {}", GetCurrentCodecName());
    }
}

void Codec::Decode() {
    const bool is_first_frame = !initialized;
    if (is_first_frame) {
        Initialize();
    }
    if (!initialized) {
        return;
    }
    bool vp9_hidden_frame = false;
    const auto& frame_data = [&]() {
        switch (current_codec) {
        case Tegra::NvdecCommon::VideoCodec::H264:
            return h264_decoder->ComposeFrame(state, is_first_frame);
        case Tegra::NvdecCommon::VideoCodec::VP8:
            return vp8_decoder->ComposeFrame(state);
        case Tegra::NvdecCommon::VideoCodec::VP9:
            vp9_decoder->ComposeFrame(state);
            vp9_hidden_frame = vp9_decoder->WasFrameHidden();
            return vp9_decoder->GetFrameBytes();
        default:
            UNREACHABLE();
            return std::vector<u8>{};
        }
    }();
    AVPacketPtr packet{av_packet_alloc(), AVPacketDeleter};
    if (!packet) {
        LOG_ERROR(Service_NVDRV, "av_packet_alloc failed");
        return;
    }
    packet->data = const_cast<u8*>(frame_data.data());
    packet->size = static_cast<s32>(frame_data.size());
    if (const int res = avcodec_send_packet(av_codec_ctx, packet.get()); res != 0) {
        LOG_DEBUG(Service_NVDRV, "avcodec_send_packet error {}", res);
        return;
    }
    // Only receive/store visible frames
    if (vp9_hidden_frame) {
        return;
    }
    AVFramePtr initial_frame{av_frame_alloc(), AVFrameDeleter};
    AVFramePtr final_frame{nullptr, AVFrameDeleter};
    ASSERT_MSG(initial_frame, "av_frame_alloc initial_frame failed");
    if (const int ret = avcodec_receive_frame(av_codec_ctx, initial_frame.get()); ret) {
        LOG_DEBUG(Service_NVDRV, "avcodec_receive_frame error {}", ret);
        return;
    }
    if (initial_frame->width == 0 || initial_frame->height == 0) {
        LOG_WARNING(Service_NVDRV, "Zero width or height in frame");
        return;
    }
    if (av_codec_ctx->hw_device_ctx) {
        final_frame = AVFramePtr{av_frame_alloc(), AVFrameDeleter};
        ASSERT_MSG(final_frame, "av_frame_alloc final_frame failed");
        // Can't use AV_PIX_FMT_YUV420P and share code with software decoding in vic.cpp
        // because Intel drivers crash unless using AV_PIX_FMT_NV12
        final_frame->format = PREFERRED_GPU_FMT;
        const int ret = av_hwframe_transfer_data(final_frame.get(), initial_frame.get(), 0);
        ASSERT_MSG(!ret, "av_hwframe_transfer_data error {}", ret);
    } else {
        final_frame = std::move(initial_frame);
    }
    if (final_frame->format != PREFERRED_CPU_FMT && final_frame->format != PREFERRED_GPU_FMT) {
        UNIMPLEMENTED_MSG("Unexpected video format: {}", final_frame->format);
        return;
    }
    av_frames.push(std::move(final_frame));
    if (av_frames.size() > 10) {
        LOG_TRACE(Service_NVDRV, "av_frames.push overflow dropped frame");
        av_frames.pop();
    }
}

AVFramePtr Codec::GetCurrentFrame() {
    // Sometimes VIC will request more frames than have been decoded.
    // in this case, return a nullptr and don't overwrite previous frame data
    if (av_frames.empty()) {
        return AVFramePtr{nullptr, AVFrameDeleter};
    }
    AVFramePtr frame = std::move(av_frames.front());
    av_frames.pop();
    return frame;
}

NvdecCommon::VideoCodec Codec::GetCurrentCodec() const {
    return current_codec;
}

std::string_view Codec::GetCurrentCodecName() const {
    switch (current_codec) {
    case NvdecCommon::VideoCodec::None:
        return "None";
    case NvdecCommon::VideoCodec::H264:
        return "H264";
    case NvdecCommon::VideoCodec::VP8:
        return "VP8";
    case NvdecCommon::VideoCodec::H265:
        return "H265";
    case NvdecCommon::VideoCodec::VP9:
        return "VP9";
    default:
        return "Unknown";
    }
}
} // namespace Tegra

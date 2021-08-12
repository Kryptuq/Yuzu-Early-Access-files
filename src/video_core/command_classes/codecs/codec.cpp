// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <fstream>
#include <vector>
#include "common/assert.h"
#include "common/settings.h"
#include "video_core/command_classes/codecs/codec.h"
#include "video_core/command_classes/codecs/h264.h"
#include "video_core/command_classes/codecs/vp9.h"
#include "video_core/gpu.h"
#include "video_core/memory_manager.h"

extern "C" {
#include <libavutil/opt.h>
}

namespace Tegra {
namespace {
constexpr AVPixelFormat PREFERRED_GPU_FMT = AV_PIX_FMT_NV12;
constexpr AVPixelFormat PREFERRED_CPU_FMT = AV_PIX_FMT_YUV420P;

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
} // namespace

void AVFrameDeleter(AVFrame* ptr) {
    av_frame_free(&ptr);
}

Codec::Codec(GPU& gpu_, const NvdecCommon::NvdecRegisters& regs)
    : gpu(gpu_), state{regs}, h264_decoder(std::make_unique<Decoder::H264>(gpu)),
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
#if defined(LIBVA_FOUND)
    static constexpr std::array<const char*, 3> VAAPI_DRIVERS = {
        "i915",
        "iHD",
        "amdgpu",
    };
    AVDictionary* hwdevice_options = nullptr;
    av_dict_set(&hwdevice_options, "connection_type", "drm", 0);
    for (const auto& driver : VAAPI_DRIVERS) {
        av_dict_set(&hwdevice_options, "kernel_driver", driver, 0);
        const int hwdevice_error = av_hwdevice_ctx_create(&av_gpu_decoder, AV_HWDEVICE_TYPE_VAAPI,
                                                          nullptr, hwdevice_options, 0);
        if (hwdevice_error >= 0) {
            LOG_INFO(Service_NVDRV, "Using VA-API with {}", driver);
            av_dict_free(&hwdevice_options);
            av_codec_ctx->pix_fmt = AV_PIX_FMT_VAAPI;
            return true;
        }
        LOG_DEBUG(Service_NVDRV, "VA-API av_hwdevice_ctx_create failed {}", hwdevice_error);
    }
    LOG_DEBUG(Service_NVDRV, "VA-API av_hwdevice_ctx_create failed for all drivers");
    av_dict_free(&hwdevice_options);
#endif
    static constexpr auto HW_CONFIG_METHOD = AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX;
    static constexpr std::array GPU_DECODER_TYPES{
        AV_HWDEVICE_TYPE_CUDA,
#ifdef _WIN32
        AV_HWDEVICE_TYPE_D3D11VA,
#else
        AV_HWDEVICE_TYPE_VDPAU,
#endif
    };
    for (const auto& type : GPU_DECODER_TYPES) {
        const int hwdevice_res = av_hwdevice_ctx_create(&av_gpu_decoder, type, nullptr, nullptr, 0);
        if (hwdevice_res < 0) {
            LOG_DEBUG(Service_NVDRV, "{} av_hwdevice_ctx_create failed {}",
                      av_hwdevice_get_type_name(type), hwdevice_res);
            continue;
        }
        for (int i = 0;; i++) {
            const AVCodecHWConfig* config = avcodec_get_hw_config(av_codec, i);
            if (!config) {
                LOG_DEBUG(Service_NVDRV, "{} decoder does not support device type {}.",
                          av_codec->name, av_hwdevice_get_type_name(type));
                break;
            }
            if (config->methods & HW_CONFIG_METHOD && config->device_type == type) {
                av_codec_ctx->pix_fmt = config->pix_fmt;
                LOG_INFO(Service_NVDRV, "Using {} GPU decoder", av_hwdevice_get_type_name(type));
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
        case NvdecCommon::VideoCodec::Vp9:
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
    std::vector<u8> frame_data;
    if (current_codec == NvdecCommon::VideoCodec::H264) {
        frame_data = h264_decoder->ComposeFrameHeader(state, is_first_frame);
    } else if (current_codec == NvdecCommon::VideoCodec::Vp9) {
        frame_data = vp9_decoder->ComposeFrameHeader(state);
        vp9_hidden_frame = vp9_decoder->WasFrameHidden();
    }
    AVPacketPtr packet{av_packet_alloc(), AVPacketDeleter};
    if (!packet) {
        LOG_ERROR(Service_NVDRV, "av_packet_alloc failed");
        return;
    }
    packet->data = frame_data.data();
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
    case NvdecCommon::VideoCodec::Vp8:
        return "VP8";
    case NvdecCommon::VideoCodec::H265:
        return "H265";
    case NvdecCommon::VideoCodec::Vp9:
        return "VP9";
    default:
        return "Unknown";
    }
}
} // namespace Tegra
